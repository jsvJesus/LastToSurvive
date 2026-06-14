using System.Data;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class ServerUserJoinedEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        IConfiguration configuration,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.ServerUserJoined");

        try
        {
            LegacyRequestParameters parameters =
                await LegacyRequestParameters.ReadAsync(context.Request, cancellationToken);

            LegacyServerSecurity.ValidateApiKey(parameters, configuration);

            int customerId = parameters.GetRequiredInt32("s_id");
            int sessionId = parameters.GetRequiredInt32("s_key");
            string remoteIp = context.Connection.RemoteIpAddress?.ToString() ?? "0.0.0.0";

            await using SqlConnection connection =
                await LegacySql.OpenAsync(connections, cancellationToken);

            LegacyProcedureResult sessionResult = await LegacySql.ValidateSessionAsync(
                connection,
                remoteIp,
                customerId,
                sessionId,
                cancellationToken);

            if (sessionResult.Code != 0)
                return LegacyApiResponse.Error(sessionResult.Code, sessionResult.Message);

            await using var command = new SqlCommand(
                "dbo.WZ_SRV_UserJoinedGame2",
                connection)
            {
                CommandType = CommandType.StoredProcedure,
                CommandTimeout = 30
            };

            command.Parameters.Add("@in_CustomerID", SqlDbType.Int).Value = customerId;
            command.Parameters.Add("@in_CharID", SqlDbType.Int).Value = parameters.GetRequiredInt32("CharID");
            command.Parameters.Add("@in_GameMapId", SqlDbType.Int).Value = parameters.GetRequiredInt32("g1");
            command.Parameters.Add("@in_GameServerId", SqlDbType.Int).Value = parameters.GetRequiredInt32("g2");

            await using SqlDataReader reader =
                await command.ExecuteReaderAsync(cancellationToken);

            LegacyProcedureResult result = await LegacySql.ReadResultAsync(
                reader,
                moveToData: false,
                cancellationToken);

            return result.Code == 0
                ? LegacyApiResponse.Success()
                : LegacyApiResponse.Error(result.Code, result.Message);
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid server user-joined request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in server user-joined endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled server user-joined endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }
}
