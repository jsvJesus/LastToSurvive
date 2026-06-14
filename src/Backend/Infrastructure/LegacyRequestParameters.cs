namespace WarZ.Api.Infrastructure;

public sealed class LegacyRequestParameters
{
    private readonly Dictionary<string, string> _values;

    private LegacyRequestParameters(Dictionary<string, string> values)
    {
        _values = values;
    }

    public static async Task<LegacyRequestParameters> ReadAsync(
        HttpRequest request,
        CancellationToken cancellationToken)
    {
        var values = new Dictionary<string, string>(
            StringComparer.OrdinalIgnoreCase);

        // Старый WebHelper сначала проверял POST Form.
        if (request.HasFormContentType)
        {
            IFormCollection form =
                await request.ReadFormAsync(cancellationToken);

            foreach (KeyValuePair<string, Microsoft.Extensions.Primitives.StringValues> item in form)
            {
                values[item.Key] = item.Value.ToString();
            }
        }

        // Query string используется только если параметра не было в POST Form.
        foreach (KeyValuePair<string, Microsoft.Extensions.Primitives.StringValues> item
                 in request.Query)
        {
            if (!values.ContainsKey(item.Key))
            {
                values[item.Key] = item.Value.ToString();
            }
        }

        return new LegacyRequestParameters(values);
    }

    public string GetRequired(string name)
    {
        if (!_values.TryGetValue(name, out string? value) ||
            string.IsNullOrWhiteSpace(value))
        {
            throw new LegacyApiException($"no parameter {name}");
        }

        return value;
    }

    public string? GetOptional(string name)
    {
        return _values.TryGetValue(name, out string? value)
            ? value
            : null;
    }

    public int GetRequiredInt32(string name)
    {
        string value = GetRequired(name);

        if (!int.TryParse(
                value,
                System.Globalization.NumberStyles.Integer,
                System.Globalization.CultureInfo.InvariantCulture,
                out int result))
        {
            throw new LegacyApiException($"bad integer parameter {name}");
        }

        return result;
    }

    public long GetRequiredInt64(string name)
    {
        string value = GetRequired(name);

        if (!long.TryParse(
                value,
                System.Globalization.NumberStyles.Integer,
                System.Globalization.CultureInfo.InvariantCulture,
                out long result))
        {
            throw new LegacyApiException($"bad integer parameter {name}");
        }

        return result;
    }
}