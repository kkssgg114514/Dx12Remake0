#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "ToolFunc.h"
#include "GameTime.h"

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

class D3D12App
{
protected:

    D3D12App(HINSTANCE hInstance);
    D3D12App(const D3D12App& rhs) = delete;
    D3D12App& operator=(const D3D12App& rhs) = delete;
    virtual ~D3D12App();

public:

    static D3D12App* GetApp();

    HINSTANCE AppInst()const;
    HWND      MainWnd()const;
    float     AspectRatio()const;

    bool Get4xMsaaState()const;
    void Set4xMsaaState(bool value);

    int Run();

    virtual bool Initialize();
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:

    virtual void OnResize();
    virtual void Update(const GameTime& gt) = 0;
    virtual void Draw(const GameTime& gt) = 0;

    // Convenience overrides for handling mouse input.
    virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
    virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
    virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

protected:

    bool InitMainWindow();
    bool InitDirect3D();

    //2创建设备
    void CreateDevice();

    //3创建围栏
    void CreateFence();

    //4获取描述符大小
    void GetDescriptorSize();

    //5设置MSAA抗锯齿属性
    void SetMSAA();

    //6创建命令队列、命令列表、命令分配器
    void CreateCommandObjects();

    //7创建交换链
    void CreateSwapChain();

    //8创建描述符堆
    virtual void CreateRtvAndDsvDescriptorHeaps();

    //9创建描述符
    void CreateRTV();
    void CreateDSV();

    //11设置视口和裁剪矩形
    void CreateViewPortAndScissorRect();

    //12实现围栏
    void FlushCommandQueue();

    ID3D12Resource* CurrentBackBuffer()const;
    D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
    D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

    void CalculateFrameStats();

protected:

    static D3D12App* mApp;

    HINSTANCE mhAppInst = nullptr; // application instance handle
    HWND      mhMainWnd = nullptr; // main window handle
    bool      mAppPaused = false;  // is the application paused?
    bool      mMinimized = false;  // is the application minimized?
    bool      mMaximized = false;  // is the application maximized?
    bool      mResizing = false;   // are the resize bars being dragged?
    bool      mFullscreenState = false;// fullscreen enabled

    //GameTime类实例声明
    GameTime gameTime;

    //2创建设备
    //所有指针的声明
    //首先创建工厂指针（Factory接口）
    Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
    //然后创建设备指针
    Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;

    //3创建围栏
    //创建围栏指针
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    UINT64 currentFence = 0;

    //4获取描述符大小
    //要获取三个描述符大小，分别是
    //RTV（渲染目标缓冲区描述符）
    //DSV（深度模板缓冲区描述符）
    //CBV_SRV_UAV（常量缓冲区描述符、着色器资源缓冲描述符和随机访问缓冲描述符）
    UINT mRtvDescriptorSize = 0;
    UINT mDsvDescriptorSize = 0;
    UINT cbv_srv_uavDescriptorSize = 0;

    //5设置MSAA抗锯齿属性
     //设置MSAA抗锯齿等级，默认不使用MSAA
    bool      m4xMsaaState = false;
    UINT      m4xMsaaQuality = 0;

    //6创建命令队列、命令列表、命令分配器
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> cmdQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> cmdAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList;

    //7创建交换链
   //创建交换链指针
    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;

    static const int SwapChainBufferCount = 2;
    //后台缓冲区索引
    int mCurrBackBuffer = 0;

    //9创建描述符
    //创建交换链缓冲区指针（用于获取交换链中的后台缓冲区资源）
    Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
    //创建深度缓冲区指针
    Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

    //8创建描述符堆
    //创建rtv堆指针
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    //创建dsv堆指针
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

    //11设置视口和裁剪矩形
    D3D12_VIEWPORT mScreenViewport;
    D3D12_RECT mScissorRect;

    // Derived class should set these in derived constructor to customize starting values.
    std::wstring mMainWndCaption = L"d3d App";
    D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    int mClientWidth = 1280;
    int mClientHeight = 720;
};

