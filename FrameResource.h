#pragma once
#include "UploadBufferResource.h"
#include "ToolFunc.h"

using namespace DirectX::PackedVector;
using namespace DirectX;

//���嶥��ṹ��
struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

//�����������ṹ��
struct ObjectConstants
{
    XMFLOAT4X4 world = MathHelper::Identity4x4();
};

//�ೣ��������
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

    //ÿ֡��Դ����Ҫ���������������
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAllocator;

    //ÿ֡����Ҫ��������Դ������
    std::unique_ptr<UploadBufferResource<ObjectConstants>> objCB = nullptr;
    std::unique_ptr<UploadBufferResource<PassConstants>> passCB = nullptr;
    //CPU��Χ��ֵ
    UINT64 fenceCPU = 0;
};

