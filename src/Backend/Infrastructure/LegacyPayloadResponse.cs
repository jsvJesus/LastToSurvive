using System.IO.Compression;
using System.Text;

namespace WarZ.Api.Infrastructure;

public static class LegacyPayloadResponse
{
    public static IResult FromText(string text, bool allowCompression = true)
    {
        return FromBytes(
            Encoding.UTF8.GetBytes(text),
            allowCompression);
    }

    public static IResult FromBytes(
        byte[] payload,
        bool allowCompression = true)
    {
        if (!allowCompression || payload.Length < 200)
        {
            return Results.Bytes(
                payload,
                "application/octet-stream");
        }

        byte[] compressed;

        using (var output = new MemoryStream())
        {
            using (var gzip = new GZipStream(
                       output,
                       CompressionLevel.SmallestSize,
                       leaveOpen: true))
            {
                gzip.Write(payload, 0, payload.Length);
            }

            compressed = output.ToArray();
        }

        if (compressed.Length >= payload.Length)
        {
            return Results.Bytes(
                payload,
                "application/octet-stream");
        }

        byte[] framed = new byte[compressed.Length + 2];
        framed[0] = (byte)'$';
        framed[1] = (byte)'1';
        Buffer.BlockCopy(
            compressed,
            0,
            framed,
            2,
            compressed.Length);

        return Results.Bytes(
            framed,
            "application/octet-stream");
    }
}
