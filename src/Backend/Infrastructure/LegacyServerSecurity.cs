namespace WarZ.Api.Infrastructure;

public static class LegacyServerSecurity
{
    public static void ValidateApiKey(
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
}
