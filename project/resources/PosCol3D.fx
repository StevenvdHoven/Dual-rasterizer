
float4x4    gWorldViewProj  : WorldViewProjection;
float4x4    gWorldMatrix    : WORLD;
float3      gCameraPosition : CAMERA;

Texture2D   gDiffuseMap     : DiffuseMap;
Texture2D   gNormalMap      : NormalMap;
Texture2D   gSpecularMap    : SpecularMap;
Texture2D   gGlossinessMap  : GlossinessMap;

static const float3 LightDirection  = float3(0.577f, -0.577f, 0.577f);
static const float  LightIntensity  = float(7.0f);
static const float  Shininess       = float(25.0f);
static const float  PI              = float(3.1415926f);

struct VS_INPUT
{
    float3 Position : POSITION;
    float2 UV : TEXCOORD;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float4 WorldPosition : WORLD;
    float2 UV : TEXCOORD;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
};

VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
    output.Position = mul(float4(input.Position, 1.0f), gWorldViewProj);
    output.WorldPosition = mul(float4(input.Position, 1.0f), gWorldMatrix);
    output.UV = input.UV;
    output.Normal = mul(normalize(input.Normal), (float3x3) gWorldMatrix);
    output.Tangent = mul(normalize(input.Tangent), (float3x3) gWorldMatrix);
    return output;
}

RasterizerState gRasterizerState
{
    Cullmode = back;
    FrontCounterClockwise = false;
};

BlendState gBlendState
{
    BlendEnable[0] = false;
};

DepthStencilState gDepthStencilState
{
    DepthEnable = true;
    DepthWriteMask = all;
    DepthFunc = less;
    StencilEnable = true;
};

SamplerState samPoint
{
    Filter = MIN_MAG_MIP_Point;
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerState samLinear
{
    Filter = MIN_MAG_MIP_Linear;
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerState samAnisotropic
{
    Filter = ANISOTROPIC;
    MaxAnisotropy = 16;
    AddressU = Wrap;
    AddressV = Wrap;
};

float4 SampleTexture(VS_OUTPUT input, Texture2D textureToSample, SamplerState samplerState)
{
    return textureToSample.Sample(samplerState, input.UV);
}

float4 PS_Point(VS_OUTPUT input) : SV_TARGET
{
    float3 binormal = normalize(cross(input.Normal, input.Tangent));
    float3x3 tangentToWorldMatrix = float3x3(
        normalize(input.Tangent),
        binormal,
        normalize(input.Normal)
    );
	
    float3 normalMapColour = saturate(SampleTexture(input, gNormalMap, samPoint).rgb);
    float3 sampledNormal = 2.0f * normalMapColour - float3(1.0f, 1.0f, 1.0f);
    float3 finalNormal = normalize(mul(sampledNormal, tangentToWorldMatrix));

    float glossiness = saturate(SampleTexture(input, gGlossinessMap, samPoint).r) * Shininess;

    float4 specularColor = saturate(SampleTexture(input, gSpecularMap, samPoint));

    float4 lambertDiffuse = (LightIntensity * saturate(SampleTexture(input, gDiffuseMap, samPoint))) / PI;
    
    float observedArea = max(dot(finalNormal, -LightDirection), 0.0f);

    float3 invViewDirection = normalize(gCameraPosition - input.WorldPosition.xyz);
    float3 refResult = reflect(LightDirection, finalNormal);
    float angle = max(dot(refResult, invViewDirection), 0.0f);
    float powRes = pow(angle, glossiness);

    float specReflection = specularColor * (powRes);
    
    return (lambertDiffuse * observedArea)
    + specReflection;
}

float4 PS_Linear(VS_OUTPUT input) : SV_TARGET
{
    float3 binormal = normalize(cross(input.Normal, input.Tangent));
    float3x3 tangentToWorldMatrix = float3x3(normalize(input.Tangent),binormal,normalize(input.Normal));
	
    float3 normalMapColour = saturate(SampleTexture(input, gNormalMap, samLinear).rgb);
    float3 sampledNormal = 2.0f * normalMapColour - float3(1.0f, 1.0f, 1.0f);
    float3 finalNormal = normalize(mul(sampledNormal, tangentToWorldMatrix));

    float glossiness = saturate(SampleTexture(input, gGlossinessMap, samLinear).r) * Shininess;

    float4 specularColor = saturate(SampleTexture(input, gSpecularMap, samLinear));

    float4 lambertDiffuse = (LightIntensity * saturate(SampleTexture(input, gDiffuseMap, samLinear))) / PI;
    float observedArea = max(dot(finalNormal, -LightDirection), 0.0f);

    float3 invViewDirection = normalize(gCameraPosition - input.WorldPosition.xyz);
    float3 refResult = reflect(LightDirection, finalNormal);
    float angle = max(dot(refResult, invViewDirection), 0.0f);
    float powRes = pow(angle, glossiness);

    float specReflection = specularColor * (powRes);
    
    return (lambertDiffuse * observedArea) + specReflection;
}

float4 PS_Anisotropic(VS_OUTPUT input) : SV_TARGET
{
    float3 binormal = normalize(cross(input.Normal, input.Tangent));
    float3x3 tangentToWorldMatrix = float3x3(normalize(input.Tangent),binormal,normalize(input.Normal));
	
    float3 normalMapColour = saturate(SampleTexture(input, gNormalMap, samAnisotropic).rgb);
    float3 sampledNormal = 2.0f * normalMapColour - float3(1.0f, 1.0f, 1.0f);
    float3 finalNormal = normalize(mul(sampledNormal, tangentToWorldMatrix));

    float glossiness = saturate(SampleTexture(input, gGlossinessMap, samAnisotropic).r) * Shininess;

    float4 specularColor = saturate(SampleTexture(input, gSpecularMap, samAnisotropic));

    float4 lambertDiffuse = (LightIntensity * saturate(SampleTexture(input, gDiffuseMap, samAnisotropic))) / PI;
    float observedArea = max(dot(finalNormal, -LightDirection), 0.0f);

    float3 invViewDirection = normalize(gCameraPosition - input.WorldPosition.xyz);
    float3 refResult = reflect(LightDirection, finalNormal);
    float angle = max(dot(refResult, invViewDirection), 0.0f);
    float powRes = pow(angle, glossiness);

    float specReflection = specularColor * (powRes);
    
    return (lambertDiffuse * observedArea) + specReflection;
}

technique11 PointTechnique
{
    pass P0
    {
        SetRasterizerState(gRasterizerState);
        SetDepthStencilState(gDepthStencilState, 0);
        SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS_Point()));
    }
}

technique11 LinearTechnique
{
    pass P0
    {
        SetRasterizerState(gRasterizerState);
        SetDepthStencilState(gDepthStencilState, 0);
        SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS_Linear()));
    }
}

technique11 AnisotropicTechnique
{
    pass P0
    {
        SetRasterizerState(gRasterizerState);
        SetDepthStencilState(gDepthStencilState, 0);
        SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS_Anisotropic()));
    }
}