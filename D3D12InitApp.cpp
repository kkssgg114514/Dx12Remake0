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

    //初始化波浪数据
    waves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);
 
    LoadTextures();
    BuildRootSignature();
    BuildSRV();
    BuildShadersAndInputLayout();
    BuildGeometry();
    BuildLakeIndexBuffer();
    //BuildSkullGeometry();
    BuildBoxGeometry();
    BuildMaterials();
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
    OnKeyboardInput(gt);

    currFrameResourceIndex = (currFrameResourceIndex + 1) % frameResourcesCount;
    //首次初始化
    currFrameResource = FrameResourcesArray[currFrameResourceIndex].get();

    //如果GPU端围栏值小于CPU端围栏值，即CPU速度快于GPU，则令CPU等待
    if (currFrameResource->fenceCPU != 0 && fence->GetCompletedValue() < currFrameResource->fenceCPU)
    {
        HANDLE eventHandle = CreateEvent(nullptr, false, false, L"FenceSetDone");
        ThrowIfFailed(fence->SetEventOnCompletion(currFrameResource->fenceCPU, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle;
    }

    AnimateMaterials(gt);
    UpdateObjCBs();
    UpdatePassCBs(gt);
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
    //一开始还是设置不透明的
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

    ////设置CBV描述符堆
    //ID3D12DescriptorHeap* descriptorHeaps[] = { cbvHeap.Get() };
    //cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    //设置SRV描述符堆，和上面没什么区别
    ID3D12DescriptorHeap* descriptorHeaps[] = { srvHeap.Get() };
    cmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    cmdList->SetGraphicsRootSignature(mRootSignature.Get());

    ////设置根描述符表
    //int passCBVIndex = (int)allRitems.size() * frameResourcesCount + currFrameResourceIndex;
    //auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvHeap->GetGPUDescriptorHandleForHeapStart());
    //passCbvHandle.Offset(passCBVIndex, cbv_srv_uavDescriptorSize);
    //cmdList->SetGraphicsRootDescriptorTable(1,  //根参数的起始索引 
    //    passCbvHandle);
   
    //设置passCB描述符
    auto passCB = currFrameResource->passCB->Resource();
    cmdList->SetGraphicsRootConstantBufferView(3,
        passCB->GetGPUVirtualAddress());

    DrawRenderItems(ritemLayer[(int)RenderLayer::Opaque]);
    cmdList->SetPipelineState(PSOs["alphaTest"].Get());
    DrawRenderItems(ritemLayer[(int)RenderLayer::AlphaTest]);
    cmdList->SetPipelineState(PSOs["transparent"].Get());
    DrawRenderItems(ritemLayer[(int)RenderLayer::Transparent]);
    
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

        // 根据输入改变旋转的角度，加减可以改变方向
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

        //限制了观察半径的范围，即摄像机距离中心点的距离
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
    //将Phi约束在[0, PI/2]之间
    sunPhi = MathHelper::Clamp(sunPhi, 0.1f, XM_PIDIV2);

}

void D3D12InitApp::BuildConstantBuffers()
{
    ////创建CBV堆
    //UINT objectCount = (UINT)allRitems.size();//物体总个数
    //int frameResourcesCount = gNumFrameResources;
    //UINT objConstSize = ToolFunc::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    //UINT passConstSize = ToolFunc::CalcConstantBufferByteSize(sizeof(PassConstants));

    //D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
    //cbvHeapDesc.NumDescriptors = (objectCount + 1) * frameResourcesCount;//一个堆包含几何体个数+1个CBV
    //cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    //cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    //cbvHeapDesc.NodeMask = 0;
    //ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&cbvHeapDesc,
    //    IID_PPV_ARGS(&cbvHeap)));
//
///*------------------------------------------------------------------------------------------------------*/
//    //elementCount为objectCount,22（22个子物体常量缓冲元素），isConstantBuffer为ture（是常量缓冲区）
//    for (int frameIndex = 0; frameIndex < frameResourcesCount; frameIndex++)
//    {
//        for (int i = 0; i < objectCount; i++)
//        {
//            D3D12_GPU_VIRTUAL_ADDRESS objCB_Address;
//            objCB_Address = FrameResourcesArray[frameIndex]->objCB->Resource()->GetGPUVirtualAddress();
//            //在基址的基础上加上物体的下标和大小相乘，获得想要的物体的地址
//            objCB_Address += i * objConstSize;
//
//            //CBV堆中的CBV元素下标（索引）
//            int heapIndex = objectCount * frameIndex + i;
//            //获得CBV堆的首地址
//            auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvHeap->GetCPUDescriptorHandleForHeapStart());
//            handle.Offset(heapIndex, cbv_srv_uavDescriptorSize);//CBV句柄可以直接根据偏移数量和堆大小找到位置
//            //创建CBV描述符
//            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
//            cbvDesc.BufferLocation = objCB_Address;
//            cbvDesc.SizeInBytes = objConstSize;
//
//            //创建CBV
//            d3dDevice->CreateConstantBufferView(
//                &cbvDesc,
//                handle);
//        }
//    }
///*------------------------------------------------------------------------------------------------------*/
//    //创建第二个CBV(passCBV)
//   
//    //获得常量缓冲区首地址
//    for (int frameIndex = 0; frameIndex < frameResourcesCount; frameIndex++)
//    {
//        D3D12_GPU_VIRTUAL_ADDRESS passCB_Address;
//        passCB_Address = FrameResourcesArray[frameIndex]->passCB->Resource()->GetGPUVirtualAddress();
//        int passCBElementIndex = 0;
//        passCB_Address += passCBElementIndex * passConstSize;
//        int heapIndex = objectCount * frameResourcesCount + frameIndex;
//        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(cbvHeap->GetCPUDescriptorHandleForHeapStart());
//        handle.Offset(heapIndex, cbv_srv_uavDescriptorSize);
//        //创建CBV描述符
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
    //SRV描述结构体
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = grassTex->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = grassTex->GetDesc().MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    //创建SRV
    d3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, handle);

    //SRV堆中子SRV（湖水的SRV）的句柄,继续偏移一个SRV
    handle.Offset(1, cbv_srv_uavDescriptorSize);
    //SRV描述结构体修改
    srvDesc.Format = lakeTex->GetDesc().Format;//视图的默认格式
    srvDesc.Texture2D.MipLevels = lakeTex->GetDesc().MipLevels;//mipmap层级数量
    //创建“湖水”的SRV
    d3dDevice->CreateShaderResourceView(lakeTex.Get(), &srvDesc, handle);

    //SRV堆中子SRV（板条箱的SRV）的句柄,继续偏移一个SRV
    handle.Offset(1, cbv_srv_uavDescriptorSize);
    //SRV描述结构体修改
    srvDesc.Format = woodCrateTex->GetDesc().Format;//视图的默认格式
    srvDesc.Texture2D.MipLevels = woodCrateTex->GetDesc().MipLevels;//mipmap层级数量
    //创建“板条箱”的SRV
    d3dDevice->CreateShaderResourceView(woodCrateTex.Get(), &srvDesc, handle);

}

void D3D12InitApp::BuildRootSignature()
{

    CD3DX12_DESCRIPTOR_RANGE srvTable;
    srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        1,
        0);
    //根参数可以是描述符表、根描述符、根常量    
    CD3DX12_ROOT_PARAMETER slotRootParameter[4];
    
    slotRootParameter[0].InitAsDescriptorTable(1,
        &srvTable,
        D3D12_SHADER_VISIBILITY_PIXEL);
    slotRootParameter[1].InitAsConstantBufferView(0);
    slotRootParameter[2].InitAsConstantBufferView(1);
    slotRootParameter[3].InitAsConstantBufferView(2);
    
    auto staticSamplers = GetStaticSamplers();

    //根签名由一组根参数构成
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4,      //根参数的数量 
        slotRootParameter,                          //根参数指针
        (UINT)staticSamplers.size(),
        staticSamplers.data(),
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

    //编译着色器时定义的宏
    const D3D_SHADER_MACRO defines[] =
    {

        NULL, NULL
    };

    const D3D_SHADER_MACRO alphaTestDefines[] =
    {
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

void D3D12InitApp::BuildGeometry()
{
#pragma region Hill
    ProceduralGeometry proceGeo;
    ProceduralGeometry::MeshData grid = proceGeo.CreateGrid(160.0f, 160.0f, 50, 50);

    //封装顶点、索引
    SubmeshGeometry gridSubmesh;
    gridSubmesh.baseVertexLocation = 0;
    gridSubmesh.startIndexLocation = 0;
    gridSubmesh.indexCount = (UINT)grid.Indices32.size();

    //创建顶点缓存
    size_t VertexCount = grid.Vertices.size();
    std::vector<Vertex> vertices(VertexCount);
    for (int i = 0; i < grid.Vertices.size(); i++)
    {
        vertices[i].Pos = grid.Vertices.at(i).Position;
        vertices[i].Pos.y = GetHillsHeight(vertices.at(i).Pos.x, vertices.at(i).Pos.z);

        vertices[i].Normal = GetHillsNormal(vertices.at(i).Pos.x, vertices.at(i).Pos.z);
        vertices[i].TexC = grid.Vertices.at(i).TexC;
        ////颜色是RGBA
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

    //创建索引缓存
    std::vector<std::uint16_t> indices = grid.GetIndices16();

    //计算顶点缓存和索引缓存大小
    auto geo = std::make_unique<MeshGeometry>();
    geo->name = "landGeo";
    const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
    geo->vertexBufferByteSize = vbByteSize;
    geo->indexBufferByteSize = ibByteSize;
    geo->vertexByteStride = sizeof(Vertex);
    geo->indexFormat = DXGI_FORMAT_R16_UINT;

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->vertexBufferCpu));     //创建顶点数据内存空间
    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->indexBufferCpu));
    CopyMemory(geo->vertexBufferCpu->GetBufferPointer(), vertices.data(), vbByteSize);
    CopyMemory(geo->indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);
    geo->vertexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize, geo->vertexBufferUploader);
    geo->indexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize, geo->indexBufferUploader);

    //将封装好的几何体SubmeshGeometry对象赋值给无序表
    geo->DrawArgs["landGrid"] = gridSubmesh;
    //将山川的网格几何体入总表
    geometries["landGeo"] = std::move(geo);


#pragma endregion
    /*------------------------------------------------------------------------------------------------------*/
#pragma region  Shapes
    //ProceduralGeometry proceGeo;
    //ProceduralGeometry::MeshData box = proceGeo.CreateBox(1.5f, 0.5f, 1.5f, 3);
    //ProceduralGeometry::MeshData grid = proceGeo.CreateGrid(20.0f, 30.0f, 60, 40);
    //ProceduralGeometry::MeshData sphere = proceGeo.CreateSphere(0.5f, 20, 20);
    //ProceduralGeometry::MeshData cylinder = proceGeo.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

    ////计算四个几何体的顶点和索引分别在总数组中的地址偏移
    //UINT boxVertexOffset = 0;
    //UINT gridVertexOffset = (UINT)box.Vertices.size();
    //UINT sphereVertexOffset = (UINT)grid.Vertices.size() + gridVertexOffset;
    //UINT cylinderVertexOffset = (UINT)sphere.Vertices.size() + sphereVertexOffset;
    ////索引
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

    ////创建总顶点缓存，所有数据存入
    //size_t totalVertexCount = box.Vertices.size() + grid.Vertices.size() + sphere.Vertices.size() + cylinder.Vertices.size();
    ////顶点数组大小
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

    ////创建总索引缓存
    //std::vector<std::uint16_t> indices;
    //indices.insert(indices.end(), box.GetIndices16().begin(), box.GetIndices16().end());
    //indices.insert(indices.end(), grid.GetIndices16().begin(), grid.GetIndices16().end());
    //indices.insert(indices.end(), sphere.GetIndices16().begin(), sphere.GetIndices16().end());
    //indices.insert(indices.end(), cylinder.GetIndices16().begin(), cylinder.GetIndices16().end());

    ////计算出各自大小，传递给全局变量
    //const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
    //const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    //auto geo = std::make_unique<MeshGeometry>();
    //geo->name = "ShapeGeo";

    //geo->vertexBufferByteSize = vbByteSize;
    //geo->indexBufferByteSize = ibByteSize;

    //ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->vertexBufferCpu));	//创建顶点数据内存空间
    //ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->indexBufferCpu));	//创建索引数据内存空间
    //CopyMemory(geo->vertexBufferCpu->GetBufferPointer(), vertices.data(), vbByteSize);	//将顶点数据拷贝至顶点系统内存中
    //CopyMemory(geo->indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);	//将索引数据拷贝至索引系统内存中
    //geo->vertexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize, geo->vertexBufferUploader);
    //geo->indexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize, geo->indexBufferUploader);

    //geo->vertexByteStride = sizeof(Vertex);
    //geo->indexFormat = DXGI_FORMAT_R16_UINT;

    ////使用无序映射表
    //
    //geo->DrawArgs["box"] = boxSubmesh;
    //geo->DrawArgs["grid"] = gridSubmesh;
    //geo->DrawArgs["sphere"] = sphereSubmesh;
    //geo->DrawArgs["cylinder"] = cylinderSubmesh;

    //geometries[geo->name] = std::move(geo);
#pragma endregion
}

void D3D12InitApp::BuildBoxGeometry()
{
    ProceduralGeometry proceGeo;
    ProceduralGeometry::MeshData box = proceGeo.CreateBox(8.0f, 8.0f, 8.0f, 3);

    //将顶点数据传入Vertex结构体的元素数组
    size_t verticesCount = box.Vertices.size();
    std::vector<Vertex> vertices(verticesCount);
    for (size_t i = 0; i < verticesCount; i++)
    {
        vertices[i].Pos = box.Vertices[i].Position;
        vertices[i].Normal = box.Vertices[i].Normal;
        vertices[i].TexC = box.Vertices[i].TexC;
    }

    std::vector<std::uint16_t> indices = box.GetIndices16();

    //顶点列表大小
    const UINT vbByteSize = (UINT)verticesCount * sizeof(Vertex);
    //索引列表大小
    const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    //绘制三参数
    SubmeshGeometry submesh;
    submesh.baseVertexLocation = 0;
    submesh.startIndexLocation = 0;
    submesh.indexCount = (UINT)indices.size();

    //赋值mg中的数据元素
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
    //要根据不同的渲染类型创建不同的PSO，分别是不透明（不需要混合），alpha测试的（不需要混合），透明（需要混合）的
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

    D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestPsoDesc = opaquePsoDesc;//使用不透明的PSO
    alphaTestPsoDesc.PS =
    {
        reinterpret_cast<BYTE*>(shaders["alphaTestPS"]->GetBufferPointer()),
        shaders["alphaTestPS"]->GetBufferSize()
    };
    alphaTestPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//双面显示
    ThrowIfFailed(d3dDevice->CreateGraphicsPipelineState(&alphaTestPsoDesc, IID_PPV_ARGS(&PSOs["alphaTest"])));

    //透明物体的PSO（需要混合）
    D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;//使用不透明的PSO
    D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;           
    transparencyBlendDesc.BlendEnable = true;                               //是否开启常规混合（默认值为false）
    transparencyBlendDesc.LogicOpEnable = false;                            //是否开启逻辑混合(默认值为false)
    transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;                 //RGB混合中的源混合因子Fsrc（这里取源颜色的alpha通道值）
    transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;            //RGB混合中的目标混合因子Fdest（这里取1-alpha）
    transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;                     //RGB混合运算符(这里选择加法)
    transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;	                //alpha混合中的源混合因子Fsrc（取1）
    transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;                //alpha混合中的目标混合因子Fsrc（取0）
    transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;                //alpha混合运算符(这里选择加法)
    transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;	                //逻辑混合运算符(空操作，即不使用)
    transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;//后台缓冲区写入遮罩（没有遮罩，即全部写入）

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

void D3D12InitApp::BuildMaterials()
{
#pragma region HillsAndLake
    //用无序映射表封装
    //定义陆地的材质
    auto grass = std::make_unique<Material>();
    grass->name = "grass";
    grass->matCBIndex = 0;
    grass->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);    //陆地反照率（颜色）
    grass->fresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);           //陆地的R0
    grass->roughness = 0.125f;                                  //陆地的粗糙度（归一化后的）
    grass->diffuseSrvHeapIndex = 0;

    //定义湖水的材质
    auto water = std::make_unique<Material>();
    water->name = "water";
    water->matCBIndex = 1;
    water->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);    //湖水的反射率
    water->fresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    water->roughness = 0.0f;
    water->diffuseSrvHeapIndex = 1;

    materials["grass"] = std::move(grass);
    materials["water"] = std::move(water);
#pragma endregion

#pragma region box
    auto wood = std::make_unique<Material>();
    wood->name = "wood";
    wood->matCBIndex = 2;
    wood->diffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    wood->fresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
    wood->roughness = 0.8f;
    wood->diffuseSrvHeapIndex = 2;

    materials["wood"] = std::move(wood);

#pragma endregion

#pragma region Shapes
   /* auto bricks0 = std::make_unique<Material>();
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
    materials["skullMat"] = std::move(skullMat);*/
#pragma endregion
}

void D3D12InitApp::BuildRenderItem()
{
#pragma region Hill
    auto landRitem = std::make_unique<RenderItem>();
    landRitem->world = MathHelper::Identity4x4();
    landRitem->objCBIndex = 0;
    landRitem->mat = materials["grass"].get();
    landRitem->geo = geometries["landGeo"].get();//赋值给当前的MeshGeometry
    landRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    landRitem->indexCount = landRitem->geo->DrawArgs["landGrid"].indexCount;
    landRitem->baseVertexLocation = landRitem->geo->DrawArgs["landGrid"].baseVertexLocation;
    landRitem->startIndexLocation = landRitem->geo->DrawArgs["landGrid"].startIndexLocation;
    XMStoreFloat4x4(&landRitem->texTransform, XMMatrixScaling(7.0f, 7.f, 1.0f));
   
    ritemLayer[(int)RenderLayer::Opaque].push_back(landRitem.get());

    allRitems.push_back(std::move(landRitem));
    //地面是不透明的

#pragma endregion

#pragma region Lake
    //构建湖泊
    auto lakeRitem = std::make_unique<RenderItem>();
    lakeRitem->world = MathHelper::Identity4x4();
    lakeRitem->objCBIndex = 1;//湖泊的常量数据（world矩阵）在物体常量缓冲区的索引1上
    lakeRitem->mat = materials["water"].get();
    lakeRitem->geo = geometries["lakeGeo"].get();
    lakeRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    lakeRitem->indexCount = lakeRitem->geo->DrawArgs["lakeGrid"].indexCount;
    lakeRitem->baseVertexLocation = lakeRitem->geo->DrawArgs["lakeGrid"].baseVertexLocation;
    lakeRitem->startIndexLocation = lakeRitem->geo->DrawArgs["lakeGrid"].startIndexLocation;
    XMStoreFloat4x4(&lakeRitem->texTransform, XMMatrixScaling(7.0f, 7.f, 1.0f));

    wavesRitem = lakeRitem.get();
    //水面是透明的
    ritemLayer[(int)RenderLayer::Transparent].push_back(lakeRitem.get());
    allRitems.push_back(std::move(lakeRitem));//被push_back后智能指针自动释放内存

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
    //箱子的纹理是需要透明度测试的    
    ritemLayer[(int)RenderLayer::AlphaTest].push_back(boxRitem.get());
    allRitems.push_back(std::move(boxRitem));

#pragma endregion

#pragma region Shapes
//    auto boxRItem = std::make_unique<RenderItem>();
//    XMStoreFloat4x4(&(boxRItem->world), XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
//    boxRItem->objCBIndex = 0;//常量数据，在0下标上
//    boxRItem->mat = materials["stone0"].get();
//    boxRItem->geo = geometries["ShapeGeo"].get();
//    boxRItem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//    boxRItem->indexCount = boxRItem->geo->DrawArgs["box"].indexCount;
//    boxRItem->baseVertexLocation = boxRItem->geo->DrawArgs["box"].baseVertexLocation;
//    boxRItem->startIndexLocation = boxRItem->geo->DrawArgs["box"].startIndexLocation;
//    allRitems.push_back(std::move(boxRItem));
//
//    auto gridRitem = std::make_unique<RenderItem>();
//    gridRitem->world = MathHelper::Identity4x4();
//    gridRitem->objCBIndex = 1;//BOX常量数据（world矩阵）在objConstantBuffer索引1上
//    gridRitem->mat = materials["tile0"].get();
//    gridRitem->geo = geometries["ShapeGeo"].get();
//    gridRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//    gridRitem->indexCount = gridRitem->geo->DrawArgs["grid"].indexCount;
//    gridRitem->baseVertexLocation = gridRitem->geo->DrawArgs["grid"].baseVertexLocation;
//    gridRitem->startIndexLocation = gridRitem->geo->DrawArgs["grid"].startIndexLocation;
//    allRitems.push_back(std::move(gridRitem));
//
//    UINT fllowObjCBIndex = 2;//接下去的几何体常量数据在CB中的索引从2开始
//    //将圆柱和圆的实例模型存入渲染项中
//    for (int i = 0; i < 5; i++)
//    {
//        auto leftCylinderRitem = std::make_unique<RenderItem>();
//        auto rightCylinderRitem = std::make_unique<RenderItem>();
//        auto leftSphereRitem = std::make_unique<RenderItem>();
//        auto rightSphereRitem = std::make_unique<RenderItem>();
//
//        XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
//        XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);
//        XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
//        XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);
//        //左边5个圆柱
//        XMStoreFloat4x4(&(leftCylinderRitem->world), leftCylWorld);
//        //此处的索引随着循环不断加1（注意：这里是先赋值再++）
//        leftCylinderRitem->objCBIndex = fllowObjCBIndex++;
//        leftCylinderRitem->mat = materials["bricks0"].get();
//        leftCylinderRitem->geo = geometries["ShapeGeo"].get();
//        leftCylinderRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//        leftCylinderRitem->indexCount = leftCylinderRitem->geo->DrawArgs["cylinder"].indexCount;
//        leftCylinderRitem->baseVertexLocation = leftCylinderRitem->geo->DrawArgs["cylinder"].baseVertexLocation;
//        leftCylinderRitem->startIndexLocation = leftCylinderRitem->geo->DrawArgs["cylinder"].startIndexLocation;
//        //右边5个圆柱
//        XMStoreFloat4x4(&(rightCylinderRitem->world), rightCylWorld);
//        rightCylinderRitem->objCBIndex = fllowObjCBIndex++;
//        rightCylinderRitem->mat = materials["bricks0"].get();
//        rightCylinderRitem->geo = geometries["ShapeGeo"].get();
//        rightCylinderRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//        rightCylinderRitem->indexCount = rightCylinderRitem->geo->DrawArgs["cylinder"].indexCount;
//        rightCylinderRitem->baseVertexLocation = rightCylinderRitem->geo->DrawArgs["cylinder"].baseVertexLocation;
//        rightCylinderRitem->startIndexLocation = rightCylinderRitem->geo->DrawArgs["cylinder"].startIndexLocation;
//        //左边5个球
//        XMStoreFloat4x4(&(leftSphereRitem->world), leftSphereWorld);
//        leftSphereRitem->objCBIndex = fllowObjCBIndex++;
//        leftSphereRitem->mat = materials["stone0"].get();
//        leftSphereRitem->geo = geometries["ShapeGeo"].get();
//        leftSphereRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//        leftSphereRitem->indexCount = leftSphereRitem->geo->DrawArgs["sphere"].indexCount;
//        leftSphereRitem->baseVertexLocation = leftSphereRitem->geo->DrawArgs["sphere"].baseVertexLocation;
//        leftSphereRitem->startIndexLocation = leftSphereRitem->geo->DrawArgs["sphere"].startIndexLocation;
//        //右边5个球
//        XMStoreFloat4x4(&(rightSphereRitem->world), rightSphereWorld);
//        rightSphereRitem->objCBIndex = fllowObjCBIndex++;
//        rightSphereRitem->mat = materials["stone0"].get();
//        rightSphereRitem->geo = geometries["ShapeGeo"].get();
//        rightSphereRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//        rightSphereRitem->indexCount = rightSphereRitem->geo->DrawArgs["sphere"].indexCount;
//        rightSphereRitem->baseVertexLocation = rightSphereRitem->geo->DrawArgs["sphere"].baseVertexLocation;
//        rightSphereRitem->startIndexLocation = rightSphereRitem->geo->DrawArgs["sphere"].startIndexLocation;
//
//        allRitems.push_back(std::move(leftCylinderRitem));
//        allRitems.push_back(std::move(rightCylinderRitem));
//        allRitems.push_back(std::move(leftSphereRitem));
//        allRitems.push_back(std::move(rightSphereRitem));
//    }
#pragma endregion
//    //读取骷髅
//    BuildSkullRenderItem();
}

void D3D12InitApp::DrawRenderItems(const std::vector<RenderItem*>& ritems)
{
    UINT objectCount = (UINT)allRitems.size();//物体总个数（包括实例）
    UINT objConstSize = ToolFunc::CalcConstantBufferByteSize(sizeof(ObjectConstants));
    UINT matConstantSize = ToolFunc::CalcConstantBufferByteSize(sizeof(MatConstants));

    auto objCB = currFrameResource->objCB->Resource();
    auto matCB = currFrameResource->matCB->Resource();

    //遍历渲染项数组
    for (size_t i = 0; i < ritems.size(); i++)
    {
        auto ritem = ritems[i];

        //一定要消除特化
        cmdList->IASetVertexBuffers(0, 1, &ritem->geo->GetVbv());
        cmdList->IASetIndexBuffer(&ritem->geo->GetIbv());
        cmdList->IASetPrimitiveTopology(ritem->primitiveType);

        //设置根描述符表，将纹理资源与流水线绑定
        CD3DX12_GPU_DESCRIPTOR_HANDLE texHandle(srvHeap->GetGPUDescriptorHandleForHeapStart());
        texHandle.Offset(ritem->mat->diffuseSrvHeapIndex, cbv_srv_uavDescriptorSize);
        
        ////设置根描述符表
        //UINT objCbvIndex = ritem->objCBIndex + currFrameResourceIndex * (UINT)allRitems.size();
        //auto handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(cbvHeap->GetGPUDescriptorHandleForHeapStart());
        //handle.Offset(objCbvIndex, cbv_srv_uavDescriptorSize);
        //cmdList->SetGraphicsRootDescriptorTable(0, handle);
        
        //设置根描述符
        auto objCBAddress = objCB->GetGPUVirtualAddress();
        objCBAddress += ritem->objCBIndex * objConstSize;
        
        //设置根描述符，将根描述符与matCB资源绑定
        auto matCBAddress = matCB->GetGPUVirtualAddress();
        matCBAddress += ritem->mat->matCBIndex * matConstantSize;
        
        cmdList->SetGraphicsRootDescriptorTable(0, texHandle);
        cmdList->SetGraphicsRootConstantBufferView(1,
            objCBAddress);
        cmdList->SetGraphicsRootConstantBufferView(2,
            matCBAddress);
        

        //绘制顶点
        cmdList->DrawIndexedInstanced(ritem->indexCount,    //每个实例绘制的索引数量
            1,                                              //实例化个数
            ritem->startIndexLocation,                      //起始索引位置
            ritem->baseVertexLocation,                      //子物体起始索引在全局索引中的位置
            0);                                             //实例化的高级技术
    }
}

void D3D12InitApp::UpdateObjCBs()
{
    ObjectConstants objConstants;
    //构建世界矩阵
    for (auto& e : allRitems)
    {
        if (e->NumFramesDirty > 0)
        {
            mWorld = e->world;
            XMMATRIX w = XMLoadFloat4x4(&mWorld);
            //转赋值
            XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(w));
            XMFLOAT4X4 texTransform = e->texTransform;
            XMMATRIX texTransMatrix = XMLoadFloat4x4(&texTransform);
            XMStoreFloat4x4(&objConstants.texTransform, XMMatrixTranspose(texTransMatrix));
            //将数据拷贝至GPU缓存
            currFrameResource->objCB->CopyData(e->objCBIndex, objConstants);

            e->NumFramesDirty--;
        }
    }

    
}

void D3D12InitApp::UpdatePassCBs(const GameTime& gt)
{
    // 构建投影矩阵
    XMMATRIX proj = XMLoadFloat4x4(&mProj);

    PassConstants passConstants;

    ////构建观察矩阵
    float x = mRadius * sinf(mPhi) * cosf(mTheta);
    float z = mRadius * sinf(mPhi) * sinf(mTheta);
    float y = mRadius * cosf(mPhi);

    //构建观察矩阵
    passConstants.eyePosW = XMFLOAT3(x, y, z);
    XMVECTOR pos = XMVectorSet(x, y, z, 10.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
    XMStoreFloat4x4(&mView, view);

    //构建变换矩阵，SRT矩阵
    XMMATRIX VP_Matrix = view * proj;
    XMStoreFloat4x4(&passConstants.viewProj, XMMatrixTranspose(VP_Matrix));

    passConstants.ambientLight = { 0.25f,0.25f,0.35f,1.0f };
    passConstants.lights[0].strength = { 1.0f,1.0f,0.9f };
    passConstants.totalTime = gt.TotalTime();

    //球坐标转换成笛卡尔坐标
    XMVECTOR sunDir = -MathHelper::SphericalToCartesian(1.0f, sunTheta, sunPhi);
    XMStoreFloat3(&passConstants.lights[0].direction, sunDir);

    currFrameResource->passCB->CopyData(0, passConstants);
}

void D3D12InitApp::BuildFrameResources()
{
    for (int i = 0; i < frameResourcesCount; i++)
    {
        FrameResourcesArray.push_back(std::make_unique<FrameResource>(
            d3dDevice.Get(),
            1,
            (UINT)allRitems.size(),     //子物体缓冲数量
            (UINT)materials.size(),     //材质数量
            waves->VertexCount()        //顶点缓冲数量
            ));
    }
}

void D3D12InitApp::UpdateWaves(const GameTime& gt)
{
    static float t_base = 0.0f;
    if ((gameTime.TotalTime() - t_base) >= 0.25f)
    {
        t_base += 0.25f;//0.25秒生成一个波浪
        //随机生成横坐标
        int i = MathHelper::Rand(4, waves->RowCount() - 5);
        //随机生成纵坐标
        int j = MathHelper::Rand(4, waves->ColumnCount() - 5);
        //随机生成波的半径
        float r = MathHelper::RandF(0.2f, 0.5f);
        //使用波动方程生成波纹
        waves->Disturb(i, j, r);
    }

    //每帧更新波浪模拟（即更新顶点坐标）
    waves->Update(gt.DeltaTime());

    //将更新的顶点坐标存入GPU上传堆中
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
    //赋值湖泊GPU的顶点缓存
    wavesRitem->geo->vertexBufferGpu = currWavesVB->Resource();
}

void D3D12InitApp::BuildLakeIndexBuffer()
{
    //初始化索引列表
    std::vector<std::uint16_t> indices(3 * waves->TriangleCount());
    assert(waves->VertexCount() < 0x0000ffff);//顶点索引数大于65536就终止程序

    //填充索引列表
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

    //计算顶点和索引缓存大小
    UINT vbByteSize = waves->VertexCount() * sizeof(Vertex);
    UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->name = "lakeGeo";

    geo->vertexBufferCpu = nullptr;
    geo->vertexBufferGpu = nullptr;

    //创建索引的CPU内存
    ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->indexBufferCpu));
    //将索引表放入内存
    CopyMemory(geo->indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);
    //索引数据上传至默认堆
    geo->indexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(),
        cmdList.Get(),
        indices.data(),
        ibByteSize,
        geo->indexBufferUploader);

    //赋值MeshGeometry相关属性
    geo->vertexBufferByteSize = vbByteSize;
    geo->vertexByteStride = sizeof(Vertex);
    geo->indexFormat = DXGI_FORMAT_R16_UINT;
    geo->indexBufferByteSize = ibByteSize;

    SubmeshGeometry LakeSubmesh;
    LakeSubmesh.indexCount = (UINT)indices.size();
    LakeSubmesh.baseVertexLocation = 0;
    LakeSubmesh.startIndexLocation = 0;
    //使用grid几何体
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
    //在Update函数中执行
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
            //将材质常量数据复制到常量缓冲区对应索引地址处
            currFrameResource->matCB->CopyData(mat->matCBIndex, matConstants);
            //更新下一个帧资源
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
        std::ifstream fin("Models/skull.txt");//读取骷髅文件

        if (!fin)
        {
            MessageBox(0, L"文件读取失败", 0, 0);
            return;
        }

        UINT vertexCount = 0;
        UINT triangleCount = 0;

        std::string ignore;

        //分别忽略第一个空格前的字符串，读取后半段的数字，是顶点数和三角形数
        fin >> ignore >> vertexCount;
        fin >> ignore >> triangleCount;
        //整行不读
        fin >> ignore >> ignore >> ignore >> ignore;

        //初始化顶点列表
        std::vector<Vertex> vertices(vertexCount);//初始化顶点列表
        //顶点列表赋值
        for (UINT i = 0; i < vertexCount; i++)
        {
            //读取顶点坐标
            fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
            //normal数据忽略
            fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
            //vertices[i].Color = XMFLOAT4(DirectX::Colors::Black);
        }

        fin >> ignore;
        fin >> ignore;
        fin >> ignore;

        //初始化索引列表
        std::vector<std::uint32_t> indices(triangleCount * 3);
        //索引列表赋值
        for (UINT i = 0; i < triangleCount; i++)
        {
            fin >> indices[i * 3 + 0] >> indices[i * 3 + 1] >> indices[i * 3 + 2];
        }

        //关闭输入流
        fin.close();

        const UINT vbByteSize = vertices.size() * sizeof(Vertex);
        const UINT ibByteSize = indices.size() * sizeof(std::int32_t);

        auto geo = std::make_unique<MeshGeometry>();
        geo->name = "skullGeo";

        //顶点和索引数据放入CPU和GPU


        ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->vertexBufferCpu));	//创建顶点数据内存空间
        ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->indexBufferCpu));	//创建索引数据内存空间
        CopyMemory(geo->vertexBufferCpu->GetBufferPointer(), vertices.data(), vbByteSize);	//将顶点数据拷贝至顶点系统内存中
        CopyMemory(geo->indexBufferCpu->GetBufferPointer(), indices.data(), ibByteSize);	//将索引数据拷贝至索引系统内存中
        geo->vertexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), vertices.data(), vbByteSize, geo->vertexBufferUploader);
        geo->indexBufferGpu = ToolFunc::CreateDefaultBuffer(d3dDevice.Get(), cmdList.Get(), indices.data(), ibByteSize, geo->indexBufferUploader);

        geo->vertexBufferByteSize = vbByteSize;
        geo->indexBufferByteSize = ibByteSize;
        geo->vertexByteStride = sizeof(Vertex);
        geo->indexFormat = DXGI_FORMAT_R32_UINT;

        //绘制三参数
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
    skullRitem->objCBIndex = 22;
    skullRitem->primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    skullRitem->mat = materials["skullMat"].get();
    skullRitem->geo = geometries["skullGeo"].get();
    skullRitem->indexCount = skullRitem->geo->DrawArgs["skull"].indexCount;
    skullRitem->baseVertexLocation = skullRitem->geo->DrawArgs["skull"].baseVertexLocation;
    skullRitem->startIndexLocation = skullRitem->geo->DrawArgs["skull"].startIndexLocation;
    allRitems.push_back(std::move(skullRitem));
}

void D3D12InitApp::LoadTextures()
{
    //板条箱纹理
    auto woodCrateTex = std::make_unique<Texture>();
    woodCrateTex->name = "woodCrateTex";
    woodCrateTex->fileName = L"Textures/WireFence.dds";
    //读取dds文件
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(),
        woodCrateTex->fileName.c_str(),
        woodCrateTex->resource, woodCrateTex->uploadHeap));

    //草地纹理
    auto grassTex = std::make_unique<Texture>();
    grassTex->name = "grassTex";
    grassTex->fileName = L"Textures/grass.dds";
    //读取dds文件
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(),
        grassTex->fileName.c_str(),
        grassTex->resource, grassTex->uploadHeap));

    //湖水纹理
    auto lakeTex = std::make_unique<Texture>();
    lakeTex->name = "lakeTex";
    lakeTex->fileName = L"Textures/water1.dds";
    //读取dds文件
    ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(d3dDevice.Get(), cmdList.Get(),
        lakeTex->fileName.c_str(),
        lakeTex->resource, lakeTex->uploadHeap));

    //加入总纹理列表
    textures["woodCrateTex"] = std::move(woodCrateTex);
    textures["grassTex"] = std::move(grassTex);
    textures["lakeTex"] = std::move(lakeTex);
}

std::array<CD3DX12_STATIC_SAMPLER_DESC, 6> D3D12InitApp::GetStaticSamplers()
{
    //过滤器POINT,寻址模式WRAP的静态采样器
    CD3DX12_STATIC_SAMPLER_DESC pointWarp(0,	//着色器寄存器
        D3D12_FILTER_MIN_MAG_MIP_POINT,		//过滤器类型为POINT(常量插值)
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//U方向上的寻址模式为WRAP（重复寻址模式）
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//V方向上的寻址模式为WRAP（重复寻址模式）
        D3D12_TEXTURE_ADDRESS_MODE_WRAP);	//W方向上的寻址模式为WRAP（重复寻址模式）

    //过滤器POINT,寻址模式CLAMP的静态采样器
    CD3DX12_STATIC_SAMPLER_DESC pointClamp(1,	//着色器寄存器
        D3D12_FILTER_MIN_MAG_MIP_POINT,		//过滤器类型为POINT(常量插值)
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//U方向上的寻址模式为CLAMP（钳位寻址模式）
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//V方向上的寻址模式为CLAMP（钳位寻址模式）
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);	//W方向上的寻址模式为CLAMP（钳位寻址模式）

    //过滤器LINEAR,寻址模式WRAP的静态采样器
    CD3DX12_STATIC_SAMPLER_DESC linearWarp(2,	//着色器寄存器
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,		//过滤器类型为LINEAR(线性插值)
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//U方向上的寻址模式为WRAP（重复寻址模式）
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//V方向上的寻址模式为WRAP（重复寻址模式）
        D3D12_TEXTURE_ADDRESS_MODE_WRAP);	//W方向上的寻址模式为WRAP（重复寻址模式）

    //过滤器LINEAR,寻址模式CLAMP的静态采样器
    CD3DX12_STATIC_SAMPLER_DESC linearClamp(3,	//着色器寄存器
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,		//过滤器类型为LINEAR(线性插值)
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//U方向上的寻址模式为CLAMP（钳位寻址模式）
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//V方向上的寻址模式为CLAMP（钳位寻址模式）
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);	//W方向上的寻址模式为CLAMP（钳位寻址模式）

    //过滤器ANISOTROPIC,寻址模式WRAP的静态采样器
    CD3DX12_STATIC_SAMPLER_DESC anisotropicWarp(4,	//着色器寄存器
        D3D12_FILTER_ANISOTROPIC,			//过滤器类型为ANISOTROPIC(各向异性)
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//U方向上的寻址模式为WRAP（重复寻址模式）
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,	//V方向上的寻址模式为WRAP（重复寻址模式）
        D3D12_TEXTURE_ADDRESS_MODE_WRAP);	//W方向上的寻址模式为WRAP（重复寻址模式）

    //过滤器LINEAR,寻址模式CLAMP的静态采样器
    CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(5,	//着色器寄存器
        D3D12_FILTER_ANISOTROPIC,			//过滤器类型为ANISOTROPIC(各向异性)
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//U方向上的寻址模式为CLAMP（钳位寻址模式）
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	//V方向上的寻址模式为CLAMP（钳位寻址模式）
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);	//W方向上的寻址模式为CLAMP（钳位寻址模式）

    return{ pointWarp, pointClamp, linearWarp, linearClamp, anisotropicWarp, anisotropicClamp };
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
    //将两个变化量存入矩阵
    matLake->matTransform(3, 0) = du;
    matLake->matTransform(3, 1) = dv;

    matLake->numFrameDirty = 3;
}
