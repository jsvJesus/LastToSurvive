using System.Data;
using System.Globalization;
using System.Text;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class ServerNotesEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        IConfiguration configuration,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.ServerNotes");

        try
        {
            LegacyRequestParameters parameters =
                await LegacyRequestParameters.ReadAsync(
                    context.Request,
                    cancellationToken);

            ValidateServerKey(parameters, configuration);

            await using SqlConnection connection =
                await LegacySql.OpenAsync(connections, cancellationToken);

            return parameters.GetRequired("func") switch
            {
                "get" => await GetNotesAsync(
                    connection,
                    parameters,
                    cancellationToken),
                "add" => await AddNoteAsync(
                    connection,
                    parameters,
                    cancellationToken),
                _ => throw new LegacyApiException("bad func")
            };
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid server notes request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in server notes endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled server notes endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }

    private static async Task<IResult> GetNotesAsync(
        SqlConnection connection,
        LegacyRequestParameters parameters,
        CancellationToken cancellationToken)
    {
        await using var command = new SqlCommand(
            "dbo.WZ_SRV_NoteGetAll",
            connection)
        {
            CommandType = CommandType.StoredProcedure,
            CommandTimeout = 30
        };

        command.Parameters.Add("@in_GameServerID", SqlDbType.Int).Value =
            parameters.GetRequiredInt32("GameSID");

        await using SqlDataReader reader =
            await command.ExecuteReaderAsync(cancellationToken);

        LegacyProcedureResult result = await LegacySql.ReadResultAsync(
            reader,
            moveToData: true,
            cancellationToken);

        if (result.Code != 0)
            return LegacyApiResponse.Error(result.Code, result.Message);

        if (!await reader.ReadAsync(cancellationToken))
            throw new LegacyApiException("notes header row missing");

        DateTime currentUtc = Convert.ToDateTime(
            reader["CurUtcDate"],
            CultureInfo.InvariantCulture);

        if (!await reader.NextResultAsync(cancellationToken))
            throw new LegacyApiException("notes result set missing");

        var xml = new StringBuilder("<?xml version=\"1.0\"?>\n<notes>\n");

        while (await reader.ReadAsync(cancellationToken))
        {
            DateTime expireUtc = Convert.ToDateTime(
                reader["ExpireUtcDate"],
                CultureInfo.InvariantCulture);

            long expireMinutes = Convert.ToInt64(
                (expireUtc - currentUtc).TotalMinutes,
                CultureInfo.InvariantCulture);

            DateTime createUtc = Convert.ToDateTime(
                reader["CreateUtcDate"],
                CultureInfo.InvariantCulture);

            xml.Append("<note ");
            xml.Append(LegacySql.XmlAttribute("NoteID", reader["NoteID"]));
            xml.Append(LegacySql.XmlAttribute("CreateDate", ToUnixTime(createUtc)));
            xml.Append(LegacySql.XmlAttribute("ExpireMins", expireMinutes));
            xml.Append(LegacySql.XmlAttribute("TextFrom", reader["TextFrom"]));
            xml.Append(LegacySql.XmlAttribute("TextSubj", reader["TextSubj"]));
            xml.Append(LegacySql.XmlAttribute("GamePos", reader["GamePos"]));
            xml.Append("/>");
        }

        xml.Append("</notes>\n");
        return LegacyPayloadResponse.FromText(xml.ToString());
    }

    private static async Task<IResult> AddNoteAsync(
        SqlConnection connection,
        LegacyRequestParameters parameters,
        CancellationToken cancellationToken)
    {
        await using var command = new SqlCommand(
            "dbo.WZ_SRV_NoteAddNew",
            connection)
        {
            CommandType = CommandType.StoredProcedure,
            CommandTimeout = 30
        };

        command.Parameters.Add("@in_CustomerID", SqlDbType.Int).Value =
            parameters.GetRequiredInt32("s_id");
        command.Parameters.Add("@in_CharID", SqlDbType.Int).Value =
            parameters.GetRequiredInt32("CharID");
        command.Parameters.Add("@in_GameServerID", SqlDbType.Int).Value =
            parameters.GetRequiredInt32("GameSID");
        command.Parameters.Add("@in_GamePos", SqlDbType.VarChar, 128).Value =
            parameters.GetRequired("GamePos");
        command.Parameters.Add("@in_ExpireMins", SqlDbType.Int).Value =
            parameters.GetRequiredInt32("ExpMins");
        command.Parameters.Add("@in_TextFrom", SqlDbType.NVarChar, 64).Value =
            parameters.GetRequired("TextFrom");
        command.Parameters.Add("@in_TextSubj", SqlDbType.NVarChar, 512).Value =
            parameters.GetRequired("TextSubj");

        await using SqlDataReader reader =
            await command.ExecuteReaderAsync(cancellationToken);

        LegacyProcedureResult result = await LegacySql.ReadResultAsync(
            reader,
            moveToData: true,
            cancellationToken);

        if (result.Code != 0)
            return LegacyApiResponse.Error(result.Code, result.Message);

        if (!await reader.ReadAsync(cancellationToken))
            throw new LegacyApiException("NoteID result row missing");

        int noteId = LegacySql.ReadInt32(reader, "NoteID");
        return LegacyApiResponse.Success(noteId.ToString());
    }

    private static void ValidateServerKey(
        LegacyRequestParameters parameters,
        IConfiguration configuration)
    {
        string expected = configuration["Legacy:ServerApiKey"] ?? string.Empty;

        if (string.IsNullOrWhiteSpace(expected))
            throw new LegacyApiException("server api key is not configured");

        if (!string.Equals(
                parameters.GetRequired("skey1"),
                expected,
                StringComparison.Ordinal))
        {
            throw new LegacyApiException("bad key");
        }
    }

    private static long ToUnixTime(DateTime dateTime)
    {
        DateTime utc = dateTime.Kind == DateTimeKind.Utc
            ? dateTime
            : DateTime.SpecifyKind(dateTime, DateTimeKind.Utc);

        return new DateTimeOffset(utc).ToUnixTimeSeconds();
    }
}
