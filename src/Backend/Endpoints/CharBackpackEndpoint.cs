using System.Data;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class CharBackpackEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        IConfiguration configuration,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.CharBackpack");

        try
        {
            LegacyRequestParameters parameters =
                await LegacyRequestParameters.ReadAsync(
                    context.Request,
                    cancellationToken);

            int customerId = parameters.GetRequiredInt32("s_id");
            int sessionId = parameters.GetRequiredInt32("s_key");
            int charId = parameters.GetRequiredInt32("CharID");
            int operation = parameters.GetRequiredInt32("op");
            long value1 = parameters.GetRequiredInt64("v1");
            int value2 = parameters.GetRequiredInt32("v2");
            int value3 = parameters.GetRequiredInt32("v3");

            string remoteIp =
                context.Connection.RemoteIpAddress?.ToString() ?? "0.0.0.0";

            await using SqlConnection connection =
                await LegacySql.OpenAsync(connections, cancellationToken);

            LegacyProcedureResult sessionResult =
                await LegacySql.ValidateSessionAsync(
                    connection,
                    remoteIp,
                    customerId,
                    sessionId,
                    cancellationToken);

            if (sessionResult.Code != 0)
            {
                return LegacyApiResponse.Error(
                    sessionResult.Code,
                    sessionResult.Message);
            }

            if (operation >= 50)
            {
                string expectedKey =
                    configuration["Legacy:ServerApiKey"] ?? string.Empty;

                if (string.IsNullOrWhiteSpace(expectedKey))
                    throw new LegacyApiException("server api key is not configured");

                string suppliedKey = parameters.GetRequired("skey1");

                if (!string.Equals(
                        suppliedKey,
                        expectedKey,
                        StringComparison.Ordinal))
                {
                    throw new LegacyApiException("bad key");
                }
            }

            await using SqlCommand command = BuildCommand(
                connection,
                configuration,
                customerId,
                charId,
                operation,
                value1,
                value2,
                value3);

            await using SqlDataReader reader =
                await command.ExecuteReaderAsync(cancellationToken);

            LegacyProcedureResult result =
                await LegacySql.ReadResultAsync(
                    reader,
                    moveToData: true,
                    cancellationToken);

            if (result.Code != 0)
                return LegacyApiResponse.Error(result.Code, result.Message);

            if (!await reader.ReadAsync(cancellationToken))
                throw new LegacyApiException("InventoryID result row missing");

            long inventoryId = LegacySql.ReadInt64(reader, "InventoryID");

            logger.LogInformation(
                "Backpack operation {Operation} completed for CustomerID={CustomerID}, CharID={CharID}",
                operation,
                customerId,
                charId);

            return LegacyApiResponse.Success(inventoryId.ToString());
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid CharBackpack request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in CharBackpack endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            return LegacyApiResponse.InternalError("request cancelled");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled CharBackpack endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }

    private static SqlCommand BuildCommand(
        SqlConnection connection,
        IConfiguration configuration,
        int customerId,
        int charId,
        int operation,
        long value1,
        int value2,
        int value3)
    {
        string procedureName = operation switch
        {
            10 => "dbo.WZ_BackpackToInv",
            11 => "dbo.WZ_BackpackFromInv",
            12 => "dbo.WZ_BackpackGridSwap",
            13 => "dbo.WZ_BackpackGridJoin",
            16 => "dbo.WZ_BackpackChange",
            50 => "dbo.WZ_Backpack_SRV_AddItem",
            51 => configuration["Legacy:BackpackRemoveProcedure"]
                  ?? "dbo.WZ_Backpack_SRV_RemoveItem",
            56 => "dbo.WZ_Backpack_SRV_Change",
            _ => throw new LegacyApiException("bad op code")
        };

        var command = new SqlCommand(procedureName, connection)
        {
            CommandType = CommandType.StoredProcedure,
            CommandTimeout = 30
        };

        command.Parameters.Add("@in_CustomerID", SqlDbType.Int).Value = customerId;
        command.Parameters.Add("@in_CharID", SqlDbType.Int).Value = charId;

        switch (operation)
        {
            case 10:
            case 11:
                command.Parameters.Add("@in_InventoryID", SqlDbType.BigInt).Value = value1;
                command.Parameters.Add("@in_Slot", SqlDbType.Int).Value = value2;
                command.Parameters.Add("@in_Amount", SqlDbType.Int).Value = value3;
                break;

            case 12:
            case 13:
                command.Parameters.Add("@in_SlotFrom", SqlDbType.Int).Value = checked((int)value1);
                command.Parameters.Add("@in_SlotTo", SqlDbType.Int).Value = value2;
                break;

            case 16:
                command.Parameters.Add("@in_InventoryID", SqlDbType.BigInt).Value = value1;
                break;

            case 50:
            case 51:
                command.Parameters.Add("@in_ItemID", SqlDbType.Int).Value = checked((int)value1);
                command.Parameters.Add("@in_Slot", SqlDbType.Int).Value = value2;
                break;

            case 56:
                command.Parameters.Add("@in_BackpackID", SqlDbType.Int).Value = checked((int)value1);
                command.Parameters.Add("@in_BackpackSize", SqlDbType.Int).Value = value2;
                break;
        }

        return command;
    }
}
