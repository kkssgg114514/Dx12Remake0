#pragma once
#include <Windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <array>
#include <unordered_map>
#include <cstdint>
#include <fstream>
#include <sstream>
#include "..\Common\d3dx12.h"
#include "..\Common\MathHelper.h"

//#AnsiToWString函数（转换成宽字符类型的字符串，wstring）
//#在Windows平台上，我们应该都使用wstring和wchar_t，处理方式是在字符串前+L
inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

class ToolFunc
{
public:
	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const void* initData,
		UINT64 byteSize,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);

	static UINT CalcConstantBufferByteSize(UINT byteSize)
	{
		// Constant buffers must be a multiple of the minimum hardware
		// allocation size (usually 256 bytes).  So round up to nearest
		// multiple of 256.  We do this by adding 255 and then masking off
		// the lower 2 bytes which store all bits < 256.
		// Example: Suppose byteSize = 300.
		// (300 + 255) & ~255
		// 555 & ~255
		// 0x022B & ~0x00ff
		// 0x022B & 0xff00
		// 0x0200
		// 512
		return (byteSize + 255) & ~255;
	}
};

//#DxException类
class DxException
{
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

	std::wstring ToString()const;

	HRESULT ErrorCode = S_OK;
	std::wstring FunctionName;
	std::wstring Filename;
	int LineNumber = -1;
};

//绘制子物体需要的三个属性
struct SubmeshGeometry
{
	UINT indexCount;
	UINT startIndexLocation;
	UINT baseVertexLocation;
};

struct MeshGeometry
{
	std::string name;

	Microsoft::WRL::ComPtr<ID3DBlob> vertexBufferCpu = nullptr;		//CPU系统内存上的顶点数据
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferUploader = nullptr;	//GPU上传堆中的顶点缓冲区
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferGpu = nullptr;		//GPU默认堆中的顶点缓冲区

	Microsoft::WRL::ComPtr<ID3DBlob> indexBufferCpu = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferGpu = nullptr;

	UINT vertexBufferByteSize = 0;
	UINT vertexByteStride = 0;
	UINT indexBufferByteSize = 0;
	DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;

	std::unordered_map<std::string, SubmeshGeometry> DrawArgs;	//无序映射表，对应三个绘制参数

	D3D12_VERTEX_BUFFER_VIEW GetVbv() const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = vertexBufferGpu->GetGPUVirtualAddress();//顶点缓冲区资源虚拟地址
		vbv.SizeInBytes = vertexBufferByteSize;	//顶点缓冲区大小（所有顶点数据大小）
		vbv.StrideInBytes = vertexByteStride;	//每个顶点元素所占用的字节数

		return vbv;
	}

	D3D12_INDEX_BUFFER_VIEW GetIbv() const
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = indexBufferGpu->GetGPUVirtualAddress();
		ibv.Format = indexFormat;
		ibv.SizeInBytes = indexBufferByteSize;

		return ibv;
	}

	//等待上传堆资源传至默认堆后，释放上传堆的内存
	void DisposeUploaders()
	{
		vertexBufferUploader = nullptr;
		indexBufferUploader = nullptr;
	}
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif