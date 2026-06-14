using System.Data;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class ClanCreateEndpoint
{
    private const string Forbidden = "!@#$%^&*()-=+_<>,./?'\":;|{}[]";

    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        IConfiguration configuration,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.ClanCreate");

        try
        {
            LegacyRequestParameters p =
                await LegacyRequestParameters.ReadAsync(context.Request, cancellationToken);

            int customerId = p.GetRequiredInt32("s_id");
            int sessionId = p.GetRequiredInt32("s_key");
            string remoteIp = context.Connection.RemoteIpAddress?.ToString() ?? "0.0.0.0";

            await using SqlConnection connection =
                await LegacySql.OpenAsync(connections, cancellationToken);

            LegacyProcedureResult session = await LegacySql.ValidateSessionAsync(
                connection,
                remoteIp,
                customerId,
                sessionId,
                cancellationToken);

            if (session.Code != 0)
                return LegacyApiResponse.Error(session.Code, session.Message);

            string function = p.GetRequired("func");
            int needMoney = await GetNeedMoneyAsync(connection, customerId, cancellationToken);

            if (function == "check1")
                return LegacyApiResponse.Success(needMoney.ToString());

            if (function != "create")
                throw new LegacyApiException("bad func");

            string clanName = p.GetRequired("ClanName");
            string clanTag = p.GetRequired("ClanTag");

            if (clanName.IndexOfAny(Forbidden.ToCharArray()) >= 0)
                return LegacyApiResponse.Raw("WO_1Clan name cannot contain special symbols");

            if (clanTag.IndexOfAny(Forbidden.ToCharArray()) >= 0)
                return LegacyApiResponse.Raw("WO_2Clan tag cannot contain special symbols");

            if (clanName.Length < 1) throw new LegacyApiException("ClanName too small");
            if (clanTag.Length < 1) throw new LegacyApiException("ClanTag too small");

            int charId = p.GetRequiredInt32("CharID");

            IResult? parameterError = await CheckParametersAsync(
                connection,
                charId,
                clanName,
                clanTag,
                cancellationToken);

            if (parameterError is not null)
                return parameterError;

            int balance = 0;
            if (needMoney > 0)
            {
                balance = await BuyCreationItemAsync(
                    connection,
                    configuration,
                    remoteIp,
                    customerId,
                    cancellationToken);
            }

            int clanId = await CreateClanAsync(
                connection,
                p,
                customerId,
                charId,
                clanName,
                clanTag,
                cancellationToken);

            return LegacyApiResponse.Success($"{clanId} {balance}");
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid clan create request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in clan create endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled clan create endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }

    private static async Task<int> GetNeedMoneyAsync(
        SqlConnection connection,
        int customerId,
        CancellationToken cancellationToken)
    {
        await using var command = new SqlCommand("dbo.WZ_ClanCreateCheckMoney", connection)
        {
            CommandType = CommandType.StoredProcedure,
            CommandTimeout = 30
        };
        command.Parameters.Add("@in_CustomerID", SqlDbType.Int).Value = customerId;

        await using SqlDataReader reader = await command.ExecuteReaderAsync(cancellationToken);
        LegacyProcedureResult result = await LegacySql.ReadResultAsync(reader, true, cancellationToken);
        if (result.Code != 0) throw new LegacyApiException(result.Message);
        if (!await reader.ReadAsync(cancellationToken)) throw new LegacyApiException("NeedMoney row missing");
        return LegacySql.ReadInt32(reader, "NeedMoney");
    }

    private static async Task<IResult?> CheckParametersAsync(
        SqlConnection connection,
        int charId,
        string clanName,
        string clanTag,
        CancellationToken cancellationToken)
    {
        await using var command = new SqlCommand("dbo.WZ_ClanCreateCheckParams", connection)
        {
            CommandType = CommandType.StoredProcedure,
            CommandTimeout = 30
        };
        command.Parameters.Add("@in_CharID", SqlDbType.Int).Value = charId;
        command.Parameters.Add("@in_ClanName", SqlDbType.NVarChar, 64).Value = clanName;
        command.Parameters.Add("@in_ClanTag", SqlDbType.NVarChar, 16).Value = clanTag;

        await using SqlDataReader reader = await command.ExecuteReaderAsync(cancellationToken);
        LegacyProcedureResult result = await LegacySql.ReadResultAsync(reader, false, cancellationToken);
        return result.Code == 0 ? null : LegacyApiResponse.Error(result.Code, result.Message);
    }

    private static async Task<int> BuyCreationItemAsync(
        SqlConnection connection,
        IConfiguration configuration,
        string remoteIp,
        int customerId,
        CancellationToken cancellationToken)
    {
        string region = configuration["Legacy:Region"] ?? "US";
        string procedure = string.Equals(region, "RU", StringComparison.OrdinalIgnoreCase)
            ? "dbo.WZ_BuyItem_GNA"
            : "dbo.WZ_BuyItem_GP";

        await using var command = new SqlCommand(procedure, connection)
        {
            CommandType = CommandType.StoredProcedure,
            CommandTimeout = 30
        };
        command.Parameters.Add("@in_IP", SqlDbType.VarChar, 100).Value = remoteIp;
        command.Parameters.Add("@in_CustomerID", SqlDbType.Int).Value = customerId;
        command.Parameters.Add("@in_ItemId", SqlDbType.Int).Value = 301151;
        command.Parameters.Add("@in_BuyDays", SqlDbType.Int).Value = 2000;

        await using SqlDataReader reader = await command.ExecuteReaderAsync(cancellationToken);
        LegacyProcedureResult result = await LegacySql.ReadResultAsync(reader, true, cancellationToken);
        if (result.Code != 0) throw new LegacyApiException(result.Message);
        if (!await reader.ReadAsync(cancellationToken)) throw new LegacyApiException("purchase row missing");
        return LegacySql.ReadInt32(reader, "Balance");
    }

    private static async Task<int> CreateClanAsync(
        SqlConnection connection,
        LegacyRequestParameters p,
        int customerId,
        int charId,
        string clanName,
        string clanTag,
        CancellationToken cancellationToken)
    {
        await using var command = new SqlCommand("dbo.WZ_ClanCreate", connection)
        {
            CommandType = CommandType.StoredProcedure,
            CommandTimeout = 30
        };
        command.Parameters.Add("@in_CustomerID", SqlDbType.Int).Value = customerId;
        command.Parameters.Add("@in_CharID", SqlDbType.Int).Value = charId;
        command.Parameters.Add("@in_ClanName", SqlDbType.NVarChar, 64).Value = clanName;
        command.Parameters.Add("@in_ClanNameColor", SqlDbType.Int).Value = p.GetRequiredInt32("ClanNameColor");
        command.Parameters.Add("@in_ClanTag", SqlDbType.NVarChar, 16).Value = clanTag;
        command.Parameters.Add("@in_ClanTagColor", SqlDbType.Int).Value = p.GetRequiredInt32("ClanTagColor");
        command.Parameters.Add("@in_ClanEmblemID", SqlDbType.Int).Value = p.GetRequiredInt32("ClanEmblemID");
        command.Parameters.Add("@in_ClanEmblemColor", SqlDbType.Int).Value = p.GetRequiredInt32("ClanEmblemColor");

        await using SqlDataReader reader = await command.ExecuteReaderAsync(cancellationToken);
        LegacyProcedureResult result = await LegacySql.ReadResultAsync(reader, true, cancellationToken);
        if (result.Code != 0) throw new LegacyApiException(result.Message);
        if (!await reader.ReadAsync(cancellationToken)) throw new LegacyApiException("ClanID row missing");
        return LegacySql.ReadInt32(reader, "ClanID");
    }
}
