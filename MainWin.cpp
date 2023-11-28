#include "Common.h"

//#���ڵľ����ShowWindow��UpdateWindow������Ҫ���ô˾��
//ȫ�ַ�Χ�ڴ���
HWND mhMainWnd = 0;

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nShowCmd)
{
	//�������Բ�
#if defined(DEBUG)|defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

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
		return 0;
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
		return 0;
	}
	//#���ڴ����ɹ�,����ʾ�����´���
	ShowWindow(mhMainWnd, nShowCmd);
	UpdateWindow(mhMainWnd);

	/*--------------------------------------------------------------------------------------------------------*/
	//��Ϣѭ���м����Ϣ

	//#��Ϣѭ��
	//#������Ϣ�ṹ��
	MSG msg = { 0 };
	BOOL bRet = 0;
	//#���GetMessage����������0��˵��û�н��ܵ�WM_QUIT
	while (bRet = GetMessage(&msg, 0, 0, 0) != 0)
	{
		//#�������-1��˵��GetMessage���������ˣ����������
		if (bRet == -1)
		{
			MessageBox(0, L"GetMessage Failed", L"Errow", MB_OK);
		}
		//�����������ֵ��˵�����յ�����Ϣ
		else
		{
			TranslateMessage(&msg);	//#���̰���ת�������������Ϣת��Ϊ�ַ���Ϣ
			DispatchMessage(&msg);	//#����Ϣ���ɸ���Ӧ�Ĵ��ڹ���//�ַ����ص�������ע�ᴰ����ʱ��д
		}
	}
	return (int)msg.wParam;
}

//#���ڹ��̺���
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
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
}