using System.Data;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class AccountRegisterEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.AccountRegister");

        try
        {
            LegacyRequestParameters p =
                await LegacyRequestParameters.ReadAsync(context.Request, cancellationToken);

            await using SqlConnection connection =
                await LegacySql.OpenAsync(connections, cancellationToken);

            await using var command = new SqlCommand("dbo.WZ_ACCOUNT_CREATE", connection)
            {
                CommandType = CommandType.StoredProcedure,
                CommandTimeout = 30
            };

            command.Parameters.Add("@in_IP", SqlDbType.VarChar, 100).Value =
                context.Connection.RemoteIpAddress?.ToString() ?? "0.0.0.0";
            command.Parameters.Add("@in_EMail", SqlDbType.VarChar, 100).Value = p.GetRequired("username");
            command.Parameters.Add("@in_Password", SqlDbType.VarChar, 100).Value = p.GetRequired("password");
            command.Parameters.Add("@in_ReferralID", SqlDbType.Int).Value = 0;
            command.Parameters.Add("@in_SerialKey", SqlDbType.VarChar, 100).Value = p.GetRequired("serial");
            command.Parameters.Add("@in_SerialEmail", SqlDbType.VarChar, 100).Value = p.GetRequired("email");

            await using SqlDataReader reader =
                await command.ExecuteReaderAsync(cancellationToken);

            LegacyProcedureResult result = await LegacySql.ReadResultAsync(
                reader,
                moveToData: true,
                cancellationToken);

            if (result.Code != 0)
                return LegacyApiResponse.Error(result.Code, result.Message);

            if (!await reader.ReadAsync(cancellationToken))
                throw new LegacyApiException("CustomerID result row missing");

            return LegacyApiResponse.Success(
                LegacySql.ReadInt32(reader, "CustomerID").ToString());
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid account registration request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error during account registration");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled account registration error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }
}
