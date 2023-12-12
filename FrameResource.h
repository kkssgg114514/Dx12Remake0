#pragma once
#include "UploadBufferResource.h"
#include "ToolFunc.h"

using namespace DirectX::PackedVector;
using namespace DirectX;

//���嶥��ṹ��
struct Vertex
{
    Vertex() = default;
    Vertex(float x, float y, float z, float nx, float ny, float nz, float u, float v) :
        Pos(x, y, z),
        Normal(nx, ny, nz),
        TexC(u, v) {}
    XMFLOAT3 Pos;
    XMFLOAT3 Normal;
    XMFLOAT2 TexC;
};

//�����������ṹ��
struct ObjectConstants
{
    XMFLOAT4X4 world = MathHelper::Identity4x4();
    XMFLOAT4X4 texTransform = MathHelper::Identity4x4();
};

//�ೣ��������
struct PassConstants
{
    XMFLOAT4X4 viewProj = MathHelper::Identity4x4();

    XMFLOAT3 eyePosW = { 0.0f,0.0f,0.0f };
    float totalTime = 0.0f;
    XMFLOAT4 ambientLight = { 0.0f,0.0f,0.0f,1.0f };
    XMFLOAT4 fogColor = { 1.0f,1.0f,1.0f,1.0f };
    float fogStart = 1.0f;
    float fogRange = 1.0f;
    XMFLOAT2 pad2 = { 0.0f,0.0f };//ռλ��

    Light lights[MAX_LIGHTS];
};

struct MatConstants
{
    XMFLOAT4 diffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };   //���ʷ�����
    XMFLOAT3 fresnelR0 = { 0.01f,0.01f,0.01f };         //RF��0��ֵ�������ʵķ�������
    float roughness = 0.25f;                            //���ʴֲڶ�
    XMFLOAT4X4 matTransform = MathHelper::Identity4x4();//������λ�ƾ���
};

class FrameResource
{
public:
    FrameResource(ID3D12Device* device, UINT passCount, UINT objCount, UINT matCount, UINT waveVertCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator = (const FrameResource& rhs) = delete;
    ~FrameResource();

    //ÿ֡��Դ����Ҫ���������������
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAllocator;

    //ÿ֡����Ҫ��������Դ������
    std::unique_ptr<UploadBufferResource<ObjectConstants>> objCB = nullptr;
    std::unique_ptr<UploadBufferResource<PassConstants>> passCB = nullptr;
    std::unique_ptr<UploadBufferResource<MatConstants>> matCB = nullptr;

    std::unique_ptr<UploadBufferResource<Vertex>> WavesVB = nullptr;

    //CPU��Χ��ֵ
    UINT64 fenceCPU = 0;
};

