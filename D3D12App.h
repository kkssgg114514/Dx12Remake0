#pragma once
#include "ToolFunc.h"
#include "GameTime.h"
#include "..\Common\MathHelper.h"

using namespace DirectX;
using namespace DirectX::PackedVector;



struct ObjectConstants
{
	//初始化物体空间变换到裁剪空间矩阵
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
	//将步骤合成到一个方法中
	virtual bool InitDirect3D();
	virtual void Draw();
	virtual void Update();

protected:
	//2创建设备
	void CreateDevice();

	//3创建围栏
	void CreateFence();

	//4获取描述符大小
	void GetDescriptorSize();

	//5设置MSAA抗锯齿属性
	void SetMSAA();

	//6创建命令队列、命令列表、命令分配器
	void CreateCommandObject();

	//7创建交换链
	void CreateSwapChain();

	//8创建描述符堆
	void CreateDescriptorHeap();

	//9创建描述符
	void CreateRTV();
	void CreateDSV();
	void CreateVertexView();
	void CreateIndexView();

	//11设置视口和裁剪矩形
	void CreateViewPortAndScissorRect();


	//12实现围栏
	void FlushCmdQueue();

	//计算每帧状态
	void CalculateFrameState();

protected:
	HWND mhMainWnd = 0;

	//2创建设备
	//所有指针的声明
	//首先创建工厂指针（Factory接口）
	ComPtr<IDXGIFactory4> dxgiFactory;
	//然后创建设备指针
	ComPtr<ID3D12Device> d3dDevice;

	//3创建围栏
	//创建围栏指针
	ComPtr<ID3D12Fence> fence;

	//4获取描述符大小
	//要获取三个描述符大小，分别是
	//RTV（渲染目标缓冲区描述符）
	//DSV（深度模板缓冲区描述符）
	//CBV_SRV_UAV（常量缓冲区描述符、着色器资源缓冲描述符和随机访问缓冲描述符）
	UINT rtvDescriptorSize;
	UINT dsvDescriptorSize;
	UINT cbv_srv_uavDescriptorSize;

	UINT objConstSize;

	//5设置MSAA抗锯齿属性
	//设置MSAA抗锯齿等级
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels;

	//6创建命令队列、命令列表、命令分配器
	ComPtr<ID3D12CommandQueue> cmdQueue;
	ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> cmdList;

	//7创建交换链
	//创建交换链指针
	ComPtr<IDXGISwapChain> swapChain;

	//8创建描述符堆
	//创建rtv堆指针
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	//创建dsv堆指针
	ComPtr<ID3D12DescriptorHeap> dsvHeap;

	//9创建描述符
	//创建交换链缓冲区指针（用于获取交换链中的后台缓冲区资源）
	ComPtr<ID3D12Resource> swapChainBuffer[2];
	//创建深度缓冲区指针
	ComPtr<ID3D12Resource> depthStencilBuffer;

	//11设置视口和裁剪矩形
	D3D12_VIEWPORT viewPort;
	D3D12_RECT scissorRect;

	//后台缓冲区索引
	UINT mCurrentBackBuffer;

	//12实现围栏
	int mCurrentFence;	//初始CPU上的围栏点为0

	//GameTime类实例声明
	GameTime gt;

	SIZE_T vbByteSize;
	SIZE_T ibByteSize;

	ComPtr<ID3D12Resource> vertexBufferUploader;
	ComPtr<ID3D12Resource> indexBufferUploader;

	ComPtr<ID3D12Resource> vertexBufferGpu;
	ComPtr<ID3D12Resource> indexBufferGpu;


	ComPtr<ID3DBlob> vertexBufferCpu;
	ComPtr<ID3DBlob> indexBufferCpu;

	//#实例化顶点结构体并填充
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
		//前
		0, 1, 2,
		0, 2, 3,

		//后
		4, 6, 5,
		4, 7, 6,

		//左
		4, 5, 1,
		4, 1, 0,

		//右
		3, 2, 6,
		3, 6, 7,

		//上
		1, 5, 6,
		1, 6, 2,

		//下
		4, 0, 3,
		4, 3, 7
	};
};

