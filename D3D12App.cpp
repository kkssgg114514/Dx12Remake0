#include "D3D12App.h"

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

D3D12App::D3D12App()
{
	mCurrentFence = 0;
	mCurrentBackBuffer = 0;
	rtvDescriptorSize = 0;
	dsvDescriptorSize = 0;
	cbv_srv_uavDescriptorSize = 0;
}

D3D12App::~D3D12App()
{
}

int D3D12App::Run()
{
	//��Ϣѭ���м����Ϣ

	//#��Ϣѭ��
	//#������Ϣ�ṹ��
	MSG msg = { 0 };
	gt.Reset();
	//#���GetMessage����������0��˵��û�н��ܵ�WM_QUIT
	while (msg.message != WM_QUIT)
	{
		//����Ϣ����ʰȡ
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);			//��Ϣת��
			DispatchMessage(&msg);			//��Ϣ�ַ�
		}
		else
		{
			gt.Tick();//����ÿ��֡���
			if (!gt.IsStoped())
			{
				//��������״̬��������Ϸ
				CalculateFrameState();
				Draw();
				Update();
			}
			else
			{
				//������ͣ״̬������100ms
				Sleep(100);
			}
		}
	}
	return (int)msg.wParam;
	/*--------------------------------------------------------------------------------------------------------*/
}

bool D3D12App::Init(HINSTANCE hInstance, int nShowCmd, std::wstring customCaption)
{
	if (!InitWindow(hInstance, nShowCmd))
	{
		return false;
	}
	else if (!InitDirect3D())
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool D3D12App::InitWindow(HINSTANCE hInstance, int nShowCmd)
{
	/*--------------------------------------------------------------------------------------------------------*/
	//��д������

	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;			//#����������߸ı䣬�����»��ƴ���
	wc.lpfnWndProc = MainWndProc;				//#ָ�����ڹ���//ָ��������Ϣ�ĺ���ָ��
	wc.cbClsExtra = 0;							//#�����������ֶ���Ϊ��ǰӦ�÷��������ڴ�ռ䣨���ﲻ���䣬������0��
	wc.cbWndExtra = 0;							//#�����������ֶ���Ϊ��ǰӦ�÷��������ڴ�ռ䣨���ﲻ���䣬������0��
	wc.hInstance = hInstance;					//#Ӧ�ó���ʵ���������WinMain���룩
	wc.hIcon = LoadIcon(0, IDC_ARROW);			//#ʹ��Ĭ�ϵ�Ӧ�ó���ͼ��
	wc.hCursor = LoadCursor(0, IDC_ARROW);		//#ʹ�ñ�׼�����ָ����ʽ
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);		//#ָ���˰�ɫ������ˢ���
	wc.lpszMenuName = 0;						//#û�в˵���
	wc.lpszClassName = L"MainWnd";				//#������

	//#������ע��ʧ��
	//ע�ᴰ����
	if (!RegisterClass(&wc))
	{
		//#��Ϣ����������1����Ϣ���������ھ������ΪNULL������2����Ϣ����ʾ���ı���Ϣ������3�������ı�������4����Ϣ����ʽ
		MessageBox(0, L"RegisterClass Failed", 0, 0);
		return false;
	}

	//#������ע��ɹ�
	RECT R;	//#�ü�����
	R.left = 0;
	R.top = 0;
	R.right = 1280;
	R.bottom = 720;
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);	//#���ݴ��ڵĿͻ�����С���㴰�ڵĴ�С
	int width = R.right - R.left;
	int hight = R.bottom - R.top;

	//#��������,���ز���ֵ
	mhMainWnd = CreateWindow(L"MainWnd", L"DX12Initialize", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, hight, 0, 0, hInstance, 0);
	//#���ڴ���ʧ��
	if (!mhMainWnd)
	{
		MessageBox(0, L"CreatWindow Failed", 0, 0);
		return false;
	}
	//#���ڴ����ɹ�,����ʾ�����´���
	ShowWindow(mhMainWnd, nShowCmd);
	UpdateWindow(mhMainWnd);

	return true;
	/*--------------------------------------------------------------------------------------------------------*/
}

bool D3D12App::InitDirect3D()
{
	//�������Բ�
#if defined(DEBUG) ||defined(_DEBUG)
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
#endif  

	CreateDevice();
	CreateFence();
	GetDescriptorSize();
	SetMSAA();
	CreateCommandObject();
	CreateSwapChain();
	CreateDescriptorHeap();
	CreateRTV();
	CreateDSV();
	CreateVertexView();
	CreateIndexView();
	CreateViewPortAndScissorRect();

	return true;
}

void D3D12App::Draw()
{
	//����������ڴ�����
	ThrowIfFailed(cmdAllocator->Reset());
	//�����б��ڴ�����
	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));

	//����̨��������Դ�ӳ���״̬ת������ȾĿ��״̬(׼������ͼ��)
	UINT& ref_mCurrentBackBuffer = mCurrentBackBuffer;
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET));

	//�����ӿںͲü�����
	cmdList->RSSetViewports(1, &viewPort);
	cmdList->RSSetScissorRects(1, &scissorRect);

	//�����̨����������Ȼ�����������ֵ����ɫ��
	//��ȡRTV���
	//��Ϊ������RTV����ҲҪ����ƫ�����ʹ�С
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), ref_mCurrentBackBuffer, rtvDescriptorSize);
	cmdList->ClearRenderTargetView(rtvHandle, DirectX::Colors::LightBlue, 0, nullptr);//���RT����
	//��ȡDSV���
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	cmdList->ClearDepthStencilView(dsvHandle,				//DSV������
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,	//FLAG
		1.0f,												//Ĭ�����ֵ
		0,													//Ĭ��ģ��ֵ
		0,													//�ü���������
		nullptr);

	//ָ��Ҫ��Ⱦ�Ļ�����
	cmdList->OMSetRenderTargets(
		1,				//���󶨵�RTV����
		&rtvHandle,		//ָ��RTV�����ָ��
		true,			//RTV�����ڶ��ڴ�����������ŵ�
		&dsvHandle		//ָ��DSV��ָ��
	);

	//�ȴ���Ⱦ��ɣ��ı��̨��������״̬Ϊ����״̬����֮������͵�ǰ̨������
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	//��ɺ�ر������б�
	ThrowIfFailed(cmdList->Close());

	//���ȴ�ִ�е������б����GPU����
	//�����ж�������б�
	ID3D12CommandList* commandLists[] = { cmdList.Get() };
	cmdQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	//����ǰ��̨����������
	ThrowIfFailed(swapChain->Present(0, 0));
	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;

	//����Χ��ֵ��ʹCPU��GPUͬ��
	FlushCmdQueue();
}

void D3D12App::Update()
{
}

void D3D12App::CreateDevice()
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

void D3D12App::CreateFence()
{
	////����Χ��ָ��
	//ComPtr<ID3D12Fence> fence;
	ThrowIfFailed(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
}

void D3D12App::GetDescriptorSize()
{
	rtvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	dsvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	cbv_srv_uavDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void D3D12App::SetMSAA()
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

void D3D12App::CreateCommandObject()
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

void D3D12App::CreateSwapChain()
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

void D3D12App::CreateDescriptorHeap()
{
	//#���ȴ���RTV��
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc;
	rtvDescriptorHeapDesc.NumDescriptors = 2;
	rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeapDesc.NodeMask = 0;
	//ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvHeap)));

	//#Ȼ�󴴽�DSV��
	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc;
	dsvDescriptorHeapDesc.NumDescriptors = 1;
	dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvDescriptorHeapDesc.NodeMask = 0;
	//ComPtr<ID3D12DescriptorHeap> dsvHeap;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&dsvHeap)));

}

void D3D12App::CreateRTV()
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

void D3D12App::CreateDSV()
{//�ȴ��������Դ 
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


	//������������б����������
	//ThrowIfFailed(cmdList->Close());								//#������������ر�
	//ID3D12CommandList* cmdLists[] = { cmdList.Get() };				//#���������������б�����//�����ж�������б�
	//cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);	//#������������б����������
}

void D3D12App::CreateVertexView()
{
	vertexBufferUploader = nullptr;
	vbByteSize = sizeof(indices);


	ThrowIfFailed(D3DCreateBlob(vbByteSize, &vertexBufferCpu));	//�������������ڴ�ռ�
	CopyMemory(vertexBufferCpu->GetBufferPointer(), vertices.data(), vbByteSize);//���������ݿ�����ϵͳ�����ڴ���
	vertexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vbByteSize, vertices.data(), vertexBufferUploader.Get());
	//���������ݰ�����Ⱦ��ˮ��
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = vertexBufferGpu->GetGPUVirtualAddress();	//���㻺������Ĭ�϶ѣ���Դ�����ַ
	vbv.SizeInBytes = sizeof(Vertex) * 8;							//���㻺������С
	vbv.StrideInBytes = sizeof(Vertex);								//ÿ������ռ�õ��ֽ���
	//���ö��㻺����
	cmdList->IASetVertexBuffers(0, 1, &vbv);
}

void D3D12App::CreateIndexView()
{
	indexBufferUploader = nullptr;
	ibByteSize = sizeof(indices);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &indexBufferCpu));	//�������������ڴ�ռ�
	CopyMemory(indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);//���������ݿ�����ϵͳ�����ڴ���
	indexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), ibByteSize, indices.data(), indexBufferUploader.Get());
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = indexBufferGpu->GetGPUVirtualAddress();
	ibv.Format = DXGI_FORMAT_R16_UINT;
	ibv.SizeInBytes = ibByteSize;
}

void D3D12App::CreateViewPortAndScissorRect()
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

void D3D12App::FlushCmdQueue()
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

void D3D12App::CalculateFrameState()
{
	static int frameCnt = 0;			//��֡��
	static float timeElapsed = 0.0f;	//������ʱ��
	frameCnt++;							//ÿ֡����һ

	//�ж�ÿ��ˢ��һ��
	if (gt.TotalTime() - timeElapsed >= 1.0f)
	{
		float fps = (float)frameCnt;	//ÿ��֡��
		float mspf = 1000.0f / fps;		//ÿ֡ʱ��

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);
		//��֡������ʾ�ڴ�����
		std::wstring windowText = L"D3D12Init	fps: " + fpsStr + L"	mspf: " + mspfStr;
		SetWindowText(mhMainWnd, windowText.c_str());

		//Ϊ������һ������
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}


//#���ڹ��̺���
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	/*--------------------------------------------------------------------------------------------------------*/
	//#��Ϣ����
	switch (msg)
	{
		//#�����ڱ�����ʱ����ֹ��Ϣѭ��
	case WM_DESTROY:
		PostQuitMessage(0);	//#��ֹ��Ϣѭ����������WM_QUIT��Ϣ
		return 0;
	default:
		break;
	}
	//#������û�д������Ϣת����Ĭ�ϵĴ��ڹ���
	return DefWindowProc(hwnd, msg, wParam, lParam);
	/*--------------------------------------------------------------------------------------------------------*/
}
