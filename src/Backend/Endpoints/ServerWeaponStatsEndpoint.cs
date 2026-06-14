using System.Data;
using System.Globalization;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class ServerWeaponStatsEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        IConfiguration configuration,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.ServerWeaponStats");

        try
        {
            LegacyRequestParameters parameters =
                await LegacyRequestParameters.ReadAsync(context.Request, cancellationToken);

            LegacyServerSecurity.ValidateApiKey(parameters, configuration);
            int mapType = parameters.GetRequiredInt32("MapType");

            await using SqlConnection connection =
                await LegacySql.OpenAsync(connections, cancellationToken);

            for (int index = 0; index < 999; index++)
            {
                string? packed = parameters.GetOptional($"w{index}");
                if (string.IsNullOrWhiteSpace(packed))
                    break;

                string[] values = packed.Split(
                    ' ',
                    StringSplitOptions.RemoveEmptyEntries);

                if (values.Length < 4)
                    throw new LegacyApiException($"bad weapon stats w{index}");

                int itemId = Parse(values[0], "ItemID");
                int shotsFired = Parse(values[1], "ShotsFired");
                int shotsHit = Parse(values[2], "ShotsHits");
                int kills = Parse(values[3], "Kills");

                await using var command = new SqlCommand(
                    "dbo.WZ_SRV_AddWeaponStats",
                    connection)
                {
                    CommandType = CommandType.StoredProcedure,
                    CommandTimeout = 30
                };

                command.Parameters.Add("@in_ItemID", SqlDbType.Int).Value = itemId;
                command.Parameters.Add("@in_ShotsFired", SqlDbType.Int).Value = shotsFired;
                command.Parameters.Add("@in_ShotsHits", SqlDbType.Int).Value = shotsHit;
                command.Parameters.Add("@in_KillsCQ", SqlDbType.Int).Value = mapType == 0 ? kills : 0;
                command.Parameters.Add("@in_KillsDM", SqlDbType.Int).Value = mapType == 1 ? kills : 0;
                command.Parameters.Add("@in_KillsSB", SqlDbType.Int).Value = mapType == 3 ? kills : 0;

                await using SqlDataReader reader =
                    await command.ExecuteReaderAsync(cancellationToken);

                LegacyProcedureResult result = await LegacySql.ReadResultAsync(
                    reader,
                    moveToData: false,
                    cancellationToken);

                if (result.Code != 0)
                    return LegacyApiResponse.Error(result.Code, result.Message);
            }

            return LegacyApiResponse.Success();
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid server weapon-stats request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in server weapon-stats endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled server weapon-stats endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }

    private static int Parse(string value, string fieldName)
    {
        if (!int.TryParse(
                value,
                NumberStyles.Integer,
                CultureInfo.InvariantCulture,
                out int result))
        {
            throw new LegacyApiException($"bad integer field {fieldName}");
        }

        return result;
    }
}
