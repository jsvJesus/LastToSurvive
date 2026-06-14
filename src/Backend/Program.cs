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
string[] legacyMethods = { "GET", "POST" };

app.MapGet("/", () => Results.Text(
    "LastToSurvive API is running",
    "text/plain; charset=utf-8"));

app.MapMethods(
    "/api_Test.aspx",
    legacyMethods,
    () => Results.Text(
        "WO_0",
        "text/plain; charset=utf-8"));

app.MapMethods(
    "/api_DbTest.aspx",
    legacyMethods,
    async (
        SqlConnectionFactory connections,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken) =>
    {
        ILogger logger = loggerFactory.CreateLogger("DatabaseTest");

        try
        {
            await using SqlConnection connection = connections.CreateConnection();
            await connection.OpenAsync(cancellationToken);

            const string sql = """
                SELECT
                    DB_NAME() AS DatabaseName,
                    CASE
                        WHEN OBJECT_ID(N'dbo.WZ_ACCOUNT_LOGIN', N'P') IS NOT NULL
                        THEN 1
                        ELSE 0
                    END AS HasLoginProcedure;
                """;

            await using var command = new SqlCommand(sql, connection);
            await using SqlDataReader reader =
                await command.ExecuteReaderAsync(cancellationToken);

            if (!await reader.ReadAsync(cancellationToken))
            {
                return Results.Text(
                    "WO_5 Database returned no result",
                    "text/plain; charset=utf-8");
            }

            string databaseName = reader.IsDBNull(0)
                ? "UNKNOWN"
                : reader.GetString(0);

            bool hasLoginProcedure =
                !reader.IsDBNull(1) && reader.GetInt32(1) == 1;

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
            logger.LogError(exception, "Failed to connect to the LTS database.");
            return Results.Text(
                "WO_5 SQL connection failed",
                "text/plain; charset=utf-8");
        }
    });

app.MapMethods("/api_Login.aspx", legacyMethods, LoginEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_Login.aspx", legacyMethods, LoginEndpoint.ExecuteAsync);
app.MapMethods("/api_LoginSessionPoller.aspx", legacyMethods, LoginSessionPollerEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_LoginSessionPoller.aspx", legacyMethods, LoginSessionPollerEndpoint.ExecuteAsync);
app.MapMethods("/api_GetProfile1.aspx", legacyMethods, GetProfileEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_GetProfile1.aspx", legacyMethods, GetProfileEndpoint.ExecuteAsync);
app.MapMethods("/api_CharSlots.aspx", legacyMethods, CharSlotsEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_CharSlots.aspx", legacyMethods, CharSlotsEndpoint.ExecuteAsync);
app.MapMethods("/api_CharBackpack.aspx", legacyMethods, CharBackpackEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_CharBackpack.aspx", legacyMethods, CharBackpackEndpoint.ExecuteAsync);
app.MapMethods("/api_GetItemsInfo.aspx", legacyMethods, ItemsInfoEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_GetItemsInfo.aspx", legacyMethods, ItemsInfoEndpoint.ExecuteAsync);
app.MapMethods("/api_GetShop1.aspx", legacyMethods, GetShopEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_GetShop1.aspx", legacyMethods, GetShopEndpoint.ExecuteAsync);
app.MapMethods("/api_BuyItem3.aspx", legacyMethods, BuyItemEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_BuyItem3.aspx", legacyMethods, BuyItemEndpoint.ExecuteAsync);
app.MapMethods("/api_LeaderboardGet.aspx", legacyMethods, LeaderboardEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_LeaderboardGet.aspx", legacyMethods, LeaderboardEndpoint.ExecuteAsync);
app.MapMethods("/api_GetDataGameRewards.aspx", legacyMethods, GameRewardsEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_GetDataGameRewards.aspx", legacyMethods, GameRewardsEndpoint.ExecuteAsync);
app.MapMethods("/api_SrvNotes.aspx", legacyMethods, ServerNotesEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_SrvNotes.aspx", legacyMethods, ServerNotesEndpoint.ExecuteAsync);
app.MapMethods("/api_SrvUserJoinedGame.aspx", legacyMethods, ServerUserJoinedEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_SrvUserJoinedGame.aspx", legacyMethods, ServerUserJoinedEndpoint.ExecuteAsync);
app.MapMethods("/api_SrvUserLeftGame.aspx", legacyMethods, ServerUserLeftEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_SrvUserLeftGame.aspx", legacyMethods, ServerUserLeftEndpoint.ExecuteAsync);
app.MapMethods("/api_SrvAddCheatAttempts.aspx", legacyMethods, ServerCheatAttemptEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_SrvAddCheatAttempts.aspx", legacyMethods, ServerCheatAttemptEndpoint.ExecuteAsync);
app.MapMethods("/api_SrvAddLogInfo.aspx", legacyMethods, ServerLogInfoEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_SrvAddLogInfo.aspx", legacyMethods, ServerLogInfoEndpoint.ExecuteAsync);
app.MapMethods("/api_SrvAddWeaponStats.aspx", legacyMethods, ServerWeaponStatsEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_SrvAddWeaponStats.aspx", legacyMethods, ServerWeaponStatsEndpoint.ExecuteAsync);

app.Run();
