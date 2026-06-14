using System.Collections.Specialized;

namespace System.Configuration
{
    public static class ConfigurationManager
    {
        public static NameValueCollection AppSettings { get; } = new();
    }
}

namespace System.Web.Configuration
{
    public sealed class LegacyConnectionStringSettings
    {
        public LegacyConnectionStringSettings(string connectionString)
        {
            ConnectionString = connectionString ?? string.Empty;
        }

        public string ConnectionString { get; }

        public override string ToString() => ConnectionString;
    }

    public sealed class LegacyConnectionStringCollection
    {
        private readonly Dictionary<string, LegacyConnectionStringSettings> _items =
            new(StringComparer.OrdinalIgnoreCase);

        public LegacyConnectionStringSettings this[string name]
        {
            get
            {
                if (!_items.TryGetValue(name, out LegacyConnectionStringSettings value))
                {
                    throw new InvalidOperationException(
                        $"Connection string '{name}' was not configured.");
                }

                return value;
            }
        }

        internal void Set(string name, string connectionString)
        {
            _items[name] = new LegacyConnectionStringSettings(connectionString);
        }
    }

    public static class WebConfigurationManager
    {
        public static LegacyConnectionStringCollection ConnectionStrings { get; } = new();

        public static void ConfigureConnectionString(string name, string connectionString)
        {
            ConnectionStrings.Set(name, connectionString);
        }
    }
}
