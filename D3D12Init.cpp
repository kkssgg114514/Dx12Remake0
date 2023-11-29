#include "D3D12Init.h"
#include "tiff.h"

//2创建设备
void D3D12Init::CreateDevice()
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
		D3D_FEATURE_LEVEL_12_0,			//#应用程序需要硬件所支持的最低功能级别
		IID_PPV_ARGS(&d3dDevice)		//#返回所建设备
	));
	/*--------------------------------------------------------------------------------------------------------*/
}

//3创建围栏
void D3D12Init::CreateFence()
{
	////创建围栏指针
	//ComPtr<ID3D12Fence> fence;
	ThrowIfFailed(d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
}

//4获取描述符大小
void D3D12Init::GetDescriptorSize()
{
	rtvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	dsvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	cbv_srv_uavDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

//5设置MSAA抗锯齿属性
void D3D12Init::SetMSAA()
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

//6创建命令队列、命令列表、命令分配器
void D3D12Init::CreateCommandObject()
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

//7创建交换链
void D3D12Init::CreateSwapChain()
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

//8创建描述符堆
void D3D12Init::CreateDescriptorHeap()
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

void D3D12Init::CreateRTV()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());
	//创建交换链缓冲区指针（用于获取交换链中的后台缓冲区资源）
	//ComPtr<ID3D12Resource> swapChainBuffer[2];
	for (int i = 0; i < 2; i++)
	{
		//#获得存于交换链中的后台缓冲区资源
		swapChain->GetBuffer(i, IID_PPV_ARGS(swapChainBuffer[i].GetAddressOf()));
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

void D3D12Init::CreateDSV()
{
	//先创建深度资源 
	D3D12_RESOURCE_DESC dsvResourceDesc;
	dsvResourceDesc.Alignment = 0;										//#指定对齐
	dsvResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;		//#指定资源维度（类型）为TEXTURE2D
	dsvResourceDesc.DepthOrArraySize = 1;								//#纹理深度为1
	dsvResourceDesc.Width = 1280;										//#资源宽
	dsvResourceDesc.Height = 720;										//#资源高
	dsvResourceDesc.MipLevels = 1;										//#MIPMAP层级数量
	dsvResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;				//#指定纹理布局（这里不指定）
	dsvResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	//#深度模板资源的Flag
	dsvResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;				//#24位深度，8位模板,还有个无类型的格式DXGI_FORMAT_R24G8_TYPELESS也可以使用
	dsvResourceDesc.SampleDesc.Count = 4;								//#多重采样数量
	dsvResourceDesc.SampleDesc.Quality = msaaQualityLevels.NumQualityLevels - 1;	//#多重采样质量

	CD3DX12_CLEAR_VALUE optClear;						//#清除资源的优化值，提高清除操作的执行速度（CreateCommittedResource函数中传入）
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;	//#24位深度，8位模板,还有个无类型的格式DXGI_FORMAT_R24G8_TYPELESS也可以使用
	optClear.DepthStencil.Depth = 1;					//#初始深度值为1
	optClear.DepthStencil.Stencil = 0;					//#初始模板值为0

	//#创建一个资源和一个堆，并将资源提交至堆中（将深度模板数据提交至GPU显存中）
	//ComPtr<ID3D12Resource> depthStencilBuffer;
	ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),	//#堆类型为默认堆（不能写入）
		D3D12_HEAP_FLAG_NONE,																			//#Flag
		&dsvResourceDesc,																				//#上面定义的DSV资源指针
		D3D12_RESOURCE_STATE_COMMON,																	//#资源的状态为初始状态
		&optClear,																						//#上面定义的优化值指针
		IID_PPV_ARGS(&depthStencilBuffer)));															//#返回深度模板资源

	d3dDevice->CreateDepthStencilView(depthStencilBuffer.Get(),
		nullptr,						//#D3D12_DEPTH_STENCIL_VIEW_DESC类型指针，可填&dsvDesc（见上注释代码），
										//#由于在创建深度模板资源时已经定义深度模板数据属性，所以这里可以指定为空指针
		dsvHeap->GetCPUDescriptorHandleForHeapStart());	//DSV句柄
	

	//将命令从命令列表传入命令队列
	//ThrowIfFailed(cmdList->Close());								//#命令添加完后将其关闭
	//ID3D12CommandList* cmdLists[] = { cmdList.Get() };				//#声明并定义命令列表数组//可能有多个命令列表
	//cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);	//#将命令从命令列表传至命令队列
}

//11设置视口和裁剪矩形
void D3D12Init::CreateViewPortAndScissorRect()
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

//12实现围栏
void D3D12Init::FlushCmdQueue()
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

bool D3D12Init::InitDirect3D()
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

void D3D12Init::Draw()
{
	//命令分配器内存重置
	ThrowIfFailed(cmdAllocator->Reset());
	//命令列表内存重置
	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));

	//将后台缓冲区资源从呈现状态转换到渲染目标状态(准备接受图像)
	UINT& ref_mCurrentBackBuffer = mCurrentBackBuffer;
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET));

	//设置视口和裁剪矩形
	cmdList->RSSetViewports(1, &viewPort);
	cmdList->RSSetScissorRects(1, &scissorRect);

	//清除后台缓冲区和深度缓冲区，并赋值（上色）
	//获取RTV句柄
	//因为有两个RTV所以也要设置偏移量和大小
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvHeap->GetCPUDescriptorHandleForHeapStart(), ref_mCurrentBackBuffer, rtvDescriptorSize);
	cmdList->ClearRenderTargetView(rtvHandle, DirectX::Colors::DarkRed, 0, nullptr);//清除RT背景
	//获取DSV句柄
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	cmdList->ClearDepthStencilView(dsvHandle,				//DSV描述符
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,	//FLAG
		1.0f,												//默认深度值
		0,													//默认模板值
		0,													//裁剪矩形数量
		nullptr);

	//指定要渲染的缓冲区
	cmdList->OMSetRenderTargets(
		1,				//待绑定的RTV数量
		&rtvHandle,		//指向RTV数组的指针
		true,			//RTV对象在堆内存中是连续存放的
		&dsvHandle		//指向DSV的指针
	);

	//等待渲染完成，改变后台缓冲区的状态为呈现状态，这之后会推送到前台缓冲区
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(swapChainBuffer[ref_mCurrentBackBuffer].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	//完成后关闭命令列表
	ThrowIfFailed(cmdList->Close());

	//将等待执行的命令列表加入GPU队列
	//可能有多个命令列表
	ID3D12CommandList* commandLists[] = { cmdList.Get() };
	cmdQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	//交换前后台缓冲区索引
	ThrowIfFailed(swapChain->Present(0, 0));
	ref_mCurrentBackBuffer = (ref_mCurrentBackBuffer + 1) % 2;

	//设置围栏值，使CPU和GPU同步
	FlushCmdQueue();
}

