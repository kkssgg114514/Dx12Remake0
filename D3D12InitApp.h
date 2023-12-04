#pragma once
#include "D3D12App.h"
#include "../Common/MathHelper.h"
#include "UploadBufferResource.h"
#include "FrameResource.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

const int gNumFrameResources = 3;

//������Ⱦ��
struct RenderItem
{
    RenderItem() = default;

    //�ü�������������
    XMFLOAT4X4 world = MathHelper::Identity4x4();

    //֡��Դ
    int NumFramesDirty = gNumFrameResources;

    //�ü�����ĳ��������ڳ���������������
    UINT objCBIndex = -1;

    //�ü������ͼԪ��������
    D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    //�ü�����Ļ���������
    UINT indexCount = 0;
    UINT startIndexLocation = 0;
    UINT baseVertexLocation = 0;
};

class D3D12InitApp : public D3D12App
{
public:
    D3D12InitApp(HINSTANCE hInstance);
    D3D12InitApp(const D3D12InitApp& rhs) = delete;
    D3D12InitApp& operator=(const D3D12InitApp& rhs) = delete;
    ~D3D12InitApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTime& gt)override;
    virtual void Draw(const GameTime& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;


    void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildGeometry();
    void BuildPSO();

    void BuildRenderItem();
    void DrawRenderItems();

    void BuildFrameResources();

    //���ɽ�ͺ���
    float GetHillsHeight(float x, float z);

    D3D12_VERTEX_BUFFER_VIEW GetVbv()const;
    D3D12_INDEX_BUFFER_VIEW GetIbv()const;

private:

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> cbvHeap = nullptr;

    int frameResourcesCount = 3;
    std::vector<std::unique_ptr<FrameResource>> FrameResourcesArray;
    FrameResource* currFrameResource = nullptr;
    int currFrameResourceIndex = 0;

    //std::unique_ptr<UploadBufferResource<ObjectConstants>> objCB = nullptr;
    ////�����ϴ���
    //std::unique_ptr<UploadBufferResource<PassConstants>> passCB = nullptr;

    std::vector<std::unique_ptr<RenderItem>> allRitems;

    ComPtr<ID3DBlob> mvsByteCode = nullptr;
    ComPtr<ID3DBlob> mpsByteCode = nullptr;

    //���������벼��������������DirectX֪������Ľṹ
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    ComPtr<ID3D12PipelineState> mPSO = nullptr;

    XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    float mTheta = 1.5f * XM_PI;
    float mPhi = XM_PIDIV4;
    float mRadius = 50.0f;

    POINT mLastMousePos;

    ComPtr<ID3D12Resource> vertexBufferUploader;
    ComPtr<ID3D12Resource> indexBufferUploader;

    ComPtr<ID3D12Resource> vertexBufferGpu;
    ComPtr<ID3D12Resource> indexBufferGpu;


    ComPtr<ID3DBlob> vertexBufferCpu;
    ComPtr<ID3DBlob> indexBufferCpu;


    UINT VertexBufferByteSize = 0;
    UINT IndexBufferByteSize = 0;

    //������������Ҫ����������
    struct SubmeshGeometry
    {
        UINT indexCount;
        UINT startIndexLocation;
        UINT baseVertexLocation;
    };

    std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

};