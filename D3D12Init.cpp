#include "D3D12Init.h"

//2�����豸
void D3D12Init::CreateDevice()
{
	/*--------------------------------------------------------------------------------------------------------*/
	//�����豸������
	//���ȴ���������Factory�ӿڣ�
	//ComPtr<IDXGIFactory4> dxgiFactory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));
	//Ȼ�󴴽��豸
	//ComPtr<ID3D12Device> d3dDevice;
	ThrowIfFailed(D3D12CreateDevice(
		nullptr,						//#�˲����������Ϊnullptr����ʹ����������
		D3D_FEATURE_LEVEL_12_0,			//#Ӧ�ó�����ҪӲ����֧�ֵ���͹��ܼ���
		IID_PPV_ARGS(&d3dDevice)		//#���������豸
	));
	/*--------------------------------------------------------------------------------------------------------*/
}

//3����Χ��
void D3D12Init::CreateFence()
{
	////����Χ��ָ��
	//ComPtr<ID3D12Fence> fence;
	ThrowIfFailed(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
}

//4��ȡ��������С
void D3D12Init::GetDescriptorSize()
{
	rtvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	dsvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	cbv_srv_uavDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

//5����MSAA���������
void D3D12Init::SetMSAA()
{
	//D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevels;
	//��д�ṹ��
	msaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				//#UNORM�ǹ�һ��������޷�������
	msaaQualityLevels.SampleCount = 1;
	msaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msaaQualityLevels.NumQualityLevels = 0;
	//#��ǰͼ��������MSAA���ز�����֧�֣�ע�⣺�ڶ������������������������//�ڶ������������á�����������
	ThrowIfFailed(d3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msaaQualityLevels,
		sizeof(msaaQualityLevels)
	));
	//#NumQualityLevels��Check��������������
	//#���֧��MSAA����Check�������ص�NumQualityLevels > 0
	//#expressionΪ�٣���Ϊ0��������ֹ�������У�����ӡһ��������Ϣ
	assert(msaaQualityLevels.NumQualityLevels > 0);
}

//6����������С������б����������
void D3D12Init::CreateCommandObject()
{
	//���ȴ���D3D12_COMMAND_QUEUE_DESC�ṹ�壨��������
	//����������Ĭ�ϲ�������������
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	//�����������ָ��
	//ComPtr<ID3D12CommandQueue> cmdQueue;
	ThrowIfFailed(d3dDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&cmdQueue)));
	//�������������ָ��
	//ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ThrowIfFailed(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));
	//���������б�ָ��
	//ComPtr<ID3D12GraphicsCommandList> cmdList;//#ע��˴��Ľӿ�����ID3D12GraphicsCommandList��������ID3D12CommandList
	ThrowIfFailed(d3dDevice->CreateCommandList(
		0,										//#����ֵΪ0����GPU
		D3D12_COMMAND_LIST_TYPE_DIRECT,			//#�����б�����
		cmdAllocator.Get(),						//#����������ӿ�ָ��
		nullptr,								//#��ˮ��״̬����PSO�����ﲻ���ƣ����Կ�ָ��
		IID_PPV_ARGS(&cmdList)					//#���ش����������б�
	));											  
	cmdList->Close();							//#���������б�ǰ���뽫��ر�
}

//7����������
void D3D12Init::CreateSwapChain()
{
	//����������ָ��
	//ComPtr<IDXGISwapChain> swapChain;
	swapChain.Reset();
	DXGI_SWAP_CHAIN_DESC swapChainDesc;										//#�����������ṹ��
	swapChainDesc.BufferDesc.Width = 1280;									//#�������ֱ��ʵĿ��
	swapChainDesc.BufferDesc.Height = 720;									//#�������ֱ��ʵĸ߶�
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;			//#����������ʾ��ʽ
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;					//#ˢ���ʵķ���
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;					//#ˢ���ʵķ�ĸ
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;		//#����ɨ��VS����ɨ��(δָ����)
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;//#ͼ�������Ļ�����죨δָ���ģ�
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;			//#��������Ⱦ����̨������������Ϊ��ȾĿ�꣩
	swapChainDesc.OutputWindow = mhMainWnd;									//#��Ⱦ���ھ��
	swapChainDesc.SampleDesc.Count = 1;										//#���ز�������
	swapChainDesc.SampleDesc.Quality = 0;									//#���ز�������
	swapChainDesc.Windowed = true;											//#�Ƿ񴰿ڻ�
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;				//#�̶�д��
	swapChainDesc.BufferCount = 2;											//#��̨������������˫���壩
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;			//#����Ӧ����ģʽ���Զ�ѡ�������ڵ�ǰ���ڳߴ����ʾģʽ��
	//#����DXGI�ӿ��µĹ����ഴ��������										   
	ThrowIfFailed(dxgiFactory->CreateSwapChain(cmdQueue.Get(), &swapChainDesc, swapChain.GetAddressOf()));
}

//8������������
void D3D12Init::CreateDescriptorHeap()
{
	//#���ȴ���RTV��
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc;
	rtvDescriptorHeapDesc.NumDescriptors = 2;
	rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeapDesc.NodeMask = 0;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvHeap)));

	//#Ȼ�󴴽�DSV��
	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc;
	dsvDescriptorHeapDesc.NumDescriptors = 1;
	dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvDescriptorHeapDesc.NodeMask = 0;
	ComPtr<ID3D12DescriptorHeap> dsvHeap;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&dsvHeap)));
}

void D3D12Init::CreateRTV()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
	//����������������ָ�루���ڻ�ȡ�������еĺ�̨��������Դ��
	//ComPtr<ID3D12Resource> swapChainBuffer[2];
	for (int i = 0; i < 2; i++)
	{
		//#��ô��ڽ������еĺ�̨��������Դ
		swapChain->GetBuffer(i, IID_PPV_ARGS(swapChainBuffer[i].GetAddressOf()));
		//#����RTV
		d3dDevice->CreateRenderTargetView(
			swapChainBuffer[i].Get(),
			nullptr,			//#�ڽ������������Ѿ������˸���Դ�����ݸ�ʽ����������ָ��Ϊ��ָ��
			rtvHeapHandle		//#����������ṹ�壨�����Ǳ��壬�̳���CD3DX12_CPU_DESCRIPTOR_HANDLE��
		);
		//#ƫ�Ƶ����������е���һ��������
		rtvHeapHandle.Offset(1, rtvDescriptorSize);
	}
}

void D3D12Init::CreateDSV()
{
	//�ȴ��������Դ 
	D3D12_RESOURCE_DESC dsvResourceDesc;
	dsvResourceDesc.Alignment = 0;										//#ָ������
	dsvResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;		//#ָ����Դά�ȣ����ͣ�ΪTEXTURE2D
	dsvResourceDesc.DepthOrArraySize = 1;								//#�������Ϊ1
	dsvResourceDesc.Width = 1280;										//#��Դ��
	dsvResourceDesc.Height = 720;										//#��Դ��
	dsvResourceDesc.MipLevels = 1;										//#MIPMAP�㼶����
	dsvResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;				//#ָ�������֣����ﲻָ����
	dsvResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	//#���ģ����Դ��Flag
	dsvResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;				//#24λ��ȣ�8λģ��,���и������͵ĸ�ʽDXGI_FORMAT_R24G8_TYPELESSҲ����ʹ��
	dsvResourceDesc.SampleDesc.Count = 4;								//#���ز�������
	dsvResourceDesc.SampleDesc.Quality = msaaQualityLevels.NumQualityLevels - 1;	//#���ز�������

	CD3DX12_CLEAR_VALUE optClear;						//#�����Դ���Ż�ֵ��������������ִ���ٶȣ�CreateCommittedResource�����д��룩
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;	//#24λ��ȣ�8λģ��,���и������͵ĸ�ʽDXGI_FORMAT_R24G8_TYPELESSҲ����ʹ��
	optClear.DepthStencil.Depth = 1;					//#��ʼ���ֵΪ1
	optClear.DepthStencil.Stencil = 0;					//#��ʼģ��ֵΪ0

	//#����һ����Դ��һ���ѣ�������Դ�ύ�����У������ģ�������ύ��GPU�Դ��У�
	//ComPtr<ID3D12Resource> depthStencilBuffer;
	ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),	//#������ΪĬ�϶ѣ�����д�룩
		D3D12_HEAP_FLAG_NONE,																			//#Flag
		&dsvResourceDesc,																				//#���涨���DSV��Դָ��
		D3D12_RESOURCE_STATE_COMMON,																	//#��Դ��״̬Ϊ��ʼ״̬
		&optClear,																						//#���涨����Ż�ֵָ��
		IID_PPV_ARGS(&depthStencilBuffer)));															//#�������ģ����Դ

	d3dDevice->CreateDepthStencilView(depthStencilBuffer.Get(),
		nullptr,						//#D3D12_DEPTH_STENCIL_VIEW_DESC����ָ�룬����&dsvDesc������ע�ʹ��룩��
										//#�����ڴ������ģ����Դʱ�Ѿ��������ģ���������ԣ������������ָ��Ϊ��ָ��
		dsvHeap->GetCPUDescriptorHandleForHeapStart());	//DSV���
	
	//������д�������б�
	cmdList->ResourceBarrier(1,				//#Barrier���ϸ���
		&CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON,		//#ת��ǰ״̬������ʱ��״̬����CreateCommittedResource�����ж����״̬��
		D3D12_RESOURCE_STATE_DEPTH_WRITE));	//#ת����״̬Ϊ��д������ͼ������һ��D3D12_RESOURCE_STATE_DEPTH_READ��ֻ�ɶ������ͼ
	//������������б����������
	ThrowIfFailed(cmdList->Close());								//#������������ر�
	ID3D12CommandList* cmdLists[] = { cmdList.Get() };				//#���������������б�����//�����ж�������б�
	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);	//#������������б����������
}

//11�����ӿںͲü�����
void D3D12Init::CreateViewPortAndScissorRect()
{
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width = 1280;
	viewPort.Height = 720;
	viewPort.MaxDepth = 1.0f;
	viewPort.MinDepth = 0.0f;
	//#�ü��������ã�����������ض������޳���
	//#ǰ����Ϊ���ϵ����꣬������Ϊ���µ�����
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = 1280;
	scissorRect.bottom = 720;
}

//12ʵ��Χ��
void D3D12Init::FlushCmdQueue()
{
	mCurrentFence++;								//#CPU��������رպ󣬽���ǰΧ��ֵ+1
	cmdQueue->Signal(fence.Get(), mCurrentFence);	//#��GPU������CPU���������󣬽�fence�ӿ��е�Χ��ֵ+1����fence->GetCompletedValue()+1
	if (fence->GetCompletedValue() < mCurrentFence)	//#���С�ڣ�˵��GPUû�д�������������
	{
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");	//#�����¼�
		fence->SetEventOnCompletion(mCurrentFence, eventHandle);//#��Χ���ﵽmCurrentFenceֵ����ִ�е�Signal����ָ���޸���Χ��ֵ��ʱ������eventHandle�¼�
		WaitForSingleObject(eventHandle, INFINITE);//#�ȴ�GPU����Χ���������¼���������ǰ�߳�ֱ���¼�������ע���Enent���������ٵȴ���
							   //#���û��Set��Wait���������ˣ�Set��Զ������ã�����Ҳ��û�߳̿��Ի�������̣߳�
		CloseHandle(eventHandle);
	}
}

