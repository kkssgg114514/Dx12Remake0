#pragma once
#include"tiff.h"

class D3D12Init
{
public:
	D3D12Init()
	{
		mCurrentFence = 0;
	}
	~D3D12Init(){}
public:
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

private:
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

	//12ʵ��Χ��
	int mCurrentFence;	//��ʼCPU�ϵ�Χ����Ϊ0
};

