using System.Data;
using System.Globalization;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class ServerCharacterUpdateEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        IConfiguration configuration,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.ServerCharacterUpdate");

        try
        {
            LegacyRequestParameters parameters =
                await LegacyRequestParameters.ReadAsync(context.Request, cancellationToken);

            LegacyServerSecurity.ValidateApiKey(parameters, configuration);

            int customerId = parameters.GetRequiredInt32("s_id");
            int sessionId = parameters.GetRequiredInt32("s_key");
            int charId = parameters.GetRequiredInt32("CharID");
            string remoteIp = context.Connection.RemoteIpAddress?.ToString() ?? "0.0.0.0";

            await using SqlConnection connection =
                await LegacySql.OpenAsync(connections, cancellationToken);

            LegacyProcedureResult sessionResult = await LegacySql.ValidateSessionAsync(
                connection,
                remoteIp,
                customerId,
                sessionId,
                cancellationToken);

            if (sessionResult.Code != 0)
                return LegacyApiResponse.Error(sessionResult.Code, sessionResult.Message);

            LegacyProcedureResult statusResult = await UpdateStatusAsync(
                connection,
                parameters,
                customerId,
                charId,
                cancellationToken);

            if (statusResult.Code != 0)
                return LegacyApiResponse.Error(statusResult.Code, statusResult.Message);

            LegacyProcedureResult backpackResult = await UpdateBackpackAsync(
                connection,
                parameters,
                customerId,
                charId,
                cancellationToken);

            if (backpackResult.Code != 0)
                return LegacyApiResponse.Error(backpackResult.Code, backpackResult.Message);

            LegacyProcedureResult attachmentResult = await UpdateAttachmentsAsync(
                connection,
                parameters,
                customerId,
                charId,
                cancellationToken);

            return attachmentResult.Code == 0
                ? LegacyApiResponse.Success()
                : LegacyApiResponse.Error(
                    attachmentResult.Code,
                    attachmentResult.Message);
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid server character-update request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in server character-update endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled server character-update endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }

    private static async Task<LegacyProcedureResult> UpdateStatusAsync(
        SqlConnection connection,
        LegacyRequestParameters parameters,
        int customerId,
        int charId,
        CancellationToken cancellationToken)
    {
        await using var command = new SqlCommand(
            "dbo.WZ_Char_SRV_SetStatus",
            connection)
        {
            CommandType = CommandType.StoredProcedure,
            CommandTimeout = 30
        };

        command.Parameters.Add("@in_CustomerID", SqlDbType.Int).Value = customerId;
        command.Parameters.Add("@in_CharID", SqlDbType.Int).Value = charId;

        AddText(command, "@in_Alive", parameters.GetRequired("s1"));
        AddText(command, "@in_GamePos", parameters.GetRequired("s2"));
        AddText(command, "@in_Health", parameters.GetRequired("s3"));
        AddText(command, "@in_Hunger", parameters.GetRequired("s4"));
        AddText(command, "@in_Thirst", parameters.GetRequired("s5"));
        AddText(command, "@in_Toxic", parameters.GetRequired("s6"));
        AddText(command, "@in_TimePlayed", parameters.GetRequired("s7"));
        AddText(command, "@in_XP", parameters.GetRequired("s8"));
        AddText(command, "@in_Reputation", parameters.GetRequired("s9"));
        AddText(command, "@in_GameFlags", parameters.GetRequired("sA"));
        AddText(command, "@in_GameDollars", parameters.GetRequired("sB"));
        AddText(command, "@in_Stat00", parameters.GetRequired("ts00"));
        AddText(command, "@in_Stat01", parameters.GetRequired("ts01"));
        AddText(command, "@in_Stat02", parameters.GetRequired("ts02"));
        AddText(command, "@in_Stat03", parameters.GetRequired("ts03"));
        AddText(command, "@in_Stat04", parameters.GetRequired("ts04"));
        AddText(command, "@in_Stat05", parameters.GetRequired("ts05"));

        await using SqlDataReader reader =
            await command.ExecuteReaderAsync(cancellationToken);

        return await LegacySql.ReadResultAsync(
            reader,
            moveToData: false,
            cancellationToken);
    }

    private static async Task<LegacyProcedureResult> UpdateBackpackAsync(
        SqlConnection connection,
        LegacyRequestParameters parameters,
        int customerId,
        int charId,
        CancellationToken cancellationToken)
    {
        await using SqlTransaction transaction =
            (SqlTransaction)await connection.BeginTransactionAsync(cancellationToken);

        try
        {
            for (int index = 0; index < 999; index++)
            {
                string? packed = parameters.GetOptional($"bp{index}");
                if (string.IsNullOrWhiteSpace(packed))
                    break;

                string[] values = packed.Split(
                    ' ',
                    StringSplitOptions.RemoveEmptyEntries);

                if (values.Length != 6)
                    throw new LegacyApiException("bad BpEntry");

                int slot = Parse(values[0], "slot");
                int operation = Parse(values[1], "operation");
                int itemId = Parse(values[2], "item id");
                int amount = Parse(values[3], "amount");
                int var1 = Parse(values[4], "var1");
                int var2 = Parse(values[5], "var2");

                string procedure = operation switch
                {
                    0 => "dbo.WZ_Backpack_SRV_AddItem",
                    1 => "dbo.WZ_Backpack_SRV_AlterItem",
                    2 => "dbo.WZ_Backpack_SRV_DeleteItem",
                    _ => throw new LegacyApiException("bad op")
                };

                await using var command = new SqlCommand(
                    procedure,
                    connection,
                    transaction)
                {
                    CommandType = CommandType.StoredProcedure,
                    CommandTimeout = 30
                };

                command.Parameters.Add("@in_CustomerID", SqlDbType.Int).Value = customerId;
                command.Parameters.Add("@in_CharID", SqlDbType.Int).Value = charId;
                command.Parameters.Add("@in_Slot", SqlDbType.Int).Value = slot;
                command.Parameters.Add("@in_ItemID", SqlDbType.Int).Value = itemId;
                command.Parameters.Add("@in_Amount", SqlDbType.Int).Value = amount;
                command.Parameters.Add("@in_Var1", SqlDbType.Int).Value = var1;
                command.Parameters.Add("@in_Var2", SqlDbType.Int).Value = var2;

                await using SqlDataReader reader =
                    await command.ExecuteReaderAsync(cancellationToken);

                LegacyProcedureResult result = await LegacySql.ReadResultAsync(
                    reader,
                    moveToData: false,
                    cancellationToken);

                if (result.Code != 0)
                {
                    await transaction.RollbackAsync(cancellationToken);
                    return result;
                }
            }

            await transaction.CommitAsync(cancellationToken);
            return new LegacyProcedureResult(0, string.Empty);
        }
        catch
        {
            try
            {
                await transaction.RollbackAsync(cancellationToken);
            }
            catch
            {
            }

            throw;
        }
    }

    private static async Task<LegacyProcedureResult> UpdateAttachmentsAsync(
        SqlConnection connection,
        LegacyRequestParameters parameters,
        int customerId,
        int charId,
        CancellationToken cancellationToken)
    {
        await using var command = new SqlCommand(
            "dbo.WZ_Char_SRV_SetAttachments",
            connection)
        {
            CommandType = CommandType.StoredProcedure,
            CommandTimeout = 30
        };

        command.Parameters.Add("@in_CustomerID", SqlDbType.Int).Value = customerId;
        command.Parameters.Add("@in_CharID", SqlDbType.Int).Value = charId;
        command.Parameters.Add("@in_Attm1", SqlDbType.VarChar, 512).Value = parameters.GetRequired("attm1");
        command.Parameters.Add("@in_Attm2", SqlDbType.VarChar, 512).Value = parameters.GetRequired("attm2");

        await using SqlDataReader reader =
            await command.ExecuteReaderAsync(cancellationToken);

        return await LegacySql.ReadResultAsync(
            reader,
            moveToData: false,
            cancellationToken);
    }

    private static void AddText(
        SqlCommand command,
        string name,
        string value)
    {
        command.Parameters.Add(name, SqlDbType.VarChar, 512).Value = value;
    }

    private static int Parse(string value, string name)
    {
        if (!int.TryParse(
                value,
                NumberStyles.Integer,
                CultureInfo.InvariantCulture,
                out int result))
        {
            throw new LegacyApiException($"bad {name}");
        }

        return result;
    }
}
