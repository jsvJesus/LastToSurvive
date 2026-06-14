using System.Data;
using System.Globalization;
using System.Net;
using Microsoft.Data.SqlClient;
using WarZ.Api.Data;

namespace WarZ.Api.Infrastructure;

public readonly record struct LegacyProcedureResult(int Code, string Message);

public static class LegacySql
{
    public static async Task<SqlConnection> OpenAsync(
        SqlConnectionFactory factory,
        CancellationToken cancellationToken)
    {
        SqlConnection connection = factory.CreateConnection();
        await connection.OpenAsync(cancellationToken);
        return connection;
    }

    public static async Task<LegacyProcedureResult> ValidateSessionAsync(
        SqlConnection connection,
        string remoteIp,
        int customerId,
        int sessionId,
        CancellationToken cancellationToken)
    {
        await using var command = new SqlCommand("dbo.WZ_UpdateLoginSession", connection)
        {
            CommandType = CommandType.StoredProcedure,
            CommandTimeout = 30
        };

        command.Parameters.Add("@in_IP", SqlDbType.VarChar, 100).Value = remoteIp;
        command.Parameters.Add("@in_CustomerID", SqlDbType.Int).Value = customerId;
        command.Parameters.Add("@in_SessionID", SqlDbType.Int).Value = sessionId;

        await using SqlDataReader reader =
            await command.ExecuteReaderAsync(cancellationToken);

        return await ReadResultAsync(reader, false, cancellationToken);
    }

    public static async Task<LegacyProcedureResult> ReadResultAsync(
        SqlDataReader reader,
        bool moveToData,
        CancellationToken cancellationToken)
    {
        if (!await reader.ReadAsync(cancellationToken))
            throw new LegacyApiException("ResultCode not set");

        int code = ReadInt32(reader, "ResultCode");
        string message = TryReadString(reader, "ResultMsg") ?? string.Empty;

        if (code == 0 && moveToData &&
            !await reader.NextResultAsync(cancellationToken))
        {
            throw new LegacyApiException("result data set missing");
        }

        return new LegacyProcedureResult(code, message);
    }

    public static int ReadInt32(SqlDataReader reader, string name)
    {
        try
        {
            int ordinal = reader.GetOrdinal(name);
            if (reader.IsDBNull(ordinal))
                throw new LegacyApiException($"bad int field {name}");

            return Convert.ToInt32(reader.GetValue(ordinal), CultureInfo.InvariantCulture);
        }
        catch (IndexOutOfRangeException exception)
        {
            throw new LegacyApiException($"bad int field {name}", exception);
        }
    }

    public static long ReadInt64(SqlDataReader reader, string name)
    {
        try
        {
            int ordinal = reader.GetOrdinal(name);
            if (reader.IsDBNull(ordinal))
                throw new LegacyApiException($"bad int64 field {name}");

            return Convert.ToInt64(reader.GetValue(ordinal), CultureInfo.InvariantCulture);
        }
        catch (IndexOutOfRangeException exception)
        {
            throw new LegacyApiException($"bad int64 field {name}", exception);
        }
    }

    public static string? TryReadString(SqlDataReader reader, string name)
    {
        try
        {
            int ordinal = reader.GetOrdinal(name);
            return reader.IsDBNull(ordinal)
                ? null
                : Convert.ToString(reader.GetValue(ordinal), CultureInfo.InvariantCulture);
        }
        catch (IndexOutOfRangeException)
        {
            return null;
        }
    }

    public static string XmlAttribute(string name, object? value)
    {
        string text = value is null || value == DBNull.Value
            ? string.Empty
            : Convert.ToString(value, CultureInfo.InvariantCulture) ?? string.Empty;

        return $"{name}=\"{WebUtility.HtmlEncode(text)}\"\n";
    }
}
