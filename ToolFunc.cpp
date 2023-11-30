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
	//创建上传堆，作用是将CPU写入的数据写入这个堆完成后传输给默认堆
	ThrowIfFailed(d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),		//传入数据大小，其它参数默认
		D3D12_RESOURCE_STATE_GENERIC_READ,				//上传堆数据需要被复制，设置为可读
		nullptr,										//不是深度模板资源，不用指定优化值
		IID_PPV_ARGS(&uploadBuffer)
	));

	//创建默认堆
	ComPtr<ID3D12Resource> defaultBuffer;
	ThrowIfFailed(d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),	//堆的类型为默认堆
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,						//默认堆为最终保存数据的地方，初始化为普通状态
		nullptr,
		IID_PPV_ARGS(&defaultBuffer)
	));

	//将资源从COMMON状态变为COPY_DEST状态，默认堆是接收数据的目标
	cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST));

	//将数据从CPU内存拷贝到GPU缓存
	//先创建子资源表单
	D3D12_SUBRESOURCE_DATA subResourceData;
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	//核心函数UpdateSubresources，将数据从CPU内存拷贝至上传堆，再从上传堆拷贝至默认堆
	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

	//再次将资源从COPT_DEST状态转换到GENERIC_READ状态（只提供给着色器访问）
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
	//若处于调试状态
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	//以调试模式编译着色器
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // defined(DEBUG) || defined(_DEBUG)

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(filename.c_str(),	//#hlsl源文件名
		defines,								//#高级选项，指定为空指针
		D3D_COMPILE_STANDARD_FILE_INCLUDE,		//#高级选项，可以指定为空指针
		entryPoint.c_str(),						//#着色器的入口点函数名
		target.c_str(),							//#指定所用着色器类型和版本的字符串
		compileFlags,							//#指示对着色器断代码应当如何编译的标志
		0,										//#高级选项
		&byteCode,								//#编译好的字节码
		&errors);								//#错误信息

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
