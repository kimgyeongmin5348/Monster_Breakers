#pragma once
#include "stdafx.h"
#include "Shader.h"
class CFireballShader :
    public CShader
{
public:
    CFireballShader();
    virtual ~CFireballShader();

    virtual D3D12_INPUT_LAYOUT_DESC  CreateInputLayout()       override;
    virtual D3D12_BLEND_DESC         CreateBlendState()        override;
    virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState() override;

    virtual D3D12_SHADER_BYTECODE CreateVertexShader() override;
    virtual D3D12_SHADER_BYTECODE CreatePixelShader()  override;

};

class CGreenSpiritShader : public CFireballShader
{
public:
    CGreenSpiritShader() = default;
    ~CGreenSpiritShader() = default;

    virtual D3D12_SHADER_BYTECODE CreateVertexShader() override;
    virtual D3D12_SHADER_BYTECODE CreatePixelShader()  override;
};

class CHitSparkShader : public CFireballShader
{
public:
    CHitSparkShader() = default;
    ~CHitSparkShader() = default;

    virtual D3D12_SHADER_BYTECODE CreateVertexShader() override;
    virtual D3D12_SHADER_BYTECODE CreatePixelShader()  override;
};