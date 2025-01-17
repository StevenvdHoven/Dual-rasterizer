#pragma once
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>
#include <iostream>
#include <vector>
#include "Matrix.h"
#include "Texture.h"
#include "DataTypes.h"

class Effect
{
public:

	enum class SampleState
	{
		Point,
		Linear,
		Anisotropic
	};

	Effect() = default;
	~Effect() = default;

	

	virtual void SetTechnique(SampleState technique) = 0;

	virtual void SetDiffuseMap(const dae::Texture* pDiffuseTexture) const = 0;
	virtual void SetNormalMap(const dae::Texture* pNormalMapTexture) const = 0;
	virtual void SetSpecularMap(const dae::Texture* pSpecularMapTexture) const = 0;
	virtual void SetGlossinessMap(const dae::Texture* pGlossMapTexture) const = 0;

	virtual void SetMatricis(const dae::Matrix& worldProjectionMatrix, const dae::Matrix& worldMatrix, const dae::Vector3& cameraOrigin) = 0;

	virtual ID3DX11EffectTechnique* GetTechnique() const = 0;
	virtual ID3D11InputLayout* GetInputLayout() const = 0;

	static ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring& asset1ile);

};

class OpaqueEffect : public Effect
{
public:
	OpaqueEffect(ID3DX11Effect* pEffect, ID3D11Device* pDevice, std::vector<dae::Vertex_In> vertices, std::vector<uint32_t> indices);
	~OpaqueEffect();

	void SetTechnique(SampleState technique) override;

	void SetDiffuseMap(const dae::Texture* pDiffuseTexture) const override;
	void SetNormalMap(const dae::Texture* pNormalMapTexture) const override;
	void SetSpecularMap(const dae::Texture* pSpecularMapTexture) const override;
	void SetGlossinessMap(const dae::Texture* pGlossMapTexture) const override;

	void SetMatricis(const dae::Matrix& worldProjectionMatrix, const dae::Matrix& worldMatrix, const dae::Vector3& cameraOrigin) override;

	ID3DX11EffectTechnique* GetTechnique() const override;
	ID3D11InputLayout* GetInputLayout() const override;

private:
	ID3DX11Effect* m_pEffect;
	ID3D11InputLayout* m_pInputLayout;
	ID3DX11EffectTechnique* m_pTechnique;

	ID3DX11EffectTechnique* m_PointTechnique;
	ID3DX11EffectTechnique* m_LinearTechnique;
	ID3DX11EffectTechnique* m_AnisotropicTechnique;

	ID3DX11EffectShaderResourceVariable* m_pDiffuseMapVariable;
	ID3DX11EffectShaderResourceVariable* m_pNormalMapVariable;
	ID3DX11EffectShaderResourceVariable* m_pSpecularMapVariable;
	ID3DX11EffectShaderResourceVariable* m_pGlossinessMapVariable;

	ID3DX11EffectVariable* m_pCameraPosition;
	ID3DX11EffectMatrixVariable* m_pWorldMatrix;
	ID3DX11EffectMatrixVariable* m_pWorldViewProjectionMatrix;

	uint32_t m_NumIndices;
};


class TransparentEffect : public Effect
{
public:
	TransparentEffect(ID3DX11Effect* pEffect, ID3D11Device* pDevice, std::vector<dae::Vertex_In> vertices, std::vector<uint32_t> indices);
	~TransparentEffect();

	void SetTechnique(SampleState technique) override;

	void SetDiffuseMap(const dae::Texture* pDiffuseTexture) const override;
	void SetNormalMap(const dae::Texture* pNormalMapTexture) const override;
	void SetSpecularMap(const dae::Texture* pSpecularMapTexture) const override;
	void SetGlossinessMap(const dae::Texture* pGlossMapTexture) const override;


	void SetMatricis(const dae::Matrix& worldProjectionMatrix, const dae::Matrix& worldMatrix, const dae::Vector3& cameraOrigin) override;

	ID3DX11EffectTechnique* GetTechnique() const override;
	ID3D11InputLayout* GetInputLayout() const override;

private:
	ID3DX11Effect* m_pEffect;
	ID3D11InputLayout* m_pInputLayout;
	ID3DX11EffectTechnique* m_pTechnique;

	ID3DX11EffectTechnique* m_PointTechnique;
	ID3DX11EffectTechnique* m_LinearTechnique;
	ID3DX11EffectTechnique* m_AnisotropicTechnique;

	ID3DX11EffectShaderResourceVariable* m_pDiffuseMapVariable;
	ID3DX11EffectMatrixVariable* m_pWorldViewProjectionMatrix;
};
