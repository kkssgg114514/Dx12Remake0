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
#include <windowsx.h>
#include <comdef.h>


//����ĺ����������ù�����⣬���ɹ̶�����ʹ�ü���
//#AnsiToWString������ת���ɿ��ַ����͵��ַ�����wstring��
//#��Windowsƽ̨�ϣ�����Ӧ�ö�ʹ��wstring��wchar_t������ʽ�����ַ���ǰ+L
inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}
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

//#�궨��ThrowIfFailed
#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

UINT CalcConstantBufferByteSize(UINT byteSize)
{
	return (byteSize + 255) & ~255;
}

ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* d3dDevice, ID3D12GraphicsCommandList* cmdList, UINT64 byteSize, const void* initData, ComPtr<ID3D12Resource>& uploadBuffer)
{
	//�����ϴ��ѣ������ǽ�CPUд�������д���������ɺ����Ĭ�϶�
	ThrowIfFailed(d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),		//�������ݴ�С����������Ĭ��
		D3D12_RESOURCE_STATE_GENERIC_READ,				//�ϴ���������Ҫ�����ƣ�����Ϊ�ɶ�
		nullptr,										//�������ģ����Դ������ָ���Ż�ֵ
		IID_PPV_ARGS(&uploadBuffer)
	));

	//����Ĭ�϶�
	ComPtr<ID3D12Resource> defaultBuffer;
	ThrowIfFailed(d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),	//�ѵ�����ΪĬ�϶�
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,						//Ĭ�϶�Ϊ���ձ������ݵĵط�����ʼ��Ϊ��ͨ״̬
		nullptr,
		IID_PPV_ARGS(&defaultBuffer)
	));

	//����Դ��COMMON״̬��ΪCOPY_DEST״̬��Ĭ�϶��ǽ������ݵ�Ŀ��
	cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST));

	//�����ݴ�CPU�ڴ濽����GPU����
	//�ȴ�������Դ��
	D3D12_SUBRESOURCE_DATA subResourceData;
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	//���ĺ���UpdateSubresources�������ݴ�CPU�ڴ濽�����ϴ��ѣ��ٴ��ϴ��ѿ�����Ĭ�϶�
	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

	//�ٴν���Դ��COPT_DEST״̬ת����GENERIC_READ״̬��ֻ�ṩ����ɫ�����ʣ�
	cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ));

	return defaultBuffer;
}