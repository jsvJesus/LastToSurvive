using System.Globalization;
using System.IO.Compression;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class ServerUploadLogEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        IConfiguration configuration,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.ServerUploadLog");

        try
        {
            LegacyRequestParameters parameters =
                await LegacyRequestParameters.ReadAsync(context.Request, cancellationToken);

            LegacyServerSecurity.ValidateApiKey(parameters, configuration);

            if (!context.Request.HasFormContentType)
                throw new LegacyApiException("multipart form is required");

            IFormCollection form = await context.Request.ReadFormAsync(cancellationToken);

            UploadedFile? logFile = await ReadFileAsync(
                form,
                parameters,
                "log",
                compressed: true,
                cancellationToken);

            UploadedFile? dumpFile = await ReadFileAsync(
                form,
                parameters,
                "dmp",
                compressed: true,
                cancellationToken);

            UploadedFile? screenshotFile = await ReadFileAsync(
                form,
                parameters,
                "jpg",
                compressed: false,
                cancellationToken);

            string crashDirectory = configuration["Storage:CrashDirectory"]
                ?? Path.Combine(AppContext.BaseDirectory, "Storage", "Crashes");

            string picturesDirectory = configuration["Storage:PicturesDirectory"]
                ?? Path.Combine(AppContext.BaseDirectory, "Storage", "Pictures");

            if (logFile is not null && dumpFile is not null)
            {
                Directory.CreateDirectory(crashDirectory);
                await SaveFileAsync(crashDirectory, logFile, cancellationToken);
                await SaveFileAsync(crashDirectory, dumpFile, cancellationToken);
            }

            if (screenshotFile is not null)
            {
                await SaveScreenshotAsync(
                    picturesDirectory,
                    screenshotFile,
                    cancellationToken);
            }
            else if (logFile is null)
            {
                throw new LegacyApiException("no log/dump files");
            }

            return LegacyApiResponse.Success();
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid upload-log request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (InvalidDataException exception)
        {
            logger.LogWarning(exception, "Invalid compressed upload data");
            return LegacyApiResponse.InternalError("invalid compressed file");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled upload-log endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }

    private static async Task<UploadedFile?> ReadFileAsync(
        IFormCollection form,
        LegacyRequestParameters parameters,
        string prefix,
        bool compressed,
        CancellationToken cancellationToken)
    {
        IFormFile? source = form.Files.GetFile(prefix + "File");
        if (source is null)
            return null;

        int expectedSize = parameters.GetRequiredInt32(prefix + "Size");
        if (expectedSize < 0 || expectedSize > 256 * 1024 * 1024)
            throw new LegacyApiException("invalid upload size");

        await using Stream input = source.OpenReadStream();
        await using var output = new MemoryStream(expectedSize);

        if (compressed)
        {
            await using var gzip = new GZipStream(
                input,
                CompressionMode.Decompress,
                leaveOpen: false);

            await gzip.CopyToAsync(output, cancellationToken);
        }
        else
        {
            await input.CopyToAsync(output, cancellationToken);
        }

        if (output.Length != expectedSize)
            throw new LegacyApiException("uploaded file size mismatch");

        string safeName = Path.GetFileName(source.FileName);
        if (string.IsNullOrWhiteSpace(safeName))
            throw new LegacyApiException("invalid upload filename");

        return new UploadedFile(safeName, output.ToArray());
    }

    private static async Task SaveFileAsync(
        string directory,
        UploadedFile file,
        CancellationToken cancellationToken)
    {
        string path = Path.Combine(directory, file.FileName);
        await File.WriteAllBytesAsync(path, file.Data, cancellationToken);
    }

    private static async Task SaveScreenshotAsync(
        string picturesDirectory,
        UploadedFile file,
        CancellationToken cancellationToken)
    {
        string[] parts = file.FileName.Split('_');
        if (parts.Length < 4)
            throw new LegacyApiException("invalid screenshot filename");

        int serverId = Parse(parts[1], "ServerID");
        int customerId = Parse(parts[2], "CustomerID");
        int charId = Parse(parts[3], "CharID");

        string customerDirectory = Path.Combine(
            picturesDirectory,
            customerId.ToString(CultureInfo.InvariantCulture));

        Directory.CreateDirectory(customerDirectory);

        DateTime now = DateTime.Now;
        string name = string.Format(
            CultureInfo.InvariantCulture,
            "{0}_{1}_{2}_{3}-{4:00}{5:00}-{6:00}{7:00}.jpg",
            customerId,
            charId,
            serverId,
            now.Year,
            now.Month,
            now.Day,
            now.Hour,
            now.Minute);

        await File.WriteAllBytesAsync(
            Path.Combine(customerDirectory, name),
            file.Data,
            cancellationToken);
    }

    private static int Parse(string value, string name)
    {
        if (!int.TryParse(
                value,
                NumberStyles.Integer,
                CultureInfo.InvariantCulture,
                out int result))
        {
            throw new LegacyApiException($"invalid {name}");
        }

        return result;
    }

    private sealed record UploadedFile(
        string FileName,
        byte[] Data);
}
