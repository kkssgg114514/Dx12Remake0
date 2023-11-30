#include "ToolFunc.h"

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber)
	:ErrorCode(hr),
	FunctionName(functionName),
	Filename(filename),
	LineNumber(lineNumber)
{
}

std::wstring DxException::ToString()const
{
	// #Get the string description of the error code.
	_com_error err(ErrorCode);
	std::wstring msg = err.ErrorMessage();

	return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}

ComPtr<ID3D12Resource> ToolFunc::CreateDefaultBuffer(ID3D12Device* d3dDevice, ID3D12GraphicsCommandList* cmdList, UINT64 byteSize, const void* initData, ComPtr<ID3D12Resource> uploadBuffer)
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

ComPtr<ID3DBlob> ToolFunc::CompileShader(
	const std::wstring& filename, 
	const D3D_SHADER_MACRO* defines, 
	const std::string& entryPoint, 
	const std::string& target)
{
	//�����ڵ���״̬
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	//�Ե���ģʽ������ɫ��
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // defined(DEBUG) || defined(_DEBUG)

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(filename.c_str(),	//#hlslԴ�ļ���
		defines,								//#�߼�ѡ�ָ��Ϊ��ָ��
		D3D_COMPILE_STANDARD_FILE_INCLUDE,		//#�߼�ѡ�����ָ��Ϊ��ָ��
		entryPoint.c_str(),						//#��ɫ������ڵ㺯����
		target.c_str(),							//#ָ��������ɫ�����ͺͰ汾���ַ���
		compileFlags,							//#ָʾ����ɫ���ϴ���Ӧ����α���ı�־
		0,										//#�߼�ѡ��
		&byteCode,								//#����õ��ֽ���
		&errors);								//#������Ϣ

	if (errors != nullptr)
	{
		OutputDebugStringA((char*)errors->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	return byteCode;
}

UINT ToolFunc::CalcConstantBufferByteSize(UINT byteSize)
{
	return (byteSize + 255) & ~255;
}
