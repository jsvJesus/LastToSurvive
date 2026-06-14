using Microsoft.Data.SqlClient;
using WarZ.Api.Data;

var builder = WebApplication.CreateBuilder(args);

var connectionString =
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
        var logger = loggerFactory.CreateLogger("DatabaseTest");

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

            await using var reader =
                await command.ExecuteReaderAsync(cancellationToken);

            if (!await reader.ReadAsync(cancellationToken))
            {
                return Results.Text(
                    "WO_5 Database returned no result",
                    "text/plain; charset=utf-8",
                    statusCode: StatusCodes.Status500InternalServerError);
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
                logger.LogError(
                    "Stored procedure dbo.WZ_ACCOUNT_LOGIN was not found.");

                return Results.Text(
                    $"WO_5 Database={databaseName}; Missing dbo.WZ_ACCOUNT_LOGIN",
                    "text/plain; charset=utf-8",
                    statusCode: StatusCodes.Status500InternalServerError);
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
                "text/plain; charset=utf-8",
                statusCode: StatusCodes.Status500InternalServerError);
        }
    });

app.Run();