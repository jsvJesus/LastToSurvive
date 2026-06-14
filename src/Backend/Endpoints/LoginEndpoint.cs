using System.Data;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class LoginEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger =
            loggerFactory.CreateLogger("Api.Login");

        try
        {
            LegacyRequestParameters parameters =
                await LegacyRequestParameters.ReadAsync(
                    context.Request,
                    cancellationToken);

            string username =
                parameters.GetRequired("username");

            string password =
                parameters.GetRequired("password");

            string remoteIp =
                context.Connection.RemoteIpAddress?.ToString()
                ?? "0.0.0.0";

            await using SqlConnection connection =
                connections.CreateConnection();

            await connection.OpenAsync(cancellationToken);

            await using var command =
                new SqlCommand("dbo.WZ_ACCOUNT_LOGIN", connection)
                {
                    CommandType = CommandType.StoredProcedure,
                    CommandTimeout = 30
                };

            command.Parameters.Add(
                new SqlParameter("@in_IP", SqlDbType.VarChar, 100)
                {
                    Value = remoteIp
                });

            command.Parameters.Add(
                new SqlParameter("@in_EMail", SqlDbType.VarChar, 100)
                {
                    Value = username
                });

            command.Parameters.Add(
                new SqlParameter("@in_Password", SqlDbType.VarChar, 100)
                {
                    Value = password
                });

            await using SqlDataReader reader =
                await command.ExecuteReaderAsync(cancellationToken);

            /*
             * Первый result set процедуры:
             *
             * ResultCode
             * ResultMsg — может отсутствовать.
             */
            if (!await reader.ReadAsync(cancellationToken))
            {
                throw new LegacyApiException(
                    "ResultCode not set");
            }

            int resultCode =
                ReadRequiredInt32(reader, "ResultCode");

            if (resultCode != 0)
            {
                string resultMessage =
                    TryReadString(reader, "ResultMsg") ?? "";

                logger.LogWarning(
                    "Login stored procedure returned result code {ResultCode}",
                    resultCode);

                return LegacyApiResponse.Error(
                    resultCode,
                    resultMessage);
            }

            /*
             * Второй result set:
             *
             * CustomerID
             * AccountStatus
             * SessionID — присутствует при найденном аккаунте.
             * IsDeveloper — старый endpoint читал, но клиенту не возвращал.
             */
            if (!await reader.NextResultAsync(cancellationToken))
            {
                throw new LegacyApiException(
                    "Login result set was not returned");
            }

            if (!await reader.ReadAsync(cancellationToken))
            {
                throw new LegacyApiException(
                    "Login result row was not returned");
            }

            int customerId =
                ReadRequiredInt32(reader, "CustomerID");

            int accountStatus =
                ReadRequiredInt32(reader, "AccountStatus");

            int sessionId = 0;

            if (customerId > 0)
            {
                sessionId =
                    ReadRequiredInt32(reader, "SessionID");
            }

            logger.LogInformation(
                "Login completed for CustomerID={CustomerID}, AccountStatus={AccountStatus}, IP={RemoteIp}",
                customerId,
                accountStatus,
                remoteIp);

            /*
             * Важно: пробела после WO_0 нет.
             *
             * Полный ответ:
             * WO_0123 456 100
             *
             * Клиент удаляет первые четыре символа WO_0,
             * после чего получает:
             * 123 456 100
             */
            return LegacyApiResponse.Success(
                $"{customerId} {sessionId} {accountStatus}");
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(
                exception,
                "Invalid login API request");

            return LegacyApiResponse.InternalError(
                exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(
                exception,
                "SQL error while executing login");

            return LegacyApiResponse.InternalError(
                "SQL Select Failed");
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            logger.LogWarning(
                "Login request was cancelled");

            return LegacyApiResponse.InternalError(
                "request cancelled");
        }
        catch (Exception exception)
        {
            logger.LogError(
                exception,
                "Unhandled login endpoint error");

            return LegacyApiResponse.InternalError(
                "internal server error");
        }
    }

    private static int ReadRequiredInt32(
        SqlDataReader reader,
        string columnName)
    {
        int ordinal;

        try
        {
            ordinal = reader.GetOrdinal(columnName);
        }
        catch (IndexOutOfRangeException exception)
        {
            throw new LegacyApiException(
                $"bad int field {columnName}",
                exception);
        }

        if (reader.IsDBNull(ordinal))
        {
            throw new LegacyApiException(
                $"bad int field {columnName}");
        }

        try
        {
            return Convert.ToInt32(
                reader.GetValue(ordinal),
                System.Globalization.CultureInfo.InvariantCulture);
        }
        catch (Exception exception)
        {
            throw new LegacyApiException(
                $"bad int field {columnName}",
                exception);
        }
    }

    private static string? TryReadString(
        SqlDataReader reader,
        string columnName)
    {
        try
        {
            int ordinal = reader.GetOrdinal(columnName);

            return reader.IsDBNull(ordinal)
                ? null
                : reader.GetValue(ordinal).ToString();
        }
        catch (IndexOutOfRangeException)
        {
            return null;
        }
    }
}