#pragma once
#include "ToolFunc.h"
#include "GameTime.h"
#include "..\Common\MathHelper.h"

using namespace DirectX;
using namespace DirectX::PackedVector;



struct ObjectConstants
{
	//��ʼ������ռ�任���ü��ռ����
	XMFLOAT4X4 worldVeiwProj = MathHelper::Identity4x4();
};

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

class D3D12App
{
protected:
	D3D12App();
	virtual ~D3D12App();

public:
	int Run();
	bool Init(HINSTANCE hInstance, int nShowCmd, std::wstring customCaption);
	bool InitWindow(HINSTANCE hInstance, int nShowCmd);
	//������ϳɵ�һ��������
	virtual bool InitDirect3D();
	virtual void Draw() = 0;
	virtual void Update() = 0;

protected:
	//2�����豸
	void CreateDevice();

	//3����Χ��
	void CreateFence();

	//4��ȡ��������С
	void GetDescriptorSize();

	//5����MSAA���������
	void SetMSAA();

	//6����������С������б����������
	void CreateCommandObject();

	//7����������
	void CreateSwapChain();

	//8������������
	void CreateDescriptorHeap();

	//9����������
	void CreateRTV();
	void CreateDSV();

	//11�����ӿںͲü�����
	void CreateViewPortAndScissorRect();


	//12ʵ��Χ��
	void FlushCmdQueue();

	//����ÿ֡״̬
	void CalculateFrameState();

protected:
	HWND mhMainWnd = 0;

	//2�����豸
	//����ָ�������
	//���ȴ�������ָ�루Factory�ӿڣ�
	ComPtr<IDXGIFactory4> dxgiFactory;
	//Ȼ�󴴽��豸ָ��
	ComPtr<ID3D12Device> d3dDevice;

	//3����Χ��
	//����Χ��ָ��
	ComPtr<ID3D12Fence> fence;

	//4��ȡ��������С
	//Ҫ��ȡ������������С���ֱ���
	//RTV����ȾĿ�껺������������
	//DSV�����ģ�建������������
	//CBV_SRV_UAV����������������������ɫ����Դ������������������ʻ�����������
	UINT rtvDescriptorSize;
	UINT dsvDescriptorSize;
	UINT cbv_srv_uavDescriptorSize;

	

	//5����MSAA���������
	//����MSAA����ݵȼ�
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels;

	//6����������С������б����������
	ComPtr<ID3D12CommandQueue> cmdQueue;
	ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> cmdList;

	//7����������
	//����������ָ��
	ComPtr<IDXGISwapChain> swapChain;

	//8������������
	//����rtv��ָ��
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	//����dsv��ָ��
	ComPtr<ID3D12DescriptorHeap> dsvHeap;

	//9����������
	//����������������ָ�루���ڻ�ȡ�������еĺ�̨��������Դ��
	ComPtr<ID3D12Resource> swapChainBuffer[2];
	//������Ȼ�����ָ��
	ComPtr<ID3D12Resource> depthStencilBuffer;


	//11�����ӿںͲü�����
	D3D12_VIEWPORT viewPort;
	D3D12_RECT scissorRect;

	//��̨����������
	UINT mCurrentBackBuffer;

	//12ʵ��Χ��
	int mCurrentFence;	//��ʼCPU�ϵ�Χ����Ϊ0

	//GameTime��ʵ������
	GameTime gt;

	SIZE_T vbByteSize;
	SIZE_T ibByteSize;

	ComPtr<ID3D12Resource> vertexBufferUploader;
	ComPtr<ID3D12Resource> indexBufferUploader;

	ComPtr<ID3D12Resource> vertexBufferGpu;
	ComPtr<ID3D12Resource> indexBufferGpu;


	ComPtr<ID3DBlob> vertexBufferCpu;
	ComPtr<ID3DBlob> indexBufferCpu;

	
};

