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

 
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildGeometry();
    BuildRenderItem();
    BuildFrameResources();
    BuildConstantBuffers();
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

    
    ////�����۲����
    float x = mRadius * sinf(mPhi) * cosf(mTheta);
    float z = mRadius * sinf(mPhi) * sinf(mTheta);
    float y = mRadius * cosf(mPhi);

    //�����۲����
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);

    ObjectConstants objConstants;
    PassConstants passConstants;
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
   // ����ͶӰ����
    XMMATRIX proj = XMLoadFloat4x4(&mProj);
    //�����任����SRT����
    XMMATRIX VP_Matrix = view * proj;
    XMStoreFloat4x4(&passConstants.viewProj, XMMatrixTranspose(VP_Matrix));
    currFrameResource->passCB->CopyData(0, passConstants);
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

    ID3D12DescriptorHeap* descriptorHeaps[] = { cbvHeap.Get() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootSignature(mRootSignature.Get());

    //���ø���������
    int passCBVIndex = (int)allRitems.size() * frameResourcesCount + currFrameResourceIndex;
    auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvHeap->GetGPUDescriptorHandleForHeapStart());
    passCbvHandle.Offset(passCBVIndex, cbv_srv_uavDescriptorSize);
    cmdList->SetGraphicsRootDescriptorTable(1,  //����������ʼ���� 
        passCbvHandle);

    DrawRenderItems();

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

        // Update angles based on input to orbit camera around box.
        mTheta += dx;
        mPhi += dy;

        // Restrict the angle mPhi.
        mPhi = MathHelper::Clamp(mPhi, 0.1f, 3.1416f - 0.1f);
    }
    else if ((btnState & MK_RBUTTON) != 0)
    {
        // Make each pixel correspond to 0.005 unit in the scene.
        float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
        float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

        // Update the camera radius based on input.
        mRadius += dx - dy;

        // Restrict the radius.
        mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
    }

    mLastMousePos.x = x;
    mLastMousePos.y = y;
}

void D3D12InitApp::BuildConstantBuffers()
{
    //����CBV��
    UINT objectCount = (UINT)allRitems.size();//�����ܸ���
    int frameResourcesCount = gNumFrameResources;
    UINT objConstSize = ToolFunc::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    UINT passConstSize = ToolFunc::CalcConstantBufferByteSize(sizeof(PassConstants));

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = (objectCount + 1) * frameResourcesCount;//һ���Ѱ������������+1��CBV
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
        IID_PPV_ARGS(&cbvHeap)));

/*------------------------------------------------------------------------------------------------------*/
    //elementCountΪobjectCount,22��22�������峣������Ԫ�أ���isConstantBufferΪture���ǳ�����������
    for (int frameIndex = 0; frameIndex < frameResourcesCount; frameIndex++)
    {
        for (int i = 0; i < objectCount; i++)
        {
            D3D12_GPU_VIRTUAL_ADDRESS objCB_Address;
            objCB_Address = FrameResourcesArray[frameIndex]->objCB->Resource()->GetGPUVirtualAddress();
            //�ڻ�ַ�Ļ����ϼ���������±�ʹ�С��ˣ������Ҫ������ĵ�ַ
            objCB_Address += i * objConstSize;

            //CBV���е�CBVԪ���±꣨������
            int heapIndex = objectCount * frameIndex + i;
            //���CBV�ѵ��׵�ַ
            auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvHeap->GetCPUDescriptorHandleForHeapStart());
            handle.Offset(heapIndex, cbv_srv_uavDescriptorSize);//CBV�������ֱ�Ӹ���ƫ�������ͶѴ�С�ҵ�λ��
            //����CBV������
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
            cbvDesc.BufferLocation = objCB_Address;
            cbvDesc.SizeInBytes = objConstSize;

            //����CBV
            d3dDevice->CreateConstantBufferView(
                &cbvDesc,
                handle);
        }
    }
/*------------------------------------------------------------------------------------------------------*/
    //�����ڶ���CBV(passCBV)
   
    //��ó����������׵�ַ
    for (int frameIndex = 0; frameIndex < frameResourcesCount; frameIndex++)
    {
        D3D12_GPU_VIRTUAL_ADDRESS passCB_Address;
        passCB_Address = FrameResourcesArray[frameIndex]->passCB->Resource()->GetGPUVirtualAddress();
        int passCBElementIndex = 0;
        passCB_Address += passCBElementIndex * passConstSize;
        int heapIndex = objectCount * frameResourcesCount + frameIndex;
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(heapIndex, cbv_srv_uavDescriptorSize);
        //����CBV������
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = passCB_Address;
        cbvDesc.SizeInBytes = passConstSize;

        d3dDevice->CreateConstantBufferView(
            &cbvDesc,
            handle
        );
    }
}

void D3D12InitApp::BuildRootSignature()
{
    //������������������������������������    
    CD3DX12_ROOT_PARAMETER slotRootParameter[2];

    //�����ɵ���CBV����ɵ���������
    CD3DX12_DESCRIPTOR_RANGE cbvTable0;
    cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,     //����������
        1,                                              //����������
        0);                                             //���������󶨵ļĴ����ۺ�
    CD3DX12_DESCRIPTOR_RANGE cbvTable1;
    cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,     //����������
        1,                                              //����������
        1);                                             //���������󶨵ļĴ����ۺ�
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
    slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

    //��ǩ����һ�����������
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2,      //������������ 
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
        vertices[k].Color = XMCOLOR(DirectX::Colors::Yellow);
    }
    for (size_t i = 0; i < grid.Vertices.size(); i++, k++)
    {
        vertices[k].Pos = grid.Vertices[i].Position;
        vertices[k].Color = XMCOLOR(DirectX::Colors::Brown);
    }
    for (size_t i = 0; i < sphere.Vertices.size(); i++, k++)
    {
        vertices[k].Pos = sphere.Vertices[i].Position;
        vertices[k].Color = XMCOLOR(DirectX::Colors::Green);
    }
    for (size_t i = 0; i < cylinder.Vertices.size(); i++, k++)
    {
        vertices[k].Pos = cylinder.Vertices[i].Position;
        vertices[k].Color = XMCOLOR(DirectX::Colors::Blue);
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
    VertexBufferByteSize = vbByteSize;
    IndexBufferByteSize = ibByteSize;

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &vertexBufferCpu));	//�������������ڴ�ռ�
    ThrowIfFailed(D3DCreateBlob(ibByteSize, &indexBufferCpu));	//�������������ڴ�ռ�
    CopyMemory(vertexBufferCpu->GetBufferPointer(), vertices.data(), vbByteSize);	//���������ݿ���������ϵͳ�ڴ���
    CopyMemory(indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);	//���������ݿ���������ϵͳ�ڴ���
    vertexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize,  vertexBufferUploader);
    indexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize,  indexBufferUploader);

    //ʹ������ӳ���
    
    DrawArgs["box"] = boxSubmesh;
    DrawArgs["grid"] = gridSubmesh;
    DrawArgs["sphere"] = sphereSubmesh;
    DrawArgs["cylinder"] = cylinderSubmesh;
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
    //psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
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

void D3D12InitApp::BuildRenderItem()
{
    auto boxRItem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&(boxRItem->world), XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
    boxRItem->objCBIndex = 0;//�������ݣ���0�±���
    boxRItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    boxRItem->indexCount = DrawArgs["box"].indexCount;
    boxRItem->baseVertexLocation = DrawArgs["box"].baseVertexLocation;
    boxRItem->startIndexLocation = DrawArgs["box"].startIndexLocation;
    allRitems.push_back(std::move(boxRItem));

    auto gridRitem = std::make_unique<RenderItem>();
    gridRitem->world = MathHelper::Identity4x4();
    gridRitem->objCBIndex = 1;//BOX�������ݣ�world������objConstantBuffer����1��
    gridRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridRitem->indexCount = DrawArgs["grid"].indexCount;
    gridRitem->baseVertexLocation = DrawArgs["grid"].baseVertexLocation;
    gridRitem->startIndexLocation = DrawArgs["grid"].startIndexLocation;
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
        leftCylinderRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftCylinderRitem->indexCount = DrawArgs["cylinder"].indexCount;
        leftCylinderRitem->baseVertexLocation = DrawArgs["cylinder"].baseVertexLocation;
        leftCylinderRitem->startIndexLocation = DrawArgs["cylinder"].startIndexLocation;
        //�ұ�5��Բ��
        XMStoreFloat4x4(&(rightCylinderRitem->world), rightCylWorld);
        rightCylinderRitem->objCBIndex = fllowObjCBIndex++;
        rightCylinderRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightCylinderRitem->indexCount = DrawArgs["cylinder"].indexCount;
        rightCylinderRitem->baseVertexLocation = DrawArgs["cylinder"].baseVertexLocation;
        rightCylinderRitem->startIndexLocation = DrawArgs["cylinder"].startIndexLocation;
        //���5����
        XMStoreFloat4x4(&(leftSphereRitem->world), leftSphereWorld);
        leftSphereRitem->objCBIndex = fllowObjCBIndex++;
        leftSphereRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftSphereRitem->indexCount = DrawArgs["sphere"].indexCount;
        leftSphereRitem->baseVertexLocation = DrawArgs["sphere"].baseVertexLocation;
        leftSphereRitem->startIndexLocation = DrawArgs["sphere"].startIndexLocation;
        //�ұ�5����
        XMStoreFloat4x4(&(rightSphereRitem->world), rightSphereWorld);
        rightSphereRitem->objCBIndex = fllowObjCBIndex++;
        rightSphereRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightSphereRitem->indexCount = DrawArgs["sphere"].indexCount;
        rightSphereRitem->baseVertexLocation = DrawArgs["sphere"].baseVertexLocation;
        rightSphereRitem->startIndexLocation = DrawArgs["sphere"].startIndexLocation;

        allRitems.push_back(std::move(leftCylinderRitem));
        allRitems.push_back(std::move(rightCylinderRitem));
        allRitems.push_back(std::move(leftSphereRitem));
        allRitems.push_back(std::move(rightSphereRitem));
    }
}

void D3D12InitApp::DrawRenderItems()
{
    std::vector<RenderItem*> ritems;
    //��װ����ͨ������
    for (auto& e : allRitems)
    {
        ritems.push_back(e.get());
    }

    //������Ⱦ������
    for (size_t i = 0; i < ritems.size(); i++)
    {
        auto ritem = ritems[i];

        cmdList->IASetVertexBuffers(0, 1, &GetVbv());
        cmdList->IASetIndexBuffer(&GetIbv());
        cmdList->IASetPrimitiveTopology(ritem->primitiveType);

        //���ø���������
        UINT objCbvIndex = ritem->objCBIndex + currFrameResourceIndex * (UINT)allRitems.size();
        auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvHeap->GetGPUDescriptorHandleForHeapStart());
        handle.Offset(objCbvIndex, cbv_srv_uavDescriptorSize);
        cmdList->SetGraphicsRootDescriptorTable(0, handle);

        //���ƶ���
        cmdList->DrawIndexedInstanced(ritem->indexCount,    //ÿ��ʵ�����Ƶ���������
            1,                                              //ʵ��������
            ritem->startIndexLocation,                      //��ʼ����λ��
            ritem->baseVertexLocation,                      //��������ʼ������ȫ�������е�λ��
            0);                                             //ʵ�����ĸ߼�����
    }
}

void D3D12InitApp::BuildFrameResources()
{
    for (int i = 0; i < frameResourcesCount; i++)
    {
        FrameResourcesArray.push_back(std::make_unique<FrameResource>(
            d3dDevice.Get(),
            1,
            (UINT)allRitems.size()
            ));
    }
}

D3D12_VERTEX_BUFFER_VIEW D3D12InitApp::GetVbv() const
{
    //���������ݰ�����Ⱦ��ˮ��
    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = vertexBufferGpu->GetGPUVirtualAddress();	//���㻺������Ĭ�϶ѣ���Դ�����ַ
    vbv.SizeInBytes = VertexBufferByteSize;							//���㻺������С
    vbv.StrideInBytes = sizeof(Vertex);								//ÿ������ռ�õ��ֽ���
    //���ö��㻺����
    return vbv;
}

D3D12_INDEX_BUFFER_VIEW D3D12InitApp::GetIbv() const
{
    D3D12_INDEX_BUFFER_VIEW ibv;
    ibv.BufferLocation = indexBufferGpu->GetGPUVirtualAddress();
    ibv.Format = DXGI_FORMAT_R16_UINT;
    ibv.SizeInBytes = IndexBufferByteSize;
    return ibv;
}
