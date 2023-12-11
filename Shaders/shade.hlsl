#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

#include "LightingTools.hlsl"

Texture2D gDiffuseMap : register(t0);

//6个不同类型的采样器
SamplerState gSamPointWarp : register(s0);
SamplerState gSamPointClamp : register(s1);
SamplerState gSamLinearWarp : register(s2);
SamplerState gSamLinearClamp : register(s3);
SamplerState gSamAnisotropicWarp : register(s4);
SamplerState gSamAnisotropicClamp : register(s5);

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld; 
    float4x4 gTexTransform; //UV顶点变换矩阵
};

cbuffer cbMaterial : register(b1)
{
    float4 gDiffuseAlbedo; //材质反照率
    float3 gFresnelR0; //RF(0)值，即材质的反射属性
    float gRoughness; //材质的粗糙度
    float4x4 gMatTransform; //UV动画变换矩阵
};

cbuffer cbPass : register(b2)
{
    float4x4 gViewProj;
    float3 gEyePosW;
    float gTotalTime;
    float4 gAmbientLight;
    Light gLights[MAX_LIGHTS];
};

struct VertexIn
{
    float3 Vertex  : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : POSITION;
    float3 WorldNormal : NORMAL;
    float2 UV : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

    float4 PosW = mul(float4(vin.Vertex, 1.0f), gWorld);
    vout.WorldPos = PosW.xyz;
    
    //只做均匀缩放，所以可以不使用逆转置矩阵
    vout.WorldNormal = mul(vin.Normal, (float3x3)gWorld);
    
    vout.Pos = mul(PosW, gViewProj);

    float4 TexCoord = mul(float4(vin.TexCoord, 0.0f, 1.0f), gTexTransform);
    vout.UV = mul(TexCoord, gMatTransform).xy;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 diffuseAlbedo = gDiffuseMap.Sample(gSamAnisotropicWarp, pin.UV) * gDiffuseAlbedo;
#ifdef ALPHA_TEST
    clip(diffuseAlbedo.a - 0.1f);
#endif
    float3 worldNormal = normalize(pin.WorldNormal);
    float3 worldView = normalize(gEyePosW - pin.WorldPos);
    
    Material mat = { diffuseAlbedo, gFresnelR0, gRoughness };
    float3 shadowFactor = 1.0f;//暂时使用1.0，不对计算产生影响
    //直接光照
    float4 directLight = ComputerLighting(gLights, mat, pin.WorldPos, worldNormal, worldView, shadowFactor);
    //环境光照，要和采样器输出值结合
    float4 ambient = gAmbientLight * diffuseAlbedo;
    
    float4 finalCol = ambient + directLight;
    finalCol.a = gDiffuseAlbedo.a * 0.3f;
    
    return finalCol;
}