using System.Data;
using System.Text;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class LeaderboardEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.Leaderboard");

        try
        {
            LegacyRequestParameters parameters =
                await LegacyRequestParameters.ReadAsync(
                    context.Request,
                    cancellationToken);

            await using SqlConnection connection =
                await LegacySql.OpenAsync(connections, cancellationToken);

            await using var command = new SqlCommand(
                "dbo.WZ_LeaderboardGet",
                connection)
            {
                CommandType = CommandType.StoredProcedure,
                CommandTimeout = 30
            };

            command.Parameters.Add("@in_Hardcore", SqlDbType.Int).Value =
                parameters.GetRequiredInt32("Hardcore");
            command.Parameters.Add("@in_Type", SqlDbType.Int).Value =
                parameters.GetRequiredInt32("Type");
            command.Parameters.Add("@in_Page", SqlDbType.Int).Value =
                parameters.GetRequiredInt32("Page");

            await using SqlDataReader reader =
                await command.ExecuteReaderAsync(cancellationToken);

            LegacyProcedureResult result = await LegacySql.ReadResultAsync(
                reader,
                moveToData: true,
                cancellationToken);

            if (result.Code != 0)
                return LegacyApiResponse.Error(result.Code, result.Message);

            if (!await reader.ReadAsync(cancellationToken))
                throw new LegacyApiException("leaderboard header row missing");

            int startPosition = LegacySql.ReadInt32(reader, "StartPos");
            int pageCount = LegacySql.ReadInt32(reader, "PageCount");

            if (!await reader.NextResultAsync(cancellationToken))
                throw new LegacyApiException("leaderboard rows missing");

            var xml = new StringBuilder();
            xml.Append("<?xml version=\"1.0\"?>\n");
            xml.Append($"<leaderboard pos=\"{startPosition}\" pc=\"{pageCount}\">");

            while (await reader.ReadAsync(cancellationToken))
            {
                xml.Append("<f ");
                xml.Append(LegacySql.XmlAttribute("gt", reader["Gamertag"]));
                xml.Append(LegacySql.XmlAttribute("a", reader["Alive"]));
                xml.Append(LegacySql.XmlAttribute("d", reader["Data"]));
                xml.Append("/>");
            }

            xml.Append("</leaderboard>");
            return LegacyPayloadResponse.FromText(xml.ToString());
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid leaderboard request");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in leaderboard endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled leaderboard endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }
}
