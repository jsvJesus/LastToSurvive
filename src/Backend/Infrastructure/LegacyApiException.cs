namespace WarZ.Api.Infrastructure;

public sealed class LegacyApiException : Exception
{
    public LegacyApiException(string message)
        : base(message)
    {
    }

    public LegacyApiException(string message, Exception innerException)
        : base(message, innerException)
    {
    }
}