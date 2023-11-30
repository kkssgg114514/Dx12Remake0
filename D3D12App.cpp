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
	//消息循环中检测消息

	//#消息循环
	//#定义消息结构体
	MSG msg = { 0 };
	gt.Reset();
	//#如果GetMessage函数不等于0，说明没有接受到WM_QUIT
	while (msg.message != WM_QUIT)
	{
		//对消息进行拾取
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);			//消息转码
			DispatchMessage(&msg);			//消息分发
		}
		else
		{
			gt.Tick();//计算每两帧间隔
			if (!gt.IsStoped())
			{
				//处于运行状态才运行游戏
				CalculateFrameState();
				Update();
				Draw();
			}
			else
			{
				//处于暂停状态，休眠100ms
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
	//填写窗口类

	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;			//#当工作区宽高改变，则重新绘制窗口
	wc.lpfnWndProc = MainWndProc;				//#指定窗口过程//指定处理消息的函数指针
	wc.cbClsExtra = 0;							//#借助这两个字段来为当前应用分配额外的内存空间（这里不分配，所以置0）
	wc.cbWndExtra = 0;							//#借助这两个字段来为当前应用分配额外的内存空间（这里不分配，所以置0）
	wc.hInstance = hInstance;					//#应用程序实例句柄（由WinMain传入）
	wc.hIcon = LoadIcon(0, IDC_ARROW);			//#使用默认的应用程序图标
	wc.hCursor = LoadCursor(0, IDC_ARROW);		//#使用标准的鼠标指针样式
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);		//#指定了白色背景画刷句柄
	wc.lpszMenuName = 0;						//#没有菜单栏
	wc.lpszClassName = L"MainWnd";				//#窗口名

	//#窗口类注册失败
	//注册窗口类
	if (!RegisterClass(&wc))
	{
		//#消息框函数，参数1：消息框所属窗口句柄，可为NULL。参数2：消息框显示的文本信息。参数3：标题文本。参数4：消息框样式
		MessageBox(0, L"RegisterClass Failed", 0, 0);
		return false;
	}

	//#窗口类注册成功
	RECT R;	//#裁剪矩形
	R.left = 0;
	R.top = 0;
	R.right = 1280;
	R.bottom = 720;
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);	//#根据窗口的客户区大小计算窗口的大小
	int width = R.right - R.left;
	int hight = R.bottom - R.top;

	//#创建窗口,返回布尔值
	mhMainWnd = CreateWindow(L"MainWnd", L"DX12Initialize", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, hight, 0, 0, hInstance, 0);
	//#窗口创建失败
	if (!mhMainWnd)
	{
		MessageBox(0, L"CreatWindow Failed", 0, 0);
		return false;
	}
	//#窗口创建成功,则显示并更新窗口
	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
	/*--------------------------------------------------------------------------------------------------------*/
}

bool D3D12App::InitDirect3D()
{
	//开启调试层
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
	CreateViewPortAndScissorRect();
	

	return true;
}

void D3D12App::CreateDevice()
{
	/*--------------------------------------------------------------------------------------------------------*/
//创建设备的两步
//首先创建工厂（Factory接口）
//ComPtr<IDXGIFactory4> dxgiFactory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));
	//然后创建设备
	//ComPtr<ID3D12Device> d3dDevice;
	ThrowIfFailed(D3D12CreateDevice(
		nullptr,						//#此参数如果设置为nullptr，则使用主适配器
		D3D_FEATURE_LEVEL_11_0,			//#应用程序需要硬件所支持的最低功能级别
		IID_PPV_ARGS(&d3dDevice)		//#返回所建设备
	));
	/*--------------------------------------------------------------------------------------------------------*/
}

void D3D12App::CreateFence()
{
	////创建围栏指针
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
	//填写结构体
	msaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				//#UNORM是归一化处理的无符号整数
	msaaQualityLevels.SampleCount = 1;
	msaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msaaQualityLevels.NumQualityLevels = 0;
	//#当前图形驱动对MSAA多重采样的支持（注意：第二个参数即是输入又是输出）//第二个参数是引用······
	ThrowIfFailed(d3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msaaQualityLevels,
		sizeof(msaaQualityLevels)
	));
	//#NumQualityLevels在Check函数里会进行设置
	//#如果支持MSAA，则Check函数返回的NumQualityLevels > 0
	//#expression为假（即为0），则终止程序运行，并打印一条出错信息
	assert(msaaQualityLevels.NumQualityLevels > 0);
}

void D3D12App::CreateCommandObject()
{
	//首先创建D3D12_COMMAND_QUEUE_DESC结构体（描述符）
	//另外两项有默认参数，不用设置
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	//创建命令队列指针
	//ComPtr<ID3D12CommandQueue> cmdQueue;
	ThrowIfFailed(d3dDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&cmdQueue)));
	//创建命令分配器指针
	//ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ThrowIfFailed(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator)));
	//创建命令列表指针
	//ComPtr<ID3D12GraphicsCommandList> cmdList;//#注意此处的接口名是ID3D12GraphicsCommandList，而不是ID3D12CommandList
	ThrowIfFailed(d3dDevice->CreateCommandList(
		0,										//#掩码值为0，单GPU
		D3D12_COMMAND_LIST_TYPE_DIRECT,			//#命令列表类型
		cmdAllocator.Get(),						//#命令分配器接口指针
		nullptr,								//#流水线状态对象PSO，这里不绘制，所以空指针
		IID_PPV_ARGS(&cmdList)					//#返回创建的命令列表
	));
	cmdList->Close();							//#重置命令列表前必须将其关闭
}

void D3D12App::CreateSwapChain()
{
	//创建交换链指针
	//ComPtr<IDXGISwapChain> swapChain;
	swapChain.Reset();
	DXGI_SWAP_CHAIN_DESC swapChainDesc;										//#交换链描述结构体
	swapChainDesc.BufferDesc.Width = 1280;									//#缓冲区分辨率的宽度
	swapChainDesc.BufferDesc.Height = 720;									//#缓冲区分辨率的高度
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;			//#缓冲区的显示格式
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;					//#刷新率的分子
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;					//#刷新率的分母
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;		//#逐行扫描VS隔行扫描(未指定的)
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;//#图像相对屏幕的拉伸（未指定的）
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;			//#将数据渲染至后台缓冲区（即作为渲染目标）
	swapChainDesc.OutputWindow = mhMainWnd;									//#渲染窗口句柄
	swapChainDesc.SampleDesc.Count = 1;										//#多重采样数量
	swapChainDesc.SampleDesc.Quality = 0;									//#多重采样质量
	swapChainDesc.Windowed = true;											//#是否窗口化
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;				//#固定写法
	swapChainDesc.BufferCount = 2;											//#后台缓冲区数量（双缓冲）
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;			//#自适应窗口模式（自动选择最适于当前窗口尺寸的显示模式）
	//#利用DXGI接口下的工厂类创建交换链										   
	ThrowIfFailed(dxgiFactory->CreateSwapChain(cmdQueue.Get(), &swapChainDesc, swapChain.GetAddressOf()));
}

void D3D12App::CreateDescriptorHeap()
{
	//#首先创建RTV堆
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc;
	rtvDescriptorHeapDesc.NumDescriptors = 2;
	rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptorHeapDesc.NodeMask = 0;
	//ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvHeap)));

	//#然后创建DSV堆
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
	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
	//创建交换链缓冲区指针（用于获取交换链中的后台缓冲区资源）
	//ComPtr<ID3D12Resource> swapChainBuffer[2];
	for (int i = 0; i < 2; i++)
	{
		//#获得存于交换链中的后台缓冲区资源
		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(swapChainBuffer[i].GetAddressOf())));
		//#创建RTV
		d3dDevice->CreateRenderTargetView(
			swapChainBuffer[i].Get(),
			nullptr,			//#在交换链创建中已经定义了该资源的数据格式，所以这里指定为空指针
			rtvHeapHandle		//#描述符句柄结构体（这里是变体，继承自CD3DX12_CPU_DESCRIPTOR_HANDLE）
		);
		//#偏移到描述符堆中的下一个缓冲区
		rtvHeapHandle.Offset(1, rtvDescriptorSize);
	}
}

void D3D12App::CreateDSV()
{//先创建深度资源 
	D3D12_RESOURCE_DESC dsvResourceDesc;
	dsvResourceDesc.Alignment = 0;										//#指定对齐
	dsvResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;		//#指定资源维度（类型）为TEXTURE2D
	dsvResourceDesc.DepthOrArraySize = 1;								//#纹理深度为1
	dsvResourceDesc.Width = 1280;										//#资源宽
	dsvResourceDesc.Height = 720;										//#资源高
	dsvResourceDesc.MipLevels = 1;										//#MIPMAP层级数量
	dsvResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;				//#指定纹理布局（这里不指定）
	dsvResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	//#深度模板资源的Flag
	dsvResourceDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;				//#24位深度，8位模板,还有个无类型的格式DXGI_FORMAT_R24G8_TYPELESS也可以使用
	dsvResourceDesc.SampleDesc.Count = 4;								//#多重采样数量
	dsvResourceDesc.SampleDesc.Quality = msaaQualityLevels.NumQualityLevels - 1;	//#多重采样质量

	CD3DX12_CLEAR_VALUE optClear;						//#清除资源的优化值，提高清除操作的执行速度（CreateCommittedResource函数中传入）
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;	//#24位深度，8位模板,还有个无类型的格式DXGI_FORMAT_R24G8_TYPELESS也可以使用
	optClear.DepthStencil.Depth = 1.0f;					//#初始深度值为1
	optClear.DepthStencil.Stencil = 0;					//#初始模板值为0

	//#创建一个资源和一个堆，并将资源提交至堆中（将深度模板数据提交至GPU显存中）
	//ComPtr<ID3D12Resource> depthStencilBuffer;
	ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),	//#堆类型为默认堆（不能写入）
		D3D12_HEAP_FLAG_NONE,																			//#Flag
		&dsvResourceDesc,																				//#上面定义的DSV资源指针
		D3D12_RESOURCE_STATE_COMMON,																	//#资源的状态为初始状态
		&optClear,																						//#上面定义的优化值指针
		IID_PPV_ARGS(&depthStencilBuffer)));															//#返回深度模板资源

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;

	d3dDevice->CreateDepthStencilView(depthStencilBuffer.Get(),
		&dsvDesc,						//#D3D12_DEPTH_STENCIL_VIEW_DESC类型指针，可填&dsvDesc（见上注释代码），
										//#由于在创建深度模板资源时已经定义深度模板数据属性，所以这里可以指定为空指针
		dsvHeap->GetCPUDescriptorHandleForHeapStart());	//DSV句柄

	//ThrowIfFailed(d3dDevice->GetDeviceRemovedReason());

	cmdList->ResourceBarrier(1,	//Barrier屏障个数
		&CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,	//转换前状态（创建时的状态，即CreateCommittedResource函数中定义的状态）
			D3D12_RESOURCE_STATE_DEPTH_WRITE));	//转换后状态为可写入的深度图，还有一个D3D12_RESOURCE_STATE_DEPTH_READ是只可读的深度图


	//将命令从命令列表传入命令队列
	ThrowIfFailed(cmdList->Close());								//#命令添加完后将其关闭
	ID3D12CommandList* cmdLists[] = { cmdList.Get() };				//#声明并定义命令列表数组//可能有多个命令列表
	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);	//#将命令从命令列表传至命令队列

	FlushCmdQueue();

}

void D3D12App::CreateViewPortAndScissorRect()
{
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width = 1280;
	viewPort.Height = 720;
	viewPort.MaxDepth = 1.0f;
	viewPort.MinDepth = 0.0f;
	//#裁剪矩形设置（矩形外的像素都将被剔除）
	//#前两个为左上点坐标，后两个为右下点坐标
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = 1280;
	scissorRect.bottom = 720;

}

void D3D12App::FlushCmdQueue()
{
	mCurrentFence++;								//#CPU传完命令并关闭后，将当前围栏值+1
	cmdQueue->Signal(fence.Get(), mCurrentFence);	//#当GPU处理完CPU传入的命令后，将fence接口中的围栏值+1，即fence->GetCompletedValue()+1
	if (fence->GetCompletedValue() < mCurrentFence)	//#如果小于，说明GPU没有处理完所有命令
	{
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");	//#创建事件
		fence->SetEventOnCompletion(mCurrentFence, eventHandle);//#当围栏达到mCurrentFence值（即执行到Signal（）指令修改了围栏值）时触发的eventHandle事件
		WaitForSingleObject(eventHandle, INFINITE);//#等待GPU命中围栏，激发事件（阻塞当前线程直到事件触发，注意此Enent需先设置再等待，
							   //#如果没有Set就Wait，就死锁了，Set永远不会调用，所以也就没线程可以唤醒这个线程）
		CloseHandle(eventHandle);
	}
}

void D3D12App::CalculateFrameState()
{
	static int frameCnt = 0;			//总帧数
	static float timeElapsed = 0.0f;	//经过的时间
	frameCnt++;							//每帧都加一

	//判断每秒刷新一次
	if (gt.TotalTime() - timeElapsed >= 1.0f)
	{
		float fps = (float)frameCnt;	//每秒帧数
		float mspf = 1000.0f / fps;		//每帧时长

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);
		//将帧数据显示在窗口上
		std::wstring windowText = L"D3D12Init	fps: " + fpsStr + L"	mspf: " + mspfStr;
		SetWindowText(mhMainWnd, windowText.c_str());

		//为计算下一组重置
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}


//#窗口过程函数
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	/*--------------------------------------------------------------------------------------------------------*/
	//#消息处理
	switch (msg)
	{
		//#当窗口被销毁时，终止消息循环
	case WM_DESTROY:
		PostQuitMessage(0);	//#终止消息循环，并发出WM_QUIT消息
		return 0;
	default:
		break;
	}
	//#将上面没有处理的消息转发给默认的窗口过程
	return DefWindowProc(hwnd, msg, wParam, lParam);
	/*--------------------------------------------------------------------------------------------------------*/
}
