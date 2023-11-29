#pragma once
#include "ToolFunc.h"

template<typename T>
class UploadBufferResource
{
public:
	UploadBufferResource(ComPtr<ID3D12Device> d3dDevice, UINT elementCount, bool isConstantBuffer)
	{
		elementByteSize = sizeof(T);//#如果不是常量缓冲区，则直接计算缓存大小

		if (isConstantBuffer)
			elementByteSize = CalcConstantBufferByteSize(sizeof(T));//#如果是常量缓冲区，则以256的倍数计算缓存大小
		//#创建上传堆和资源
		ThrowIfFailed(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(elementByteSize * elementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer)));
		//#返回欲更新资源的指针
		uploadBuffer->Map(0,										//#子资源索引，对于缓冲区来说，他的子资源就是自己
			nullptr,												//#对整个资源进行映射
			ThrowIfFailed(reinterpret_cast<void**>(&mappedData)));	//#返回待映射资源数据的目标内存块	
	}

	~UploadBufferResource()
	{
		if (uploadBuffer != nullptr)
			uploadBuffer->Unmap(0, nullptr);//#取消映射

		mappedData = nullptr;//#释放映射内存
	}

	void CopyData(int elementIndex, const T& Data)
	{
		memcpy(&mappedData[elementIndex * elementByteSize], &Data, sizeof(T));
	}

	ComPtr<ID3D12Resource> Resource()const
	{
		return uploadBuffer;
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;

	UINT elementByteSize = 0;
	BYTE* mappedData = nullptr;
};
