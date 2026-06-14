using System.Data;
using System.Globalization;
using System.Net;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class ServerLogInfoEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        IConfiguration configuration,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.ServerLogInfo");

        try
        {
            LegacyRequestParameters parameters =
                await LegacyRequestParameters.ReadAsync(context.Request, cancellationToken);

            LegacyServerSecurity.ValidateApiKey(parameters, configuration);

            long packedIp = long.Parse(
                parameters.GetRequired("IP"),
                NumberStyles.Integer,
                CultureInfo.InvariantCulture);

            string customerIp = new IPAddress(packedIp).ToString();

            await using SqlConnection connection =
                await LegacySql.OpenAsync(connections, cancellationToken);

            await using var command = new SqlCommand(
                "dbo.WZ_SRV_AddLogInfo",
                connection)
            {
                CommandType = CommandType.StoredProcedure,
                CommandTimeout = 30
            };

            command.Parameters.Add("@in_CustomerID", SqlDbType.Int).Value = parameters.GetRequiredInt32("s_id");
            command.Parameters.Add("@in_CharID", SqlDbType.Int).Value = parameters.GetRequiredInt32("CharID");
            command.Parameters.Add("@in_Gamertag", SqlDbType.NVarChar, 64).Value = parameters.GetRequired("Gamertag");
            command.Parameters.Add("@in_GameSessionID", SqlDbType.Int).Value = parameters.GetRequiredInt32("GameSessionID");
            command.Parameters.Add("@in_CustomerIP", SqlDbType.VarChar, 64).Value = customerIp;
            command.Parameters.Add("@in_CheatID", SqlDbType.Int).Value = parameters.GetRequiredInt32("CheatID");
            command.Parameters.Add("@in_Msg", SqlDbType.NVarChar, 1024).Value = parameters.GetRequired("Msg");
            command.Parameters.Add("@in_Data", SqlDbType.NVarChar, -1).Value = parameters.GetRequired("Data");

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
            logger.LogWarning(exception, "Invalid server log-info request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in server log-info endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled server log-info endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }
}
