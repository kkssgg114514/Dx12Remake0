#pragma once
#include "UploadBufferResource.h"
#include "ToolFunc.h"

using namespace DirectX::PackedVector;
using namespace DirectX;

//定义顶点结构体
struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

//常量缓冲区结构体
struct ObjectConstants
{
    XMFLOAT4X4 world = MathHelper::Identity4x4();
};

//多常量缓冲区
struct PassConstants
{
    XMFLOAT4X4 viewProj = MathHelper::Identity4x4();

    XMFLOAT4 ambientLight = { 0.0f,0.0f,0.0f,1.0f };
    Light lights[MAX_LIGHTS];
};

struct MatConstants
{
    XMFLOAT4 diffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };   //材质反照率
    XMFLOAT3 fresnelR0 = { 0.01f,0.01f,0.01f };         //RF（0）值，即材质的反射属性
    float roughness = 0.25f;                            //材质粗糙度
};

class FrameResource
{
public:
    FrameResource(ID3D12Device* device, UINT passCount, UINT objCount, UINT matCount, UINT waveVertCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator = (const FrameResource& rhs) = delete;
    ~FrameResource();

    //每帧资源都需要独立的命令分配器
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAllocator;

    //每帧都需要单独的资源缓冲区
    std::unique_ptr<UploadBufferResource<ObjectConstants>> objCB = nullptr;
    std::unique_ptr<UploadBufferResource<PassConstants>> passCB = nullptr;
    std::unique_ptr<UploadBufferResource<MatConstants>> matCB = nullptr;

    std::unique_ptr<UploadBufferResource<Vertex>> WavesVB = nullptr;

    //CPU的围栏值
    UINT64 fenceCPU = 0;
};

