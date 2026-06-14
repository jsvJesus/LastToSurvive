using System.Reflection;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Http.Features;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using LegacyPage = System.Web.UI.Page;

namespace WarZ.LegacyHost.Hosting;

public sealed class LegacyPageExecutor
{
    private readonly IHostEnvironment _environment;
    private readonly ILogger<LegacyPageExecutor> _logger;

    public LegacyPageExecutor(
        IHostEnvironment environment,
        ILogger<LegacyPageExecutor> logger)
    {
        _environment = environment;
        _logger = logger;
    }

    public async Task ExecuteAsync(
        HttpContext context,
        Type pageType)
    {
        context.Response.StatusCode = StatusCodes.Status200OK;

        try
        {
            PreparedRequest prepared = await PrepareRequestAsync(
                context,
                context.RequestAborted);

            var legacyRequest = new System.Web.HttpRequest(
                context,
                prepared.Form,
                prepared.RawBody);

            var legacyResponse = new System.Web.HttpResponse();
            var legacyServer = new System.Web.HttpServerUtility(
                _environment.ContentRootPath);

            if (Activator.CreateInstance(pageType) is not LegacyPage page)
            {
                throw new InvalidOperationException(
                    $"Unable to create legacy page '{pageType.FullName}'.");
            }

            page.AttachLegacyContext(
                legacyRequest,
                legacyResponse,
                legacyServer);

            MethodInfo pageLoad = FindPageLoad(pageType)
                ?? throw new MissingMethodException(
                    pageType.FullName,
                    "Page_Load");

            try
            {
                pageLoad.Invoke(
                    page,
                    new object[] { page, EventArgs.Empty });
            }
            catch (TargetInvocationException exception)
                when (exception.InnerException != null)
            {
                throw exception.InnerException;
            }

            byte[] responseBytes = legacyResponse.ToArray();

            context.Response.ContentType = string.IsNullOrWhiteSpace(legacyResponse.ContentType)
                ? "application/octet-stream"
                : legacyResponse.ContentType;

            context.Response.ContentLength = responseBytes.LongLength;
            await context.Response.Body.WriteAsync(
                responseBytes,
                context.RequestAborted);
        }
        catch (OperationCanceledException)
            when (context.RequestAborted.IsCancellationRequested)
        {
            _logger.LogWarning(
                "Legacy request {Path} was cancelled.",
                context.Request.Path);
        }
        catch (Exception exception)
        {
            _logger.LogError(
                exception,
                "Unhandled legacy endpoint failure for {Path} ({PageType}).",
                context.Request.Path,
                pageType.FullName);

            if (!context.Response.HasStarted)
            {
                const string error = "WO_5internal server error";
                byte[] bytes = System.Text.Encoding.UTF8.GetBytes(error);

                context.Response.StatusCode = StatusCodes.Status200OK;
                context.Response.ContentType = "text/plain; charset=utf-8";
                context.Response.ContentLength = bytes.LongLength;
                await context.Response.Body.WriteAsync(
                    bytes,
                    context.RequestAborted);
            }
        }
    }

    private static MethodInfo FindPageLoad(Type pageType)
    {
        for (Type current = pageType;
             current != null;
             current = current.BaseType)
        {
            MethodInfo method = current.GetMethod(
                "Page_Load",
                BindingFlags.Instance |
                BindingFlags.Public |
                BindingFlags.NonPublic |
                BindingFlags.DeclaredOnly);

            if (method != null)
                return method;
        }

        return null;
    }

    private static async Task<PreparedRequest> PrepareRequestAsync(
        HttpContext context,
        CancellationToken cancellationToken)
    {
        context.Request.EnableBuffering();

        var rawBody = new MemoryStream();

        if (context.Request.Body.CanSeek)
            context.Request.Body.Position = 0;

        await context.Request.Body.CopyToAsync(
            rawBody,
            cancellationToken);

        rawBody.Position = 0;

        if (context.Request.Body.CanSeek)
            context.Request.Body.Position = 0;

        IFormCollection form = null;

        if (context.Request.HasFormContentType)
        {
            form = await context.Request.ReadFormAsync(
                cancellationToken);
        }

        if (context.Request.Body.CanSeek)
            context.Request.Body.Position = 0;

        return new PreparedRequest(form, rawBody);
    }

    private sealed record PreparedRequest(
        IFormCollection Form,
        MemoryStream RawBody);
}
