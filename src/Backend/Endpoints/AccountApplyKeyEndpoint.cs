using System.Data;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class AccountApplyKeyEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.AccountApplyKey");

        try
        {
            LegacyRequestParameters p =
                await LegacyRequestParameters.ReadAsync(context.Request, cancellationToken);

            await using SqlConnection connection =
                await LegacySql.OpenAsync(connections, cancellationToken);

            await using var command = new SqlCommand("dbo.WZ_ACCOUNT_APPLYKEY", connection)
            {
                CommandType = CommandType.StoredProcedure,
                CommandTimeout = 30
            };

            command.Parameters.Add("@in_CustomerID", SqlDbType.Int).Value = p.GetRequiredInt32("s_id");
            command.Parameters.Add("@in_SerialKey", SqlDbType.VarChar, 100).Value = p.GetRequired("serial");

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
            logger.LogWarning(exception, "Invalid account apply-key request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error while applying account key");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled account apply-key error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }
}
