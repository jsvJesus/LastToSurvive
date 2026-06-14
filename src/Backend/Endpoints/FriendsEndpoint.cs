using System.Data;
using System.Text;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class FriendsEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.Friends");

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
                "addReq" => await AddRequestAsync(connection, p, customerId, cancellationToken),
                "addAns" => await AddAnswerAsync(connection, p, customerId, cancellationToken),
                "remove" => await RemoveAsync(connection, p, customerId, cancellationToken),
                "stats" => await StatsAsync(connection, p, customerId, cancellationToken),
                _ => throw new LegacyApiException("bad func")
            };
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid friends request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in friends endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled friends endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }

    private static async Task<IResult> AddRequestAsync(
        SqlConnection connection,
        LegacyRequestParameters p,
        int customerId,
        CancellationToken cancellationToken)
    {
        await using SqlCommand command = Create(connection, "dbo.WZ_FriendAddReq", customerId);
        command.Parameters.Add("@in_FriendGamerTag", SqlDbType.NVarChar, 64).Value = p.GetRequired("name");

        await using SqlDataReader reader = await command.ExecuteReaderAsync(cancellationToken);
        LegacyProcedureResult result = await LegacySql.ReadResultAsync(reader, true, cancellationToken);
        if (result.Code != 0) return LegacyApiResponse.Error(result.Code, result.Message);
        if (!await reader.ReadAsync(cancellationToken)) throw new LegacyApiException("FriendStatus row missing");

        return LegacyApiResponse.Success(LegacySql.ReadInt32(reader, "FriendStatus").ToString());
    }

    private static async Task<IResult> AddAnswerAsync(
        SqlConnection connection,
        LegacyRequestParameters p,
        int customerId,
        CancellationToken cancellationToken)
    {
        await using SqlCommand command = Create(connection, "dbo.WZ_FriendAddAns", customerId);
        command.Parameters.Add("@in_FriendID", SqlDbType.Int).Value = p.GetRequiredInt32("FriendID");
        command.Parameters.Add("@in_Allow", SqlDbType.Int).Value = p.GetRequiredInt32("Allow");
        return await ExecuteSimpleAsync(command, cancellationToken);
    }

    private static async Task<IResult> RemoveAsync(
        SqlConnection connection,
        LegacyRequestParameters p,
        int customerId,
        CancellationToken cancellationToken)
    {
        await using SqlCommand command = Create(connection, "dbo.WZ_FriendRemove", customerId);
        command.Parameters.Add("@in_FriendID", SqlDbType.Int).Value = p.GetRequiredInt32("FriendID");
        return await ExecuteSimpleAsync(command, cancellationToken);
    }

    private static async Task<IResult> StatsAsync(
        SqlConnection connection,
        LegacyRequestParameters p,
        int customerId,
        CancellationToken cancellationToken)
    {
        await using SqlCommand command = Create(connection, "dbo.WZ_FriendGetStats", customerId);
        command.Parameters.Add("@in_FriendID", SqlDbType.Int).Value = p.GetRequiredInt32("FriendID");

        await using SqlDataReader reader = await command.ExecuteReaderAsync(cancellationToken);
        LegacyProcedureResult result = await LegacySql.ReadResultAsync(reader, true, cancellationToken);
        if (result.Code != 0) return LegacyApiResponse.Error(result.Code, result.Message);

        var xml = new StringBuilder("<?xml version=\"1.0\"?>\n<friends>");
        while (await reader.ReadAsync(cancellationToken))
        {
            xml.Append("<f ");
            xml.Append(LegacySql.XmlAttribute("ID", reader["FriendID"]));
            xml.Append(LegacySql.XmlAttribute("XP", reader["HonorPoints"]));
            xml.Append(LegacySql.XmlAttribute("k", reader["Kills"]));
            xml.Append(LegacySql.XmlAttribute("d", reader["Deaths"]));
            xml.Append(LegacySql.XmlAttribute("w", reader["Wins"]));
            xml.Append(LegacySql.XmlAttribute("l", reader["Losses"]));
            xml.Append(LegacySql.XmlAttribute("t", reader["TimePlayed"]));
            xml.Append("/>");
        }
        xml.Append("</friends>");
        return LegacyPayloadResponse.FromText(xml.ToString());
    }

    private static SqlCommand Create(SqlConnection connection, string procedure, int customerId)
    {
        var command = new SqlCommand(procedure, connection)
        {
            CommandType = CommandType.StoredProcedure,
            CommandTimeout = 30
        };
        command.Parameters.Add("@in_CustomerID", SqlDbType.Int).Value = customerId;
        return command;
    }

    private static async Task<IResult> ExecuteSimpleAsync(
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
