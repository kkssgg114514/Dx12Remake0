#include "D3D12Init.h"
#include "tiff.h"
#include "GameTime.h"

//创建D3D12Init对象
D3D12Init* p = new D3D12Init();
GameTime* gt = new GameTime();

void CalculateFrameState()
{
	static int frameCnt = 0;			//总帧数
	static float timeElapsed = 0.0f;	//经过的时间
	frameCnt++;							//每帧都加一

	//判断每秒刷新一次
	if (gt->TotalTime() - timeElapsed >= 1.0f)
	{
		float fps = (float)frameCnt;	//每秒帧数
		float mspf = 1000.0f / fps;		//每帧时长

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);
		//将帧数据显示在窗口上
		std::wstring windowText = L"D3D12Init	fps: " + fpsStr + L"	mspf: " + mspfStr;
		SetWindowText(mhMainWnd, windowText.c_str());

		//为计算下一组重置
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool InitWindow(HINSTANCE hInstance, int nShowCmd)
{
	/*--------------------------------------------------------------------------------------------------------*/
	//填写窗口类

	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;			//#当工作区宽高改变，则重新绘制窗口
	wc.lpfnWndProc = MainWndProc;				//#指定窗口过程//指定处理消息的函数指针
	wc.cbClsExtra = 0;							//#借助这两个字段来为当前应用分配额外的内存空间（这里不分配，所以置0）
	wc.cbWndExtra = 0;							//#借助这两个字段来为当前应用分配额外的内存空间（这里不分配，所以置0）
	wc.hInstance = hInstance;					//#应用程序实例句柄（由WinMain传入）
	wc.hIcon = LoadIcon(0, IDC_ARROW);			//#使用默认的应用程序图标
	wc.hCursor = LoadCursor(0, IDC_ARROW);		//#使用标准的鼠标指针样式
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);		//#指定了白色背景画刷句柄
	wc.lpszMenuName = 0;						//#没有菜单栏
	wc.lpszClassName = L"MainWnd";				//#窗口名

	//#窗口类注册失败
	//注册窗口类
	if (!RegisterClass(&wc))
	{
		//#消息框函数，参数1：消息框所属窗口句柄，可为NULL。参数2：消息框显示的文本信息。参数3：标题文本。参数4：消息框样式
		MessageBox(0, L"RegisterClass Failed", 0, 0);
		return false;
	}

	//#窗口类注册成功
	RECT R;	//#裁剪矩形
	R.left = 0;
	R.top = 0;
	R.right = 1280;
	R.bottom = 720;
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);	//#根据窗口的客户区大小计算窗口的大小
	int width = R.right - R.left;
	int hight = R.bottom - R.top;

	//#创建窗口,返回布尔值
	mhMainWnd = CreateWindow(L"MainWnd", L"DX12Initialize", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, hight, 0, 0, hInstance, 0);
	//#窗口创建失败
	if (!mhMainWnd)
	{
		MessageBox(0, L"CreatWindow Failed", 0, 0);
		return false;
	}
	//#窗口创建成功,则显示并更新窗口
	ShowWindow(mhMainWnd, nShowCmd);
	UpdateWindow(mhMainWnd);

	return true;
	/*--------------------------------------------------------------------------------------------------------*/
}

int Run()
{
	//消息循环中检测消息

	//#消息循环
	//#定义消息结构体
	MSG msg = { 0 };
	gt->Reset();
	//#如果GetMessage函数不等于0，说明没有接受到WM_QUIT
	while (msg.message != WM_QUIT)
	{
		//对消息进行拾取
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);			//消息转码
			DispatchMessage(&msg);			//消息分发
		}
		else
		{
			gt->Tick();//计算每两帧间隔
			if (!gt->IsStoped())
			{
				//处于运行状态才运行游戏
				CalculateFrameState();
				p->Draw();
			}
			else
			{
				//处于暂停状态，休眠100ms
				Sleep(100);
			}
		}
	}
	return (int)msg.wParam;
	/*--------------------------------------------------------------------------------------------------------*/
}

bool Init(HINSTANCE hInstance, int nShowCmd)
{
	if (!InitWindow(hInstance, nShowCmd))
	{
		return false;
	}
	else if (!p->InitDirect3D())
	{
		return false;
	}
	else
	{
		return true;
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nShowCmd)
{
	//创建调试层
#if defined(DEBUG)|defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	try
	{
		if (!Init(hInstance, nShowCmd))
		{
			return 0;
		}
		return Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
	
}

//#窗口过程函数
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	/*--------------------------------------------------------------------------------------------------------*/
	//#消息处理
	switch (msg)
	{
		//#当窗口被销毁时，终止消息循环
	case WM_DESTROY:
		PostQuitMessage(0);	//#终止消息循环，并发出WM_QUIT消息
		return 0;
	default:
		break;
	}
	//#将上面没有处理的消息转发给默认的窗口过程
	return DefWindowProc(hwnd, msg, wParam, lParam);
	/*--------------------------------------------------------------------------------------------------------*/
}


