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

//#AnsiToWString������ת���ɿ��ַ����͵��ַ�����wstring��
//#��Windowsƽ̨�ϣ�����Ӧ�ö�ʹ��wstring��wchar_t������ʽ�����ַ���ǰ+L
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

//#DxException��
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

//������������Ҫ����������
struct SubmeshGeometry
{
	UINT indexCount;
	UINT startIndexLocation;
	UINT baseVertexLocation;
};

struct MeshGeometry
{
	std::string name;

	Microsoft::WRL::ComPtr<ID3DBlob> vertexBufferCpu = nullptr;		//CPUϵͳ�ڴ��ϵĶ�������
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferUploader = nullptr;	//GPU�ϴ����еĶ��㻺����
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferGpu = nullptr;		//GPUĬ�϶��еĶ��㻺����

	Microsoft::WRL::ComPtr<ID3DBlob> indexBufferCpu = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferGpu = nullptr;

	UINT vertexBufferByteSize = 0;
	UINT vertexByteStride = 0;
	UINT indexBufferByteSize = 0;
	DXGI_FORMAT indexFormat = DXGI_FORMAT_R16_UINT;

	std::unordered_map<std::string, SubmeshGeometry> DrawArgs;	//����ӳ�����Ӧ�������Ʋ���

	D3D12_VERTEX_BUFFER_VIEW GetVbv() const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = vertexBufferGpu->GetGPUVirtualAddress();//���㻺������Դ�����ַ
		vbv.SizeInBytes = vertexBufferByteSize;	//���㻺������С�����ж������ݴ�С��
		vbv.StrideInBytes = vertexByteStride;	//ÿ������Ԫ����ռ�õ��ֽ���

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

	//�ȴ��ϴ�����Դ����Ĭ�϶Ѻ��ͷ��ϴ��ѵ��ڴ�
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