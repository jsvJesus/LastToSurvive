using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Endpoints;

var builder = WebApplication.CreateBuilder(args);

string connectionString =
    builder.Configuration.GetConnectionString("LTS")
    ?? throw new InvalidOperationException(
        "Connection string 'LTS' was not configured.");

builder.Services.AddSingleton(
    new SqlConnectionFactory(connectionString));

var app = builder.Build();

app.MapGet("/", () =>
{
    return Results.Text(
        "LastToSurvive API is running",
        "text/plain; charset=utf-8");
});

app.MapMethods(
    "/api_Test.aspx",
    new[] { "GET", "POST" },
    () =>
    {
        return Results.Text(
            "WO_0",
            "text/plain; charset=utf-8");
    });

app.MapMethods(
    "/api_DbTest.aspx",
    new[] { "GET", "POST" },
    async (
        SqlConnectionFactory connections,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken) =>
    {
        ILogger logger =
            loggerFactory.CreateLogger("DatabaseTest");

        try
        {
            await using SqlConnection connection =
                connections.CreateConnection();

            await connection.OpenAsync(cancellationToken);

            const string sql = """
                SELECT
                    DB_NAME() AS DatabaseName,
                    CASE
                        WHEN OBJECT_ID(
                            N'dbo.WZ_ACCOUNT_LOGIN',
                            N'P') IS NOT NULL
                        THEN 1
                        ELSE 0
                    END AS HasLoginProcedure;
                """;

            await using var command =
                new SqlCommand(sql, connection);

            await using SqlDataReader reader =
                await command.ExecuteReaderAsync(cancellationToken);

            if (!await reader.ReadAsync(cancellationToken))
            {
                return Results.Text(
                    "WO_5 Database returned no result",
                    "text/plain; charset=utf-8");
            }

            string databaseName =
                reader.IsDBNull(0)
                    ? "UNKNOWN"
                    : reader.GetString(0);

            bool hasLoginProcedure =
                !reader.IsDBNull(1) &&
                reader.GetInt32(1) == 1;

            if (!hasLoginProcedure)
            {
                return Results.Text(
                    $"WO_5 Database={databaseName}; Missing dbo.WZ_ACCOUNT_LOGIN",
                    "text/plain; charset=utf-8");
            }

            return Results.Text(
                $"WO_0 Database={databaseName}; WZ_ACCOUNT_LOGIN=OK",
                "text/plain; charset=utf-8");
        }
        catch (Exception exception)
        {
            logger.LogError(
                exception,
                "Failed to connect to the LTS database.");

            return Results.Text(
                "WO_5 SQL connection failed",
                "text/plain; charset=utf-8");
        }
    });

/*
 * Оставляем оба адреса:
 *
 * /api_Login.aspx     — удобно тестировать вручную.
 * /APS/api_Login.aspx — именно этот адрес использует игровой клиент.
 */
app.MapMethods(
    "/api_Login.aspx",
    new[] { "GET", "POST" },
    LoginEndpoint.ExecuteAsync);

app.MapMethods(
    "/APS/api_Login.aspx",
    new[] { "GET", "POST" },
    LoginEndpoint.ExecuteAsync);

app.MapMethods(
    "/api_LoginSessionPoller.aspx",
    new[] { "GET", "POST" },
    LoginSessionPollerEndpoint.ExecuteAsync);

app.MapMethods(
    "/APS/api_LoginSessionPoller.aspx",
    new[] { "GET", "POST" },
    LoginSessionPollerEndpoint.ExecuteAsync);

app.Run();