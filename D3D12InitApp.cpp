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
    BuildConstantBuffers();
    BuildPSO();

    // Execute the initialization commands.
    ThrowIfFailed(cmdList->Close());
    ID3D12CommandList* cmdsLists[] = { cmdList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

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
    ObjectConstants objConstants;
    PassConstants passConstants;
    ////构建观察矩阵
    float x = mRadius * sinf(mPhi) * cosf(mTheta);
    float z = mRadius * sinf(mPhi) * sinf(mTheta);
    float y = mRadius * cosf(mPhi);

    //构建观察矩阵
    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);

    //构建世界矩阵
    for (auto& e : allRitems)
    {
        mWorld = e->world;
        XMMATRIX w = XMLoadFloat4x4(&mWorld);
        //将矩阵复制给4x4
        XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(w));
        //将数据拷贝给GPU缓存
        objCB->CopyData(e->objCBIndex, objConstants);
    }
   // 构建投影矩阵
    XMMATRIX proj = XMLoadFloat4x4(&mProj);
    //构建变换矩阵，SRT矩阵
    XMMATRIX VP_Matrix = view * proj;
    XMStoreFloat4x4(&passConstants.viewProj, XMMatrixTranspose(VP_Matrix));
    passCB->CopyData(0, passConstants);
}

void D3D12InitApp::Draw(const GameTime& gt)
{
    // Reuse the memory associated with command recording.
    // We can only reset when the associated command lists have finished execution on the GPU.
    ThrowIfFailed(cmdAllocator->Reset());

    // A command list can be reset after it has been added to the command queue via ExecuteCommandList.
    // Reusing the command list reuses memory.
    ThrowIfFailed(cmdList->Reset(cmdAllocator.Get(), mPSO.Get()));

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

    //设置根描述符表
    int passCBVIndex = (int)allRitems.size();
    auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvHeap->GetGPUDescriptorHandleForHeapStart());
    passCbvHandle.Offset(passCBVIndex, cbv_srv_uavDescriptorSize);
    cmdList->SetGraphicsRootDescriptorTable(1,  //根参数的起始索引 
        passCbvHandle);

    DrawRenderItems();

    // Indicate a state transition on the resource usage.
    cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    // Done recording commands.
    ThrowIfFailed(cmdList->Close());

    // Add the command list to the queue for execution.
    ID3D12CommandList* cmdsLists[] = { cmdList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // swap the back and front buffers
    ThrowIfFailed(mSwapChain->Present(0, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

    // Wait until frame commands are complete.  This waiting is inefficient and is
    // done for simplicity.  Later we will show how to organize our rendering code
    // so we do not have to wait per frame.
    FlushCommandQueue();
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
    //创建CBV堆
    UINT objectCount = (UINT)allRitems.size();//物体总个数
    UINT objConstSize = ToolFunc::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    UINT passConstSize = ToolFunc::CalcConstantBufferByteSize(sizeof(PassConstants));

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    cbvHeapDesc.NumDescriptors = objectCount + 1;//一个堆包含几何体个数+1个CBV
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
        IID_PPV_ARGS(&cbvHeap)));

/*------------------------------------------------------------------------------------------------------*/
    //elementCount为objectCount,22（22个子物体常量缓冲元素），isConstantBuffer为ture（是常量缓冲区）
    objCB = std::make_unique<UploadBufferResource<ObjectConstants>>(d3dDevice.Get(), objectCount, true);
    //获得常量缓冲区首地址
    for (int i = 0; i < objectCount; i++)
    {
        D3D12_GPU_VIRTUAL_ADDRESS objCB_Address;
        objCB_Address = objCB->Resource()->GetGPUVirtualAddress();
        //常量缓冲区子物体下标
        int objCBElementIndex = i;
        //在基址的基础上加上物体的下标和大小相乘，获得想要的物体的地址
        objCB_Address += objCBElementIndex * objConstSize;

        //CBV堆中的CBV元素下标（索引）
        int heapIndex = i;
        //获得CBV堆的首地址
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(heapIndex, cbv_srv_uavDescriptorSize);//CBV句柄可以直接根据偏移数量和堆大小找到位置
        //创建CBV描述符
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = objCB_Address;
        cbvDesc.SizeInBytes = objConstSize;

        //创建CBV
        d3dDevice->CreateConstantBufferView(
            &cbvDesc,
            handle);
    }
/*------------------------------------------------------------------------------------------------------*/
    //创建第二个CBV(passCBV)
    passCB = std::make_unique<UploadBufferResource<PassConstants>>(d3dDevice.Get(), 1, true);
    //获得常量缓冲区首地址
    D3D12_GPU_VIRTUAL_ADDRESS passCB_Address;
    passCB_Address = passCB->Resource()->GetGPUVirtualAddress();
    int passCBElementIndex = 0;
    passCB_Address += passCBElementIndex * passConstSize;
    int heapIndex = objectCount;
    auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvHeap->GetCPUDescriptorHandleForHeapStart());
    handle.Offset(heapIndex, cbv_srv_uavDescriptorSize);
    //创建CBV描述符
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = passCB_Address;
    cbvDesc.SizeInBytes = passConstSize;

    d3dDevice->CreateConstantBufferView(
        &cbvDesc,
        handle
    );
}

void D3D12InitApp::BuildRootSignature()
{
    //根参数可以是描述符表、根描述符、根常量    
    CD3DX12_ROOT_PARAMETER slotRootParameter[2];

    //创建由单个CBV所组成的描述符表
    CD3DX12_DESCRIPTOR_RANGE cbvTable0;
    cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,     //描述符类型
        1,                                              //描述符数量
        0);                                             //描述符锁绑定的寄存器槽号
    CD3DX12_DESCRIPTOR_RANGE cbvTable1;
    cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,     //描述符类型
        1,                                              //描述符数量
        1);                                             //描述符锁绑定的寄存器槽号
    slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
    slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

    //根签名由一组根参数构成
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2,      //根参数的数量 
        slotRootParameter,                          //根参数指针
        0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    //用单个寄存器槽来创建一个根签名，该槽位指向一个仅含有单个常量缓冲区的描述符区域
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

    //计算四个几何体的顶点和索引分别在总数组中的地址偏移
    UINT boxVertexOffset = 0;
    UINT gridVertexOffset = (UINT)box.Vertices.size();
    UINT sphereVertexOffset = (UINT)grid.Vertices.size() + gridVertexOffset;
    UINT cylinderVertexOffset = (UINT)sphere.Vertices.size() + sphereVertexOffset;
    //索引
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

    //创建总顶点缓存，所有数据存入
    size_t totalVertexCount = box.Vertices.size() + grid.Vertices.size() + sphere.Vertices.size() + cylinder.Vertices.size();
    //顶点数组大小
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

    //创建总索引缓存
    std::vector<std::uint16_t> indices;
    indices.insert(indices.end(), box.GetIndices16().begin(), box.GetIndices16().end());
    indices.insert(indices.end(), grid.GetIndices16().begin(), grid.GetIndices16().end());
    indices.insert(indices.end(), sphere.GetIndices16().begin(), sphere.GetIndices16().end());
    indices.insert(indices.end(), cylinder.GetIndices16().begin(), cylinder.GetIndices16().end());

    //计算出各自大小，传递给全局变量
    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
    VertexBufferByteSize = vbByteSize;
    IndexBufferByteSize = ibByteSize;

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &vertexBufferCpu));	//创建顶点数据内存空间
    ThrowIfFailed(D3DCreateBlob(ibByteSize, &indexBufferCpu));	//创建索引数据内存空间
    CopyMemory(vertexBufferCpu->GetBufferPointer(), vertices.data(), vbByteSize);	//将顶点数据拷贝至顶点系统内存中
    CopyMemory(indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);	//将索引数据拷贝至索引系统内存中
    vertexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize,  vertexBufferUploader);
    indexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize,  indexBufferUploader);

    //使用无序映射表
    
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
    boxRItem->objCBIndex = 0;//常量数据，在0下标上
    boxRItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    boxRItem->indexCount = DrawArgs["box"].indexCount;
    boxRItem->baseVertexLocation = DrawArgs["box"].baseVertexLocation;
    boxRItem->startIndexLocation = DrawArgs["box"].startIndexLocation;
    allRitems.push_back(std::move(boxRItem));

    auto gridRitem = std::make_unique<RenderItem>();
    gridRitem->world = MathHelper::Identity4x4();
    gridRitem->objCBIndex = 1;//BOX常量数据（world矩阵）在objConstantBuffer索引1上
    gridRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    gridRitem->indexCount = DrawArgs["grid"].indexCount;
    gridRitem->baseVertexLocation = DrawArgs["grid"].baseVertexLocation;
    gridRitem->startIndexLocation = DrawArgs["grid"].startIndexLocation;
    allRitems.push_back(std::move(gridRitem));

    UINT fllowObjCBIndex = 2;//接下去的几何体常量数据在CB中的索引从2开始
    //将圆柱和圆的实例模型存入渲染项中
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
        //左边5个圆柱
        XMStoreFloat4x4(&(leftCylinderRitem->world), leftCylWorld);
        //此处的索引随着循环不断加1（注意：这里是先赋值再++）
        leftCylinderRitem->objCBIndex = fllowObjCBIndex++;
        leftCylinderRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftCylinderRitem->indexCount = DrawArgs["cylinder"].indexCount;
        leftCylinderRitem->baseVertexLocation = DrawArgs["cylinder"].baseVertexLocation;
        leftCylinderRitem->startIndexLocation = DrawArgs["cylinder"].startIndexLocation;
        //右边5个圆柱
        XMStoreFloat4x4(&(rightCylinderRitem->world), rightCylWorld);
        rightCylinderRitem->objCBIndex = fllowObjCBIndex++;
        rightCylinderRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        rightCylinderRitem->indexCount = DrawArgs["cylinder"].indexCount;
        rightCylinderRitem->baseVertexLocation = DrawArgs["cylinder"].baseVertexLocation;
        rightCylinderRitem->startIndexLocation = DrawArgs["cylinder"].startIndexLocation;
        //左边5个球
        XMStoreFloat4x4(&(leftSphereRitem->world), leftSphereWorld);
        leftSphereRitem->objCBIndex = fllowObjCBIndex++;
        leftSphereRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        leftSphereRitem->indexCount = DrawArgs["sphere"].indexCount;
        leftSphereRitem->baseVertexLocation = DrawArgs["sphere"].baseVertexLocation;
        leftSphereRitem->startIndexLocation = DrawArgs["sphere"].startIndexLocation;
        //右边5个球
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
    //先装进普通数组中
    for (auto& e : allRitems)
    {
        ritems.push_back(e.get());
    }

    //遍历渲染项数组
    for (size_t i = 0; i < ritems.size(); i++)
    {
        auto ritem = ritems[i];

        cmdList->IASetVertexBuffers(0, 1, &GetVbv());
        cmdList->IASetIndexBuffer(&GetIbv());
        cmdList->IASetPrimitiveTopology(ritem->primitiveType);

        //设置根描述符表
        UINT objCbvIndex = ritem->objCBIndex;
        auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvHeap->GetGPUDescriptorHandleForHeapStart());
        handle.Offset(objCbvIndex, cbv_srv_uavDescriptorSize);
        cmdList->SetGraphicsRootDescriptorTable(0, handle);

        //绘制顶点
        cmdList->DrawIndexedInstanced(ritem->indexCount,    //每个实例绘制的索引数量
            1,                                              //实例化个数
            ritem->startIndexLocation,                      //起始索引位置
            ritem->baseVertexLocation,                      //子物体起始索引在全局索引中的位置
            0);                                             //实例化的高级技术
    }
}

D3D12_VERTEX_BUFFER_VIEW D3D12InitApp::GetVbv() const
{
    //将顶点数据绑定至渲染流水线
    D3D12_VERTEX_BUFFER_VIEW vbv;
    vbv.BufferLocation = vertexBufferGpu->GetGPUVirtualAddress();	//顶点缓冲区（默认堆）资源虚拟地址
    vbv.SizeInBytes = VertexBufferByteSize;							//顶点缓冲区大小
    vbv.StrideInBytes = sizeof(Vertex);								//每个顶点占用的字节数
    //设置顶点缓冲区
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
