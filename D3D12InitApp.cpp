#include "D3D12InitApp.h"
#include "ProceduralGeometry.h"


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
 
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildGeometry();
    BuildLakeIndexBuffer();
    BuildSkullGeometry();
    BuildRenderItem();
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
    currFrameResourceIndex = (currFrameResourceIndex + 1) % frameResourcesCount;
    currFrameResource = FrameResourcesArray[currFrameResourceIndex].get();

    //���GPU��Χ��ֵС��CPU��Χ��ֵ����CPU�ٶȿ���GPU������CPU�ȴ�
    if (currFrameResource->fenceCPU != 0 && fence->GetCompletedValue() < currFrameResource->fenceCPU)
    {
        HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");
        ThrowIfFailed(fence->SetEventOnCompletion(currFrameResource->fenceCPU, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle;
    }

    UpdateObjCBs();
    UpdatePassCBs();
    UpdateMatCBs();
    UpdateWaves(gt);
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
    ThrowIfFailed(cmdList->Reset(currCmdAllocator.Get(), mPSO.Get()));

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

    cmdList->SetGraphicsRootSignature(mRootSignature.Get());

    ////���ø���������
    //int passCBVIndex = (int)allRitems.size() * frameResourcesCount + currFrameResourceIndex;
    //auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvHeap->GetGPUDescriptorHandleForHeapStart());
    //passCbvHandle.Offset(passCBVIndex, cbv_srv_uavDescriptorSize);
    //cmdList->SetGraphicsRootDescriptorTable(1,  //����������ʼ���� 
    //    passCbvHandle);
   
    DrawRenderItems();
    
    //����passCB������
    auto passCB = currFrameResource->passCB->Resource();
    cmdList->SetGraphicsRootConstantBufferView(2,
        passCB->GetGPUVirtualAddress());

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

void D3D12InitApp::BuildRootSignature()
{
    //������������������������������������    
    CD3DX12_ROOT_PARAMETER slotRootParameter[3];
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsConstantBufferView(1);
    slotRootParameter[2].InitAsConstantBufferView(2);

    //��ǩ����һ�����������
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3,      //������������ 
        slotRootParameter,                          //������ָ��
        0, nullptr,
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

    mvsByteCode = ToolFunc::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
    mpsByteCode = ToolFunc::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

    mInputLayout =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
}

void D3D12InitApp::BuildGeometry()
{
#pragma region Hill
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

        //��ɫ��RGBA
        if (vertices.at(i).Pos.y < -10.0f)
        {
            vertices[i].Color = XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
        }
        else if (vertices[i].Pos.y < 5.0f)
        {
            vertices[i].Color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
        }
        else if (vertices[i].Pos.y < 12.0f)
        {
            vertices[i].Color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
        }
        else if (vertices[i].Pos.y < 20.0f)
        {
            vertices[i].Color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
        }
        else
        {
            vertices[i].Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        }
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

#pragma endregion
    /*------------------------------------------------------------------------------------------------------*/
#pragma region  Shapes
    //ProceduralGeometry proceGeo;
    //ProceduralGeometry::MeshData box = proceGeo.CreateBox(1.5f, 0.5f, 1.5f, 3);
    //ProceduralGeometry::MeshData grid = proceGeo.CreateGrid(20.0f, 30.0f, 60, 40);
    //ProceduralGeometry::MeshData sphere = proceGeo.CreateSphere(0.5f, 20, 20);
    //ProceduralGeometry::MeshData cylinder = proceGeo.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

    ////�����ĸ�������Ķ���������ֱ����������еĵ�ַƫ��
    //UINT boxVertexOffset = 0;
    //UINT gridVertexOffset = (UINT)box.Vertices.size();
    //UINT sphereVertexOffset = (UINT)grid.Vertices.size() + gridVertexOffset;
    //UINT cylinderVertexOffset = (UINT)sphere.Vertices.size() + sphereVertexOffset;
    ////����
    //UINT boxIndexOffset = 0;
    //UINT gridIndexOffset = (UINT)box.Indices32.size();
    //UINT sphereIndexOffset = (UINT)grid.Indices32.size() + gridIndexOffset;
    //UINT cylinderIndexOffset = (UINT)sphere.Indices32.size() + sphereIndexOffset;

    //SubmeshGeometry boxSubmesh;
    //boxSubmesh.indexCount = (UINT)box.Indices32.size();
    //boxSubmesh.baseVertexLocation = boxVertexOffset;
    //boxSubmesh.startIndexLocation = boxIndexOffset;

    //SubmeshGeometry gridSubmesh;
    //gridSubmesh.indexCount = (UINT)grid.Indices32.size();
    //gridSubmesh.baseVertexLocation = gridVertexOffset;
    //gridSubmesh.startIndexLocation = gridIndexOffset;

    //SubmeshGeometry sphereSubmesh;
    //sphereSubmesh.indexCount = (UINT)sphere.Indices32.size();
    //sphereSubmesh.baseVertexLocation = sphereVertexOffset;
    //sphereSubmesh.startIndexLocation = sphereIndexOffset;

    //SubmeshGeometry cylinderSubmesh;
    //cylinderSubmesh.indexCount = (UINT)cylinder.Indices32.size();
    //cylinderSubmesh.baseVertexLocation = cylinderVertexOffset;
    //cylinderSubmesh.startIndexLocation = cylinderIndexOffset;

    ////�����ܶ��㻺�棬�������ݴ���
    //size_t totalVertexCount = box.Vertices.size() + grid.Vertices.size() + sphere.Vertices.size() + cylinder.Vertices.size();
    ////���������С
    //std::vector<Vertex> vertices(totalVertexCount);
    //int k = 0;
    //for (size_t i = 0; i < box.Vertices.size(); i++, k++)
    //{
    //    vertices[k].Pos = box.Vertices[i].Position;
    //    vertices[k].Color = XMFLOAT4(DirectX::Colors::Blue);
    //}
    //for (size_t i = 0; i < grid.Vertices.size(); i++, k++)
    //{
    //    vertices[k].Pos = grid.Vertices[i].Position;
    //    vertices[k].Color = XMFLOAT4(DirectX::Colors::Red);
    //}
    //for (size_t i = 0; i < sphere.Vertices.size(); i++, k++)
    //{
    //    vertices[k].Pos = sphere.Vertices[i].Position;
    //    vertices[k].Color = XMFLOAT4(DirectX::Colors::Yellow);
    //}
    //for (size_t i = 0; i < cylinder.Vertices.size(); i++, k++)
    //{
    //    vertices[k].Pos = cylinder.Vertices[i].Position;
    //    vertices[k].Color = XMFLOAT4(DirectX::Colors::Green);
    //}

    ////��������������
    //std::vector<std::uint16_t> indices;
    //indices.insert(indices.end(), box.GetIndices16().begin(), box.GetIndices16().end());
    //indices.insert(indices.end(), grid.GetIndices16().begin(), grid.GetIndices16().end());
    //indices.insert(indices.end(), sphere.GetIndices16().begin(), sphere.GetIndices16().end());
    //indices.insert(indices.end(), cylinder.GetIndices16().begin(), cylinder.GetIndices16().end());

    ////��������Դ�С�����ݸ�ȫ�ֱ���
    //const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    //const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    //auto geo = std::make_unique<MeshGeometry>();
    //geo->name = "ShapeGeo";

    //geo->vertexBufferByteSize = vbByteSize;
    //geo->indexBufferByteSize = ibByteSize;

    //ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->vertexBufferCpu));	//�������������ڴ�ռ�
    //ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->indexBufferCpu));	//�������������ڴ�ռ�
    //CopyMemory(geo->vertexBufferCpu->GetBufferPointer(), vertices.data(), vbByteSize);	//���������ݿ���������ϵͳ�ڴ���
    //CopyMemory(geo->indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);	//���������ݿ���������ϵͳ�ڴ���
    //geo->vertexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize, geo->vertexBufferUploader);
    //geo->indexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize, geo->indexBufferUploader);

    //geo->vertexByteStride = sizeof(Vertex);
    //geo->indexFormat = DXGI_FORMAT_R16_UINT;

    ////ʹ������ӳ���
    //
    //geo->DrawArgs["box"] = boxSubmesh;
    //geo->DrawArgs["grid"] = gridSubmesh;
    //geo->DrawArgs["sphere"] = sphereSubmesh;
    //geo->DrawArgs["cylinder"] = cylinderSubmesh;

    //geometries[geo->name] = std::move(geo);
#pragma endregion
}

void D3D12InitApp::BuildPSO()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
    psoDesc.pRootSignature = mRootSignature.Get();
    psoDesc.VS =
    {
        reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
        mvsByteCode->GetBufferSize()
    };
    psoDesc.PS =
    {
        reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
        mpsByteCode->GetBufferSize()
    };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = mBackBufferFormat;
    psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
    psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    psoDesc.DSVFormat = mDepthStencilFormat;
    ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));
}

void D3D12InitApp::BuildMaterials()
{
    //������ӳ����װ
    //����½�صĲ���
    auto grass = std::make_unique<Material>();
    grass->name = "grass";
    grass->matCBIndex = 0;
    grass->diffuseAlbedo = XMFLOAT4(0.2f, 0.6f, 0.2f, 1.0f);    //½�ط����ʣ���ɫ��
    grass->fresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);           //½�ص�R0
    grass->roughness = 0.125f;                                  //½�صĴֲڶȣ���һ����ģ�

    //�����ˮ�Ĳ���
    auto water = std::make_unique<Material>();
    water->name = "water";
    water->matCBIndex = 1;
    water->diffuseAlbedo = XMFLOAT4(0.0f, 0.2f, 0.6f, 1.0f);    //��ˮ�ķ�����
    water->fresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    water->roughness = 0.0f;

    materials["grass"] = std::move(grass);
    materials["water"] = std::move(water);
}

void D3D12InitApp::BuildRenderItem()
{
#pragma region Hill
    auto landRitem = std::make_unique<RenderItem>();
    landRitem->world = MathHelper::Identity4x4();
    landRitem->objCBIndex = 0;
    landRitem->geo = geometries["landGeo"].get();//��ֵ����ǰ��MeshGeometry
    landRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    landRitem->indexCount = landRitem->geo->DrawArgs["landGrid"].indexCount;
    landRitem->baseVertexLocation = landRitem->geo->DrawArgs["landGrid"].baseVertexLocation;
    landRitem->startIndexLocation = landRitem->geo->DrawArgs["landGrid"].startIndexLocation;
   
    allRitems.push_back(std::move(landRitem));

#pragma endregion

#pragma region Lake
    //��������
    auto lakeRitem = std::make_unique<RenderItem>();
    lakeRitem->world = MathHelper::Identity4x4();
    lakeRitem->objCBIndex = 1;//�����ĳ������ݣ�world���������峣��������������1��
    lakeRitem->geo = geometries["lakeGeo"].get();
    lakeRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    lakeRitem->indexCount = lakeRitem->geo->DrawArgs["lakeGrid"].indexCount;
    lakeRitem->baseVertexLocation = lakeRitem->geo->DrawArgs["lakeGrid"].baseVertexLocation;
    lakeRitem->startIndexLocation = lakeRitem->geo->DrawArgs["lakeGrid"].startIndexLocation;

    wavesRitem = lakeRitem.get();

    allRitems.push_back(std::move(lakeRitem));//��push_back������ָ���Զ��ͷ��ڴ�

#pragma endregion

#pragma region Shapes
    //auto boxRItem = std::make_unique<RenderItem>();
    //XMStoreFloat4x4(&(boxRItem->world), XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
    //boxRItem->objCBIndex = 0;//�������ݣ���0�±���
    //boxRItem->geo = geometries["ShapeGeo"].get();
    //boxRItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    //boxRItem->indexCount = boxRItem->geo->DrawArgs["box"].indexCount;
    //boxRItem->baseVertexLocation = boxRItem->geo->DrawArgs["box"].baseVertexLocation;
    //boxRItem->startIndexLocation = boxRItem->geo->DrawArgs["box"].startIndexLocation;
    //allRitems.push_back(std::move(boxRItem));

    //auto gridRitem = std::make_unique<RenderItem>();
    //gridRitem->world = MathHelper::Identity4x4();
    //gridRitem->objCBIndex = 1;//BOX�������ݣ�world������objConstantBuffer����1��
    //gridRitem->geo = geometries["ShapeGeo"].get();
    //gridRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    //gridRitem->indexCount = gridRitem->geo->DrawArgs["grid"].indexCount;
    //gridRitem->baseVertexLocation = gridRitem->geo->DrawArgs["grid"].baseVertexLocation;
    //gridRitem->startIndexLocation = gridRitem->geo->DrawArgs["grid"].startIndexLocation;
    //allRitems.push_back(std::move(gridRitem));

    //UINT fllowObjCBIndex = 2;//����ȥ�ļ����峣��������CB�е�������2��ʼ
    ////��Բ����Բ��ʵ��ģ�ʹ�����Ⱦ����
    //for (int i = 0; i < 5; i++)
    //{
    //    auto leftCylinderRitem = std::make_unique<RenderItem>();
    //    auto rightCylinderRitem = std::make_unique<RenderItem>();
    //    auto leftSphereRitem = std::make_unique<RenderItem>();
    //    auto rightSphereRitem = std::make_unique<RenderItem>();

    //    XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
    //    XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);
    //    XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
    //    XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);
    //    //���5��Բ��
    //    XMStoreFloat4x4(&(leftCylinderRitem->world), leftCylWorld);
    //    //�˴�����������ѭ�����ϼ�1��ע�⣺�������ȸ�ֵ��++��
    //    leftCylinderRitem->objCBIndex = fllowObjCBIndex++;
    //    rightCylinderRitem->geo = geometries["ShapeGeo"].get();
    //    leftCylinderRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    //    leftCylinderRitem->indexCount = leftCylinderRitem->geo->DrawArgs["cylinder"].indexCount;
    //    leftCylinderRitem->baseVertexLocation = leftCylinderRitem->geo->DrawArgs["cylinder"].baseVertexLocation;
    //    leftCylinderRitem->startIndexLocation = leftCylinderRitem->geo->DrawArgs["cylinder"].startIndexLocation;
    //    //�ұ�5��Բ��
    //    XMStoreFloat4x4(&(rightCylinderRitem->world), rightCylWorld);
    //    rightCylinderRitem->objCBIndex = fllowObjCBIndex++;
    //    rightCylinderRitem->geo = geometries["ShapeGeo"].get();
    //    rightCylinderRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    //    rightCylinderRitem->indexCount = rightCylinderRitem->geo->DrawArgs["cylinder"].indexCount;
    //    rightCylinderRitem->baseVertexLocation = rightCylinderRitem->geo->DrawArgs["cylinder"].baseVertexLocation;
    //    rightCylinderRitem->startIndexLocation = rightCylinderRitem->geo->DrawArgs["cylinder"].startIndexLocation;
    //    //���5����
    //    XMStoreFloat4x4(&(leftSphereRitem->world), leftSphereWorld);
    //    leftSphereRitem->objCBIndex = fllowObjCBIndex++;
    //    leftSphereRitem->geo = geometries["ShapeGeo"].get();
    //    leftSphereRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    //    leftSphereRitem->indexCount = leftSphereRitem->geo->DrawArgs["sphere"].indexCount;
    //    leftSphereRitem->baseVertexLocation = leftSphereRitem->geo->DrawArgs["sphere"].baseVertexLocation;
    //    leftSphereRitem->startIndexLocation = leftSphereRitem->geo->DrawArgs["sphere"].startIndexLocation;
    //    //�ұ�5����
    //    XMStoreFloat4x4(&(rightSphereRitem->world), rightSphereWorld);
    //    rightSphereRitem->objCBIndex = fllowObjCBIndex++;
    //    rightSphereRitem->geo = geometries["ShapeGeo"].get();
    //    rightSphereRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    //    rightSphereRitem->indexCount = rightSphereRitem->geo->DrawArgs["sphere"].indexCount;
    //    rightSphereRitem->baseVertexLocation = rightSphereRitem->geo->DrawArgs["sphere"].baseVertexLocation;
    //    rightSphereRitem->startIndexLocation = rightSphereRitem->geo->DrawArgs["sphere"].startIndexLocation;

    //    allRitems.push_back(std::move(leftCylinderRitem));
    //    allRitems.push_back(std::move(rightCylinderRitem));
    //    allRitems.push_back(std::move(leftSphereRitem));
    //    allRitems.push_back(std::move(rightSphereRitem));
    //}
#pragma endregion
    //��ȡ����
    BuildSkullRenderItem();
}

void D3D12InitApp::DrawRenderItems()
{
    std::vector<RenderItem*> ritems;
    UINT objectCount = (UINT)allRitems.size();//�����ܸ���������ʵ����
    UINT objConstSize = ToolFunc::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    UINT matConstantSize = ToolFunc::CalcConstantBufferByteSize(sizeof(MatConstants));
    auto objCB = currFrameResource->objCB->Resource();
    /*objCBByteSize = objConstSize;
    passCBByteSize = passConstSize;*/
    //��װ����ͨ������
    for (auto& e : allRitems)
    {
        ritems.push_back(e.get());
    }

    //������Ⱦ������
    for (size_t i = 0; i < ritems.size(); i++)
    {
        auto ritem = ritems[i];

        //һ��Ҫ�����ػ�
        cmdList->IASetVertexBuffers(0, 1, &ritem->geo->GetVbv());
        cmdList->IASetIndexBuffer(&ritem->geo->GetIbv());
        cmdList->IASetPrimitiveTopology(ritem->primitiveType);

        ////���ø���������
        //UINT objCbvIndex = ritem->objCBIndex + currFrameResourceIndex * (UINT)allRitems.size();
        //auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvHeap->GetGPUDescriptorHandleForHeapStart());
        //handle.Offset(objCbvIndex, cbv_srv_uavDescriptorSize);
        //cmdList->SetGraphicsRootDescriptorTable(0, handle);
        
        //���ø�������
        auto objCBAddress = objCB->GetGPUVirtualAddress();
        objCBAddress += ritem->objCBIndex * objConstSize;
        cmdList->SetGraphicsRootConstantBufferView(0,
            objCBAddress);

        //���ø���������������������matCB��Դ��
        auto matCB = currFrameResource->matCB->Resource();
        auto matCBAddress = matCB->GetGPUVirtualAddress();
        matCBAddress += ritem->mat->matCBIndex * matConstantSize;
        cmdList->SetGraphicsRootConstantBufferView(1,
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
            currFrameResource->objCB->CopyData(e->objCBIndex, objConstants);

            e->NumFramesDirty--;
        }
    }
}

void D3D12InitApp::UpdatePassCBs()
{
    PassConstants passConstants;

    ////�����۲����
    float x = mRadius * sinf(mPhi) * cosf(mTheta);
    float z = mRadius * sinf(mPhi) * sinf(mTheta);
    float y = mRadius * cosf(mPhi);

    //�����۲����
    XMVECTOR pos = XMVectorSet(x, y, z, 10.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);

    // ����ͶӰ����
    XMMATRIX proj = XMLoadFloat4x4(&mProj);
    //�����任����SRT����
    XMMATRIX VP_Matrix = view * proj;
    XMStoreFloat4x4(&passConstants.viewProj, XMMatrixTranspose(VP_Matrix));

    passConstants.ambientLight = { 0.25f,0.25f,0.35f,1.0f };
    passConstants.lights[0].strength = { 1.0f,1.0f,0.9f };
    passConstants.lights[0].direction = { 0.0f,0.0f,0.0f };

    currFrameResource->passCB->CopyData(0, passConstants);
}

void D3D12InitApp::BuildFrameResources()
{
    for (int i = 0; i < frameResourcesCount; i++)
    {
        FrameResourcesArray.push_back(std::make_unique<FrameResource>(
            d3dDevice.Get(),
            1,
            (UINT)allRitems.size(),     //�����建������
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
        v.Color = XMFLOAT4(DirectX::Colors::Red);

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

void D3D12InitApp::BuildSkullGeometry()
{
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
            fin >> ignore >> ignore >> ignore;
            vertices[i].Color = XMFLOAT4(DirectX::Colors::Black);
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
}
    
void D3D12InitApp::BuildSkullRenderItem()
{
    auto skullRitem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&skullRitem->world, XMMatrixScaling(0.5f, 0.5f, 0.5f) * XMMatrixTranslation(0.0f, 1.0f, 0.0f));
    skullRitem->objCBIndex = 2;
    skullRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    skullRitem->geo = geometries["skullGeo"].get();
    skullRitem->indexCount = skullRitem->geo->DrawArgs["skull"].indexCount;
    skullRitem->baseVertexLocation = skullRitem->geo->DrawArgs["skull"].baseVertexLocation;
    skullRitem->startIndexLocation = skullRitem->geo->DrawArgs["skull"].startIndexLocation;
    allRitems.push_back(std::move(skullRitem));
}