using System.Data;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class ClanManagerEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        IConfiguration configuration,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.ClanManager");

        try
        {
            LegacyRequestParameters p =
                await LegacyRequestParameters.ReadAsync(context.Request, cancellationToken);

            int customerId = p.GetRequiredInt32("s_id");
            int sessionId = p.GetRequiredInt32("s_key");
            int charId = p.GetRequiredInt32("CharID");
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

            return function switch
            {
                "leave" => await ExecuteSimpleAsync(connection, "dbo.WZ_ClanLeave", charId, null, cancellationToken),
                "kick" => await ExecuteSimpleAsync(connection, "dbo.WZ_ClanKickMember", charId,
                    ("@in_MemberID", p.GetRequiredInt32("MemberID")), cancellationToken),
                "setrank" => await SetRankAsync(connection, p, charId, cancellationToken),
                "setlore" => await SetLoreAsync(connection, p, charId, cancellationToken),
                "gpmember" => await DonateMemberAsync(connection, p, charId, cancellationToken),
                "gpclan" => await DonateClanAsync(connection, p, customerId, charId, cancellationToken),
                "buyslot" => await BuySlotAsync(connection, configuration, remoteIp, p, customerId, charId, cancellationToken),
                _ => throw new LegacyApiException("bad func")
            };
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid clan manager request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in clan manager endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled clan manager endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }

    private static async Task<IResult> SetRankAsync(
        SqlConnection connection,
        LegacyRequestParameters p,
        int charId,
        CancellationToken cancellationToken)
    {
        await using var command = Create(connection, "dbo.WZ_ClanSetMemberRank", charId);
        command.Parameters.Add("@in_MemberID", SqlDbType.Int).Value = p.GetRequiredInt32("MemberID");
        command.Parameters.Add("@in_Rank", SqlDbType.Int).Value = p.GetRequiredInt32("Rank");
        return await RunSimpleAsync(command, cancellationToken);
    }

    private static async Task<IResult> SetLoreAsync(
        SqlConnection connection,
        LegacyRequestParameters p,
        int charId,
        CancellationToken cancellationToken)
    {
        await using var command = Create(connection, "dbo.WZ_ClanSetLore", charId);
        command.Parameters.Add("@in_Lore", SqlDbType.NVarChar, 2048).Value = p.GetRequired("Lore");
        return await RunSimpleAsync(command, cancellationToken);
    }

    private static async Task<IResult> DonateMemberAsync(
        SqlConnection connection,
        LegacyRequestParameters p,
        int charId,
        CancellationToken cancellationToken)
    {
        await using var command = Create(connection, "dbo.WZ_ClanDonateToMemberGP", charId);
        command.Parameters.Add("@in_GP", SqlDbType.Int).Value = p.GetRequiredInt32("GP");
        command.Parameters.Add("@in_MemberID", SqlDbType.Int).Value = p.GetRequiredInt32("MemberID");
        return await RunSimpleAsync(command, cancellationToken);
    }

    private static async Task<IResult> DonateClanAsync(
        SqlConnection connection,
        LegacyRequestParameters p,
        int customerId,
        int charId,
        CancellationToken cancellationToken)
    {
        await using var command = Create(connection, "dbo.WZ_ClanDonateToClanGP", charId);
        command.Parameters.Add("@in_CustomerID", SqlDbType.Int).Value = customerId;
        command.Parameters.Add("@in_GP", SqlDbType.Int).Value = p.GetRequiredInt32("GP");
        return await RunSimpleAsync(command, cancellationToken);
    }

    private static async Task<IResult> BuySlotAsync(
        SqlConnection connection,
        IConfiguration configuration,
        string remoteIp,
        LegacyRequestParameters p,
        int customerId,
        int charId,
        CancellationToken cancellationToken)
    {
        int index = p.GetRequiredInt32("idx");
        if (index < 0 || index >= 6)
            throw new LegacyApiException("bad idx");

        int itemId = 301152 + index;
        string region = configuration["Legacy:Region"] ?? "US";
        string buyProcedure = string.Equals(region, "RU", StringComparison.OrdinalIgnoreCase)
            ? "dbo.WZ_BuyItem_GNA"
            : "dbo.WZ_BuyItem_GP";

        await using (var buy = new SqlCommand(buyProcedure, connection)
        {
            CommandType = CommandType.StoredProcedure,
            CommandTimeout = 30
        })
        {
            buy.Parameters.Add("@in_IP", SqlDbType.VarChar, 100).Value = remoteIp;
            buy.Parameters.Add("@in_CustomerID", SqlDbType.Int).Value = customerId;
            buy.Parameters.Add("@in_ItemId", SqlDbType.Int).Value = itemId;
            buy.Parameters.Add("@in_BuyDays", SqlDbType.Int).Value = 2000;

            await using SqlDataReader reader = await buy.ExecuteReaderAsync(cancellationToken);
            LegacyProcedureResult result = await LegacySql.ReadResultAsync(reader, true, cancellationToken);
            if (result.Code != 0) return LegacyApiResponse.Error(result.Code, result.Message);
            if (!await reader.ReadAsync(cancellationToken)) throw new LegacyApiException("purchase row missing");
        }

        await using var addSlots = Create(connection, "dbo.WZ_ClanAddClanMembers", charId);
        addSlots.Parameters.Add("@in_ItemID", SqlDbType.Int).Value = itemId;
        return await RunSimpleAsync(addSlots, cancellationToken);
    }

    private static async Task<IResult> ExecuteSimpleAsync(
        SqlConnection connection,
        string procedure,
        int charId,
        (string Name, int Value)? extra,
        CancellationToken cancellationToken)
    {
        await using var command = Create(connection, procedure, charId);
        if (extra.HasValue)
            command.Parameters.Add(extra.Value.Name, SqlDbType.Int).Value = extra.Value.Value;
        return await RunSimpleAsync(command, cancellationToken);
    }

    private static SqlCommand Create(SqlConnection connection, string procedure, int charId)
    {
        var command = new SqlCommand(procedure, connection)
        {
            CommandType = CommandType.StoredProcedure,
            CommandTimeout = 30
        };
        command.Parameters.Add("@in_CharID", SqlDbType.Int).Value = charId;
        return command;
    }

    private static async Task<IResult> RunSimpleAsync(
        SqlCommand command,
        CancellationToken cancellationToken)
    {
        await using SqlDataReader reader = await command.ExecuteReaderAsync(cancellationToken);
        LegacyProcedureResult result = await LegacySql.ReadResultAsync(reader, false, cancellationToken);
        return result.Code == 0
            ? LegacyApiResponse.Success()
            : LegacyApiResponse.Error(result.Code, result.Message);
    }
}
