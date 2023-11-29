#pragma once
#include "ToolFunc.h"
#include "GameTime.h"
#include "..\Common\d3dx12.h"
#include "..\Common\MathHelper.h"


using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

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

//#ʵ��������ṹ�岢���
std::array<Vertex, 8> vertices =
{
	Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
	Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
	Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
	Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
	Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
	Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
	Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
	Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
};

std::array<std::uint16_t, 36> indices =
{
	//ǰ
	0, 1, 2,
	0, 2, 3,

	//��
	4, 6, 5,
	4, 7, 6,

	//��
	4, 5, 1,
	4, 1, 0,

	//��
	3, 2, 6,
	3, 6, 7,

	//��
	1, 5, 6,
	1, 6, 2,

	//��
	4, 0, 3,
	4, 3, 7
};

class D3D12App
{
protected:
	D3D12App();
	virtual ~D3D12App();

public:
	int Run();
	bool Init(HINSTANCE hInstance, int nShowCmd);
	bool InitWindow(HINSTANCE hInstance, int nShowCmd);
	//������ϳɵ�һ��������
	virtual bool InitDirect3D();
	virtual void Draw();

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
	void CreateVertexView();
	void CreateIndexView();
	void CreateCBV();

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

	UINT objConstSize;

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

	ComPtr<ID3D12DescriptorHeap> cbvHeap;

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

	ComPtr<ID3D12Resource> vertexBufferGpu;
	ComPtr<ID3D12Resource> indexBufferGpu;

	ComPtr<ID3DBlob> indexBufferCpu;
};

