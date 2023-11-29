#include "D3D12Init.h"
#include "tiff.h"
#include "GameTime.h"

//����D3D12Init����
D3D12Init* p = new D3D12Init();
GameTime* gt = new GameTime();

void CalculateFrameState()
{
	static int frameCnt = 0;			//��֡��
	static float timeElapsed = 0.0f;	//������ʱ��
	frameCnt++;							//ÿ֡����һ

	//�ж�ÿ��ˢ��һ��
	if (gt->TotalTime() - timeElapsed >= 1.0f)
	{
		float fps = (float)frameCnt;	//ÿ��֡��
		float mspf = 1000.0f / fps;		//ÿ֡ʱ��

		std::wstring fpsStr = std::to_wstring(fps);
		std::wstring mspfStr = std::to_wstring(mspf);
		//��֡������ʾ�ڴ�����
		std::wstring windowText = L"D3D12Init	fps: " + fpsStr + L"	mspf: " + mspfStr;
		SetWindowText(mhMainWnd, windowText.c_str());

		//Ϊ������һ������
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool InitWindow(HINSTANCE hInstance, int nShowCmd)
{
	/*--------------------------------------------------------------------------------------------------------*/
	//��д������

	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;			//#����������߸ı䣬�����»��ƴ���
	wc.lpfnWndProc = MainWndProc;				//#ָ�����ڹ���//ָ��������Ϣ�ĺ���ָ��
	wc.cbClsExtra = 0;							//#�����������ֶ���Ϊ��ǰӦ�÷��������ڴ�ռ䣨���ﲻ���䣬������0��
	wc.cbWndExtra = 0;							//#�����������ֶ���Ϊ��ǰӦ�÷��������ڴ�ռ䣨���ﲻ���䣬������0��
	wc.hInstance = hInstance;					//#Ӧ�ó���ʵ���������WinMain���룩
	wc.hIcon = LoadIcon(0, IDC_ARROW);			//#ʹ��Ĭ�ϵ�Ӧ�ó���ͼ��
	wc.hCursor = LoadCursor(0, IDC_ARROW);		//#ʹ�ñ�׼�����ָ����ʽ
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);		//#ָ���˰�ɫ������ˢ���
	wc.lpszMenuName = 0;						//#û�в˵���
	wc.lpszClassName = L"MainWnd";				//#������

	//#������ע��ʧ��
	//ע�ᴰ����
	if (!RegisterClass(&wc))
	{
		//#��Ϣ����������1����Ϣ���������ھ������ΪNULL������2����Ϣ����ʾ���ı���Ϣ������3�������ı�������4����Ϣ����ʽ
		MessageBox(0, L"RegisterClass Failed", 0, 0);
		return false;
	}

	//#������ע��ɹ�
	RECT R;	//#�ü�����
	R.left = 0;
	R.top = 0;
	R.right = 1280;
	R.bottom = 720;
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);	//#���ݴ��ڵĿͻ�����С���㴰�ڵĴ�С
	int width = R.right - R.left;
	int hight = R.bottom - R.top;

	//#��������,���ز���ֵ
	mhMainWnd = CreateWindow(L"MainWnd", L"DX12Initialize", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, hight, 0, 0, hInstance, 0);
	//#���ڴ���ʧ��
	if (!mhMainWnd)
	{
		MessageBox(0, L"CreatWindow Failed", 0, 0);
		return false;
	}
	//#���ڴ����ɹ�,����ʾ�����´���
	ShowWindow(mhMainWnd, nShowCmd);
	UpdateWindow(mhMainWnd);

	return true;
	/*--------------------------------------------------------------------------------------------------------*/
}

int Run()
{
	//��Ϣѭ���м����Ϣ

	//#��Ϣѭ��
	//#������Ϣ�ṹ��
	MSG msg = { 0 };
	gt->Reset();
	//#���GetMessage����������0��˵��û�н��ܵ�WM_QUIT
	while (msg.message != WM_QUIT)
	{
		//����Ϣ����ʰȡ
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);			//��Ϣת��
			DispatchMessage(&msg);			//��Ϣ�ַ�
		}
		else
		{
			gt->Tick();//����ÿ��֡���
			if (!gt->IsStoped())
			{
				//��������״̬��������Ϸ
				CalculateFrameState();
				p->Draw();
			}
			else
			{
				//������ͣ״̬������100ms
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
	//�������Բ�
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

//#���ڹ��̺���
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	/*--------------------------------------------------------------------------------------------------------*/
	//#��Ϣ����
	switch (msg)
	{
		//#�����ڱ�����ʱ����ֹ��Ϣѭ��
	case WM_DESTROY:
		PostQuitMessage(0);	//#��ֹ��Ϣѭ����������WM_QUIT��Ϣ
		return 0;
	default:
		break;
	}
	//#������û�д������Ϣת����Ĭ�ϵĴ��ڹ���
	return DefWindowProc(hwnd, msg, wParam, lParam);
	/*--------------------------------------------------------------------------------------------------------*/
}


