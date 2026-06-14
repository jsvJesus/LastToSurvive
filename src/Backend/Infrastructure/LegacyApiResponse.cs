namespace WarZ.Api.Infrastructure;

public static class LegacyApiResponse
{
    public static IResult Success(string body = "")
    {
        return Text($"WO_0{body}");
    }

    public static IResult Error(int resultCode, string message = "")
    {
        string separator = string.IsNullOrEmpty(message) ? "" : " ";
        return Text($"WO_{resultCode}{separator}{message}");
    }

    public static IResult InternalError(string message)
    {
        return Error(5, message);
    }

    private static IResult Text(string body)
    {
        // Старый клиент требует HTTP 200 даже для WO_5 и других ошибок.
        return Results.Text(
            body,
            "text/plain; charset=utf-8",
            System.Text.Encoding.UTF8,
            StatusCodes.Status200OK);
    }
}