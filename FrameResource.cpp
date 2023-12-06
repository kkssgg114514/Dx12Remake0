#include "FrameResource.h"

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objCount, UINT matCount, UINT waveVertCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&cmdAllocator)
	));

	objCB = std::make_unique<UploadBufferResource<ObjectConstants>>(device, objCount, true);
	passCB = std::make_unique <UploadBufferResource<PassConstants>>(device, passCount, true);
	matCB = std::make_unique<UploadBufferResource<MatConstants>>(device, matCount, true);

	//因为波浪缓冲区不是常量缓冲，所以最后一个参数是false
	WavesVB = std::make_unique<UploadBufferResource<Vertex>>(device, waveVertCount, false);
}

FrameResource::~FrameResource()
{
}
