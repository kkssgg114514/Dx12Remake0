#pragma once
#include "UploadBufferResource.h"
#include "ToolFunc.h"

using namespace DirectX::PackedVector;
using namespace DirectX;

//定义顶点结构体
struct Vertex
{
    XMFLOAT3 Pos;
    XMCOLOR Color;
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
};

class FrameResource
{
public:
    FrameResource(ID3D12Device* device, UINT passCount, UINT objCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator = (const FrameResource& rhs) = delete;
    ~FrameResource();

    //每帧资源都需要独立的命令分配器
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAllocator;

    //每帧都需要单独的资源缓冲区
    std::unique_ptr<UploadBufferResource<ObjectConstants>> objCB = nullptr;
    std::unique_ptr<UploadBufferResource<PassConstants>> passCB = nullptr;
    //CPU的围栏值
    UINT64 fenceCPU = 0;
};

