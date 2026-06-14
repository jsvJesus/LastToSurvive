using System.Data;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class LoginSessionPollerEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger =
            loggerFactory.CreateLogger("Api.LoginSessionPoller");

        try
        {
            LegacyRequestParameters parameters =
                await LegacyRequestParameters.ReadAsync(
                    context.Request,
                    cancellationToken);

            int customerId =
                parameters.GetRequiredInt32("s_id");

            int sessionId =
                parameters.GetRequiredInt32("s_key");

            string remoteIp =
                context.Connection.RemoteIpAddress?.ToString()
                ?? "0.0.0.0";

            await using SqlConnection connection =
                connections.CreateConnection();

            await connection.OpenAsync(cancellationToken);

            await using var command =
                new SqlCommand(
                    "dbo.WZ_UpdateLoginSession",
                    connection)
                {
                    CommandType = CommandType.StoredProcedure,
                    CommandTimeout = 30
                };

            command.Parameters.Add(
                new SqlParameter(
                    "@in_IP",
                    SqlDbType.VarChar,
                    32)
                {
                    Value = remoteIp
                });

            command.Parameters.Add(
                new SqlParameter(
                    "@in_CustomerID",
                    SqlDbType.Int)
                {
                    Value = customerId
                });

            command.Parameters.Add(
                new SqlParameter(
                    "@in_SessionID",
                    SqlDbType.Int)
                {
                    Value = sessionId
                });

            await using SqlDataReader reader =
                await command.ExecuteReaderAsync(
                    cancellationToken);

            if (!await reader.ReadAsync(cancellationToken))
            {
                throw new LegacyApiException(
                    "ResultCode not set");
            }

            int resultCode =
                ReadResultCode(reader);

            if (resultCode != 0)
            {
                logger.LogWarning(
                    "Session validation failed for CustomerID={CustomerID}, ResultCode={ResultCode}",
                    customerId,
                    resultCode);

                return LegacyApiResponse.Error(resultCode);
            }

            logger.LogDebug(
                "Session updated for CustomerID={CustomerID}",
                customerId);

            return LegacyApiResponse.Success();
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(
                exception,
                "Invalid session poller request");

            return LegacyApiResponse.InternalError(
                exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(
                exception,
                "SQL error while updating login session");

            return LegacyApiResponse.InternalError(
                "SQL Select Failed");
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            logger.LogWarning(
                "Login session poller request was cancelled");

            return LegacyApiResponse.InternalError(
                "request cancelled");
        }
        catch (Exception exception)
        {
            logger.LogError(
                exception,
                "Unhandled login session poller error");

            return LegacyApiResponse.InternalError(
                "internal server error");
        }
    }

    private static int ReadResultCode(
        SqlDataReader reader)
    {
        try
        {
            int ordinal =
                reader.GetOrdinal("ResultCode");

            if (reader.IsDBNull(ordinal))
            {
                throw new LegacyApiException(
                    "ResultCode not set");
            }

            return Convert.ToInt32(
                reader.GetValue(ordinal),
                System.Globalization.CultureInfo.InvariantCulture);
        }
        catch (IndexOutOfRangeException exception)
        {
            throw new LegacyApiException(
                "ResultCode not set",
                exception);
        }
    }
}