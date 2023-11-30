#include "D3D12InitApp.h"


D3D12InitApp::D3D12InitApp()
	:D3D12App()
{
	vertexBufferGpu = nullptr; 
	indexBufferGpu = nullptr;
	indexBufferCpu = nullptr;
	rootSignature = nullptr;
	PSO = nullptr;
}

D3D12InitApp::~D3D12InitApp()
{
}

bool D3D12InitApp::Init(HINSTANCE hInstance, int nShowCmd, std::wstring customCaption)
{
	if (!D3D12App::Init(hInstance, nShowCmd, customCaption))
	{
		return false;
	}
	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));

	CreateCBV();
	BuildRootSignature();
	BuildByteCodeAndInputLayout();
	BuildGeometry();
	BuildPSO();

	ThrowIfFailed(cmdList->Close());
	ID3D12CommandList* cmdLists[] = { cmdList.Get() };
	cmdQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCmdQueue();

	return true;
}

void D3D12InitApp::BuildRootSignature()
{
	//根参数可以是描述符表、根描述符、根常量，此为描述符表
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];
	//创建由单个CBV组成的描述符表
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,	//描述符类型
		1,											//描述符数量
		0);											//描述符绑定的寄存器槽号
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);
	//根签名由一组根参数构成
	CD3DX12_ROOT_SIGNATURE_DESC rootSig(
		1,																//根参数的数量
		slotRootParameter,												//根参数指针
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);
	//用单个寄存器槽创建一个根签名，该槽位指向一个仅含有单个常量缓冲区的描述符区域
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSig, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob);

	if (errorBlob != nullptr)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(d3dDevice->CreateRootSignature(0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature)));
}

void D3D12InitApp::BuildByteCodeAndInputLayout()
{
	inputLayoutDesc =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	vsBytecode = nullptr;
	psBytecode = nullptr;
	HRESULT hr = S_OK;
	vsBytecode = ToolFunc::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	psBytecode = ToolFunc::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");
}

void D3D12InitApp::BuildGeometry()
{
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &vertexBufferCpu));		//创建顶点数据内存空间
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &indexBufferCpu));		//创建索引数据内存空间
	CopyMemory(vertexBufferCpu->GetBufferPointer(), vertices.data(), vbByteSize);	//将顶点数据拷贝至顶点系统内存中
	CopyMemory(indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);		//将索引数据拷贝至索引系统内存中
	vertexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vbByteSize, vertices.data(), vertexBufferUploader);
	indexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), ibByteSize, indices.data(), indexBufferUploader);
}

void D3D12InitApp::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { inputLayoutDesc.data(), (UINT)inputLayoutDesc.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = { reinterpret_cast<BYTE*>(vsBytecode->GetBufferPointer()), vsBytecode->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<BYTE*>(psBytecode->GetBufferPointer()), psBytecode->GetBufferSize() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;	//0xffffffff,全部采样，没有遮罩
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	//归一化的无符号整型
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;	//不使用4XMSAA
	psoDesc.SampleDesc.Quality = 0;	////不使用4XMSAA

	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PSO)));
}

void D3D12InitApp::CreateCBV()
{
	objConstSize = ToolFunc::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	//#创建CBV堆
	//ComPtr<ID3D12DescriptorHeap> cbvHeap;
	D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc;
	cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbHeapDesc.NumDescriptors = 1;
	cbHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(&cbvHeap)));

	//#定义并获得物体的常量缓冲区，然后得到其首地址
	objCB = nullptr;
	//#elementCount为1（1个子物体常量缓冲元素），isConstantBuffer为ture（是常量缓冲区）
	objCB = std::make_unique<UploadBufferResource<ObjectConstants>>(d3dDevice.Get(), 1, true);
	//#获得常量缓冲区首地址
	D3D12_GPU_VIRTUAL_ADDRESS address;
	address = objCB->Resource()->GetGPUVirtualAddress();
	//#通过常量缓冲区元素偏移值计算最终的元素地址
	int cbElementIndex = 0;	//#常量缓冲区元素下标
	address += cbElementIndex * objConstSize;
	//#创建CBV描述符
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = address;
	cbvDesc.SizeInBytes = objConstSize;
	d3dDevice->CreateConstantBufferView(&cbvDesc, cbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void D3D12InitApp::Update()
{
	D3D12App::Update();
	ObjectConstants objConstants;
	//构建观察矩阵
	float x = 0.0f;
	float y = 0.0f;
	float z = 5.0f;
	XMVECTOR pos = XMVectorSet(z, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX v = XMMatrixLookAtLH(pos, target, up);

	//构建投影矩阵
	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * 3.1416f, 1280.0f / 720.0f, 1.0f, 1000.0f);
	//XMStoreFloat4x4(&proj, p);
	//构建世界矩阵
	XMMATRIX w = XMLoadFloat4x4(&world);
	//矩阵计算
	XMMATRIX WVP_Matrix = v * p;
	//XMMATRIX赋值给XMLOAT4X4
	XMStoreFloat4x4(&objConstants.worldVeiwProj, XMMatrixTranspose(WVP_Matrix));
	//将数据拷贝至GPU缓存
	objCB->CopyData(0, objConstants);
}

D3D12_VERTEX_BUFFER_VIEW D3D12InitApp::GetVbv() const
{
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = vertexBufferGpu->GetGPUVirtualAddress();	//顶点缓冲区资源虚拟地址
	vbv.SizeInBytes = vbByteSize;									//顶点缓冲区大小（所有顶点）
	vbv.StrideInBytes = sizeof(Vertex);								//每个顶点元素所占用的字节数

	return vbv;
}

D3D12_INDEX_BUFFER_VIEW D3D12InitApp::GetIbv() const
{
	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = indexBufferGpu->GetGPUVirtualAddress();
	ibv.Format = DXGI_FORMAT_R16_UINT;
	ibv.SizeInBytes = ibByteSize;

	return ibv;
}

void D3D12InitApp::Draw()
{
	ID3D12DescriptorHeap* descriHeaps[] = { cbvHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descriHeaps), descriHeaps);
	//设置根签名
	cmdList->SetGraphicsRootSignature(rootSignature.Get());
	//设置顶点缓冲区
	cmdList->IASetVertexBuffers(0, 1, &GetVbv());
	//将图元拓扑类型传入流水线
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//设置跟描述符表
	cmdList->SetGraphicsRootDescriptorTable(0,			//根参数的起始索引
		cbvHeap->GetGPUDescriptorHandleForHeapStart());

	//绘制顶点（通过索引缓冲区绘制）
	cmdList->DrawIndexedInstanced(sizeof(indices),	//#每个实例要绘制的索引数
		1,											//#实例化个数
		0,											//#起始索引位置
		0,											//#子物体起始索引在全局索引中的位置
		0);											//#实例化的高级技术，暂时设置为0

	//调用父类方法
	D3D12App::Draw();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nShowCmd)
{
	//创建调试层
#if defined(DEBUG)|defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	try
	{
		D3D12InitApp theApp;
		if (!theApp.Init(hInstance, nShowCmd, L""))
		{
			return 0;
		}
		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}

}