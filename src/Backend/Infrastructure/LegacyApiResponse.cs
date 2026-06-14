using System.Text;

namespace WarZ.Api.Infrastructure;

public static class LegacyApiResponse
{
    public static IResult Success(string body = "")
    {
        return Text($"WO_0{body}");
    }

    public static IResult Error(int resultCode, string message = "")
    {
        string separator =
            string.IsNullOrEmpty(message)
                ? ""
                : " ";

        return Text(
            $"WO_{resultCode}{separator}{message}");
    }

    public static IResult InternalError(string message)
    {
        return Error(5, message);
    }

    public static IResult Raw(string body)
    {
        return Text(body);
    }

    public static IResult Xml(string xml)
    {
        return Results.Text(
            xml,
            "text/xml; charset=utf-8",
            Encoding.UTF8,
            StatusCodes.Status200OK);
    }

    private static IResult Text(string body)
    {
        return Results.Text(
            body,
            "text/plain; charset=utf-8",
            Encoding.UTF8,
            StatusCodes.Status200OK);
    }
}