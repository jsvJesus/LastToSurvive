using System.Data;
using System.Globalization;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class CharSlotsEndpoint
{
    private const string ForbiddenGamertagCharacters =
        "!@#$%^&*()-=+_<>,./?'\":;|{}[]";

    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger =
            loggerFactory.CreateLogger("Api.CharSlots");

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

            string function =
                parameters.GetRequired("func");

            string remoteIp =
                context.Connection.RemoteIpAddress?.ToString()
                ?? "0.0.0.0";

            await using SqlConnection connection =
                connections.CreateConnection();

            await connection.OpenAsync(cancellationToken);

            ProcedureResult sessionResult =
                await ValidateSessionAsync(
                    connection,
                    remoteIp,
                    customerId,
                    sessionId,
                    cancellationToken);

            if (sessionResult.Code != 0)
            {
                logger.LogWarning(
                    "CharSlots session rejected. CustomerID={CustomerID}, ResultCode={ResultCode}",
                    customerId,
                    sessionResult.Code);

                return LegacyApiResponse.Error(
                    sessionResult.Code,
                    sessionResult.Message);
            }

            return function switch
            {
                "create" =>
                    LegacyApiResponse.Error(
                        7,
                        "Additional character creation is disabled"),

                "delete" =>
                    LegacyApiResponse.Error(
                        7,
                        "Permanent survivor cannot be deleted"),

                "rename" => await RenameCharacterAsync(
                    connection,
                    parameters,
                    customerId,
                    logger,
                    cancellationToken),

                "revive" => await ReviveCharacterAsync(
                    connection,
                    parameters,
                    customerId,
                    logger,
                    cancellationToken),

                _ => throw new LegacyApiException(
                    "bad func")
            };
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(
                exception,
                "Invalid CharSlots request");

            return LegacyApiResponse.InternalError(
                exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(
                exception,
                "SQL error in CharSlots endpoint");

            return LegacyApiResponse.InternalError(
                "SQL Select Failed");
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            logger.LogWarning(
                "CharSlots request was cancelled");

            return LegacyApiResponse.InternalError(
                "request cancelled");
        }
        catch (Exception exception)
        {
            logger.LogError(
                exception,
                "Unhandled CharSlots endpoint error");

            return LegacyApiResponse.InternalError(
                "internal server error");
        }
    }
    
    private static async Task<IResult> RenameCharacterAsync(
        SqlConnection connection,
        LegacyRequestParameters parameters,
        int customerId,
        ILogger logger,
        CancellationToken cancellationToken)
    {
        int charId =
            parameters.GetRequiredInt32(
                "CharID");

        string gamertag =
            parameters
                .GetRequired("Gamertag")
                .Trim();

        if (gamertag.Length is < 4 or > 16)
        {
            return LegacyApiResponse.Error(
                9,
                "Nickname must contain between 4 and 16 characters");
        }

        foreach (char character in gamertag)
        {
            bool isLetter =
                character is >= 'A' and <= 'Z'
                ||
                character is >= 'a' and <= 'z';

            bool isDigit =
                character is >= '0' and <= '9';

            if (!isLetter && !isDigit)
            {
                return LegacyApiResponse.Error(
                    9,
                    "Nickname contains invalid characters");
            }
        }

        await using var command =
            new SqlCommand(
                "dbo.WZ_CharRename",
                connection)
            {
                CommandType =
                    CommandType.StoredProcedure,

                CommandTimeout = 30
            };

        command.Parameters.Add(
            "@in_CustomerID",
            SqlDbType.Int
        ).Value = customerId;

        command.Parameters.Add(
            "@in_CharID",
            SqlDbType.Int
        ).Value = charId;

        command.Parameters.Add(
            "@in_Gamertag",
            SqlDbType.NVarChar,
            64
        ).Value = gamertag;

        await using SqlDataReader reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        ProcedureResult procedureResult =
            await ReadProcedureResultAsync(
                reader,
                cancellationToken);

        if (procedureResult.Code != 0)
        {
            logger.LogWarning(
                "Character rename failed. CustomerID={CustomerID}, CharID={CharID}, ResultCode={ResultCode}, Message={Message}",
                customerId,
                charId,
                procedureResult.Code,
                procedureResult.Message);

            return LegacyApiResponse.Error(
                procedureResult.Code,
                procedureResult.Message);
        }

        logger.LogInformation(
            "Character renamed. CustomerID={CustomerID}, CharID={CharID}, Gamertag={Gamertag}",
            customerId,
            charId,
            gamertag);

        return LegacyApiResponse.Success();
    }

    private static async Task<IResult> CreateCharacterAsync(
        SqlConnection connection,
        LegacyRequestParameters parameters,
        int customerId,
        ILogger logger,
        CancellationToken cancellationToken)
    {
        string gamertag =
            parameters.GetRequired("Gamertag");

        if (gamertag.IndexOfAny(
                ForbiddenGamertagCharacters.ToCharArray()) >= 0)
        {
            return LegacyApiResponse.Raw(
                "WO_7Character name cannot contain special symbols");
        }

        int hardcore =
            parameters.GetRequiredInt32("Hardcore");

        int heroItemId =
            parameters.GetRequiredInt32("HeroItemID");

        int headIndex =
            parameters.GetRequiredInt32("HeadIdx");

        int bodyIndex =
            parameters.GetRequiredInt32("BodyIdx");

        int legsIndex =
            parameters.GetRequiredInt32("LegsIdx");

        await using var command =
            new SqlCommand(
                "dbo.WZ_CharCreate",
                connection)
            {
                CommandType = CommandType.StoredProcedure,
                CommandTimeout = 30
            };

        command.Parameters.Add(
            "@in_CustomerID",
            SqlDbType.Int).Value = customerId;

        command.Parameters.Add(
            "@in_Hardcore",
            SqlDbType.Int).Value = hardcore;

        command.Parameters.Add(
            "@in_Gamertag",
            SqlDbType.NVarChar,
            64).Value = gamertag;

        command.Parameters.Add(
            "@in_HeroItemID",
            SqlDbType.Int).Value = heroItemId;

        command.Parameters.Add(
            "@in_HeadIdx",
            SqlDbType.Int).Value = headIndex;

        command.Parameters.Add(
            "@in_BodyIdx",
            SqlDbType.Int).Value = bodyIndex;

        command.Parameters.Add(
            "@in_LegsIdx",
            SqlDbType.Int).Value = legsIndex;

        await using SqlDataReader reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        ProcedureResult procedureResult =
            await ReadProcedureResultAsync(
                reader,
                cancellationToken);

        if (procedureResult.Code != 0)
        {
            logger.LogWarning(
                "Character creation failed. CustomerID={CustomerID}, ResultCode={ResultCode}, Message={Message}",
                customerId,
                procedureResult.Code,
                procedureResult.Message);

            return LegacyApiResponse.Error(
                procedureResult.Code,
                procedureResult.Message);
        }

        if (!await reader.NextResultAsync(
                cancellationToken))
        {
            throw new LegacyApiException(
                "character result set missing");
        }

        if (!await reader.ReadAsync(
                cancellationToken))
        {
            throw new LegacyApiException(
                "character result row missing");
        }

        int charId =
            ReadRequiredInt32(
                reader,
                "CharID");

        logger.LogInformation(
            "Character created. CustomerID={CustomerID}, CharID={CharID}, Gamertag={Gamertag}",
            customerId,
            charId,
            gamertag);

        return LegacyApiResponse.Success(
            charId.ToString(
                CultureInfo.InvariantCulture));
    }

    private static async Task<IResult> DeleteCharacterAsync(
        SqlConnection connection,
        LegacyRequestParameters parameters,
        int customerId,
        ILogger logger,
        CancellationToken cancellationToken)
    {
        int charId =
            parameters.GetRequiredInt32("CharID");

        await using var command =
            new SqlCommand(
                "dbo.WZ_CharDelete",
                connection)
            {
                CommandType = CommandType.StoredProcedure,
                CommandTimeout = 30
            };

        command.Parameters.Add(
            "@in_CustomerID",
            SqlDbType.Int).Value = customerId;

        command.Parameters.Add(
            "@in_CharID",
            SqlDbType.Int).Value = charId;

        await using SqlDataReader reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        ProcedureResult procedureResult =
            await ReadProcedureResultAsync(
                reader,
                cancellationToken);

        if (procedureResult.Code != 0)
        {
            logger.LogWarning(
                "Character deletion failed. CustomerID={CustomerID}, CharID={CharID}, ResultCode={ResultCode}, Message={Message}",
                customerId,
                charId,
                procedureResult.Code,
                procedureResult.Message);

            return LegacyApiResponse.Error(
                procedureResult.Code,
                procedureResult.Message);
        }

        logger.LogInformation(
            "Character deleted. CustomerID={CustomerID}, CharID={CharID}",
            customerId,
            charId);

        return LegacyApiResponse.Success();
    }

    private static async Task<IResult> ReviveCharacterAsync(
        SqlConnection connection,
        LegacyRequestParameters parameters,
        int customerId,
        ILogger logger,
        CancellationToken cancellationToken)
    {
        int charId =
            parameters.GetRequiredInt32("CharID");

        await using var command =
            new SqlCommand(
                "dbo.WZ_CharRevive",
                connection)
            {
                CommandType = CommandType.StoredProcedure,
                CommandTimeout = 30
            };

        command.Parameters.Add(
            "@in_CustomerID",
            SqlDbType.Int).Value = customerId;

        command.Parameters.Add(
            "@in_CharID",
            SqlDbType.Int).Value = charId;

        await using SqlDataReader reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        ProcedureResult procedureResult =
            await ReadProcedureResultAsync(
                reader,
                cancellationToken);

        if (procedureResult.Code != 0)
        {
            logger.LogWarning(
                "Character revive failed. CustomerID={CustomerID}, CharID={CharID}, ResultCode={ResultCode}, Message={Message}",
                customerId,
                charId,
                procedureResult.Code,
                procedureResult.Message);

            return LegacyApiResponse.Error(
                procedureResult.Code,
                procedureResult.Message);
        }

        logger.LogInformation(
            "Character revived. CustomerID={CustomerID}, CharID={CharID}",
            customerId,
            charId);

        return LegacyApiResponse.Success();
    }

    private static async Task<ProcedureResult> ValidateSessionAsync(
        SqlConnection connection,
        string remoteIp,
        int customerId,
        int sessionId,
        CancellationToken cancellationToken)
    {
        await using var command =
            new SqlCommand(
                "dbo.WZ_UpdateLoginSession",
                connection)
            {
                CommandType = CommandType.StoredProcedure,
                CommandTimeout = 30
            };

        command.Parameters.Add(
            "@in_IP",
            SqlDbType.VarChar,
            32).Value = remoteIp;

        command.Parameters.Add(
            "@in_CustomerID",
            SqlDbType.Int).Value = customerId;

        command.Parameters.Add(
            "@in_SessionID",
            SqlDbType.Int).Value = sessionId;

        await using SqlDataReader reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        return await ReadProcedureResultAsync(
            reader,
            cancellationToken);
    }

    private static async Task<ProcedureResult> ReadProcedureResultAsync(
        SqlDataReader reader,
        CancellationToken cancellationToken)
    {
        if (!await reader.ReadAsync(
                cancellationToken))
        {
            throw new LegacyApiException(
                "ResultCode not set");
        }

        int resultCode =
            ReadRequiredInt32(
                reader,
                "ResultCode");

        string resultMessage =
            TryReadString(
                reader,
                "ResultMsg") ?? "";

        return new ProcedureResult(
            resultCode,
            resultMessage);
    }

    private static int ReadRequiredInt32(
        SqlDataReader reader,
        string columnName)
    {
        try
        {
            int ordinal =
                reader.GetOrdinal(columnName);

            if (reader.IsDBNull(ordinal))
            {
                throw new LegacyApiException(
                    $"bad int field {columnName}");
            }

            return Convert.ToInt32(
                reader.GetValue(ordinal),
                CultureInfo.InvariantCulture);
        }
        catch (IndexOutOfRangeException exception)
        {
            throw new LegacyApiException(
                $"bad int field {columnName}",
                exception);
        }
        catch (FormatException exception)
        {
            throw new LegacyApiException(
                $"bad int field {columnName}",
                exception);
        }
        catch (InvalidCastException exception)
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
            int ordinal =
                reader.GetOrdinal(columnName);

            if (reader.IsDBNull(ordinal))
            {
                return null;
            }

            return reader
                .GetValue(ordinal)
                .ToString();
        }
        catch (IndexOutOfRangeException)
        {
            return null;
        }
    }

    private readonly record struct ProcedureResult(
        int Code,
        string Message);
}