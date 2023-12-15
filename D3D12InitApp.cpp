#include "D3D12InitApp.h"
#include "ProceduralGeometry.h"
#include "Common/DDSTextureLoader.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		D3D12InitApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

D3D12InitApp::D3D12InitApp(HINSTANCE hInstance)
	: D3D12App(hInstance)
{
}

D3D12InitApp::~D3D12InitApp()
{
}

bool D3D12InitApp::Initialize()
{
	if (!D3D12App::Initialize())
		return false;

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), nullptr));

	//��ʼ����������
	waves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

	//LoadBoxTextures();
	

	
	//BuildSRV();
	


	////����ɽ������
	//BuildHillGeometry();
	//BuildLakeIndexBuffer();
	//BuildBoxGeometry();
	//BuildHillMaterials();
	//BuildHillRenderItem();
	LoadRoomTextures();
	BuildRootSignature();
	CreateRoomSRV();
	BuildShadersAndInputLayout();
	//��������
	BuildRoomGeometry();
	BuildSkullGeometry();
	BuildMaterials();
	BuildRoomRenderItem();

	//BuildSkullGeometry();

	BuildFrameResources();
	//BuildConstantBuffers();
	BuildPSO();

	// Execute the initialization commands.
	ThrowIfFailed(cmdList->Close());
	ID3D12CommandList* cmdsLists[] = { cmdList.Get() };
	cmdQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until initialization is complete.
	FlushCommandQueue();

	return true;
}

void D3D12InitApp::OnResize()
{
	D3D12App::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * 3.1416f, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void D3D12InitApp::Update(const GameTime& gt)
{
	OnKeyboardInput(gt);

	currFrameResourceIndex = (currFrameResourceIndex + 1) % frameResourcesCount;
	//�״γ�ʼ��
	currFrameResource = FrameResourcesArray[currFrameResourceIndex].get();

	//���GPU��Χ��ֵС��CPU��Χ��ֵ����CPU�ٶȿ���GPU������CPU�ȴ�
	if (currFrameResource->fenceCPU != 0 && fence->GetCompletedValue() < currFrameResource->fenceCPU)
	{
		HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");
		ThrowIfFailed(fence->SetEventOnCompletion(currFrameResource->fenceCPU, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle;
	}

	//AnimateMaterials(gt);
	UpdateObjCBs();
	UpdatePassCBs(gt);
	UpdateMatCBs();
	//UpdateWaves(gt);
}

void D3D12InitApp::Draw(const GameTime& gt)
{
	//// Reuse the memory associated with command recording.
	//// We can only reset when the associated command lists have finished execution on the GPU.
	//ThrowIfFailed(cmdAllocator->Reset());

	//// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	//// Reusing the command list reuses memory.
	//ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), mPSO.Get()));

	auto currCmdAllocator = currFrameResource->cmdAllocator;
	ThrowIfFailed(currCmdAllocator->Reset());
	//һ��ʼ�������ò�͸����
	ThrowIfFailed(cmdList->Reset(currCmdAllocator.Get(), PSOs["opaque"].Get()));

	cmdList->RSSetViewports(1, &mScreenViewport);
	cmdList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	cmdList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	cmdList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	cmdList->OMSetRenderTargets(1, &CurrentBackBufferView(), true,
		&DepthStencilView());

	////����CBV��������
	//ID3D12DescriptorHeap* descriptorHeaps[] = { cbvHeap.Get() };
	//cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	//����SRV�������ѣ�������ûʲô����
	ID3D12DescriptorHeap* descriptorHeaps[] = { srvHeap.Get() };
	cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	cmdList->SetGraphicsRootSignature(mRootSignature.Get());

	////���ø���������
	//int passCBVIndex = (int)allRitems.size() * frameResourcesCount + currFrameResourceIndex;
	//auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvHeap->GetGPUDescriptorHandleForHeapStart());
	//passCbvHandle.Offset(passCBVIndex, cbv_srv_uavDescriptorSize);
	//cmdList->SetGraphicsRootDescriptorTable(1,  //����������ʼ����
	//    passCbvHandle);
	UINT passCBByteSize = ToolFunc::CalcConstantBufferByteSize(sizeof(PassConstants));

	//����passCB������
	auto passCB = currFrameResource->passCB->Resource();
	cmdList->SetGraphicsRootConstantBufferView(3,
		passCB->GetGPUVirtualAddress());

	DrawRenderItems(ritemLayer[(int)RenderLayer::Opaque]);

	cmdList->SetGraphicsRootConstantBufferView(3,
		passCB->GetGPUVirtualAddress() + 1 * passCBByteSize);
	DrawRenderItems(ritemLayer[(int)RenderLayer::Reflects]);
	//cmdList->SetPipelineState(PSOs["alphaTest"].Get());
	//DrawRenderItems(ritemLayer[(int)RenderLayer::AlphaTest]);
	//cmdList->SetPipelineState(PSOs["transparent"].Get());
	//DrawRenderItems(ritemLayer[(int)RenderLayer::Transparent]);

	// Indicate a state transition on the resource usage.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(cmdList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { cmdList.Get() };
	cmdQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	//FlushCommandQueue();

	currFrameResource->fenceCPU = ++currentFence;
	cmdQueue->Signal(fence.Get(), currentFence);
}

void D3D12InitApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void D3D12InitApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void D3D12InitApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		// ��������ı���ת�ĽǶȣ��Ӽ����Ըı䷽��
		mTheta -= dx;
		mPhi -= dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, 3.1416f - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.005 unit in the scene.
		float dx = 0.05f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.05f * static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		//�����˹۲�뾶�ķ�Χ����������������ĵ�ľ���
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 120.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void D3D12InitApp::OnKeyboardInput(const GameTime& gt)
{
	const float dt = gt.DeltaTime();
	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		sunTheta -= 1.0f * dt;
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		sunTheta += 1.0f * dt;
	}
	if (GetAsyncKeyState(VK_UP) & 0x8000)
	{
		sunPhi -= 1.0f * dt;
	}
	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
	{
		sunPhi += 1.0f * dt;
	}
	//��PhiԼ����[0, PI/2]֮��
	sunPhi = MathHelper::Clamp(sunPhi, 0.1f, XM_PIDIV2);
}

void D3D12InitApp::BuildConstantBuffers()
{
	////����CBV��
	//UINT objectCount = (UINT)allRitems.size();//�����ܸ���
	//int frameResourcesCount = gNumFrameResources;
	//UINT objConstSize = ToolFunc::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	//UINT passConstSize = ToolFunc::CalcConstantBufferByteSize(sizeof(PassConstants));

	//D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	//cbvHeapDesc.NumDescriptors = (objectCount + 1) * frameResourcesCount;//һ���Ѱ������������+1��CBV
	//cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//cbvHeapDesc.NodeMask = 0;
	//ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
	//    IID_PPV_ARGS(&cbvHeap)));
//
///*------------------------------------------------------------------------------------------------------*/
//    //elementCountΪobjectCount,22��22�������峣������Ԫ�أ���isConstantBufferΪture���ǳ�����������
//    for (int frameIndex = 0; frameIndex < frameResourcesCount; frameIndex++)
//    {
//        for (int i = 0; i < objectCount; i++)
//        {
//            D3D12_GPU_VIRTUAL_ADDRESS objCB_Address;
//            objCB_Address = FrameResourcesArray[frameIndex]->objCB->Resource()->GetGPUVirtualAddress();
//            //�ڻ�ַ�Ļ����ϼ���������±�ʹ�С��ˣ������Ҫ������ĵ�ַ
//            objCB_Address += i * objConstSize;
//
//            //CBV���е�CBVԪ���±꣨������
//            int heapIndex = objectCount * frameIndex + i;
//            //���CBV�ѵ��׵�ַ
//            auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvHeap->GetCPUDescriptorHandleForHeapStart());
//            handle.Offset(heapIndex, cbv_srv_uavDescriptorSize);//CBV�������ֱ�Ӹ���ƫ�������ͶѴ�С�ҵ�λ��
//            //����CBV������
//            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
//            cbvDesc.BufferLocation = objCB_Address;
//            cbvDesc.SizeInBytes = objConstSize;
//
//            //����CBV
//            d3dDevice->CreateConstantBufferView(
//                &cbvDesc,
//                handle);
//        }
//    }
///*------------------------------------------------------------------------------------------------------*/
//    //�����ڶ���CBV(passCBV)
//
//    //��ó����������׵�ַ
//    for (int frameIndex = 0; frameIndex < frameResourcesCount; frameIndex++)
//    {
//        D3D12_GPU_VIRTUAL_ADDRESS passCB_Address;
//        passCB_Address = FrameResourcesArray[frameIndex]->passCB->Resource()->GetGPUVirtualAddress();
//        int passCBElementIndex = 0;
//        passCB_Address += passCBElementIndex * passConstSize;
//        int heapIndex = objectCount * frameResourcesCount + frameIndex;
//        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvHeap->GetCPUDescriptorHandleForHeapStart());
//        handle.Offset(heapIndex, cbv_srv_uavDescriptorSize);
//        //����CBV������
//        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
//        cbvDesc.BufferLocation = passCB_Address;
//        cbvDesc.SizeInBytes = passConstSize;
//
//        d3dDevice->CreateConstantBufferView(
//            &cbvDesc,
//            handle
//        );
//    }
}

void D3D12InitApp::BuildSRV()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NumDescriptors = 3;
	srvHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap)));

	auto woodCrateTex = textures["woodCrateTex"]->resource;
	auto grassTex = textures["grassTex"]->resource;
	auto lakeTex = textures["lakeTex"]->resource;

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(srvHeap->GetCPUDescriptorHandleForHeapStart());
	//SRV�����ṹ��
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = grassTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = grassTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	//����SRV
	d3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, handle);

	//SRV������SRV����ˮ��SRV���ľ��,����ƫ��һ��SRV
	handle.Offset(1, cbv_srv_uavDescriptorSize);
	//SRV�����ṹ���޸�
	srvDesc.Format = lakeTex->GetDesc().Format;//��ͼ��Ĭ�ϸ�ʽ
	srvDesc.Texture2D.MipLevels = lakeTex->GetDesc().MipLevels;//mipmap�㼶����
	//��������ˮ����SRV
	d3dDevice->CreateShaderResourceView(lakeTex.Get(), &srvDesc, handle);

	//SRV������SRV���������SRV���ľ��,����ƫ��һ��SRV
	handle.Offset(1, cbv_srv_uavDescriptorSize);
	//SRV�����ṹ���޸�
	srvDesc.Format = woodCrateTex->GetDesc().Format;//��ͼ��Ĭ�ϸ�ʽ
	srvDesc.Texture2D.MipLevels = woodCrateTex->GetDesc().MipLevels;//mipmap�㼶����
	//�����������䡱��SRV
	d3dDevice->CreateShaderResourceView(woodCrateTex.Get(), &srvDesc, handle);
}

void D3D12InitApp::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE srvTable;
	srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		1,
		0);
	//������������������������������������
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	slotRootParameter[0].InitAsDescriptorTable(1,
		&srvTable,
		D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

	//��ǩ����һ�����������
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4,      //������������
		slotRootParameter,                          //������ָ��
		(UINT)staticSamplers.size(),
		staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	//�õ����Ĵ�����������һ����ǩ�����ò�λָ��һ�������е�������������������������
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(d3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature)));
}

void D3D12InitApp::BuildShadersAndInputLayout()
{
	HRESULT hr = S_OK;

	//������ɫ��ʱ����ĺ�
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	shaders["standardVS"] = ToolFunc::CompileShader(L"Shaders\\shade.hlsl", nullptr, "VS", "vs_5_0");
	shaders["opaquePS"] = ToolFunc::CompileShader(L"Shaders\\shade.hlsl", defines, "PS", "ps_5_0");
	shaders["alphaTestPS"] = ToolFunc::CompileShader(L"Shaders\\shade.hlsl", alphaTestDefines, "PS", "ps_5_0");
	/*   mvsByteCode = ToolFunc::CompileShader(L"Shaders\\shade.hlsl", nullptr, "VS", "vs_5_0");
	   mpsByteCode = ToolFunc::CompileShader(L"Shaders\\shade.hlsl", nullptr, "PS", "ps_5_0");*/

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void D3D12InitApp::BuildHillGeometry()
{
	ProceduralGeometry proceGeo;
	ProceduralGeometry::MeshData grid = proceGeo.CreateGrid(160.0f, 160.0f, 50, 50);

	//��װ���㡢����
	SubmeshGeometry gridSubmesh;
	gridSubmesh.baseVertexLocation = 0;
	gridSubmesh.startIndexLocation = 0;
	gridSubmesh.indexCount = (UINT)grid.Indices32.size();

	//�������㻺��
	size_t VertexCount = grid.Vertices.size();
	std::vector<Vertex> vertices(VertexCount);
	for (int i = 0; i < grid.Vertices.size(); i++)
	{
		vertices[i].Pos = grid.Vertices.at(i).Position;
		vertices[i].Pos.y = GetHillsHeight(vertices.at(i).Pos.x, vertices.at(i).Pos.z);

		vertices[i].Normal = GetHillsNormal(vertices.at(i).Pos.x, vertices.at(i).Pos.z);
		vertices[i].TexC = grid.Vertices.at(i).TexC;
		////��ɫ��RGBA
		//if (vertices.at(i).Pos.y < -10.0f)
		//{
		//    vertices[i].Color = XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
		//}
		//else if (vertices[i].Pos.y < 5.0f)
		//{
		//    vertices[i].Color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
		//}
		//else if (vertices[i].Pos.y < 12.0f)
		//{
		//    vertices[i].Color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
		//}
		//else if (vertices[i].Pos.y < 20.0f)
		//{
		//    vertices[i].Color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
		//}
		//else
		//{
		//    vertices[i].Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		//}
	}

	//������������
	std::vector<std::uint16_t> indices = grid.GetIndices16();

	//���㶥�㻺������������С
	auto geo = std::make_unique<MeshGeometry>();
	geo->name = "landGeo";
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
	geo->vertexBufferByteSize = vbByteSize;
	geo->indexBufferByteSize = ibByteSize;
	geo->vertexByteStride = sizeof(Vertex);
	geo->indexFormat = DXGI_FORMAT_R16_UINT;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->vertexBufferCpu));     //�������������ڴ�ռ�
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->indexBufferCpu));
	CopyMemory(geo->vertexBufferCpu->GetBufferPointer(), vertices.data(), vbByteSize);
	CopyMemory(geo->indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);
	geo->vertexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize, geo->vertexBufferUploader);
	geo->indexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize, geo->indexBufferUploader);

	//����װ�õļ�����SubmeshGeometry����ֵ�������
	geo->DrawArgs["landGrid"] = gridSubmesh;
	//��ɽ�������񼸺������ܱ�
	geometries["landGeo"] = std::move(geo);
}

void D3D12InitApp::BuildBoxGeometry()
{
	ProceduralGeometry proceGeo;
	ProceduralGeometry::MeshData box = proceGeo.CreateBox(8.0f, 8.0f, 8.0f, 3);

	//���������ݴ���Vertex�ṹ���Ԫ������
	size_t verticesCount = box.Vertices.size();
	std::vector<Vertex> vertices(verticesCount);
	for (size_t i = 0; i < verticesCount; i++)
	{
		vertices[i].Pos = box.Vertices[i].Position;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].TexC = box.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices = box.GetIndices16();

	//�����б��С
	const UINT vbByteSize = (UINT)verticesCount * sizeof(Vertex);
	//�����б��С
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	//����������
	SubmeshGeometry submesh;
	submesh.baseVertexLocation = 0;
	submesh.startIndexLocation = 0;
	submesh.indexCount = (UINT)indices.size();

	//��ֵmg�е�����Ԫ��
	auto geo = std::make_unique<MeshGeometry>();
	geo->name = "boxGeo";
	geo->vertexByteStride = sizeof(Vertex);
	geo->vertexBufferByteSize = vbByteSize;
	geo->indexBufferByteSize = ibByteSize;
	geo->indexFormat = DXGI_FORMAT_R16_UINT;
	geo->DrawArgs["box"] = submesh;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->vertexBufferCpu));
	CopyMemory(geo->vertexBufferCpu->GetBufferPointer(), vertices.data(), vbByteSize);
	geo->vertexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(),
		vertices.data(), vbByteSize, geo->vertexBufferUploader);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->indexBufferCpu));
	CopyMemory(geo->indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);
	geo->indexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(),
		indices.data(), ibByteSize, geo->indexBufferUploader);

	geometries["boxGeo"] = std::move(geo);
}

void D3D12InitApp::BuildPSO()
{
	//Ҫ���ݲ�ͬ����Ⱦ���ʹ�����ͬ��PSO���ֱ��ǲ�͸��������Ҫ��ϣ���alpha���Եģ�����Ҫ��ϣ���͸������Ҫ��ϣ���
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc = {};
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(shaders["standardVS"]->GetBufferPointer()),
		shaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["opaquePS"]->GetBufferPointer()),
		shaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&PSOs["opaque"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestPsoDesc = opaquePsoDesc;//ʹ�ò�͸����PSO
	alphaTestPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(shaders["alphaTestPS"]->GetBufferPointer()),
		shaders["alphaTestPS"]->GetBufferSize()
	};
	alphaTestPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//˫����ʾ
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&alphaTestPsoDesc, IID_PPV_ARGS(&PSOs["alphaTest"])));

	//͸�������PSO����Ҫ��ϣ�
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;//ʹ�ò�͸����PSO
	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;                               //�Ƿ��������ϣ�Ĭ��ֵΪfalse��
	transparencyBlendDesc.LogicOpEnable = false;                            //�Ƿ����߼����(Ĭ��ֵΪfalse)
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;                 //RGB����е�Դ�������Fsrc������ȡԴ��ɫ��alphaͨ��ֵ��
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;            //RGB����е�Ŀ��������Fdest������ȡ1-alpha��
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;                     //RGB��������(����ѡ��ӷ�)
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;	                //alpha����е�Դ�������Fsrc��ȡ1��
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;                //alpha����е�Ŀ��������Fsrc��ȡ0��
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;                //alpha��������(����ѡ��ӷ�)
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;	                //�߼���������(�ղ���������ʹ��)
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;//��̨������д�����֣�û�����֣���ȫ��д�룩

	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&PSOs["transparent"])));

	//D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	//ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	//psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	//psoDesc.pRootSignature = mRootSignature.Get();
	//psoDesc.VS =
	//{
	//    reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
	//    mvsByteCode->GetBufferSize()
	//};
	//psoDesc.PS =
	//{
	//    reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
	//    mpsByteCode->GetBufferSize()
	//};
	//psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	////psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	//psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	//psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	//psoDesc.SampleMask = UINT_MAX;
	//psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//psoDesc.NumRenderTargets = 1;
	//psoDesc.RTVFormats[0] = mBackBufferFormat;
	//psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	//psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	//psoDesc.DSVFormat = mDepthStencilFormat;
	//ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}

void D3D12InitApp::BuildHillMaterials()
{
	//������ӳ����װ
	//����½�صĲ���
	auto grass = std::make_unique<Material>();
	grass->name = "grass";
	grass->matCBIndex = 0;
	grass->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);    //½�ط����ʣ���ɫ��
	grass->fresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);           //½�ص�R0
	grass->roughness = 0.125f;                                  //½�صĴֲڶȣ���һ����ģ�
	grass->diffuseSrvHeapIndex = 0;

	//�����ˮ�Ĳ���
	auto water = std::make_unique<Material>();
	water->name = "water";
	water->matCBIndex = 1;
	water->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);    //��ˮ�ķ�����
	water->fresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	water->roughness = 0.0f;
	water->diffuseSrvHeapIndex = 1;

	materials["grass"] = std::move(grass);
	materials["water"] = std::move(water);

	auto wood = std::make_unique<Material>();
	wood->name = "wood";
	wood->matCBIndex = 2;
	wood->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	wood->fresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	wood->roughness = 0.8f;
	wood->diffuseSrvHeapIndex = 2;

	materials["wood"] = std::move(wood);
}

void D3D12InitApp::BuildHillRenderItem()
{
#pragma region Hill
	auto landRitem = std::make_unique<RenderItem>();
	landRitem->world = MathHelper::Identity4x4();
	landRitem->objCBIndex = 0;
	landRitem->mat = materials["grass"].get();
	landRitem->geo = geometries["landGeo"].get();//��ֵ����ǰ��MeshGeometry
	landRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	landRitem->indexCount = landRitem->geo->DrawArgs["landGrid"].indexCount;
	landRitem->baseVertexLocation = landRitem->geo->DrawArgs["landGrid"].baseVertexLocation;
	landRitem->startIndexLocation = landRitem->geo->DrawArgs["landGrid"].startIndexLocation;
	XMStoreFloat4x4(&landRitem->texTransform, XMMatrixScaling(7.0f, 7.f, 1.0f));

	ritemLayer[(int)RenderLayer::Opaque].push_back(landRitem.get());

	allRitems.push_back(std::move(landRitem));
	//�����ǲ�͸����

#pragma endregion

#pragma region Lake
	//��������
	auto lakeRitem = std::make_unique<RenderItem>();
	lakeRitem->world = MathHelper::Identity4x4();
	lakeRitem->objCBIndex = 1;//�����ĳ������ݣ�world���������峣��������������1��
	lakeRitem->mat = materials["water"].get();
	lakeRitem->geo = geometries["lakeGeo"].get();
	lakeRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	lakeRitem->indexCount = lakeRitem->geo->DrawArgs["lakeGrid"].indexCount;
	lakeRitem->baseVertexLocation = lakeRitem->geo->DrawArgs["lakeGrid"].baseVertexLocation;
	lakeRitem->startIndexLocation = lakeRitem->geo->DrawArgs["lakeGrid"].startIndexLocation;
	XMStoreFloat4x4(&lakeRitem->texTransform, XMMatrixScaling(7.0f, 7.f, 1.0f));

	wavesRitem = lakeRitem.get();
	//ˮ����͸����
	ritemLayer[(int)RenderLayer::Transparent].push_back(lakeRitem.get());
	allRitems.push_back(std::move(lakeRitem));//��push_back������ָ���Զ��ͷ��ڴ�

#pragma endregion

#pragma region box
	auto boxRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&boxRitem->world, XMMatrixTranslation(0.0f, 4.0f, -7.0f));
	boxRitem->objCBIndex = 2;
	boxRitem->mat = materials["wood"].get();
	boxRitem->geo = geometries["boxGeo"].get();
	boxRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->indexCount = boxRitem->geo->DrawArgs["box"].indexCount;
	boxRitem->baseVertexLocation = boxRitem->geo->DrawArgs["box"].baseVertexLocation;
	boxRitem->startIndexLocation = boxRitem->geo->DrawArgs["box"].startIndexLocation;
	boxRitem->texTransform = MathHelper::Identity4x4();
	//���ӵ���������Ҫ͸���Ȳ��Ե�
	ritemLayer[(int)RenderLayer::AlphaTest].push_back(boxRitem.get());
	allRitems.push_back(std::move(boxRitem));

#pragma endregion
}

void D3D12InitApp::DrawRenderItems(const std::vector<RenderItem*>& ritems)
{
	UINT objectCount = (UINT)allRitems.size();//�����ܸ���������ʵ����
	UINT objConstSize = ToolFunc::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matConstantSize = ToolFunc::CalcConstantBufferByteSize(sizeof(MatConstants));

	auto objCB = currFrameResource->objCB->Resource();
	auto matCB = currFrameResource->matCB->Resource();

	//������Ⱦ������
	for (size_t i = 0; i < ritems.size(); i++)
	{
		auto ritem = ritems[i];

		//һ��Ҫ�����ػ�
		cmdList->IASetVertexBuffers(0, 1, &ritem->geo->GetVbv());
		cmdList->IASetIndexBuffer(&ritem->geo->GetIbv());
		cmdList->IASetPrimitiveTopology(ritem->primitiveType);

		//���ø�����������������Դ����ˮ�߰�
		CD3DX12_GPU_DESCRIPTOR_HANDLE texHandle(srvHeap->GetGPUDescriptorHandleForHeapStart());
		texHandle.Offset(ritem->mat->diffuseSrvHeapIndex, cbv_srv_uavDescriptorSize);

		////���ø���������
		//UINT objCbvIndex = ritem->objCBIndex + currFrameResourceIndex * (UINT)allRitems.size();
		//auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvHeap->GetGPUDescriptorHandleForHeapStart());
		//handle.Offset(objCbvIndex, cbv_srv_uavDescriptorSize);
		//cmdList->SetGraphicsRootDescriptorTable(0, handle);

		//���ø�������
		auto objCBAddress = objCB->GetGPUVirtualAddress();
		objCBAddress += ritem->objCBIndex * objConstSize;

		//���ø���������������������matCB��Դ��
		auto matCBAddress = matCB->GetGPUVirtualAddress();
		matCBAddress += ritem->mat->matCBIndex * matConstantSize;

		cmdList->SetGraphicsRootDescriptorTable(0, texHandle);
		cmdList->SetGraphicsRootConstantBufferView(1,
			objCBAddress);
		cmdList->SetGraphicsRootConstantBufferView(2,
			matCBAddress);

		//���ƶ���
		cmdList->DrawIndexedInstanced(ritem->indexCount,    //ÿ��ʵ�����Ƶ���������
			1,                                              //ʵ��������
			ritem->startIndexLocation,                      //��ʼ����λ��
			ritem->baseVertexLocation,                      //��������ʼ������ȫ�������е�λ��
			0);                                             //ʵ�����ĸ߼�����
	}
}

void D3D12InitApp::UpdateObjCBs()
{
	ObjectConstants objConstants;
	//�����������
	for (auto& e : allRitems)
	{
		if (e->NumFramesDirty > 0)
		{
			mWorld = e->world;
			XMMATRIX w = XMLoadFloat4x4(&mWorld);
			//ת��ֵ
			XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(w));
			XMFLOAT4X4 texTransform = e->texTransform;
			XMMATRIX texTransMatrix = XMLoadFloat4x4(&texTransform);
			XMStoreFloat4x4(&objConstants.texTransform, XMMatrixTranspose(texTransMatrix));
			//�����ݿ�����GPU����
			currFrameResource->objCB->CopyData(e->objCBIndex, objConstants);

			e->NumFramesDirty--;
		}
	}
}

void D3D12InitApp::UpdatePassCBs(const GameTime& gt)
{
	// ����ͶӰ����
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	////�����۲����
	float x = mRadius * sinf(mPhi) * cosf(mTheta);
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	//�����۲����
	passConstants.eyePosW = XMFLOAT3(x, y, z);
	XMVECTOR pos = XMVectorSet(x, y, z, 5.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);

	//�����任����SRT����
	XMMATRIX VP_Matrix = view * proj;
	XMStoreFloat4x4(&passConstants.viewProj, XMMatrixTranspose(VP_Matrix));

	passConstants.ambientLight = { 0.25f,0.25f,0.35f,1.0f };
	passConstants.lights[0].strength = { 1.0f,1.0f,0.9f };
	passConstants.totalTime = gt.TotalTime();

	//������ת���ɵѿ�������
	XMVECTOR sunDir = -MathHelper::SphericalToCartesian(1.0f, sunTheta, sunPhi);
	XMStoreFloat3(&passConstants.lights[0].direction, sunDir);

	passConstants.fogColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	passConstants.fogRange = 200.0f;
	passConstants.fogStart = 2.0f;
	passConstants.pad2 = { 0.0f, 0.0f };

	currFrameResource->passCB->CopyData(0, passConstants);
	UpdateReflectPassCB(gt);
}

void D3D12InitApp::BuildFrameResources()
{
	for (int i = 0; i < frameResourcesCount; i++)
	{
		FrameResourcesArray.push_back(std::make_unique<FrameResource>(
			d3dDevice.Get(),
			2,
			(UINT)allRitems.size(),     //�����建������
			(UINT)materials.size(),     //��������
			waves->VertexCount()        //���㻺������
			));
	}
}

void D3D12InitApp::UpdateWaves(const GameTime& gt)
{
	static float t_base = 0.0f;
	if ((gameTime.TotalTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;//0.25������һ������
		//������ɺ�����
		int i = MathHelper::Rand(4, waves->RowCount() - 5);
		//�������������
		int j = MathHelper::Rand(4, waves->ColumnCount() - 5);
		//������ɲ��İ뾶
		float r = MathHelper::RandF(0.2f, 0.5f);
		//ʹ�ò����������ɲ���
		waves->Disturb(i, j, r);
	}

	//ÿ֡���²���ģ�⣨�����¶������꣩
	waves->Update(gt.DeltaTime());

	//�����µĶ����������GPU�ϴ�����
	auto currWavesVB = currFrameResource->WavesVB.get();
	for (int i = 0; i < waves->VertexCount(); i++)
	{
		Vertex v;

		v.Pos = waves->Position(i);
		v.Normal = waves->Normal(i);
		//v.Color = XMFLOAT4(DirectX::Colors::Red);
		v.TexC.x = 0.5f + v.Pos.x / waves->Width();
		v.TexC.y = 0.5f - v.Pos.z / waves->Depth();

		currWavesVB->CopyData(i, v);
	}
	//��ֵ����GPU�Ķ��㻺��
	wavesRitem->geo->vertexBufferGpu = currWavesVB->Resource();
}

void D3D12InitApp::BuildLakeIndexBuffer()
{
	//��ʼ�������б�
	std::vector<std::uint16_t> indices(3 * waves->TriangleCount());
	assert(waves->VertexCount() < 0x0000ffff);//��������������65536����ֹ����

	//��������б�
	int m = waves->RowCount();
	int n = waves->ColumnCount();
	int k = 0;
	for (int i = 0; i < m - 1; i++)
	{
		for (int j = 0; j < n - 1; ++j)
		{
			indices[k] = i * n + j;
			indices[k + 1] = i * n + j + 1;
			indices[k + 2] = (i + 1) * n + j;

			indices[k + 3] = (i + 1) * n + j;
			indices[k + 4] = i * n + j + 1;
			indices[k + 5] = (i + 1) * n + j + 1;

			k += 6; // next quad
		}
	}

	//���㶥������������С
	UINT vbByteSize = waves->VertexCount() * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->name = "lakeGeo";

	geo->vertexBufferCpu = nullptr;
	geo->vertexBufferGpu = nullptr;

	//����������CPU�ڴ�
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->indexBufferCpu));
	//������������ڴ�
	CopyMemory(geo->indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);
	//���������ϴ���Ĭ�϶�
	geo->indexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(),
		cmdList.Get(),
		indices.data(),
		ibByteSize,
		geo->indexBufferUploader);

	//��ֵMeshGeometry�������
	geo->vertexBufferByteSize = vbByteSize;
	geo->vertexByteStride = sizeof(Vertex);
	geo->indexFormat = DXGI_FORMAT_R16_UINT;
	geo->indexBufferByteSize = ibByteSize;

	SubmeshGeometry LakeSubmesh;
	LakeSubmesh.indexCount = (UINT)indices.size();
	LakeSubmesh.baseVertexLocation = 0;
	LakeSubmesh.startIndexLocation = 0;
	//ʹ��grid������
	geo->DrawArgs["lakeGrid"] = LakeSubmesh;
	geometries["lakeGeo"] = std::move(geo);
}

XMFLOAT3 D3D12InitApp::GetHillsNormal(float x, float z) const
{
	XMFLOAT3 normal = XMFLOAT3(-0.03f * z * cosf(0.1f * x) - cosf(0.1f * z),
		1.0f,
		-0.3f * sinf(0.1f * x) + 0.1f * x * sinf(0.1f * z));
	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&normal));
	XMStoreFloat3(&normal, unitNormal);

	return normal;
}

void D3D12InitApp::UpdateMatCBs()
{
	//��Update������ִ��
	for (auto& e : materials)
	{
		Material* mat = e.second.get();
		if (mat->numFrameDirty > 0)
		{
			MatConstants matConstants;
			matConstants.diffuseAlbedo = mat->diffuseAlbedo;
			matConstants.fresnelR0 = mat->fresnelR0;
			matConstants.roughness = mat->roughness;
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->matTransform);
			XMStoreFloat4x4(&matConstants.matTransform, XMMatrixTranspose(matTransform));
			//�����ʳ������ݸ��Ƶ�������������Ӧ������ַ��
			currFrameResource->matCB->CopyData(mat->matCBIndex, matConstants);
			//������һ��֡��Դ
			mat->numFrameDirty--;
		}
	}
}

float D3D12InitApp::GetHillsHeight(float x, float z)
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

void D3D12InitApp::BuildShapeGeometry()
{
	ProceduralGeometry proceGeo;
	ProceduralGeometry::MeshData box = proceGeo.CreateBox(1.5f, 0.5f, 1.5f, 3);
	ProceduralGeometry::MeshData grid = proceGeo.CreateGrid(20.0f, 30.0f, 60, 40);
	ProceduralGeometry::MeshData sphere = proceGeo.CreateSphere(0.5f, 20, 20);
	ProceduralGeometry::MeshData cylinder = proceGeo.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	//�����ĸ�������Ķ���������ֱ����������еĵ�ַƫ��
	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = (UINT)grid.Vertices.size() + gridVertexOffset;
	UINT cylinderVertexOffset = (UINT)sphere.Vertices.size() + sphereVertexOffset;
	//����
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = (UINT)grid.Indices32.size() + gridIndexOffset;
	UINT cylinderIndexOffset = (UINT)sphere.Indices32.size() + sphereIndexOffset;

	SubmeshGeometry boxSubmesh;
	boxSubmesh.indexCount = (UINT)box.Indices32.size();
	boxSubmesh.baseVertexLocation = boxVertexOffset;
	boxSubmesh.startIndexLocation = boxIndexOffset;

	SubmeshGeometry gridSubmesh;
	gridSubmesh.indexCount = (UINT)grid.Indices32.size();
	gridSubmesh.baseVertexLocation = gridVertexOffset;
	gridSubmesh.startIndexLocation = gridIndexOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.indexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.baseVertexLocation = sphereVertexOffset;
	sphereSubmesh.startIndexLocation = sphereIndexOffset;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.indexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.baseVertexLocation = cylinderVertexOffset;
	cylinderSubmesh.startIndexLocation = cylinderIndexOffset;

	//�����ܶ��㻺�棬�������ݴ���
	size_t totalVertexCount = box.Vertices.size() + grid.Vertices.size() + sphere.Vertices.size() + cylinder.Vertices.size();
	//���������С
	std::vector<Vertex> vertices(totalVertexCount);
	int k = 0;
	for (size_t i = 0; i < box.Vertices.size(); i++, k++)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		//vertices[k].Color = XMFLOAT4(DirectX::Colors::Blue);
	}
	for (size_t i = 0; i < grid.Vertices.size(); i++, k++)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		//vertices[k].Color = XMFLOAT4(DirectX::Colors::Red);
	}
	for (size_t i = 0; i < sphere.Vertices.size(); i++, k++)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		//vertices[k].Color = XMFLOAT4(DirectX::Colors::Yellow);
	}
	for (size_t i = 0; i < cylinder.Vertices.size(); i++, k++)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		//vertices[k].Color = XMFLOAT4(DirectX::Colors::Green);
	}

	//��������������
	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), box.GetIndices16().begin(), box.GetIndices16().end());
	indices.insert(indices.end(), grid.GetIndices16().begin(), grid.GetIndices16().end());
	indices.insert(indices.end(), sphere.GetIndices16().begin(), sphere.GetIndices16().end());
	indices.insert(indices.end(), cylinder.GetIndices16().begin(), cylinder.GetIndices16().end());

	//��������Դ�С�����ݸ�ȫ�ֱ���
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->name = "ShapeGeo";

	geo->vertexBufferByteSize = vbByteSize;
	geo->indexBufferByteSize = ibByteSize;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->vertexBufferCpu));	//�������������ڴ�ռ�
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->indexBufferCpu));	//�������������ڴ�ռ�
	CopyMemory(geo->vertexBufferCpu->GetBufferPointer(), vertices.data(), vbByteSize);	//���������ݿ���������ϵͳ�ڴ���
	CopyMemory(geo->indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);	//���������ݿ���������ϵͳ�ڴ���
	geo->vertexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize, geo->vertexBufferUploader);
	geo->indexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize, geo->indexBufferUploader);

	geo->vertexByteStride = sizeof(Vertex);
	geo->indexFormat = DXGI_FORMAT_R16_UINT;

	//ʹ������ӳ���

	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["cylinder"] = cylinderSubmesh;

	geometries[geo->name] = std::move(geo);
}

void D3D12InitApp::BuildShapeMaterials()
{
	auto bricks0 = std::make_unique<Material>();
	bricks0->name = "bricks0";
	bricks0->matCBIndex = 0;
	bricks0->diffuseAlbedo = XMFLOAT4(Colors::ForestGreen);
	bricks0->fresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	bricks0->roughness = 0.1f;

	auto stone0 = std::make_unique<Material>();
	stone0->name = "stone0";
	stone0->matCBIndex = 1;
	stone0->diffuseAlbedo = XMFLOAT4(Colors::LightSteelBlue);
	stone0->fresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	stone0->roughness = 0.3f;

	auto tile0 = std::make_unique<Material>();
	tile0->name = "tile0";
	tile0->matCBIndex = 2;
	tile0->diffuseAlbedo = XMFLOAT4(Colors::LightGray);
	tile0->fresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	tile0->roughness = 0.2f;

	auto skullMat = std::make_unique<Material>();
	skullMat->name = "skullMat";
	skullMat->matCBIndex = 3;
	skullMat->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	skullMat->fresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	skullMat->roughness = 0.3f;

	materials["bricks0"] = std::move(bricks0);
	materials["stone0"] = std::move(stone0);
	materials["tile0"] = std::move(tile0);
	materials["skullMat"] = std::move(skullMat);
}

void D3D12InitApp::BuildSkullGeometry()
{
	std::ifstream fin("Models/skull.txt");//��ȡ�����ļ�

	if (!fin)
	{
		MessageBox(0, L"�ļ���ȡʧ��", 0, 0);
		return;
	}

	UINT vertexCount = 0;
	UINT triangleCount = 0;

	std::string ignore;

	//�ֱ���Ե�һ���ո�ǰ���ַ�������ȡ���ε����֣��Ƕ���������������
	fin >> ignore >> vertexCount;
	fin >> ignore >> triangleCount;
	//���в���
	fin >> ignore >> ignore >> ignore >> ignore;

	//��ʼ�������б�
	std::vector<Vertex> vertices(vertexCount);//��ʼ�������б�
	//�����б�ֵ
	for (UINT i = 0; i < vertexCount; i++)
	{
		//��ȡ��������
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		//normal���ݺ���
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
		//vertices[i].Color = XMFLOAT4(DirectX::Colors::Black);
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	//��ʼ�������б�
	std::vector<std::uint32_t> indices(triangleCount * 3);
	//�����б�ֵ
	for (UINT i = 0; i < triangleCount; i++)
	{
		fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
	}

	//�ر�������
	fin.close();

	const UINT vbByteSize = vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = indices.size() * sizeof(std::int32_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->name = "skullGeo";

	//������������ݷ���CPU��GPU

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->vertexBufferCpu));	//�������������ڴ�ռ�
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->indexBufferCpu));	//�������������ڴ�ռ�
	CopyMemory(geo->vertexBufferCpu->GetBufferPointer(), vertices.data(), vbByteSize);	//���������ݿ���������ϵͳ�ڴ���
	CopyMemory(geo->indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);	//���������ݿ���������ϵͳ�ڴ���
	geo->vertexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize, geo->vertexBufferUploader);
	geo->indexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize, geo->indexBufferUploader);

	geo->vertexBufferByteSize = vbByteSize;
	geo->indexBufferByteSize = ibByteSize;
	geo->vertexByteStride = sizeof(Vertex);
	geo->indexFormat = DXGI_FORMAT_R32_UINT;

	//����������
	SubmeshGeometry skullSubmesh;
	skullSubmesh.indexCount = (UINT)indices.size();
	skullSubmesh.baseVertexLocation = 0;
	skullSubmesh.startIndexLocation = 0;

	geo->DrawArgs["skull"] = skullSubmesh;

	geometries["skullGeo"] = std::move(geo);
}

void D3D12InitApp::BuildSkullRenderItem()
{
	auto skullRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&skullRitem->world, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	skullRitem->objCBIndex = 22;
	skullRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRitem->mat = materials["skullMat"].get();
	skullRitem->geo = geometries["skullGeo"].get();
	skullRitem->indexCount = skullRitem->geo->DrawArgs["skull"].indexCount;
	skullRitem->baseVertexLocation = skullRitem->geo->DrawArgs["skull"].baseVertexLocation;
	skullRitem->startIndexLocation = skullRitem->geo->DrawArgs["skull"].startIndexLocation;
	allRitems.push_back(std::move(skullRitem));
}

void D3D12InitApp::BuildShapeRenderItem()
{
#pragma region Shapes
	auto boxRItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&(boxRItem->world), XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
	boxRItem->objCBIndex = 0;//�������ݣ���0�±���
	boxRItem->mat = materials["stone0"].get();
	boxRItem->geo = geometries["ShapeGeo"].get();
	boxRItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRItem->indexCount = boxRItem->geo->DrawArgs["box"].indexCount;
	boxRItem->baseVertexLocation = boxRItem->geo->DrawArgs["box"].baseVertexLocation;
	boxRItem->startIndexLocation = boxRItem->geo->DrawArgs["box"].startIndexLocation;
	allRitems.push_back(std::move(boxRItem));

	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->world = MathHelper::Identity4x4();
	gridRitem->objCBIndex = 1;//BOX�������ݣ�world������objConstantBuffer����1��
	gridRitem->mat = materials["tile0"].get();
	gridRitem->geo = geometries["ShapeGeo"].get();
	gridRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->indexCount = gridRitem->geo->DrawArgs["grid"].indexCount;
	gridRitem->baseVertexLocation = gridRitem->geo->DrawArgs["grid"].baseVertexLocation;
	gridRitem->startIndexLocation = gridRitem->geo->DrawArgs["grid"].startIndexLocation;
	allRitems.push_back(std::move(gridRitem));

	UINT fllowObjCBIndex = 2;//����ȥ�ļ����峣��������CB�е�������2��ʼ
	//��Բ����Բ��ʵ��ģ�ʹ�����Ⱦ����
	for (int i = 0; i < 5; i++)
	{
		auto leftCylinderRitem = std::make_unique<RenderItem>();
		auto rightCylinderRitem = std::make_unique<RenderItem>();
		auto leftSphereRitem = std::make_unique<RenderItem>();
		auto rightSphereRitem = std::make_unique<RenderItem>();

		XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
		XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);
		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
		XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);
		//���5��Բ��
		XMStoreFloat4x4(&(leftCylinderRitem->world), leftCylWorld);
		//�˴�����������ѭ�����ϼ�1��ע�⣺�������ȸ�ֵ��++��
		leftCylinderRitem->objCBIndex = fllowObjCBIndex++;
		leftCylinderRitem->mat = materials["bricks0"].get();
		leftCylinderRitem->geo = geometries["ShapeGeo"].get();
		leftCylinderRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylinderRitem->indexCount = leftCylinderRitem->geo->DrawArgs["cylinder"].indexCount;
		leftCylinderRitem->baseVertexLocation = leftCylinderRitem->geo->DrawArgs["cylinder"].baseVertexLocation;
		leftCylinderRitem->startIndexLocation = leftCylinderRitem->geo->DrawArgs["cylinder"].startIndexLocation;
		//�ұ�5��Բ��
		XMStoreFloat4x4(&(rightCylinderRitem->world), rightCylWorld);
		rightCylinderRitem->objCBIndex = fllowObjCBIndex++;
		rightCylinderRitem->mat = materials["bricks0"].get();
		rightCylinderRitem->geo = geometries["ShapeGeo"].get();
		rightCylinderRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylinderRitem->indexCount = rightCylinderRitem->geo->DrawArgs["cylinder"].indexCount;
		rightCylinderRitem->baseVertexLocation = rightCylinderRitem->geo->DrawArgs["cylinder"].baseVertexLocation;
		rightCylinderRitem->startIndexLocation = rightCylinderRitem->geo->DrawArgs["cylinder"].startIndexLocation;
		//���5����
		XMStoreFloat4x4(&(leftSphereRitem->world), leftSphereWorld);
		leftSphereRitem->objCBIndex = fllowObjCBIndex++;
		leftSphereRitem->mat = materials["stone0"].get();
		leftSphereRitem->geo = geometries["ShapeGeo"].get();
		leftSphereRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->indexCount = leftSphereRitem->geo->DrawArgs["sphere"].indexCount;
		leftSphereRitem->baseVertexLocation = leftSphereRitem->geo->DrawArgs["sphere"].baseVertexLocation;
		leftSphereRitem->startIndexLocation = leftSphereRitem->geo->DrawArgs["sphere"].startIndexLocation;
		//�ұ�5����
		XMStoreFloat4x4(&(rightSphereRitem->world), rightSphereWorld);
		rightSphereRitem->objCBIndex = fllowObjCBIndex++;
		rightSphereRitem->mat = materials["stone0"].get();
		rightSphereRitem->geo = geometries["ShapeGeo"].get();
		rightSphereRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->indexCount = rightSphereRitem->geo->DrawArgs["sphere"].indexCount;
		rightSphereRitem->baseVertexLocation = rightSphereRitem->geo->DrawArgs["sphere"].baseVertexLocation;
		rightSphereRitem->startIndexLocation = rightSphereRitem->geo->DrawArgs["sphere"].startIndexLocation;

		allRitems.push_back(std::move(leftCylinderRitem));
		allRitems.push_back(std::move(rightCylinderRitem));
		allRitems.push_back(std::move(leftSphereRitem));
		allRitems.push_back(std::move(rightSphereRitem));
	}
#pragma endregion
	//��ȡ����
	BuildSkullRenderItem();
}

void D3D12InitApp::BuildRoomGeometry()
{
	//���㻺��
	std::array<Vertex, 20> vertices =
	{
		//�ذ�ģ�͵Ķ���Pos��Normal��TexCoord
		Vertex(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f), // 0 
		Vertex(-3.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
		Vertex(7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f),
		Vertex(7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f),

		//ǽ��ģ�͵Ķ���Pos��Normal��TexCoord
		Vertex(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 4
		Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f),
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f),

		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 8 
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f),
		Vertex(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f),

		Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 12
		Vertex(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f),
		Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f),

		//����ģ�͵Ķ���Pos��Normal��TexCoord
		Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 16
		Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),
		Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f)
	};

	//��������
	std::array<std::int16_t, 30> indices =
	{
		// �ذ壨Floor��
		0, 1, 2,
		0, 2, 3,

		// ǽ�ڣ�Walls��
		4, 5, 6,
		4, 6, 7,

		8, 9, 10,
		8, 10, 11,

		12, 13, 14,
		12, 14, 15,

		// ���ӣ�Mirror��
		16, 17, 18,
		16, 18, 19
	};

	//��������Ļ������������ϲ������������������
	SubmeshGeometry floorSubmesh;
	floorSubmesh.indexCount = 6;
	floorSubmesh.startIndexLocation = 0;
	floorSubmesh.baseVertexLocation = 0;

	SubmeshGeometry wallSubmesh;
	wallSubmesh.indexCount = 18;
	wallSubmesh.startIndexLocation = 6;
	wallSubmesh.baseVertexLocation = 0;

	SubmeshGeometry mirrorSubmesh;
	mirrorSubmesh.indexCount = 6;
	mirrorSubmesh.startIndexLocation = 24;
	mirrorSubmesh.baseVertexLocation = 0;

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->name = "roomGeo";

	//�����㻺�洫��CPUϵͳ�ڴ�
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->vertexBufferCpu));
	CopyMemory(geo->vertexBufferCpu->GetBufferPointer(), vertices.data(), vbByteSize);
	//���������洫��CPUϵͳ�ڴ�
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->indexBufferCpu));
	CopyMemory(geo->indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);
	//�����㻺��ͨ���ϴ��Ѵ���GPUĬ�϶�
	geo->vertexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(),
		cmdList.Get(), vertices.data(), vbByteSize, geo->vertexBufferUploader);
	//����������ͨ���ϴ��Ѵ���GPUĬ�϶�
	geo->indexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(),
		cmdList.Get(), indices.data(), ibByteSize, geo->indexBufferUploader);

	geo->vertexByteStride = sizeof(Vertex);
	geo->vertexBufferByteSize = vbByteSize;
	geo->indexFormat = DXGI_FORMAT_R16_UINT;
	geo->indexBufferByteSize = ibByteSize;

	//��װ����������
	geo->DrawArgs["floor"] = floorSubmesh;
	geo->DrawArgs["wall"] = wallSubmesh;
	geo->DrawArgs["mirror"] = mirrorSubmesh;
	//��װroomģ�͵�����
	geometries["roomGeo"] = std::move(geo);

}

void D3D12InitApp::LoadRoomTextures()
{
	/*��ɫ����*/
	auto white1x1Tex = std::make_unique<Texture>();
	white1x1Tex->name = "white1x1Tex";
	white1x1Tex->fileName = L"Textures\\white1x1.dds";
	//��ȡDDS�ļ�
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(),
		white1x1Tex->fileName.c_str(),//��wstringת��wChar_t
		white1x1Tex->resource, white1x1Tex->uploadHeap));

	/*��ש����*/
	auto floorTex = std::make_unique<Texture>();
	floorTex->name = "floorTex";
	floorTex->fileName = L"Textures\\checkboard.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(),
		floorTex->fileName.c_str(),
		floorTex->resource, floorTex->uploadHeap));

	/*שǽ����*/
	auto wallTex = std::make_unique<Texture>();
	wallTex->name = "wallTex";
	wallTex->fileName = L"Textures\\bricks3.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(),
		wallTex->fileName.c_str(),
		wallTex->resource, wallTex->uploadHeap));

	/*��������*/
	auto mirrorTex = std::make_unique<Texture>();
	mirrorTex->name = "mirrorTex";
	mirrorTex->fileName = L"Textures\\ice.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(),
		mirrorTex->fileName.c_str(),
		mirrorTex->resource, mirrorTex->uploadHeap));

	//����������ܵ�����ӳ���
	textures["white1x1Tex"] = std::move(white1x1Tex);
	textures["floorTex"] = std::move(floorTex);
	textures["wallTex"] = std::move(wallTex);
	textures["mirrorTex"] = std::move(mirrorTex);
}

void D3D12InitApp::CreateRoomSRV()
{
	//����SRV��
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NumDescriptors = 4;//һ�����а���4��SRV
	srvHeapDesc.NodeMask = 0;
	ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap)));

	//�õ�������Դ(comptr����)
	auto white1x1Tex = textures["white1x1Tex"]->resource;
	auto floorTex = textures["floorTex"]->resource;
	auto wallTex = textures["wallTex"]->resource;
	auto mirrorTex = textures["mirrorTex"]->resource;

	//SRV������SRV����שtile��SRV���ľ��
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(srvHeap->GetCPUDescriptorHandleForHeapStart());
	//SRV�����ṹ��
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = floorTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = floorTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	//����SRV
	//��������ש������SRV
	d3dDevice->CreateShaderResourceView(floorTex.Get(), &srvDesc, handle);

	//SRV������SRV�ľ��,����ƫ��һ��SRV
	handle.Offset(1, cbv_srv_uavDescriptorSize);
	srvDesc.Format = wallTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = wallTex->GetDesc().MipLevels;
	d3dDevice->CreateShaderResourceView(wallTex.Get(), &srvDesc, handle);

	handle.Offset(1, cbv_srv_uavDescriptorSize);
	srvDesc.Format = mirrorTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = mirrorTex->GetDesc().MipLevels;
	d3dDevice->CreateShaderResourceView(mirrorTex.Get(), &srvDesc, handle);

	handle.Offset(1, cbv_srv_uavDescriptorSize);
	srvDesc.Format = white1x1Tex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = white1x1Tex->GetDesc().MipLevels;
	d3dDevice->CreateShaderResourceView(white1x1Tex.Get(), &srvDesc, handle);
}

void D3D12InitApp::BuildMaterials()
{
	//����ذ�Ĳ���
	auto floor = std::make_unique<Material>();
	floor->name = "floor";
	floor->matCBIndex = 0;
	floor->diffuseSrvHeapIndex = 0;
	floor->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	floor->fresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.48f);
	floor->roughness = 0.2f;

	//����שǽ�Ĳ���
	auto wall = std::make_unique<Material>();
	wall->name = "wall";
	wall->matCBIndex = 1;
	wall->diffuseSrvHeapIndex = 1;
	wall->diffuseAlbedo = XMFLOAT4(0.91f, 0.91f, 0.91f, 1.0f);
	wall->fresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	wall->roughness = 0.6f;

	//���徵��Ĳ���
	auto mirror = std::make_unique<Material>();
	mirror->name = "mirror";
	mirror->matCBIndex = 2;
	mirror->diffuseSrvHeapIndex = 2;
	mirror->diffuseAlbedo = XMFLOAT4(0.91f, 0.91f, 0.91f, 1.0f);
	mirror->fresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	mirror->roughness = 0.1f;

	//�����ͷ�Ĳ���
	auto bone = std::make_unique<Material>();
	bone->name = "bone";
	bone->matCBIndex = 3;
	bone->diffuseSrvHeapIndex = 3;
	bone->diffuseAlbedo = XMFLOAT4(0.91f, 0.91f, 0.91f, 1.0f);
	bone->fresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	bone->roughness = 0.3f;

	materials["floor"] = std::move(floor);
	materials["wall"] = std::move(wall);
	materials["mirror"] = std::move(mirror);
	materials["bone"] = std::move(bone);
}

void D3D12InitApp::BuildRoomRenderItem()
{
	auto floorRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&(floorRitem->world), DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f) * DirectX::XMMatrixTranslation(0.0f, -2.0f, 3.0f));
	//floorRitem->world = MathHelper::Identity4x4();
	floorRitem->objCBIndex = 0;//floor�������ݣ�world������objConstantBuffer����1��
	floorRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	floorRitem->geo = geometries["roomGeo"].get();
	floorRitem->mat = materials["floor"].get();//�����ͷ���ʸ�����ͷ
	XMStoreFloat4x4(&floorRitem->texTransform, DirectX::XMMatrixScaling(1.6f, 1.6f, 1.6f));
	floorRitem->indexCount = floorRitem->geo->DrawArgs["floor"].indexCount;
	floorRitem->baseVertexLocation = floorRitem->geo->DrawArgs["floor"].baseVertexLocation;
	floorRitem->startIndexLocation = floorRitem->geo->DrawArgs["floor"].startIndexLocation;

	ritemLayer[(int)RenderLayer::Opaque].push_back(floorRitem.get());


	auto wallRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&(wallRitem->world), DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f) * DirectX::XMMatrixTranslation(0.0f, -2.0f, 3.0f));
	//wallRitem->world = MathHelper::Identity4x4();
	wallRitem->objCBIndex = 1;//floor�������ݣ�world������objConstantBuffer����1��
	wallRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wallRitem->geo = geometries["roomGeo"].get();
	wallRitem->mat = materials["wall"].get();//�����ͷ���ʸ�����ͷ
	XMStoreFloat4x4(&wallRitem->texTransform, DirectX::XMMatrixScaling(0.8f, 0.8f, 0.8f));
	wallRitem->indexCount = wallRitem->geo->DrawArgs["wall"].indexCount;
	wallRitem->baseVertexLocation = wallRitem->geo->DrawArgs["wall"].baseVertexLocation;
	wallRitem->startIndexLocation = wallRitem->geo->DrawArgs["wall"].startIndexLocation;

	ritemLayer[(int)RenderLayer::Opaque].push_back(wallRitem.get());


	auto mirrorRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&(mirrorRitem->world), DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f) * DirectX::XMMatrixTranslation(0.0f, -2.0f, 3.0f));
	//mirrorRitem->world = MathHelper::Identity4x4();
	mirrorRitem->objCBIndex = 2;//floor�������ݣ�world������objConstantBuffer����1��
	mirrorRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mirrorRitem->geo = geometries["roomGeo"].get();
	mirrorRitem->mat = materials["mirror"].get();//�����ͷ���ʸ�����ͷ
	XMStoreFloat4x4(&mirrorRitem->texTransform, DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f));
	mirrorRitem->indexCount = mirrorRitem->geo->DrawArgs["mirror"].indexCount;
	mirrorRitem->baseVertexLocation = mirrorRitem->geo->DrawArgs["mirror"].baseVertexLocation;
	mirrorRitem->startIndexLocation = mirrorRitem->geo->DrawArgs["mirror"].startIndexLocation;

	ritemLayer[(int)RenderLayer::Opaque].push_back(mirrorRitem.get());


	auto skullRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&skullRitem->world,
		DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f) * DirectX::XMMatrixTranslation(0.0f, -1.5f, -1.0f) * DirectX::XMMatrixRotationY(1.57f));
	skullRitem->objCBIndex = 3;//skull�������ݣ�world������objConstantBuffer����1��
	skullRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRitem->geo = geometries["skullGeo"].get();
	skullRitem->mat = materials["bone"].get();//�����ͷ���ʸ�����ͷ
	//XMStoreFloat4x4(&skullRitem->texTransform, XMMatrixScaling(7.0f, 7.0f, 7.0f));
	skullRitem->indexCount = skullRitem->geo->DrawArgs["skull"].indexCount;
	skullRitem->baseVertexLocation = skullRitem->geo->DrawArgs["skull"].baseVertexLocation;
	skullRitem->startIndexLocation = skullRitem->geo->DrawArgs["skull"].startIndexLocation;

	ritemLayer[(int)RenderLayer::Opaque].push_back(skullRitem.get());

	auto skullMirrorRitem = std::make_unique<RenderItem>();
	*skullMirrorRitem = *skullRitem;
	XMStoreFloat4x4(&skullMirrorRitem->world,
		DirectX::XMMatrixScaling(0.5f, 0.5f, 0.5f) * DirectX::XMMatrixRotationY(1.57f) * DirectX::XMMatrixTranslation(0.0f, -1.5f, 7.0f));
	skullMirrorRitem->objCBIndex = 4;

	ritemLayer[(int)RenderLayer::Reflects].push_back(skullMirrorRitem.get());


	//move���ͷ�Ritem�ڴ棬���Ա�����ritemLayer֮��ִ��
	allRitems.push_back(std::move(floorRitem));
	allRitems.push_back(std::move(wallRitem));
	allRitems.push_back(std::move(mirrorRitem));
	allRitems.push_back(std::move(skullRitem));
	allRitems.push_back(std::move(skullMirrorRitem));
}

void D3D12InitApp::LoadBoxTextures()
{
	//����������
	auto woodCrateTex = std::make_unique<Texture>();
	woodCrateTex->name = "woodCrateTex";
	woodCrateTex->fileName = L"Textures/WireFence.dds";
	//��ȡdds�ļ�
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(),
		woodCrateTex->fileName.c_str(),
		woodCrateTex->resource, woodCrateTex->uploadHeap));

	//�ݵ�����
	auto grassTex = std::make_unique<Texture>();
	grassTex->name = "grassTex";
	grassTex->fileName = L"Textures/grass.dds";
	//��ȡdds�ļ�
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(),
		grassTex->fileName.c_str(),
		grassTex->resource, grassTex->uploadHeap));

	//��ˮ����
	auto lakeTex = std::make_unique<Texture>();
	lakeTex->name = "lakeTex";
	lakeTex->fileName = L"Textures/water1.dds";
	//��ȡdds�ļ�
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(),
		lakeTex->fileName.c_str(),
		lakeTex->resource, lakeTex->uploadHeap));

	//�����������б�
	textures["woodCrateTex"] = std::move(woodCrateTex);
	textures["grassTex"] = std::move(grassTex);
	textures["lakeTex"] = std::move(lakeTex);
}

std::array<CD3DX12_STATIC_SAMPLER_DESC, 6> D3D12InitApp::GetStaticSamplers()
{
	//������POINT,ѰַģʽWRAP�ľ�̬������
	CD3DX12_STATIC_SAMPLER_DESC pointWarp(0,	//��ɫ���Ĵ���
		D3D12_FILTER_MIN_MAG_MIP_POINT,		//����������ΪPOINT(������ֵ)
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//U�����ϵ�ѰַģʽΪWRAP���ظ�Ѱַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//V�����ϵ�ѰַģʽΪWRAP���ظ�Ѱַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);	//W�����ϵ�ѰַģʽΪWRAP���ظ�Ѱַģʽ��

	//������POINT,ѰַģʽCLAMP�ľ�̬������
	CD3DX12_STATIC_SAMPLER_DESC pointClamp(1,	//��ɫ���Ĵ���
		D3D12_FILTER_MIN_MAG_MIP_POINT,		//����������ΪPOINT(������ֵ)
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//U�����ϵ�ѰַģʽΪCLAMP��ǯλѰַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//V�����ϵ�ѰַģʽΪCLAMP��ǯλѰַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);	//W�����ϵ�ѰַģʽΪCLAMP��ǯλѰַģʽ��

	//������LINEAR,ѰַģʽWRAP�ľ�̬������
	CD3DX12_STATIC_SAMPLER_DESC linearWarp(2,	//��ɫ���Ĵ���
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,		//����������ΪLINEAR(���Բ�ֵ)
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//U�����ϵ�ѰַģʽΪWRAP���ظ�Ѱַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//V�����ϵ�ѰַģʽΪWRAP���ظ�Ѱַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);	//W�����ϵ�ѰַģʽΪWRAP���ظ�Ѱַģʽ��

	//������LINEAR,ѰַģʽCLAMP�ľ�̬������
	CD3DX12_STATIC_SAMPLER_DESC linearClamp(3,	//��ɫ���Ĵ���
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,		//����������ΪLINEAR(���Բ�ֵ)
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//U�����ϵ�ѰַģʽΪCLAMP��ǯλѰַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//V�����ϵ�ѰַģʽΪCLAMP��ǯλѰַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);	//W�����ϵ�ѰַģʽΪCLAMP��ǯλѰַģʽ��

	//������ANISOTROPIC,ѰַģʽWRAP�ľ�̬������
	CD3DX12_STATIC_SAMPLER_DESC anisotropicWarp(4,	//��ɫ���Ĵ���
		D3D12_FILTER_ANISOTROPIC,			//����������ΪANISOTROPIC(��������)
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//U�����ϵ�ѰַģʽΪWRAP���ظ�Ѱַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//V�����ϵ�ѰַģʽΪWRAP���ظ�Ѱַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);	//W�����ϵ�ѰַģʽΪWRAP���ظ�Ѱַģʽ��

	//������LINEAR,ѰַģʽCLAMP�ľ�̬������
	CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(5,	//��ɫ���Ĵ���
		D3D12_FILTER_ANISOTROPIC,			//����������ΪANISOTROPIC(��������)
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//U�����ϵ�ѰַģʽΪCLAMP��ǯλѰַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//V�����ϵ�ѰַģʽΪCLAMP��ǯλѰַģʽ��
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);	//W�����ϵ�ѰַģʽΪCLAMP��ǯλѰַģʽ��

	return{ pointWarp, pointClamp, linearWarp, linearClamp, anisotropicWarp, anisotropicClamp };
}

void D3D12InitApp::UpdateReflectPassCB(const GameTime& gt)
{
	reflectPassConstants = passConstants;
	XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 3.0f, 0.0f);
	XMMATRIX R = XMMatrixReflect(mirrorPlane);

	//����ƹ�
	for (int i = 0; i < 3; ++i)
	{
		XMVECTOR lightDir = XMLoadFloat3(&passConstants.lights[i].direction);
		XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&reflectPassConstants.lights[i].direction, reflectedLightDir);
	}

	currFrameResource->passCB->CopyData(1, reflectPassConstants);
}

void D3D12InitApp::AnimateMaterials(const GameTime& gt)
{
	auto matLake = materials["water"].get();
	float& du = matLake->matTransform(3, 0);
	float& dv = matLake->matTransform(3, 1);

	du += 0.1f * gt.DeltaTime();
	dv += 0.02f * gt.DeltaTime();

	if (du >= 1.0f)
	{
		du = 0.0f;
	}
	if (dv >= 1.0f)
	{
		dv = 0.0f;
	}
	//�������仯���������
	matLake->matTransform(3, 0) = du;
	matLake->matTransform(3, 1) = dv;

	matLake->numFrameDirty = 3;
}