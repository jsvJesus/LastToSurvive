using System.Data;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class ServerUserLeftEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        IConfiguration configuration,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.ServerUserLeft");

        try
        {
            LegacyRequestParameters parameters =
                await LegacyRequestParameters.ReadAsync(context.Request, cancellationToken);

            LegacyServerSecurity.ValidateApiKey(parameters, configuration);

            await using SqlConnection connection =
                await LegacySql.OpenAsync(connections, cancellationToken);

            await using var command = new SqlCommand(
                "dbo.WZ_SRV_UserLeftGame",
                connection)
            {
                CommandType = CommandType.StoredProcedure,
                CommandTimeout = 30
            };

            command.Parameters.Add("@in_CustomerID", SqlDbType.Int).Value = parameters.GetRequiredInt32("s_id");
            command.Parameters.Add("@in_CharID", SqlDbType.Int).Value = parameters.GetRequiredInt32("CharID");
            command.Parameters.Add("@in_GameMapId", SqlDbType.Int).Value = parameters.GetRequiredInt32("g1");
            command.Parameters.Add("@in_GameServerId", SqlDbType.Int).Value = parameters.GetRequiredInt32("g2");
            command.Parameters.Add("@in_TimePlayed", SqlDbType.Int).Value = parameters.GetRequiredInt32("s1");

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
            logger.LogWarning(exception, "Invalid server user-left request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in server user-left endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled server user-left endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }
}
