using System.Data;
using System.Text;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class ClanInfoEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.ClanInfo");

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

            return p.GetRequired("func") switch
            {
                "lb" => await GetLeaderboardAsync(connection, cancellationToken),
                "info" => await GetInfoAsync(connection, p, cancellationToken),
                _ => throw new LegacyApiException("bad func")
            };
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid clan info request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in clan info endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled clan info endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }

    private static async Task<IResult> GetLeaderboardAsync(
        SqlConnection connection,
        CancellationToken cancellationToken)
    {
        await using SqlCommand command = CreateCommand(connection, 0, 0);
        await using SqlDataReader reader = await command.ExecuteReaderAsync(cancellationToken);

        LegacyProcedureResult result = await LegacySql.ReadResultAsync(reader, true, cancellationToken);
        if (result.Code != 0) return LegacyApiResponse.Error(result.Code, result.Message);

        var xml = new StringBuilder("<?xml version=\"1.0\"?>\n<clans>");
        while (await reader.ReadAsync(cancellationToken))
            AppendClan(xml, reader);
        xml.Append("</clans>");

        return LegacyPayloadResponse.FromText(xml.ToString());
    }

    private static async Task<IResult> GetInfoAsync(
        SqlConnection connection,
        LegacyRequestParameters p,
        CancellationToken cancellationToken)
    {
        int clanId = p.GetRequiredInt32("ClanID");
        int getMembers = p.GetRequiredInt32("GetMembers");

        await using SqlCommand command = CreateCommand(connection, clanId, getMembers);
        await using SqlDataReader reader = await command.ExecuteReaderAsync(cancellationToken);

        LegacyProcedureResult result = await LegacySql.ReadResultAsync(reader, true, cancellationToken);
        if (result.Code != 0) return LegacyApiResponse.Error(result.Code, result.Message);
        if (!await reader.ReadAsync(cancellationToken)) throw new LegacyApiException("clan info row missing");

        var xml = new StringBuilder("<?xml version=\"1.0\"?>\n<clan_info>");
        AppendClan(xml, reader);

        if (getMembers != 0)
        {
            if (!await reader.NextResultAsync(cancellationToken))
                throw new LegacyApiException("clan members result set missing");

            xml.Append("<members>");
            while (await reader.ReadAsync(cancellationToken))
            {
                xml.Append("<m ");
                xml.Append(LegacySql.XmlAttribute("id", reader["CharID"]));
                xml.Append(LegacySql.XmlAttribute("cr", reader["ClanRank"]));
                xml.Append(LegacySql.XmlAttribute("gt", reader["Gamertag"]));
                xml.Append(LegacySql.XmlAttribute("cgp", reader["ClanContributedGP"]));
                xml.Append(LegacySql.XmlAttribute("cxp", reader["ClanContributedXP"]));
                xml.Append(LegacySql.XmlAttribute("xp", reader["XP"]));
                xml.Append(LegacySql.XmlAttribute("tp", reader["TimePlayed"]));
                xml.Append(LegacySql.XmlAttribute("r", reader["Reputation"]));
                xml.Append(LegacySql.XmlAttribute("ts00", reader["Stat00"]));
                xml.Append(LegacySql.XmlAttribute("ts01", reader["Stat01"]));
                xml.Append(LegacySql.XmlAttribute("ts02", reader["Stat02"]));
                xml.Append("/>");
            }
            xml.Append("</members>");
        }

        xml.Append("</clan_info>");
        return LegacyPayloadResponse.FromText(xml.ToString());
    }

    private static SqlCommand CreateCommand(SqlConnection connection, int clanId, int getMembers)
    {
        var command = new SqlCommand("dbo.WZ_ClanGetInfo", connection)
        {
            CommandType = CommandType.StoredProcedure,
            CommandTimeout = 30
        };
        command.Parameters.Add("@in_ClanID", SqlDbType.Int).Value = clanId;
        command.Parameters.Add("@in_GetMembers", SqlDbType.Int).Value = getMembers;
        return command;
    }

    private static void AppendClan(StringBuilder xml, SqlDataReader reader)
    {
        xml.Append("<clan ");
        xml.Append(LegacySql.XmlAttribute("ClanID", reader["ClanID"]));
        xml.Append(LegacySql.XmlAttribute("ClanNameColor", reader["ClanNameColor"]));
        xml.Append(LegacySql.XmlAttribute("ClanTagColor", reader["ClanTagColor"]));
        xml.Append(LegacySql.XmlAttribute("ClanEmblemID", reader["ClanEmblemID"]));
        xml.Append(LegacySql.XmlAttribute("ClanEmblemColor", reader["ClanEmblemColor"]));
        xml.Append(LegacySql.XmlAttribute("ClanXP", reader["ClanXP"]));
        xml.Append(LegacySql.XmlAttribute("ClanLevel", reader["ClanLevel"]));
        xml.Append(LegacySql.XmlAttribute("ClanGP", reader["ClanGP"]));
        xml.Append(LegacySql.XmlAttribute("NumClanMembers", reader["NumClanMembers"]));
        xml.Append(LegacySql.XmlAttribute("MaxClanMembers", reader["MaxClanMembers"]));
        xml.Append(LegacySql.XmlAttribute("ClanName", reader["ClanName"]));
        xml.Append(LegacySql.XmlAttribute("ClanTag", reader["ClanTag"]));
        xml.Append(LegacySql.XmlAttribute("ClanLore", reader["ClanLore"]));
        xml.Append(LegacySql.XmlAttribute("OwnerGamertag", reader["gamertag"]));
        xml.Append("/>");
    }
}
