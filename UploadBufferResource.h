#pragma once
#include "ToolFunc.h"

template<typename T>
class UploadBufferResource
{
public:
	UploadBufferResource(ID3D12Device* d3dDevice, UINT elementCount, bool isConstantBuffer)
	{
		elementByteSize = sizeof(T);//#������ǳ�������������ֱ�Ӽ��㻺���С

		if (isConstantBuffer)
			elementByteSize = ToolFunc::CalcConstantBufferByteSize(sizeof(T));//#����ǳ���������������256�ı������㻺���С
		//#�����ϴ��Ѻ���Դ
		ThrowIfFailed(d3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(elementByteSize * elementCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer)));
		//#������������Դ��ָ��
		ThrowIfFailed(uploadBuffer->Map(0,					//#����Դ���������ڻ�������˵����������Դ�����Լ�
			nullptr,										//#��������Դ����ӳ��
			reinterpret_cast<void**>(&mappedData)));		//#���ش�ӳ����Դ���ݵ�Ŀ���ڴ��	
	}

	~UploadBufferResource()
	{
		if (uploadBuffer != nullptr)
			uploadBuffer->Unmap(0, nullptr);//#ȡ��ӳ��

		mappedData = nullptr;//#�ͷ�ӳ���ڴ�
	}

	void CopyData(int elementIndex, const T& Data)
	{
		memcpy(&mappedData[elementIndex * elementByteSize], &Data, sizeof(T));
	}

	ID3D12Resource* Resource()const
	{
		return uploadBuffer.Get();
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;

	UINT elementByteSize = 0;
	BYTE* mappedData = nullptr;
};
