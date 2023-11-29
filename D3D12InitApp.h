#pragma once
#include "D3D12App.h"
class D3D12InitApp :
    public D3D12App
{
public:
    D3D12InitApp();
    ~D3D12InitApp();

private:
    virtual void Draw() override;
};

