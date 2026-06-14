using System.Data;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class ClanInvitesEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.ClanInvites");

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
            if (function == "check")
                throw new LegacyApiException("use api_LoginSessionPoller::ClanGetInvites");

            string procedure = function switch
            {
                "send" => "dbo.WZ_ClanInviteSendToPlayer",
                "answer" => "dbo.WZ_ClanInviteAnswer",
                _ => throw new LegacyApiException("bad func")
            };

            await using var command = new SqlCommand(procedure, connection)
            {
                CommandType = CommandType.StoredProcedure,
                CommandTimeout = 30
            };

            command.Parameters.Add("@in_CharID", SqlDbType.Int).Value = p.GetRequiredInt32("CharID");

            if (function == "send")
            {
                command.Parameters.Add("@in_InvGamertag", SqlDbType.NVarChar, 64).Value = p.GetRequired("Gamertag");
            }
            else
            {
                command.Parameters.Add("@in_ClanInviteID", SqlDbType.Int).Value = p.GetRequiredInt32("InviteID");
                command.Parameters.Add("@in_Answer", SqlDbType.Int).Value = p.GetRequiredInt32("Answer");
            }

            await using SqlDataReader reader = await command.ExecuteReaderAsync(cancellationToken);
            LegacyProcedureResult result = await LegacySql.ReadResultAsync(
                reader,
                moveToData: function == "answer",
                cancellationToken);

            if (result.Code != 0)
                return LegacyApiResponse.Error(result.Code, result.Message);

            if (function == "send")
                return LegacyApiResponse.Success();

            if (!await reader.ReadAsync(cancellationToken))
                throw new LegacyApiException("ClanID result row missing");

            return LegacyApiResponse.Success(LegacySql.ReadInt32(reader, "ClanID").ToString());
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid clan invite request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in clan invites endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled clan invites endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }
}
