using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class LootBoxConfigEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        IConfiguration configuration,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.LootBoxConfig");

        try
        {
            LegacyRequestParameters parameters =
                await LegacyRequestParameters.ReadAsync(
                    context.Request,
                    cancellationToken);

            LegacyServerSecurity.ValidateApiKey(parameters, configuration);

            const string xml = "<?xml version=\"1.0\"?>\n<LootBoxDB></LootBoxDB>";
            return LegacyPayloadResponse.FromText(xml);
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid lootbox config request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled lootbox config endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }
}
