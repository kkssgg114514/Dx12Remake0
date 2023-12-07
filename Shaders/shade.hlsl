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



cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld; 
};

cbuffer cbMaterial : register(b1)
{
    float4 gDiffuseAlbedo; //材质反照率
    float3 gFresnelR0; //RF(0)值，即材质的反射属性
    float gRoughness; //材质的粗糙度
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
};

struct VertexOut
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : POSITION;
    float3 WorldNormal : NORMAL;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

    float4 PosW = mul(float4(vin.Vertex, 1.0f), gWorld);
    vout.WorldPos = PosW.xyz;
    
    //只做均匀缩放，所以可以不使用逆转置矩阵
    vout.WorldNormal = mul(vin.Normal, (float3x3)gWorld);
    
    vout.Pos = mul(PosW, gViewProj);
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float3 worldNormal = normalize(pin.WorldNormal);
    float3 worldView = normalize(gEyePosW - pin.WorldPos);
    
    Material mat = { gDiffuseAlbedo, gFresnelR0, gRoughness };
    float3 shadowFactor = 1.0f;//暂时使用1.0，不对计算产生影响

    //卡通着色
    float3 lightVec = -gLights[0].direction;
    float m = (1.0f - mat.roughness) * 256.0f;
    float3 halfVec = normalize(lightVec + worldView);

    float ks = pow(max(dot(worldNormal, halfVec), 0.0f), m);
    if(ks >= 0.0f && ks <= 0.1f)
        ks = 0.0f;
    if(ks > 0.1f && ks <= 0.0f)
        ks = 0.5f;
    if(ks > 0.8f && ks <= 1.0f)
        ks = 0.8f;

    float roughnessFactor = (m + 8.0f) / 8.0f * ks;
    float3 fresnelFactor = SchlickFresnel(mat.fresnelR0, halfVec, lightVec);

    float3 specAlbedo = fresnelFactor * roughnessFactor;
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    float kd = max(dot(worldNormal, lightVec), 0.0f);
    if (kd <= 0.1f)   
        kd = 0.1f;    
    if (kd > 0.1f && kd <= 0.5f)
        kd = 0.15f;
    if (kd > 0.5f && kd <= 1.0f)
        kd = 0.25f;

     //最终光强
    float3 lightStrength = kd*5 * gLights[0].strength; //方向光单位面积上的辐照度 

    //漫反射+高光反射=入射光量*总的反照率
    float3 directLight = lightStrength * (mat.diffuseAlbedo.rgb + specAlbedo); 
    
    //环境光照
    float4 ambient = gAmbientLight * gDiffuseAlbedo;
    
    float3 finalCol = ambient.xyz + directLight;

    
    return float4(finalCol, gDiffuseAlbedo.a);
}