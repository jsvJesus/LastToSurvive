using System.Reflection;
using Microsoft.AspNetCore.Builder;
using Microsoft.Extensions.DependencyInjection;
using LegacyPage = System.Web.UI.Page;

namespace WarZ.LegacyHost.Hosting;

public static class LegacyEndpointRegistry
{
    public static IReadOnlyList<string> MapLegacyAspxEndpoints(
        this WebApplication app)
    {
        Type pageBaseType = typeof(LegacyPage);

        Type[] pageTypes = Assembly.GetExecutingAssembly()
            .GetTypes()
            .Where(type =>
                !type.IsAbstract &&
                pageBaseType.IsAssignableFrom(type) &&
                type.Name.StartsWith("api_", StringComparison.Ordinal))
            .OrderBy(type => type.Name, StringComparer.Ordinal)
            .ToArray();

        var routes = new List<string>(pageTypes.Length * 2);

        foreach (Type discoveredType in pageTypes)
        {
            Type pageType = discoveredType;
            string fileName = pageType.Name + ".aspx";
            string apsRoute = "/APS/" + fileName;
            string rootRoute = "/" + fileName;

            app.MapMethods(
                apsRoute,
                new[] { "GET", "POST" },
                async context =>
                {
                    LegacyPageExecutor executor =
                        context.RequestServices.GetRequiredService<LegacyPageExecutor>();

                    await executor.ExecuteAsync(context, pageType);
                });

            app.MapMethods(
                rootRoute,
                new[] { "GET", "POST" },
                async context =>
                {
                    LegacyPageExecutor executor =
                        context.RequestServices.GetRequiredService<LegacyPageExecutor>();

                    await executor.ExecuteAsync(context, pageType);
                });

            routes.Add(apsRoute);
            routes.Add(rootRoute);
        }

        return routes;
    }
}
