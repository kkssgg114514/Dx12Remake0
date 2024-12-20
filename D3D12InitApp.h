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

//构造渲染项
struct RenderItem
{
    RenderItem() = default;

    //该几何体的世界矩阵
    XMFLOAT4X4 world = MathHelper::Identity4x4();

    //帧资源
    int NumFramesDirty = gNumFrameResources;

    //该几何体的常量数据在常量缓冲区的索引
    UINT objCBIndex = -1;

    //该几何体的图元拓扑类型
    D3D12_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    //该几何体的绘制三参数
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
 
    //构建山川湖泊场景
    void BuildHillGeometry();
    void BuildBoxGeometry();
    void BuildHillMaterials();
    //计算湖水的贴图变换矩阵
    void AnimateMaterials(const GameTime& gt);
    //获得山峦海拔
    float GetHillsHeight(float x, float z);
    void UpdateWaves(const GameTime& gt);
    void BuildLakeIndexBuffer();
    XMFLOAT3 GetHillsNormal(float x, float z)const;
    void LoadBoxTextures();
    void BuildHillRenderItem();

    //构建经典场景
    void BuildShapeGeometry();
    void BuildShapeMaterials();
    void BuildSkullGeometry();
    void BuildSkullRenderItem();
    void BuildShapeRenderItem();

    //构建室内场景
    void BuildRoomGeometry();
    void LoadRoomTextures();
    void CreateRoomSRV();
    void BuildMaterials();
    void BuildRoomRenderItem();

    //镜像中的光照反射
    void UpdateReflectPassCB(const GameTime& gt);

    //返回6种采样器
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

    //声明了输入布局描述，用来让DirectX知道顶点的结构
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    //提取指针
    RenderItem* mSkullRitem = nullptr;
    RenderItem* mSkullMirrorRitem = nullptr;
    RenderItem* mSkullShadowRitem = nullptr;

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

    //骷髅的位置变换
    XMFLOAT3 skullTranslation = { 0.0f,1.0f,-5.0f };

    //太阳（平行光）位置的球坐标
    float sunTheta = 1.25f * XM_PI;
    float sunPhi = XM_PIDIV4;

};