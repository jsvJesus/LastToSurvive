using System.Collections.Specialized;
using System.Net;
using System.Text;
using Microsoft.AspNetCore.Http;

namespace System.Web
{
    public static class HttpUtility
    {
        public static string HtmlEncode(string value)
        {
            return WebUtility.HtmlEncode(value ?? string.Empty);
        }

        public static string HtmlEncode(object value)
        {
            return WebUtility.HtmlEncode(value?.ToString() ?? string.Empty);
        }
    }

    public sealed class HttpPostedFile
    {
        private readonly IFormFile _file;

        internal HttpPostedFile(IFormFile file)
        {
            _file = file;
        }

        public string FileName => _file.FileName;
        public string ContentType => _file.ContentType;
        public int ContentLength => checked((int)_file.Length);
        public Stream InputStream => _file.OpenReadStream();
    }

    public sealed class HttpFileCollection
    {
        private readonly Dictionary<string, HttpPostedFile> _files =
            new(StringComparer.OrdinalIgnoreCase);

        internal HttpFileCollection(IFormFileCollection files)
        {
            if (files == null)
                return;

            foreach (IFormFile file in files)
            {
                _files[file.Name] = new HttpPostedFile(file);
            }
        }

        public HttpPostedFile this[string name]
        {
            get
            {
                _files.TryGetValue(name, out HttpPostedFile file);
                return file;
            }
        }

        public int Count => _files.Count;
    }

    public sealed class HttpRequest
    {
        internal HttpRequest(
            Microsoft.AspNetCore.Http.HttpContext context,
            IFormCollection form,
            Stream rawBody)
        {
            Form = ToNameValueCollection(form);
            QueryString = ToNameValueCollection(context.Request.Query);
            Files = new HttpFileCollection(form?.Files);
            InputStream = rawBody ?? Stream.Null;

            string scheme = string.IsNullOrWhiteSpace(context.Request.Scheme)
                ? "http"
                : context.Request.Scheme;

            string host = context.Request.Host.HasValue
                ? context.Request.Host.Value
                : "localhost";

            string path = context.Request.Path.HasValue
                ? context.Request.Path.Value
                : "/";

            string query = context.Request.QueryString.HasValue
                ? context.Request.QueryString.Value
                : string.Empty;

            Url = new Uri($"{scheme}://{host}{path}{query}");
            UserHostAddress = context.Connection.RemoteIpAddress?.ToString() ?? "0.0.0.0";
        }

        public NameValueCollection Form { get; }
        public NameValueCollection QueryString { get; }
        public HttpFileCollection Files { get; }
        public Stream InputStream { get; }
        public Uri Url { get; }
        public string UserHostAddress { get; }

        private static NameValueCollection ToNameValueCollection(
            IEnumerable<KeyValuePair<string, Microsoft.Extensions.Primitives.StringValues>> source)
        {
            var result = new NameValueCollection(StringComparer.OrdinalIgnoreCase);

            if (source == null)
                return result;

            foreach (KeyValuePair<string, Microsoft.Extensions.Primitives.StringValues> item in source)
            {
                result[item.Key] = item.Value.ToString();
            }

            return result;
        }
    }

    public sealed class HttpResponse
    {
        private readonly MemoryStream _body = new();

        public Encoding ContentEncoding { get; set; } = Encoding.UTF8;
        public string ContentType { get; set; } = "text/plain; charset=utf-8";
        public bool BufferOutput { get; set; } = true;

        public void Write(string value)
        {
            byte[] bytes = ContentEncoding.GetBytes(value ?? string.Empty);
            _body.Write(bytes, 0, bytes.Length);
        }

        public void Write(object value)
        {
            Write(value?.ToString() ?? string.Empty);
        }

        public void BinaryWrite(byte[] data)
        {
            if (data == null || data.Length == 0)
                return;

            _body.Write(data, 0, data.Length);
        }

        public void Flush()
        {
            // ASP.NET Core response is flushed after Page_Load completes.
        }

        internal byte[] ToArray() => _body.ToArray();
    }

    public sealed class HttpServerUtility
    {
        private readonly string _contentRoot;

        internal HttpServerUtility(string contentRoot)
        {
            _contentRoot = string.IsNullOrWhiteSpace(contentRoot)
                ? Directory.GetCurrentDirectory()
                : contentRoot;
        }

        public string MapPath(string virtualPath)
        {
            if (string.IsNullOrWhiteSpace(virtualPath))
                return _contentRoot;

            string normalized = virtualPath
                .Replace('\\', Path.DirectorySeparatorChar)
                .Replace('/', Path.DirectorySeparatorChar)
                .TrimStart('~', Path.DirectorySeparatorChar);

            return Path.GetFullPath(Path.Combine(_contentRoot, normalized));
        }
    }
}

namespace System.Web.UI
{
    public class Page
    {
        public System.Web.HttpRequest Request { get; private set; }
        public System.Web.HttpResponse Response { get; private set; }
        public System.Web.HttpServerUtility Server { get; private set; }

        internal void AttachLegacyContext(
            System.Web.HttpRequest request,
            System.Web.HttpResponse response,
            System.Web.HttpServerUtility server)
        {
            Request = request;
            Response = response;
            Server = server;
        }
    }
}

namespace System.Web.UI.WebControls
{
    public class Control
    {
    }
}
