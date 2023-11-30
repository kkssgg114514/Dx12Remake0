#pragma once
#include "D3D12App.h"
#include "UploadBufferResource.h"

class D3D12InitApp :
    public D3D12App
{
public:
    D3D12InitApp();
    ~D3D12InitApp();

    bool Init(HINSTANCE hInstance, int nShowCmd, std::wstring customCaption);

    void BuildRootSignature();

    void BuildByteCodeAndInputLayout();

    void BuildGeometry();

    void BuildPSO();

    void CreateCBV();

    virtual void Update() override;
    //ºÃ≥–÷ÿ‘ÿªÊ÷∆∫Ø ˝
    virtual void Draw() override;

    D3D12_VERTEX_BUFFER_VIEW GetVbv()const;
    D3D12_INDEX_BUFFER_VIEW GetIbv()const;

private:
	
    ComPtr<ID3D12RootSignature> rootSignature;

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayoutDesc;

    ComPtr<ID3DBlob> vsBytecode;
    ComPtr<ID3DBlob> psBytecode;

    ComPtr<ID3D12DescriptorHeap> cbvHeap;

    ComPtr<ID3D12PipelineState> PSO;

    XMFLOAT4X4 world = MathHelper::Identity4x4();
    XMFLOAT4X4 view = MathHelper::Identity4x4();
    XMFLOAT4X4 proj = MathHelper::Identity4x4();

    std::unique_ptr<UploadBufferResource<ObjectConstants>> objCB;

};

