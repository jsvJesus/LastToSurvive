using System.Data;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class ClanApplyEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.ClanApply");

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
            if (function == "get")
                throw new LegacyApiException("use api_LoginSessionPoller::ClanGetApplications");

            string procedure = function switch
            {
                "apply" => "dbo.WZ_ClanApplyToJoin",
                "answer" => "dbo.WZ_ClanApplyAnswer",
                _ => throw new LegacyApiException("bad func")
            };

            await using var command = new SqlCommand(procedure, connection)
            {
                CommandType = CommandType.StoredProcedure,
                CommandTimeout = 30
            };

            command.Parameters.Add("@in_CharID", SqlDbType.Int).Value = p.GetRequiredInt32("CharID");

            if (function == "apply")
            {
                command.Parameters.Add("@in_ClanID", SqlDbType.Int).Value = p.GetRequiredInt32("ClanID");
                command.Parameters.Add("@in_ApplicationText", SqlDbType.NVarChar, 512).Value = p.GetRequired("Note");
            }
            else
            {
                command.Parameters.Add("@in_ClanApplicationID", SqlDbType.Int).Value = p.GetRequiredInt32("ApplID");
                command.Parameters.Add("@in_Answer", SqlDbType.Int).Value = p.GetRequiredInt32("Answer");
            }

            await using SqlDataReader reader = await command.ExecuteReaderAsync(cancellationToken);
            LegacyProcedureResult result = await LegacySql.ReadResultAsync(reader, false, cancellationToken);

            return result.Code == 0
                ? LegacyApiResponse.Success()
                : LegacyApiResponse.Error(result.Code, result.Message);
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid clan application request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in clan application endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled clan application endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }
}
