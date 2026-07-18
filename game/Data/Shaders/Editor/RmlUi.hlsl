cbuffer RmlUiConstants : register(b0)
{
    float2 Translation;
    float2 InverseViewport;
    float4x4 Transform;
};

struct VertexInput { float2 Position : POSITION; float4 Colour : COLOR0; float2 TexCoord : TEXCOORD0; };
struct PixelInput { float4 Position : SV_POSITION; float4 Colour : COLOR0; float2 TexCoord : TEXCOORD0; };

PixelInput VsMain(VertexInput input)
{
    PixelInput output;
    float2 pixel = input.Position + Translation;
    float4 clip = float4(pixel.x * InverseViewport.x * 2.0 - 1.0,
                         1.0 - pixel.y * InverseViewport.y * 2.0, 0.0, 1.0);
    output.Position = mul(Transform, clip);
    output.Colour = input.Colour;
    output.TexCoord = input.TexCoord;
    return output;
}

Texture2D UiTexture : register(t0);
SamplerState UiSampler : register(s0);

float4 PsMain(PixelInput input) : SV_TARGET
{
    return input.Colour * UiTexture.Sample(UiSampler, input.TexCoord);
}
