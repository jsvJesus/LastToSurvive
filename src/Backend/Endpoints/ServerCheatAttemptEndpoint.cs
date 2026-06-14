using System.Data;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class ServerCheatAttemptEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        IConfiguration configuration,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.ServerCheatAttempt");

        try
        {
            LegacyRequestParameters parameters =
                await LegacyRequestParameters.ReadAsync(context.Request, cancellationToken);

            LegacyServerSecurity.ValidateApiKey(parameters, configuration);

            await using SqlConnection connection =
                await LegacySql.OpenAsync(connections, cancellationToken);

            await using var command = new SqlCommand(
                "dbo.WZ_SRV_AddCheatAttempt",
                connection)
            {
                CommandType = CommandType.StoredProcedure,
                CommandTimeout = 30
            };

            command.Parameters.Add("@in_IP", SqlDbType.VarChar, 100).Value =
                context.Connection.RemoteIpAddress?.ToString() ?? "0.0.0.0";
            command.Parameters.Add("@in_CustomerID", SqlDbType.Int).Value =
                parameters.GetRequiredInt32("s_id");
            command.Parameters.Add("@in_GameSessionID", SqlDbType.Int).Value =
                parameters.GetRequiredInt32("GameSessionID");
            command.Parameters.Add("@in_CheatID", SqlDbType.Int).Value =
                parameters.GetRequiredInt32("CheatID");

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
            logger.LogWarning(exception, "Invalid server cheat-attempt request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in server cheat-attempt endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled server cheat-attempt endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }
}
