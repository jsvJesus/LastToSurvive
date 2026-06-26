using Microsoft.AspNetCore.Http.Features;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Endpoints;

var builder = WebApplication.CreateBuilder(args);
ServerHostSettings serverHost = ServerHostSettings.Load(builder.Environment.ContentRootPath);
builder.WebHost.UseUrls(serverHost.ApiUrls);

string connectionString =
    ServerHostSettings.Expand(
        builder.Configuration.GetConnectionString("LTS") // Big Dick
        ?? throw new InvalidOperationException(
            "Connection string 'LTS' was not configured."),
        serverHost);

long maxRequestBodySize =
    (builder.Configuration.GetValue<int?>("Legacy:MaxRequestBodySizeMb") ?? 256)
    * 1024L
    * 1024L;

builder.WebHost.ConfigureKestrel(options =>
{
    options.Limits.MaxRequestBodySize = maxRequestBodySize;
});

builder.Services.Configure<FormOptions>(options =>
{
    options.MultipartBodyLengthLimit = maxRequestBodySize;
    options.ValueLengthLimit = int.MaxValue;
    options.MultipartHeadersLengthLimit = 64 * 1024;
});

builder.Services.AddSingleton(new SqlConnectionFactory(connectionString));

var app = builder.Build();
string[] legacyMethods = { "GET", "POST" };

app.MapGet("/", () => Results.Text(
    "LastToSurvive API is running",
    "text/plain; charset=utf-8"));

app.MapMethods(
    "/api_Test.aspx",
    legacyMethods,
    () => Results.Text("WO_0", "text/plain; charset=utf-8"));

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
                return Results.Text("WO_5 Database returned no result", "text/plain; charset=utf-8");

            string databaseName = reader.IsDBNull(0) ? "UNKNOWN" : reader.GetString(0);
            bool hasLoginProcedure = !reader.IsDBNull(1) && reader.GetInt32(1) == 1;

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
            return Results.Text("WO_5 SQL connection failed", "text/plain; charset=utf-8");
        }
    });

app.MapMethods("/api_Login.aspx", legacyMethods, LoginEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_Login.aspx", legacyMethods, LoginEndpoint.ExecuteAsync);
app.MapMethods("/api_LoginSessionPoller.aspx", legacyMethods, LoginSessionPollerEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_LoginSessionPoller.aspx", legacyMethods, LoginSessionPollerEndpoint.ExecuteAsync);
app.MapMethods("/api_AccRegister.aspx", legacyMethods, AccountRegisterEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_AccRegister.aspx", legacyMethods, AccountRegisterEndpoint.ExecuteAsync);
app.MapMethods("/api_AccCheckKey.aspx", legacyMethods, AccountCheckKeyEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_AccCheckKey.aspx", legacyMethods, AccountCheckKeyEndpoint.ExecuteAsync);
app.MapMethods("/api_AccApplyKey.aspx", legacyMethods, AccountApplyKeyEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_AccApplyKey.aspx", legacyMethods, AccountApplyKeyEndpoint.ExecuteAsync);
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
app.MapMethods("/api_Friends.aspx", legacyMethods, FriendsEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_Friends.aspx", legacyMethods, FriendsEndpoint.ExecuteAsync);
app.MapMethods("/api_LeaderboardGet.aspx", legacyMethods, LeaderboardEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_LeaderboardGet.aspx", legacyMethods, LeaderboardEndpoint.ExecuteAsync);
app.MapMethods("/api_GetDataGameRewards.aspx", legacyMethods, GameRewardsEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_GetDataGameRewards.aspx", legacyMethods, GameRewardsEndpoint.ExecuteAsync);
app.MapMethods("/api_GetLootBoxConfig.aspx", legacyMethods, LootBoxConfigEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_GetLootBoxConfig.aspx", legacyMethods, LootBoxConfigEndpoint.ExecuteAsync);
app.MapMethods("/WarZ/api/php/api_GetLootBoxConfig.php", legacyMethods, LootBoxConfigEndpoint.ExecuteAsync);
app.MapMethods("/api_ClanCreate.aspx", legacyMethods, ClanCreateEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_ClanCreate.aspx", legacyMethods, ClanCreateEndpoint.ExecuteAsync);
app.MapMethods("/api_ClanApply.aspx", legacyMethods, ClanApplyEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_ClanApply.aspx", legacyMethods, ClanApplyEndpoint.ExecuteAsync);
app.MapMethods("/api_ClanInvites.aspx", legacyMethods, ClanInvitesEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_ClanInvites.aspx", legacyMethods, ClanInvitesEndpoint.ExecuteAsync);
app.MapMethods("/api_ClanGetInfo.aspx", legacyMethods, ClanInfoEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_ClanGetInfo.aspx", legacyMethods, ClanInfoEndpoint.ExecuteAsync);
app.MapMethods("/api_ClanGetStatus.aspx", legacyMethods, ClanStatusEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_ClanGetStatus.aspx", legacyMethods, ClanStatusEndpoint.ExecuteAsync);
app.MapMethods("/api_ClanMgr.aspx", legacyMethods, ClanManagerEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_ClanMgr.aspx", legacyMethods, ClanManagerEndpoint.ExecuteAsync);
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
app.MapMethods("/api_SrvCharUpdate.aspx", legacyMethods, ServerCharacterUpdateEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_SrvCharUpdate.aspx", legacyMethods, ServerCharacterUpdateEndpoint.ExecuteAsync);
app.MapMethods("/api_SrvUploadLogFile.aspx", legacyMethods, ServerUploadLogEndpoint.ExecuteAsync);
app.MapMethods("/APS/api_SrvUploadLogFile.aspx", legacyMethods, ServerUploadLogEndpoint.ExecuteAsync);

app.Run();

internal sealed class ServerHostSettings
{
    public string PublicIp { get; private init; } = "127.0.0.1";
    public string ApiIp { get; private init; } = "127.0.0.1";
    public string DatabaseIp { get; private init; } = "127.0.0.1";
    public int ApiPort { get; private init; } = 8080;

    public string ApiUrl => $"http://{ApiIp}:{ApiPort}";
    public string[] ApiUrls
    {
        get
        {
            string localhostUrl = $"http://localhost:{ApiPort}";

            return string.Equals(
                    ApiIp,
                    "localhost",
                    StringComparison.OrdinalIgnoreCase) ||
                string.Equals(
                    ApiIp,
                    "127.0.0.1",
                    StringComparison.OrdinalIgnoreCase)
                ? new[] { ApiUrl }
                : new[] { ApiUrl, localhostUrl };
        }
    }

    public static ServerHostSettings Load(string contentRootPath)
    {
        Dictionary<string, string> values = ReadValues(contentRootPath);

        string publicIp = Get(values, "publicIp", "127.0.0.1");
        string apiIp = Get(values, "apiIp", publicIp);
        string databaseIp = Get(values, "databaseIp", publicIp);
        int apiPort = int.TryParse(Get(values, "apiPort", "8080"), out int parsedPort)
            ? parsedPort
            : 8080;

        return new ServerHostSettings
        {
            PublicIp = publicIp,
            ApiIp = apiIp,
            DatabaseIp = databaseIp,
            ApiPort = apiPort
        };
    }

    public static string Expand(string value, ServerHostSettings settings)
    {
        return value
            .Replace("{PublicIp}", settings.PublicIp, StringComparison.OrdinalIgnoreCase)
            .Replace("{ApiIp}", settings.ApiIp, StringComparison.OrdinalIgnoreCase)
            .Replace("{DatabaseIp}", settings.DatabaseIp, StringComparison.OrdinalIgnoreCase);
    }

    private static Dictionary<string, string> ReadValues(string contentRootPath)
    {
        var values = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        string? path = FindConfigPath(contentRootPath);

        if (path is null)
            return values;

        bool inServerHostSection = false;

        foreach (string rawLine in File.ReadLines(path))
        {
            string line = rawLine.Trim();

            if (line.Length == 0 || line.StartsWith(';') || line.StartsWith('#'))
                continue;

            if (line.StartsWith('[') && line.EndsWith(']'))
            {
                inServerHostSection = string.Equals(
                    line[1..^1],
                    "ServerHost",
                    StringComparison.OrdinalIgnoreCase);
                continue;
            }

            if (!inServerHostSection)
                continue;

            int equalsIndex = line.IndexOf('=');
            if (equalsIndex <= 0)
                continue;

            string key = line[..equalsIndex].Trim();
            string settingValue = line[(equalsIndex + 1)..].Trim();
            values[key] = settingValue;
        }

        return values;
    }

    private static string? FindConfigPath(string contentRootPath)
    {
        foreach (string startPath in new[] { Environment.CurrentDirectory, contentRootPath, AppContext.BaseDirectory })
        {
            var directory = new DirectoryInfo(startPath);

            while (directory is not null)
            {
                string localPath = Path.Combine(directory.FullName, "ServerHost.cfg");
                if (File.Exists(localPath))
                    return localPath;

                string binPath = Path.Combine(directory.FullName, "bin", "ServerHost.cfg");
                if (File.Exists(binPath))
                    return binPath;

                directory = directory.Parent;
            }
        }

        return null;
    }

    private static string Get(
        IReadOnlyDictionary<string, string> values,
        string key,
        string fallback)
    {
        return values.TryGetValue(key, out string? value) && !string.IsNullOrWhiteSpace(value)
            ? value
            : fallback;
    }
}
