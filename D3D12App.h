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

    //2�����豸
    void CreateDevice();

    //3����Χ��
    void CreateFence();

    //4��ȡ��������С
    void GetDescriptorSize();

    //5����MSAA���������
    void SetMSAA();

    //6����������С������б����������
    void CreateCommandObjects();

    //7����������
    void CreateSwapChain();

    //8������������
    virtual void CreateRtvAndDsvDescriptorHeaps();

    //9����������
    void CreateRTV();
    void CreateDSV();

    //11�����ӿںͲü�����
    void CreateViewPortAndScissorRect();

    //12ʵ��Χ��
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

    //GameTime��ʵ������
    GameTime mTimer;

    //2�����豸
    //����ָ�������
    //���ȴ�������ָ�루Factory�ӿڣ�
    Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
    //Ȼ�󴴽��豸ָ��
    Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

    //3����Χ��
    //����Χ��ָ��
    Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
    UINT64 mCurrentFence = 0;

    //4��ȡ��������С
    //Ҫ��ȡ������������С���ֱ���
    //RTV����ȾĿ�껺������������
    //DSV�����ģ�建������������
    //CBV_SRV_UAV����������������������ɫ����Դ������������������ʻ�����������
    UINT mRtvDescriptorSize = 0;
    UINT mDsvDescriptorSize = 0;
    UINT mCbvSrvUavDescriptorSize = 0;

    //5����MSAA���������
     //����MSAA����ݵȼ���Ĭ�ϲ�ʹ��MSAA
    bool      m4xMsaaState = false;
    UINT      m4xMsaaQuality = 0;

    //6����������С������б����������
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

    //7����������
   //����������ָ��
    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;

    static const int SwapChainBufferCount = 2;
    //��̨����������
    int mCurrBackBuffer = 0;

    //9����������
    //����������������ָ�루���ڻ�ȡ�������еĺ�̨��������Դ��
    Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
    //������Ȼ�����ָ��
    Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

    //8������������
    //����rtv��ָ��
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    //����dsv��ָ��
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

    //11�����ӿںͲü�����
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

