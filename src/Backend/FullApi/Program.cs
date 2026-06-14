using Microsoft.AspNetCore.Http.Features;
using System.Configuration;
using System.Web.Configuration;
using WarZ.LegacyHost.Hosting;

var builder = WebApplication.CreateBuilder(args);

string connectionString =
    builder.Configuration.GetConnectionString("LTS")
    ?? builder.Configuration.GetConnectionString("dbConnectionString")
    ?? string.Empty;

if (string.IsNullOrWhiteSpace(connectionString))
{
    throw new InvalidOperationException(
        "Connection string 'LTS' is empty. Configure it with: " +
        "dotnet user-secrets set \"ConnectionStrings:LTS\" \"...\"");
}

string region =
    builder.Configuration["Legacy:Region"]
    ?? "US";

int maxRequestBodySizeMb =
    builder.Configuration.GetValue<int?>("Legacy:MaxRequestBodySizeMb")
    ?? 256;

long maxRequestBodySizeBytes =
    Math.Max(1, maxRequestBodySizeMb) * 1024L * 1024L;

builder.WebHost.ConfigureKestrel(options =>
{
    options.Limits.MaxRequestBodySize = maxRequestBodySizeBytes;
});

builder.Services.Configure<FormOptions>(options =>
{
    options.MultipartBodyLengthLimit = maxRequestBodySizeBytes;
    options.ValueLengthLimit = int.MaxValue;
    options.MultipartHeadersLengthLimit = 64 * 1024;
});

builder.Services.AddSingleton<LegacyPageExecutor>();

WebConfigurationManager.ConfigureConnectionString(
    "dbConnectionString",
    connectionString);

ConfigurationManager.AppSettings["WO_Region"] = region;

var app = builder.Build();

app.MapGet("/", () => Results.Text(
    "LastToSurvive legacy ASPX compatibility host is running.",
    "text/plain; charset=utf-8"));

IReadOnlyList<string> legacyRoutes =
    app.MapLegacyAspxEndpoints();

app.MapGet("/__endpoints", () => Results.Json(new
{
    Count = legacyRoutes.Count,
    Routes = legacyRoutes
}));

app.Logger.LogInformation(
    "Mapped {EndpointCount} legacy ASPX routes.",
    legacyRoutes.Count);

app.Run();
