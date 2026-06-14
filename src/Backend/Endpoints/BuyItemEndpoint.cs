using System.Data;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class BuyItemEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        IConfiguration configuration,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.BuyItem");

        try
        {
            LegacyRequestParameters parameters =
                await LegacyRequestParameters.ReadAsync(
                    context.Request,
                    cancellationToken);

            int customerId = parameters.GetRequiredInt32("s_id");
            int sessionId = parameters.GetRequiredInt32("s_key");
            int itemId = parameters.GetRequiredInt32("ItemID");
            int buyIndex = parameters.GetRequiredInt32("BuyIdx");

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
                return LegacyApiResponse.Error(sessionResult.Code, sessionResult.Message);

            string region = configuration["Legacy:Region"] ?? "US";
            string procedureName = ResolveProcedure(buyIndex, region);
            int buyDays = ResolveBuyDays(buyIndex);

            await using var command = new SqlCommand(procedureName, connection)
            {
                CommandType = CommandType.StoredProcedure,
                CommandTimeout = 30
            };

            command.Parameters.Add("@in_IP", SqlDbType.VarChar, 100).Value = remoteIp;
            command.Parameters.Add("@in_CustomerID", SqlDbType.Int).Value = customerId;
            command.Parameters.Add("@in_ItemId", SqlDbType.Int).Value = itemId;
            command.Parameters.Add("@in_BuyDays", SqlDbType.Int).Value = buyDays;

            await using SqlDataReader reader =
                await command.ExecuteReaderAsync(cancellationToken);

            LegacyProcedureResult result = await LegacySql.ReadResultAsync(
                reader,
                moveToData: true,
                cancellationToken);

            if (result.Code != 0)
                return LegacyApiResponse.Error(result.Code, result.Message);

            if (!await reader.ReadAsync(cancellationToken))
                throw new LegacyApiException("purchase result row missing");

            int balance = LegacySql.ReadInt32(reader, "Balance");
            long inventoryId = LegacySql.ReadInt64(reader, "InventoryID");

            logger.LogInformation(
                "CustomerID={CustomerID} bought ItemID={ItemID}, BuyIdx={BuyIndex}",
                customerId,
                itemId,
                buyIndex);

            return LegacyApiResponse.Success($"{balance} {inventoryId}");
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid BuyItem request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in BuyItem endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            return LegacyApiResponse.InternalError("request cancelled");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled BuyItem endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }

    private static string ResolveProcedure(int buyIndex, string region)
    {
        if (buyIndex is >= 5 and <= 8)
            return "dbo.WZ_BuyItem_GD";

        if (buyIndex is >= 1 and <= 4)
        {
            if (string.Equals(region, "RU", StringComparison.OrdinalIgnoreCase))
                throw new LegacyApiException("GP Buy in russian region");

            return "dbo.WZ_BuyItem_GP";
        }

        if (buyIndex is >= 9 and <= 12)
        {
            if (!string.Equals(region, "RU", StringComparison.OrdinalIgnoreCase))
                throw new LegacyApiException("Gamenet Buy not in russian region");

            return "dbo.WZ_BuyItem_GNA";
        }

        throw new LegacyApiException("bad BuyIdx");
    }

    private static int ResolveBuyDays(int buyIndex)
    {
        return buyIndex switch
        {
            1 or 5 or 9 => 1,
            2 or 6 or 10 => 7,
            3 or 7 or 11 => 30,
            4 or 8 or 12 => 2000,
            _ => throw new LegacyApiException("bad BuyIdx")
        };
    }
}
