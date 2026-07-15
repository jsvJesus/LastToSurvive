cbuffer FrameConstants : register(b0)
{
    row_major float4x4 viewProjection;
    float4 cameraTime;
    float4 lightDirectionIntensity;
    float4 lightColor;
    float4 ambientDebug;
};
cbuffer ObjectConstants : register(b1)
{
    row_major float4x4 world;
    row_major float4x4 normalMatrix;
    float4 tint;
    float4 objectData;
};
cbuffer MaterialConstants : register(b2)
{
    float4 baseColorFactor;
    float4 emissiveFactorStrength;
    float4 surfaceParams;
    float4 specularParams; // metalness strength, specular exponent, reflection/chrome
};
struct VertexInput { float3 position:POSITION; float3 normal:NORMAL; float4 tangent:TANGENT; float2 texcoord:TEXCOORD0; };
struct VertexOutput { float4 position:SV_Position; float3 worldNormal:NORMAL; float3 worldPosition:TEXCOORD1; float4 worldTangent:TANGENT; float2 texcoord:TEXCOORD0; };

VertexOutput VSMain(VertexInput i)
{
    VertexOutput o;
    float4 wp=mul(float4(i.position,1),world);
    o.position=mul(wp,viewProjection); o.worldPosition=wp.xyz;
    o.worldNormal=normalize(mul(float4(i.normal,0),normalMatrix).xyz);
    o.worldTangent=float4(normalize(mul(float4(i.tangent.xyz,0),normalMatrix).xyz),i.tangent.w*objectData.y);
    o.texcoord=i.texcoord; return o;
}

Texture2D baseColorTexture:register(t0);
Texture2D normalTexture:register(t1);
Texture2D specularGlossTexture:register(t2);
Texture2D roughnessTexture:register(t3);
Texture2D emissiveTexture:register(t4);
Texture2D specularPowerTexture:register(t5);
SamplerState materialSampler:register(s0);

float3 CompatibilityEnvironment(float3 r, float roughness)
{
    float horizon=pow(saturate(1-abs(r.y)),3);
    float3 ground=float3(.07,.085,.105);
    float3 sky=float3(.38,.52,.72);
    float3 environment=lerp(ground,sky,saturate(r.y*.5+.5));
    environment+=horizon*float3(.42,.44,.46);
    float sun=pow(saturate(dot(r,normalize(lightDirectionIntensity.xyz))),lerp(96,4,roughness));
    return environment+sun*lightColor.rgb*lightDirectionIntensity.w;
}

float4 PSMain(VertexOutput i,bool front:SV_IsFrontFace):SV_Target0
{
    float4 color=baseColorTexture.Sample(materialSampler,i.texcoord)*baseColorFactor*tint;
    if(surfaceParams.z>0.5&&surfaceParams.z<1.5) clip(color.a-surfaceParams.y);
    float3 n=normalize(i.worldNormal)*(front?1:-1);
    float3 t=normalize(i.worldTangent.xyz-n*dot(n,i.worldTangent.xyz));
    float3 b=normalize(cross(n,t))*i.worldTangent.w;
    float3 tn=normalTexture.Sample(materialSampler,i.texcoord).xyz*2-1;
    tn.xy*=surfaceParams.w; n=normalize(t*tn.x+b*tn.y+n*tn.z);
    float metalMask=specularGlossTexture.Sample(materialSampler,i.texcoord).r;
    float metalness=saturate(metalMask*specularParams.x);
    float rough=saturate(surfaceParams.x*roughnessTexture.Sample(materialSampler,i.texcoord).r);
    float control=saturate((log2(max(specularParams.y,2))-1)*.1);
    float exponent=exp2(1+control*specularPowerTexture.Sample(materialSampler,i.texcoord).r*10);
    float3 l=normalize(lightDirectionIntensity.xyz),v=normalize(cameraTime.xyz-i.worldPosition),h=normalize(l+v);
    float ndl=saturate(dot(n,l));
    float directSpec=pow(saturate(dot(n,h)),lerp(exponent,max(1,exponent*.125),rough));
    float3 specularColor=lerp(.04.xxx,color.rgb,metalness);
    float fresnel=pow(1-saturate(dot(n,v)),5);
    float3 reflection=CompatibilityEnvironment(reflect(-v,n),rough);
    float reflectionAmount=saturate(lerp(.08,1,metalness)*(1-rough*.7)*max(specularParams.z,.05));
    float3 diffuse=color.rgb*(1-metalness)*(ambientDebug.rgb+lightColor.rgb*lightDirectionIntensity.w*ndl);
    float3 specular=specularColor*lightColor.rgb*directSpec*lightDirectionIntensity.w;
    float3 environment=reflection*lerp(specularColor,1.0.xxx,fresnel)*reflectionAmount;
    float3 emissive=emissiveTexture.Sample(materialSampler,i.texcoord).rgb*emissiveFactorStrength.rgb*emissiveFactorStrength.a;
    float3 lit=diffuse+specular+environment+emissive;
    int mode=(int)(ambientDebug.w+.5);
    if(mode==1)lit=color.rgb; else if(mode==2)lit=n*.5+.5; else if(mode==3)lit=tn*.5+.5;
    else if(mode==4)lit=rough.xxx; else if(mode==5)lit=metalness.xxx; else if(mode==6)lit=emissive;
    return float4(lit,color.a);
}
