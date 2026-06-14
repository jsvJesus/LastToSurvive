using System.Data;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class GetShopEndpoint
{
    private static readonly byte[] Header = { 83, 72, 79, 49 };

    public static async Task<IResult> ExecuteAsync(
        SqlConnectionFactory connections,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.GetShop");

        try
        {
            await using SqlConnection connection =
                await LegacySql.OpenAsync(connections, cancellationToken);

            await using var command = new SqlCommand(
                "dbo.WZ_GetShopInfo1",
                connection)
            {
                CommandType = CommandType.StoredProcedure,
                CommandTimeout = 30
            };

            await using SqlDataReader reader =
                await command.ExecuteReaderAsync(cancellationToken);

            LegacyProcedureResult result =
                await LegacySql.ReadResultAsync(
                    reader,
                    moveToData: true,
                    cancellationToken);

            if (result.Code != 0)
                return LegacyApiResponse.Error(result.Code, result.Message);

            using var payload = new MemoryStream();
            payload.Write(Header, 0, Header.Length);

            while (await reader.ReadAsync(cancellationToken))
            {
                int gameDollarPrice = LegacySql.ReadInt32(reader, "PriceP");
                int gamePointPrice = LegacySql.ReadInt32(reader, "GPriceP");

                if (gameDollarPrice == 0 && gamePointPrice == 0)
                    continue;

                int itemId = LegacySql.ReadInt32(reader, "ItemId");
                byte flags = LegacySql.ReadInt32(reader, "IsNew") > 0
                    ? (byte)1
                    : (byte)0;

                WriteInt32(payload, itemId);
                payload.WriteByte(flags);
                WriteInt32(payload, gameDollarPrice);
                WriteInt32(payload, gamePointPrice);
            }

            payload.Write(Header, 0, Header.Length);

            byte[] prefix = System.Text.Encoding.UTF8.GetBytes("WO_0");
            byte[] shopData = payload.ToArray();
            byte[] response = new byte[prefix.Length + shopData.Length];

            Buffer.BlockCopy(prefix, 0, response, 0, prefix.Length);
            Buffer.BlockCopy(shopData, 0, response, prefix.Length, shopData.Length);

            return LegacyPayloadResponse.FromBytes(response);
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid GetShop response");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in GetShop endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled GetShop endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }

    private static void WriteInt32(Stream stream, int value)
    {
        stream.WriteByte((byte)(value & 0xff));
        stream.WriteByte((byte)((value >> 8) & 0xff));
        stream.WriteByte((byte)((value >> 16) & 0xff));
        stream.WriteByte((byte)((value >> 24) & 0xff));
    }
}
