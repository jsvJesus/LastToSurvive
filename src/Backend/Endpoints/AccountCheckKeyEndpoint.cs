using System.Data;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class AccountCheckKeyEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.AccountCheckKey");

        try
        {
            LegacyRequestParameters p =
                await LegacyRequestParameters.ReadAsync(context.Request, cancellationToken);

            await using SqlConnection connection =
                await LegacySql.OpenAsync(connections, cancellationToken);

            await using var command = new SqlCommand(
                "[BreezeNet].[dbo].BN_WarZ_SerialCheck",
                connection)
            {
                CommandType = CommandType.StoredProcedure,
                CommandTimeout = 30
            };

            command.Parameters.Add("@in_EMail", SqlDbType.VarChar, 100).Value = p.GetRequired("email");
            command.Parameters.Add("@in_SerialKey", SqlDbType.VarChar, 100).Value = p.GetRequired("serial");

            await using SqlDataReader reader =
                await command.ExecuteReaderAsync(cancellationToken);

            LegacyProcedureResult result = await LegacySql.ReadResultAsync(
                reader,
                moveToData: true,
                cancellationToken);

            if (result.Code != 0)
                return LegacyApiResponse.Error(result.Code, result.Message);

            if (!await reader.ReadAsync(cancellationToken))
                throw new LegacyApiException("serial check result row missing");

            int checkCode = LegacySql.ReadInt32(reader, "CheckCode");
            int serialType = LegacySql.ReadInt32(reader, "SerialType");
            string message = LegacySql.TryReadString(reader, "CheckMsg") ?? string.Empty;

            return LegacyApiResponse.Success($"{checkCode} {serialType} :{message}");
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid account key-check request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error while checking account key");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled account key-check error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }
}
