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
        ILogger logger =
            loggerFactory.CreateLogger(
                "Api.AccountRegister");

        try
        {
            LegacyRequestParameters parameters =
                await LegacyRequestParameters.ReadAsync(
                    context.Request,
                    cancellationToken);

            string email =
                (
                    parameters.GetOptional("email")
                    ??
                    parameters.GetOptional("username")
                    ??
                    throw new LegacyApiException(
                        "no parameter email")
                ).Trim();

            string nickname =
                parameters
                    .GetRequired("nickname")
                    .Trim();

            string password =
                parameters.GetRequired("password");

            if (!IsValidEmail(email))
            {
                return LegacyApiResponse.Error(
                    2,
                    "Invalid email address");
            }

            if (!IsValidNickname(nickname))
            {
                return LegacyApiResponse.Error(
                    9,
                    "Nickname must contain 4-16 ASCII letters or digits");
            }

            if (password.Length is < 6 or > 64)
            {
                return LegacyApiResponse.Error(
                    4,
                    "Password must contain between 6 and 64 characters");
            }

            await using SqlConnection connection =
                await LegacySql.OpenAsync(
                    connections,
                    cancellationToken);

            await using var command =
                new SqlCommand(
                    "dbo.WZ_ACCOUNT_CREATE",
                    connection)
                {
                    CommandType =
                        CommandType.StoredProcedure,

                    CommandTimeout = 30
                };

            command.Parameters.Add(
                "@in_IP",
                SqlDbType.VarChar,
                64
            ).Value =
                context.Connection
                    .RemoteIpAddress?
                    .ToString()
                ?? "0.0.0.0";

            command.Parameters.Add(
                "@in_Email",
                SqlDbType.VarChar,
                128
            ).Value = email;

            command.Parameters.Add(
                "@in_NickName",
                SqlDbType.NVarChar,
                64
            ).Value = nickname;

            command.Parameters.Add(
                "@in_Password",
                SqlDbType.VarChar,
                64
            ).Value = password;

            command.Parameters.Add(
                "@in_ReferralID",
                SqlDbType.Int
            ).Value = 0;

            await using SqlDataReader reader =
                await command.ExecuteReaderAsync(
                    cancellationToken);

            LegacyProcedureResult result =
                await LegacySql.ReadResultAsync(
                    reader,
                    moveToData: true,
                    cancellationToken);

            if (result.Code != 0)
            {
                logger.LogWarning(
                    "Account registration rejected. Email={Email}, Nickname={Nickname}, ResultCode={ResultCode}, Message={Message}",
                    email,
                    nickname,
                    result.Code,
                    result.Message);

                return LegacyApiResponse.Error(
                    result.Code,
                    result.Message);
            }

            if (!await reader.ReadAsync(
                    cancellationToken))
            {
                throw new LegacyApiException(
                    "CustomerID result row missing");
            }

            int customerId =
                LegacySql.ReadInt32(
                    reader,
                    "CustomerID");

            int charId =
                LegacySql.ReadInt32(
                    reader,
                    "CharID");

            logger.LogInformation(
                "Account registered. CustomerID={CustomerID}, CharID={CharID}, Nickname={Nickname}",
                customerId,
                charId,
                nickname);

            return LegacyApiResponse.Success(
                customerId.ToString());
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(
                exception,
                "Invalid account registration request");

            return LegacyApiResponse.InternalError(
                exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(
                exception,
                "SQL error during account registration");

            return LegacyApiResponse.InternalError(
                "SQL Select Failed");
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            logger.LogWarning(
                "Account registration was cancelled");

            return LegacyApiResponse.InternalError(
                "request cancelled");
        }
        catch (Exception exception)
        {
            logger.LogError(
                exception,
                "Unhandled account registration error");

            return LegacyApiResponse.InternalError(
                "internal server error");
        }
    }

    private static bool IsValidEmail(
        string email)
    {
        if (email.Length is < 5 or > 128)
            return false;

        int atIndex =
            email.IndexOf('@');

        if (atIndex <= 0)
            return false;

        if (atIndex >= email.Length - 3)
            return false;

        return
            email.IndexOf(
                ' ',
                StringComparison.Ordinal) < 0;
    }

    private static bool IsValidNickname(
        string nickname)
    {
        if (nickname.Length is < 4 or > 16)
            return false;

        foreach (char character in nickname)
        {
            bool isLetter =
                character is >= 'A' and <= 'Z'
                ||
                character is >= 'a' and <= 'z';

            bool isDigit =
                character is >= '0' and <= '9';

            if (!isLetter && !isDigit)
                return false;
        }

        return true;
    }
}