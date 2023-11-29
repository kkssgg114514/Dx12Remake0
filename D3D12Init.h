#pragma once
#include "Common.h"

class D3D12Init
{
public:
	D3D12Init()
	{
		mCurrentFence = 0;
		mCurrentBackBuffer = 0;
		rtvDescriptorSize = 0;
		dsvDescriptorSize = 0;
		cbv_srv_uavDescriptorSize = 0;
	}
	~D3D12Init(){}
private:
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

	//11设置视口和裁剪矩形
	void CreateViewPortAndScissorRect();

	//12实现围栏
	void FlushCmdQueue();

public:
	//将以上步骤合成到一个方法中
	bool InitDirect3D();

	//绘制方法
	void Draw();

private:
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
};

