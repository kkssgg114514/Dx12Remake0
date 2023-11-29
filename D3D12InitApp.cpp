#include "D3D12InitApp.h"

D3D12InitApp::D3D12InitApp()
	:D3D12App()
{
	vertexBufferGpu = nullptr; 
	indexBufferGpu = nullptr;
	indexBufferCpu = nullptr;
}

D3D12InitApp::~D3D12InitApp()
{
}

void D3D12InitApp::Draw()
{
	//调用父类方法
	__super::Draw();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nShowCmd)
{
	//创建调试层
#if defined(DEBUG)|defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	try
	{
		D3D12InitApp theApp;
		if (!theApp.Init(hInstance, nShowCmd))
		{
			return 0;
		}
		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}

}