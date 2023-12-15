#pragma once
#include "D3D12App.h"
#include "Common/MathHelper.h"
#include "UploadBufferResource.h"
#include "FrameResource.h"
#include "Waves.h"


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

    Material* mat = nullptr;
    MeshGeometry* geo = nullptr;

    XMFLOAT4X4 texTransform = MathHelper::Identity4x4();
};

enum RenderLayer :int
{
    Opaque = 0,
    Mirrors,
    Reflects,
    Transparent,
    AlphaTest,
    Shadow,
    Count
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
    void OnKeyboardInput(const GameTime& gt);

    void BuildConstantBuffers();
    void BuildSRV();
    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    
    void BuildPSO();
    void DrawRenderItems(const std::vector<RenderItem*>& ritems);

    void UpdateObjCBs();
    void UpdatePassCBs(const GameTime& gt);
    void UpdateMatCBs();

    void BuildFrameResources();
 
    //����ɽ����������
    void BuildHillGeometry();
    void BuildBoxGeometry();
    void BuildHillMaterials();
    //�����ˮ����ͼ�任����
    void AnimateMaterials(const GameTime& gt);
    //���ɽ�ͺ���
    float GetHillsHeight(float x, float z);
    void UpdateWaves(const GameTime& gt);
    void BuildLakeIndexBuffer();
    XMFLOAT3 GetHillsNormal(float x, float z)const;
    void LoadBoxTextures();
    void BuildHillRenderItem();

    //�������䳡��
    void BuildShapeGeometry();
    void BuildShapeMaterials();
    void BuildSkullGeometry();
    void BuildSkullRenderItem();
    void BuildShapeRenderItem();

    //�������ڳ���
    void BuildRoomGeometry();
    void LoadRoomTextures();
    void CreateRoomSRV();
    void BuildMaterials();
    void BuildRoomRenderItem();

    //�����еĹ��շ���
    void UpdateReflectPassCB(const GameTime& gt);

    //����6�ֲ�����
    std::array<CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> cbvHeap = nullptr;
    ComPtr<ID3D12DescriptorHeap> srvHeap = nullptr;

    int frameResourcesCount = 3;
    FrameResource* currFrameResource = nullptr;
    int currFrameResourceIndex = 0;

    std::unique_ptr<Waves> waves;
    RenderItem* wavesRitem = nullptr;

    std::vector<std::unique_ptr<FrameResource>> FrameResourcesArray;
    std::vector<std::unique_ptr<RenderItem>> allRitems;
    std::vector<RenderItem*> ritemLayer[(int)RenderLayer::Count];

    std::unordered_map<std::string, std::unique_ptr<Material>> materials;
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> geometries;
    std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> shaders;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> PSOs;

    ComPtr<ID3DBlob> mvsByteCode = nullptr;
    ComPtr<ID3DBlob> mpsByteCode = nullptr;

    //���������벼��������������DirectX֪������Ľṹ
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    ComPtr<ID3D12PipelineState> mPSO = nullptr;

    PassConstants passConstants;
    PassConstants reflectPassConstants;

    XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    float mTheta = 1.5f * XM_PI;
    float mPhi = XM_PIDIV4;
    float mRadius = 17.0f;

    POINT mLastMousePos;

    //̫����ƽ�й⣩λ�õ�������
    float sunTheta = 1.25f * XM_PI;
    float sunPhi = XM_PIDIV4;

};