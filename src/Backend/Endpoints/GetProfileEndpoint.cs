using System.Data;
using System.Globalization;
using System.Net;
using System.Text;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;
using WarZ.Api.Infrastructure;

namespace WarZ.Api.Endpoints;

public static class GetProfileEndpoint
{
    public static async Task<IResult> ExecuteAsync(
        HttpContext context,
        SqlConnectionFactory connections,
        ILoggerFactory loggerFactory,
        CancellationToken cancellationToken)
    {
        ILogger logger =
            loggerFactory.CreateLogger("Api.GetProfile");

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

            int charId =
                ReadOptionalInt32(
                    parameters.GetOptional("CharID"));

            string remoteIp =
                context.Connection.RemoteIpAddress?.ToString()
                ?? "0.0.0.0";

            await using SqlConnection connection =
                connections.CreateConnection();

            await connection.OpenAsync(cancellationToken);

            int sessionResultCode =
                await ValidateSessionAsync(
                    connection,
                    remoteIp,
                    customerId,
                    sessionId,
                    cancellationToken);

            if (sessionResultCode != 0)
            {
                logger.LogWarning(
                    "Profile request rejected. CustomerID={CustomerID}, ResultCode={ResultCode}",
                    customerId,
                    sessionResultCode);

                return LegacyApiResponse.Error(
                    sessionResultCode);
            }

            await using var command =
                new SqlCommand(
                    "dbo.WZ_GetAccountInfo2",
                    connection)
                {
                    CommandType = CommandType.StoredProcedure,
                    CommandTimeout = 30
                };

            command.Parameters.Add(
                new SqlParameter(
                    "@in_CustomerID",
                    SqlDbType.Int)
                {
                    Value = customerId
                });

            command.Parameters.Add(
                new SqlParameter(
                    "@in_CharID",
                    SqlDbType.Int)
                {
                    Value = charId
                });

            await using SqlDataReader reader =
                await command.ExecuteReaderAsync(
                    cancellationToken);

            int resultCode =
                await ReadInitialResultCodeAsync(
                    reader,
                    cancellationToken);

            if (resultCode != 0)
            {
                logger.LogWarning(
                    "WZ_GetAccountInfo2 failed. CustomerID={CustomerID}, ResultCode={ResultCode}",
                    customerId,
                    resultCode);

                return LegacyApiResponse.Error(
                    resultCode);
            }

            if (!await reader.NextResultAsync(
                    cancellationToken))
            {
                throw new LegacyApiException(
                    "account result set missing");
            }

            if (!await reader.ReadAsync(
                    cancellationToken))
            {
                throw new LegacyApiException(
                    "account result row missing");
            }

            var xml = new StringBuilder();

            xml.Append(
                "<?xml version=\"1.0\"?>\n");

            xml.Append("<account ");

            xml.Append(
                XmlAttribute(
                    "CustomerID",
                    reader["CustomerID"]));

            xml.Append(
                XmlAttribute(
                    "AccountStatus",
                    reader["AccountStatus"]));

            xml.Append(
                XmlAttribute(
                    "AccountType",
                    reader["AccountType"]));

            xml.Append(
                XmlAttribute(
                    "GamePoints",
                    reader["GamePoints"]));

            xml.Append(
                XmlAttribute(
                    "GameDollars",
                    reader["GameDollars"]));

            DateTime now =
                DateTime.Now;

            string currentTime =
                string.Format(
                    CultureInfo.InvariantCulture,
                    "{0} {1} {2} {3} {4}",
                    now.Year,
                    now.Month,
                    now.Day,
                    now.Hour,
                    now.Minute);

            xml.Append(
                XmlAttribute(
                    "time",
                    currentTime));

            string isDeveloper =
                ValueToString(
                    reader["IsDeveloper"]);

            if (isDeveloper != "0" &&
                !string.IsNullOrEmpty(isDeveloper))
            {
                xml.Append(
                    XmlAttribute(
                        "IsDeveloper",
                        isDeveloper));
            }

            string gameServerId =
                ValueToString(
                    reader["GameServerId"]);

            if (!string.IsNullOrEmpty(gameServerId) &&
                gameServerId != "0")
            {
                long secondsFromLastGame =
                    Convert.ToInt64(
                        reader["SecFromLastGame"],
                        CultureInfo.InvariantCulture);

                if (secondsFromLastGame < 40)
                {
                    xml.Append(
                        XmlAttribute(
                            "DataDirty",
                            secondsFromLastGame));
                }
            }

            xml.Append(">\n");

            await AppendCharactersAsync(
                reader,
                xml,
                cancellationToken);

            await AppendInventoryAsync(
                reader,
                xml,
                cancellationToken);

            await AppendBackpacksAsync(
                reader,
                xml,
                cancellationToken);

            xml.Append("</account>");

            logger.LogInformation(
                "Profile returned. CustomerID={CustomerID}, CharID={CharID}",
                customerId,
                charId);

            return LegacyApiResponse.Xml(
                xml.ToString());
        }
        catch (LegacyApiException exception)
        {
            logger.LogWarning(
                exception,
                "Invalid GetProfile request");

            return LegacyApiResponse.InternalError(
                exception.Message);
        }
        catch (SqlException exception)
        {
            logger.LogError(
                exception,
                "SQL error while loading profile");

            return LegacyApiResponse.InternalError(
                "SQL Select Failed");
        }
        catch (OperationCanceledException)
            when (cancellationToken.IsCancellationRequested)
        {
            logger.LogWarning(
                "GetProfile request was cancelled");

            return LegacyApiResponse.InternalError(
                "request cancelled");
        }
        catch (Exception exception)
        {
            logger.LogError(
                exception,
                "Unhandled GetProfile error");

            return LegacyApiResponse.InternalError(
                "internal server error");
        }
    }

    private static async Task<int> ValidateSessionAsync(
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
            new SqlParameter(
                "@in_IP",
                SqlDbType.VarChar,
                32)
            {
                Value = remoteIp
            });

        command.Parameters.Add(
            new SqlParameter(
                "@in_CustomerID",
                SqlDbType.Int)
            {
                Value = customerId
            });

        command.Parameters.Add(
            new SqlParameter(
                "@in_SessionID",
                SqlDbType.Int)
            {
                Value = sessionId
            });

        await using SqlDataReader reader =
            await command.ExecuteReaderAsync(
                cancellationToken);

        return await ReadInitialResultCodeAsync(
            reader,
            cancellationToken);
    }

    private static async Task<int> ReadInitialResultCodeAsync(
        SqlDataReader reader,
        CancellationToken cancellationToken)
    {
        if (!await reader.ReadAsync(
                cancellationToken))
        {
            throw new LegacyApiException(
                "ResultCode not set");
        }

        try
        {
            int ordinal =
                reader.GetOrdinal("ResultCode");

            if (reader.IsDBNull(ordinal))
            {
                throw new LegacyApiException(
                    "ResultCode not set");
            }

            return Convert.ToInt32(
                reader.GetValue(ordinal),
                CultureInfo.InvariantCulture);
        }
        catch (IndexOutOfRangeException exception)
        {
            throw new LegacyApiException(
                "ResultCode not set",
                exception);
        }
    }

    private static async Task AppendCharactersAsync(
        SqlDataReader reader,
        StringBuilder xml,
        CancellationToken cancellationToken)
    {
        if (!await reader.NextResultAsync(
                cancellationToken))
        {
            throw new LegacyApiException(
                "characters result set missing");
        }

        xml.Append("<chars>\n");

        while (await reader.ReadAsync(
                   cancellationToken))
        {
            DateTime deathTime =
                Convert.ToDateTime(
                    reader["DeathUtcTime"],
                    CultureInfo.InvariantCulture);

            xml.Append("<c ");

            xml.Append(XmlAttribute(
                "CharID",
                reader["CharID"]));

            xml.Append(XmlAttribute(
                "Gamertag",
                reader["Gamertag"]));

            xml.Append(XmlAttribute(
                "Alive",
                reader["Alive"]));

            xml.Append(XmlAttribute(
                "DeathTime",
                ToUnixTime(deathTime)));

            xml.Append(XmlAttribute(
                "SecToRevive",
                reader["SecToRevive"]));

            xml.Append(XmlAttribute(
                "Hardcore",
                reader["Hardcore"]));

            xml.Append(XmlAttribute(
                "XP",
                reader["XP"]));

            xml.Append(XmlAttribute(
                "TimePlayed",
                reader["TimePlayed"]));

            xml.Append(XmlAttribute(
                "GameMapId",
                reader["GameMapId"]));

            xml.Append(XmlAttribute(
                "GameServerId",
                reader["GameServerId"]));

            xml.Append(XmlAttribute(
                "GamePos",
                reader["GamePos"]));

            xml.Append(XmlAttribute(
                "GameFlags",
                reader["GameFlags"]));

            xml.Append(XmlAttribute(
                "Health",
                reader["Health"]));

            xml.Append(XmlAttribute(
                "Hunger",
                reader["Food"]));

            xml.Append(XmlAttribute(
                "Thirst",
                reader["Water"]));

            xml.Append(XmlAttribute(
                "Toxic",
                reader["Toxic"]));

            xml.Append(XmlAttribute(
                "Reputation",
                reader["Reputation"]));

            xml.Append(XmlAttribute(
                "HeroItemID",
                reader["HeroItemID"]));

            xml.Append(XmlAttribute(
                "HeadIdx",
                reader["HeadIdx"]));

            xml.Append(XmlAttribute(
                "BodyIdx",
                reader["BodyIdx"]));

            xml.Append(XmlAttribute(
                "LegsIdx",
                reader["LegsIdx"]));

            xml.Append(XmlAttribute(
                "HairIdx",
                reader["HairIdx"]));

            xml.Append(XmlAttribute(
                "FeetIdx",
                reader["FeetIdx"]));

            xml.Append(XmlAttribute(
                "BackpackSize",
                reader["BackpackSize"]));

            xml.Append(XmlAttribute(
                "BackpackID",
                reader["BackpackID"]));

            xml.Append(XmlAttribute(
                "ClanID",
                reader["ClanID"]));

            xml.Append(XmlAttribute(
                "ClanRank",
                reader["ClanRank"]));

            xml.Append(XmlAttribute(
                "ClanTag",
                reader["ClanTag"]));

            xml.Append(XmlAttribute(
                "ClanTagColor",
                reader["ClanTagColor"]));

            xml.Append(XmlAttribute(
                "attm1",
                reader["Attachment1"]));

            xml.Append(XmlAttribute(
                "attm2",
                reader["Attachment2"]));

            xml.Append(XmlAttribute(
                "ts00",
                reader["Stat00"]));

            xml.Append(XmlAttribute(
                "ts01",
                reader["Stat01"]));

            xml.Append(XmlAttribute(
                "ts02",
                reader["Stat02"]));

            xml.Append(XmlAttribute(
                "ts03",
                reader["Stat03"]));

            xml.Append(XmlAttribute(
                "ts04",
                reader["Stat04"]));

            xml.Append(XmlAttribute(
                "ts05",
                reader["Stat05"]));

            xml.Append("/>");
        }

        xml.Append("</chars>\n");
    }

    private static async Task AppendInventoryAsync(
        SqlDataReader reader,
        StringBuilder xml,
        CancellationToken cancellationToken)
    {
        if (!await reader.NextResultAsync(
                cancellationToken))
        {
            throw new LegacyApiException(
                "inventory result set missing");
        }

        xml.Append("<inventory>\n");

        while (await reader.ReadAsync(
                   cancellationToken))
        {
            long inventoryId =
                Convert.ToInt64(
                    reader["InventoryID"],
                    CultureInfo.InvariantCulture);

            int itemId =
                Convert.ToInt32(
                    reader["ItemID"],
                    CultureInfo.InvariantCulture);

            int quantity =
                Convert.ToInt32(
                    reader["Quantity"],
                    CultureInfo.InvariantCulture);

            int var1 =
                Convert.ToInt32(
                    reader["Var1"],
                    CultureInfo.InvariantCulture);

            int var2 =
                Convert.ToInt32(
                    reader["Var2"],
                    CultureInfo.InvariantCulture);

            xml.Append("<i ");

            xml.Append(
                XmlAttribute(
                    "id",
                    inventoryId));

            xml.Append(
                XmlAttribute(
                    "itm",
                    itemId));

            xml.Append(
                XmlAttribute(
                    "qt",
                    quantity));

            if (var1 >= 0)
            {
                xml.Append(
                    XmlAttribute(
                        "v1",
                        var1));
            }

            if (var2 >= 0)
            {
                xml.Append(
                    XmlAttribute(
                        "v2",
                        var2));
            }

            xml.Append("/>");
        }

        xml.Append("</inventory>\n");
    }

    private static async Task AppendBackpacksAsync(
        SqlDataReader reader,
        StringBuilder xml,
        CancellationToken cancellationToken)
    {
        if (!await reader.NextResultAsync(
                cancellationToken))
        {
            throw new LegacyApiException(
                "backpacks result set missing");
        }

        int currentCharId = 0;

        xml.Append("<backpacks>\n");

        while (await reader.ReadAsync(
                   cancellationToken))
        {
            long inventoryId =
                Convert.ToInt64(
                    reader["InventoryID"],
                    CultureInfo.InvariantCulture);

            int charId =
                Convert.ToInt32(
                    reader["CharID"],
                    CultureInfo.InvariantCulture);

            int itemId =
                Convert.ToInt32(
                    reader["ItemID"],
                    CultureInfo.InvariantCulture);

            int quantity =
                Convert.ToInt32(
                    reader["Quantity"],
                    CultureInfo.InvariantCulture);

            int slot =
                Convert.ToInt32(
                    reader["BackpackSlot"],
                    CultureInfo.InvariantCulture);

            int var1 =
                Convert.ToInt32(
                    reader["Var1"],
                    CultureInfo.InvariantCulture);

            int var2 =
                Convert.ToInt32(
                    reader["Var2"],
                    CultureInfo.InvariantCulture);

            if (charId != currentCharId)
            {
                if (currentCharId > 0)
                {
                    xml.Append("</b>");
                }

                xml.Append(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "<b CharID=\"{0}\">",
                        charId));

                currentCharId = charId;
            }

            xml.Append("<i ");

            xml.Append(
                XmlAttribute(
                    "id",
                    inventoryId));

            xml.Append(
                XmlAttribute(
                    "itm",
                    itemId));

            xml.Append(
                XmlAttribute(
                    "qt",
                    quantity));

            xml.Append(
                XmlAttribute(
                    "s",
                    slot));

            if (var1 >= 0)
            {
                xml.Append(
                    XmlAttribute(
                        "v1",
                        var1));
            }

            if (var2 >= 0)
            {
                xml.Append(
                    XmlAttribute(
                        "v2",
                        var2));
            }

            xml.Append("/>");
        }

        if (currentCharId > 0)
        {
            xml.Append("</b>");
        }

        xml.Append("</backpacks>\n");
    }

    private static string XmlAttribute(
        string name,
        object? value)
    {
        string text =
            ValueToString(value);

        string encoded =
            WebUtility.HtmlEncode(text);

        return string.Format(
            CultureInfo.InvariantCulture,
            "{0}=\"{1}\"\n",
            name,
            encoded);
    }

    private static string ValueToString(
        object? value)
    {
        if (value is null ||
            value == DBNull.Value)
        {
            return string.Empty;
        }

        if (value is IFormattable formattable)
        {
            return formattable.ToString(
                       null,
                       CultureInfo.InvariantCulture)
                   ?? string.Empty;
        }

        return value.ToString()
               ?? string.Empty;
    }

    private static long ToUnixTime(
        DateTime date)
    {
        DateTime epoch =
            new(
                1970,
                1,
                1,
                0,
                0,
                0,
                DateTimeKind.Utc);

        return Convert.ToInt64(
            (date - epoch).TotalSeconds);
    }

    private static int ReadOptionalInt32(
        string? value)
    {
        if (string.IsNullOrWhiteSpace(value))
        {
            return 0;
        }

        return int.TryParse(
            value,
            NumberStyles.Integer,
            CultureInfo.InvariantCulture,
            out int parsed)
            ? parsed
            : 0;
    }
}
