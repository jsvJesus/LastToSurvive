using System.Data;
using System.Text;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class ClanStatusEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.ClanStatus");

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

            var xml = new StringBuilder("<?xml version=\"1.0\"?>\n<clan>");

            PlayerClanData playerClan = await GetPlayerClanAsync(
                connection,
                charId,
                cancellationToken);

            if (playerClan.ClanId == 0)
            {
                xml.Append("<cldata ");
                xml.Append(LegacySql.XmlAttribute("ID", 0));
                xml.Append("/>\n");
                await AppendInvitesAsync(connection, charId, xml, cancellationToken);
            }
            else
            {
                xml.Append("<cldata ");
                xml.Append(LegacySql.XmlAttribute("ID", playerClan.ClanId));
                xml.Append(LegacySql.XmlAttribute("rank", playerClan.Rank));
                xml.Append(LegacySql.XmlAttribute("cm1", playerClan.MemberCount));
                xml.Append(LegacySql.XmlAttribute("cm2", playerClan.MaxMembers));
                xml.Append("/>\n");

                if (playerClan.Rank <= 1)
                    await AppendApplicationsAsync(connection, charId, xml, cancellationToken);
            }

            xml.Append("</clan>");
            return LegacyPayloadResponse.FromText(xml.ToString());
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid clan status request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in clan status endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled clan status endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }

    private static async Task<PlayerClanData> GetPlayerClanAsync(
        SqlConnection connection,
        int charId,
        CancellationToken cancellationToken)
    {
        await using var command = CreateCommand(connection, "dbo.WZ_ClanGetPlayerData", charId);
        await using SqlDataReader reader = await command.ExecuteReaderAsync(cancellationToken);

        LegacyProcedureResult result = await LegacySql.ReadResultAsync(reader, true, cancellationToken);
        if (result.Code != 0) throw new LegacyApiException(result.Message);
        if (!await reader.ReadAsync(cancellationToken)) throw new LegacyApiException("player clan row missing");

        int clanId = LegacySql.ReadInt32(reader, "ClanID");
        int rank = LegacySql.ReadInt32(reader, "ClanRank");
        int memberCount = clanId == 0 ? 0 : LegacySql.ReadInt32(reader, "NumClanMembers");
        int maxMembers = clanId == 0 ? 0 : LegacySql.ReadInt32(reader, "MaxClanMembers");

        return new PlayerClanData(clanId, rank, memberCount, maxMembers);
    }

    private static async Task AppendInvitesAsync(
        SqlConnection connection,
        int charId,
        StringBuilder xml,
        CancellationToken cancellationToken)
    {
        await using var command = CreateCommand(connection, "dbo.WZ_ClanInviteGetInvitesForPlayer", charId);
        await using SqlDataReader reader = await command.ExecuteReaderAsync(cancellationToken);

        LegacyProcedureResult result = await LegacySql.ReadResultAsync(reader, true, cancellationToken);
        if (result.Code != 0) throw new LegacyApiException(result.Message);

        xml.Append("<clinvites>");
        while (await reader.ReadAsync(cancellationToken))
        {
            xml.Append("<inv ");
            xml.Append(LegacySql.XmlAttribute("id", reader["ClanInviteID"]));
            xml.Append(LegacySql.XmlAttribute("gt", reader["Gamertag"]));
            xml.Append(LegacySql.XmlAttribute("cname", reader["ClanName"]));
            xml.Append(LegacySql.XmlAttribute("cl", reader["ClanLevel"]));
            xml.Append(LegacySql.XmlAttribute("cem", reader["ClanEmblemID"]));
            xml.Append(LegacySql.XmlAttribute("cemc", reader["ClanEmblemColor"]));
            xml.Append(LegacySql.XmlAttribute("cm1", reader["MaxClanMembers"]));
            xml.Append(LegacySql.XmlAttribute("cm2", reader["NumClanMembers"]));
            xml.Append("/>\n");
        }
        xml.Append("</clinvites>");
    }

    private static async Task AppendApplicationsAsync(
        SqlConnection connection,
        int charId,
        StringBuilder xml,
        CancellationToken cancellationToken)
    {
        await using var command = CreateCommand(connection, "dbo.WZ_ClanApplyGetList", charId);
        await using SqlDataReader reader = await command.ExecuteReaderAsync(cancellationToken);

        LegacyProcedureResult result = await LegacySql.ReadResultAsync(reader, true, cancellationToken);
        if (result.Code != 0) throw new LegacyApiException(result.Message);

        xml.Append("<clapps>");
        while (await reader.ReadAsync(cancellationToken))
        {
            xml.Append("<app ");
            xml.Append(LegacySql.XmlAttribute("id", reader["ClanApplicationID"]));
            xml.Append(LegacySql.XmlAttribute("note", reader["ApplicationText"]));
            xml.Append(LegacySql.XmlAttribute("gt", reader["Gamertag"]));
            xml.Append(LegacySql.XmlAttribute("xp", reader["XP"]));
            xml.Append(LegacySql.XmlAttribute("tp", reader["TimePlayed"]));
            xml.Append(LegacySql.XmlAttribute("r", reader["Reputation"]));
            xml.Append(LegacySql.XmlAttribute("ts00", reader["Stat00"]));
            xml.Append(LegacySql.XmlAttribute("ts01", reader["Stat01"]));
            xml.Append(LegacySql.XmlAttribute("ts02", reader["Stat02"]));
            xml.Append("/>\n");
        }
        xml.Append("</clapps>");
    }

    private static SqlCommand CreateCommand(
        SqlConnection connection,
        string procedure,
        int charId)
    {
        var command = new SqlCommand(procedure, connection)
        {
            CommandType = CommandType.StoredProcedure,
            CommandTimeout = 30
        };
        command.Parameters.Add("@in_CharID", SqlDbType.Int).Value = charId;
        return command;
    }

    private readonly record struct PlayerClanData(
        int ClanId,
        int Rank,
        int MemberCount,
        int MaxMembers);
}
