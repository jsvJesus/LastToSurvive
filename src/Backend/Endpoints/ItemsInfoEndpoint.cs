using System.Data;
using System.Text;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class ItemsInfoEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        SqlConnectionFactory connections,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger = loggerFactory.CreateLogger("Api.ItemsInfo");

        try
        {
            await using SqlConnection connection =
                await LegacySql.OpenAsync(connections, cancellationToken);

            await using var command = new SqlCommand("dbo.WZ_GetItemsData", connection)
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

            var xml = new StringBuilder("<?xml version=\"1.0\"?>\n<items>");

            WriteGearRows(reader, xml);
            await NextResultAsync(reader, cancellationToken);
            WriteWeaponRows(reader, xml);
            await NextResultAsync(reader, cancellationToken);
            WriteGenericRows(reader, xml);
            await NextResultAsync(reader, cancellationToken);
            WritePackageRows(reader, xml);

            xml.Append("</items>");
            return LegacyPayloadResponse.FromText(xml.ToString());
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(exception, "Invalid items info response");
            return LegacyApiResponse.InternalError(exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(exception, "SQL error in items info endpoint");
            return LegacyApiResponse.InternalError("SQL Select Failed");
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Unhandled items info endpoint error");
            return LegacyApiResponse.InternalError("internal server error");
        }
    }

    private static void WriteGearRows(SqlDataReader reader, StringBuilder xml)
    {
        xml.Append("<gears>");
        while (reader.Read())
        {
            if (!HasPrice(reader)) continue;
            xml.Append("<g ");
            Add(xml, reader, "ID", "ItemID");
            Add(xml, reader, "lv", "LevelRequired");
            Add(xml, reader, "wg", "Weight");
            Add(xml, reader, "dp", "DamagePerc");
            Add(xml, reader, "dm", "DamageMax");
            xml.Append("/>\n");
        }
        xml.Append("</gears>");
    }

    private static void WriteWeaponRows(SqlDataReader reader, StringBuilder xml)
    {
        xml.Append("<weapons>");
        while (reader.Read())
        {
            if (!HasPrice(reader)) continue;
            xml.Append("<w ");
            Add(xml, reader, "ID", "ItemID");
            Add(xml, reader, "lv", "LevelRequired");
            Add(xml, reader, "d1", "Damage");
            Add(xml, reader, "d2", "DamageDecay");
            Add(xml, reader, "rf", "RateOfFire");
            Add(xml, reader, "sp", "Spread");
            Add(xml, reader, "rc", "Recoil");
            xml.Append("/>\n");
        }
        xml.Append("</weapons>");
    }

    private static void WriteGenericRows(SqlDataReader reader, StringBuilder xml)
    {
        xml.Append("<generics>");
        while (reader.Read())
        {
            if (!HasPrice(reader)) continue;
            string category = Convert.ToString(reader["Category"]) ?? string.Empty;
            if (category != "7" && category != "3") continue;
            xml.Append("<b ");
            Add(xml, reader, "ID", "ItemID");
            Add(xml, reader, "name", "Name");
            Add(xml, reader, "fname", "FNAME");
            Add(xml, reader, "desc", "Description");
            xml.Append("/>\n");
        }
        xml.Append("</generics>");
    }

    private static void WritePackageRows(SqlDataReader reader, StringBuilder xml)
    {
        xml.Append("<packages>");
        while (reader.Read())
        {
            if (!HasPrice(reader)) continue;
            xml.Append("<p ");
            Add(xml, reader, "ID", "ItemID");
            Add(xml, reader, "name", "Name");
            Add(xml, reader, "fname", "FNAME");
            Add(xml, reader, "desc", "Description");
            Add(xml, reader, "gd", "AddGP");
            Add(xml, reader, "sp", "AddSP");
            for (int index = 1; index <= 6; index++)
            {
                Add(xml, reader, $"i{index}i", $"Item{index}_ID");
                Add(xml, reader, $"i{index}e", $"Item{index}_Exp");
            }
            xml.Append("/>\n");
        }
        xml.Append("</packages>");
    }

    private static bool HasPrice(SqlDataReader reader) =>
        LegacySql.ReadInt32(reader, "Price1") != 0 ||
        LegacySql.ReadInt32(reader, "Price7") != 0 ||
        LegacySql.ReadInt32(reader, "Price30") != 0 ||
        LegacySql.ReadInt32(reader, "PriceP") != 0 ||
        LegacySql.ReadInt32(reader, "GPrice1") != 0 ||
        LegacySql.ReadInt32(reader, "GPrice7") != 0 ||
        LegacySql.ReadInt32(reader, "GPrice30") != 0 ||
        LegacySql.ReadInt32(reader, "GPriceP") != 0;

    private static void Add(StringBuilder xml, SqlDataReader reader, string name, string column) =>
        xml.Append(LegacySql.XmlAttribute(name, reader[column]));

    private static async Task NextResultAsync(
        SqlDataReader reader,
        CancellationToken cancellationToken)
    {
        if (!await reader.NextResultAsync(cancellationToken))
            throw new LegacyApiException("items data result set missing");
    }
}
