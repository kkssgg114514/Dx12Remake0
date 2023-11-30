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
	//����������������������������������������Ϊ��������
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];
	//�����ɵ���CBV��ɵ���������
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,	//����������
		1,											//����������
		0);											//�������󶨵ļĴ����ۺ�
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);
	//��ǩ����һ�����������
	CD3DX12_ROOT_SIGNATURE_DESC rootSig(
		1,																//������������
		slotRootParameter,												//������ָ��
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);
	//�õ����Ĵ����۴���һ����ǩ�����ò�λָ��һ�������е�������������������������
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
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &vertexBufferCpu));		//�������������ڴ�ռ�
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &indexBufferCpu));		//�������������ڴ�ռ�
	CopyMemory(vertexBufferCpu->GetBufferPointer(), vertices.data(), vbByteSize);	//���������ݿ���������ϵͳ�ڴ���
	CopyMemory(indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);		//���������ݿ���������ϵͳ�ڴ���
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
	psoDesc.SampleMask = UINT_MAX;	//0xffffffff,ȫ��������û������
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;	//��һ�����޷�������
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;	//��ʹ��4XMSAA
	psoDesc.SampleDesc.Quality = 0;	////��ʹ��4XMSAA

	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PSO)));
}

void D3D12InitApp::CreateCBV()
{
	objConstSize = ToolFunc::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	//#����CBV��
	//ComPtr<ID3D12DescriptorHeap> cbvHeap;
	D3D12_DESCRIPTOR_HEAP_DESC cbHeapDesc;
	cbHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbHeapDesc.NumDescriptors = 1;
	cbHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&cbHeapDesc, IID_PPV_ARGS(&cbvHeap)));

	//#���岢�������ĳ�����������Ȼ��õ����׵�ַ
	objCB = nullptr;
	//#elementCountΪ1��1�������峣������Ԫ�أ���isConstantBufferΪture���ǳ�����������
	objCB = std::make_unique<UploadBufferResource<ObjectConstants>>(d3dDevice.Get(), 1, true);
	//#��ó����������׵�ַ
	D3D12_GPU_VIRTUAL_ADDRESS address;
	address = objCB->Resource()->GetGPUVirtualAddress();
	//#ͨ������������Ԫ��ƫ��ֵ�������յ�Ԫ�ص�ַ
	int cbElementIndex = 0;	//#����������Ԫ���±�
	address += cbElementIndex * objConstSize;
	//#����CBV������
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = address;
	cbvDesc.SizeInBytes = objConstSize;
	d3dDevice->CreateConstantBufferView(&cbvDesc, cbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void D3D12InitApp::Update()
{
	D3D12App::Update();
	ObjectConstants objConstants;
	//�����۲����
	float x = 0.0f;
	float y = 0.0f;
	float z = 5.0f;
	XMVECTOR pos = XMVectorSet(z, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX v = XMMatrixLookAtLH(pos, target, up);

	//����ͶӰ����
	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * 3.1416f, 1280.0f / 720.0f, 1.0f, 1000.0f);
	//XMStoreFloat4x4(&proj, p);
	//�����������
	XMMATRIX w = XMLoadFloat4x4(&world);
	//�������
	XMMATRIX WVP_Matrix = v * p;
	//XMMATRIX��ֵ��XMLOAT4X4
	XMStoreFloat4x4(&objConstants.worldVeiwProj, XMMatrixTranspose(WVP_Matrix));
	//�����ݿ�����GPU����
	objCB->CopyData(0, objConstants);
}

D3D12_VERTEX_BUFFER_VIEW D3D12InitApp::GetVbv() const
{
	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = vertexBufferGpu->GetGPUVirtualAddress();	//���㻺������Դ�����ַ
	vbv.SizeInBytes = vbByteSize;									//���㻺������С�����ж��㣩
	vbv.StrideInBytes = sizeof(Vertex);								//ÿ������Ԫ����ռ�õ��ֽ���

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
	//���ø�ǩ��
	cmdList->SetGraphicsRootSignature(rootSignature.Get());
	//���ö��㻺����
	cmdList->IASetVertexBuffers(0, 1, &GetVbv());
	//��ͼԪ�������ʹ�����ˮ��
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//���ø���������
	cmdList->SetGraphicsRootDescriptorTable(0,			//����������ʼ����
		cbvHeap->GetGPUDescriptorHandleForHeapStart());

	//���ƶ��㣨ͨ���������������ƣ�
	cmdList->DrawIndexedInstanced(sizeof(indices),	//#ÿ��ʵ��Ҫ���Ƶ�������
		1,											//#ʵ��������
		0,											//#��ʼ����λ��
		0,											//#��������ʼ������ȫ�������е�λ��
		0);											//#ʵ�����ĸ߼���������ʱ����Ϊ0

	//���ø��෽��
	D3D12App::Draw();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nShowCmd)
{
	//�������Բ�
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