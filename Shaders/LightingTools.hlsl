struct Light
{
    float3 strength;        //光源颜色
    float falloffStart;     //点光源和聚光灯的开始衰减距离
    float3 direction;       //方向光和聚光灯的方向向量
    float falloffEnd;       //点光和聚光灯的衰减结束距离
    float3 position;        //点光源和聚光灯的坐标
    float spotPower;        //聚光灯因子中的幂参数
};

struct Material
{
    float4 diffuseAlbedo;   //材质反射率
    float3 fresnelR0;       //RF（0）值，即材质的反射属性
    float roughness;        //材质的粗糙度
};

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    float att = saturate((falloffEnd - d) / (falloffEnd - falloffStart));
    return att;  
}

float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVector)
{
    //根据石里克方程计算出入射光线被反射的百分比
    float3 reflectPercent = R0 + (1.0f - R0) * pow(1 - saturate(dot(normal, lightVector)), 5.0f);
    return reflectPercent;
}

float3 BlinnPhong(Material mat, float3 normal, float3 toEye, float3 lightVec, float3 lightStrength)
{
    float m = (1.0f - mat.roughness) * 256.0f;
    float3 halfVec = normalize(lightVec + toEye);   //半角向量
    float roughnessFactor = (m + 8.0f) / 8.0f * pow(max(dot(normal, halfVec), 0.0f), m);
    float3 fresnelFactor = SchlickFresnel(mat.fresnelR0, halfVec, lightVec);

    float3 specAlbedo = fresnelFactor * roughnessFactor;
    specAlbedo = specAlbedo / (specAlbedo + 1);

    float3 diff_Spec = lightStrength * (mat.diffuseAlbedo.rgb + specAlbedo);
    return diff_Spec;
}

float3 ComputerDirectionalLight(Light light, Material mat, float3 normal, float3 toEye)
{
    float3 lightVec = -light.direction;
    float3 lightStrength = max(dot(normal, lightVec), 0.0f) * light.strength;

    return BlinnPhong(mat, normal, toEye, lightVec, lightStrength);
}

float3 ComputerPointLight(Light light, Material mat, float3 pos, float3 normal, float3 toEye)
{
    float3 lightVec = light.position - pos;
    float distance = length(lightVec);

    if(distance > light.falloffEnd)
        return 0;
    
    lightVec /= distance;
    float nDot1 = max(dot(lightVec, normal), 0);
    float3 lightStrength = nDot1 * light.strength;

    float att = CalcAttenuation(distance, light.falloffStart, light.falloffEnd);
    lightStrength *= att;

    return BlinnPhong(mat, normal, toEye, lightVec, lightStrength);
}

float3 ComputerSpotLight(Light light, Material mat, float3 pos, float3 normal, float3 toEye)
{
    float3 lightVec = light.position - pos;
    float distance = length(lightVec);

    if(distance > light.falloffEnd)
        return 0;

    lightVec /= distance;
    float nDot1 = max(dot(lightVec, normal), 0);
    float3 lightStrength = nDot1 * light.strength;

    float att = CalcAttenuation(distance, light.falloffStart, light.falloffEnd);
    lightStrength *= att;

    float spotFactor = pow(max(dot(-lightVec, light.direction), 0), light.spotPower);
    lightStrength *= spotFactor;

    return BlinnPhong(mat, normal, toEye, lightVec, lightStrength);
}

#define MAX_LIGHTS 16

float3 ComputerLighting(Light light[MAX_LIGHTS], Material mat, float3 pos, float3 normal, float3 toEye, float3 shadowFactor)
{
    float3 result = 0.0f;
    int i = 0;

#if(NUM_DIR_LIGHTS > 0)
    for(i = 0; i < NUM_DIR_LIGHTS; i++)
    {
        result += shadowFactor[i] * ComputerDirectionalLight(light[i], mat, normal, toEye);
    }
#endif

#if(NUM_POINT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS; i < NUM_POINT_LIGHTS + NUM_DIR_LIGHTS; i++)
    {
        result += ComputerPointLight(light[i], mat, pos, normal, toEye);
    }
#endif

#if(NUM_SPOT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; i++)
    {
        result += ComputerSpotLight(light[i], mat, pos, normal, toEye);
    }
#endif

    return float4(result, 0.0f);
}