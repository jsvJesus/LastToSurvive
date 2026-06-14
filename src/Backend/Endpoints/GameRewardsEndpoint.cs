using System.Data;
using System.Text;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class GameRewardsEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        SqlConnectionFactory connections,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.GameRewards");

        try
        {
            await using SqlConnection connection =
                await LegacySql.OpenAsync(connections, cancellationToken);

            await using var command = new SqlCommand(
                "dbo.WZ_GetDataGameRewards",
                connection)
            {
                CommandType = CommandType.StoredProcedure,
                CommandTimeout = 30
            };

            await using SqlDataReader reader =
                await command.ExecuteReaderAsync(cancellationToken);

            LegacyProcedureResult result = await LegacySql.ReadResultAsync(
                reader,
                moveToData: true,
                cancellationToken);

            if (result.Code != 0)
                return LegacyApiResponse.Error(result.Code, result.Message);

            var xml = new StringBuilder("<?xml version=\"1.0\"?>\n<rewards>");

            while (await reader.ReadAsync(cancellationToken))
            {
                xml.Append("<rwd ");
                xml.Append(LegacySql.XmlAttribute("ID", reader["ID"]));
                xml.Append(LegacySql.XmlAttribute("Name", reader["Name"]));
                xml.Append(LegacySql.XmlAttribute("GD_SOFT", reader["GD_SOFT"]));
                xml.Append(LegacySql.XmlAttribute("XP_SOFT", reader["XP_SOFT"]));
                xml.Append(LegacySql.XmlAttribute("GD_HARD", reader["GD_HARD"]));
                xml.Append(LegacySql.XmlAttribute("XP_HARD", reader["XP_HARD"]));
                xml.Append("/>\n");
            }

            xml.Append("</rewards>");
            return LegacyPayloadResponse.FromText(xml.ToString());
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid game rewards response");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in game rewards endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled game rewards endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }
}
