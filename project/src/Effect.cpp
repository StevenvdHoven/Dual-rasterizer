#include "Effect.h"
#include "pch.h"
#include "Mesh.h"
#include "Matrix.h"
#include "Texture.h"





ID3DX11Effect* Effect::LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile)
{
	HRESULT result;
	ID3D10Blob* pErrorBlob{ nullptr };
	ID3DX11Effect* pEffect;

	DWORD shaderFlags = 0;

#if (defined(DEBUG) || defined(_DEBUG))
	shaderFlags |= D3DCOMPILE_DEBUG;
	shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	result = D3DX11CompileEffectFromFile(assetFile.c_str(),
		nullptr,
		nullptr,
		shaderFlags,
		0,
		pDevice,
		&pEffect,
		&pErrorBlob);

	if (FAILED(result))
	{
		if (pErrorBlob != nullptr)
		{
			const char* pErros = static_cast<char*>(pErrorBlob->GetBufferPointer());

			std::wstringstream ss;
			for (unsigned int index{ 0 }; index < pErrorBlob->GetBufferSize(); ++index)
				ss << pErros[index];

			OutputDebugStringW(ss.str().c_str());
			pErrorBlob->Release();
			pErrorBlob = nullptr;

			std::wcout << ss.str() << std::endl;
		}
		else
		{
			std::wstringstream ss;
			ss << "Effectloaded : Failed to CreateEffectFromFile!\nPath: " << assetFile;
			std::wcout << ss.str() << std::endl;
		}
	}

	return pEffect;
}

OpaqueEffect::OpaqueEffect(ID3DX11Effect* pEffect, ID3D11Device* pDevice, std::vector<dae::Vertex_In> vertices, std::vector<uint32_t> indices):
	m_pEffect{pEffect}
{
	m_PointTechnique = m_pEffect->GetTechniqueByName("PointTechnique");
	m_LinearTechnique = m_pEffect->GetTechniqueByName("LinearTechnique");
	m_AnisotropicTechnique = m_pEffect->GetTechniqueByName("AnisotropicTechnique");

	m_pTechnique = m_PointTechnique;

	if (!m_pTechnique->IsValid())
	{
		std::cout << "Failed!";
	}

	// Create Vertex Layout
	static constexpr uint32_t numElements{ 4 };
	D3D11_INPUT_ELEMENT_DESC vertexDesc[numElements]{};

	vertexDesc[0].SemanticName = "Position";
	vertexDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[0].AlignedByteOffset = 0;
	vertexDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[1].SemanticName = "TEXCOORD";
	vertexDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	vertexDesc[1].AlignedByteOffset = 12;
	vertexDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[2].SemanticName = "Normal";
	vertexDesc[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[2].AlignedByteOffset = 20;
	vertexDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[3].SemanticName = "Tangent";
	vertexDesc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[3].AlignedByteOffset = 32;
	vertexDesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	// Create Input Layout
	D3DX11_PASS_DESC passDesc{};
	m_pTechnique->GetPassByIndex(0)->GetDesc(&passDesc);
	HRESULT result = pDevice->CreateInputLayout(
		vertexDesc,
		numElements,
		passDesc.pIAInputSignature,
		passDesc.IAInputSignatureSize,
		&m_pInputLayout);

	if (FAILED(result))
	{
		return;
	}

	m_pWorldViewProjectionMatrix = m_pEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
	if (!m_pWorldViewProjectionMatrix->IsValid())
	{
		std::wcout << L"WorldViewProjection matrix not valid" << std::endl;
	}

	m_pDiffuseMapVariable = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();

	if (!m_pDiffuseMapVariable->IsValid())
	{
		std::wcout << L"DiffuseMap variable not valid\n";
	}

	m_pNormalMapVariable = m_pEffect->GetVariableByName("gNormalMap")->AsShaderResource();

	if (!m_pNormalMapVariable->IsValid())
	{
		std::wcout << L"NormalMap variable not valid\n";
	}

	m_pSpecularMapVariable = m_pEffect->GetVariableByName("gSpecularMap")->AsShaderResource();

	if (!m_pSpecularMapVariable->IsValid())
	{
		std::wcout << L"SpecularMap variable not valid\n";
	}

	m_pGlossinessMapVariable = m_pEffect->GetVariableByName("gGlossinessMap")->AsShaderResource();

	if (!m_pGlossinessMapVariable->IsValid())
	{
		std::wcout << L"GlossinessMap variable not valid\n";
	}

	m_pWorldMatrix = m_pEffect->GetVariableByName("gWorldMatrix")->AsMatrix();
	if (!m_pWorldMatrix->IsValid())
	{
		std::wcout << L"WorldMatrix not valid!" << std::endl;
	}

	m_pCameraPosition = m_pEffect->GetVariableByName("gCameraPosition")->AsVector();

	if (!m_pCameraPosition->IsValid())
	{
		std::wcout << L"Camera Pos not valid\n";
	}

	
}

OpaqueEffect::~OpaqueEffect()
{

}

void OpaqueEffect::SetTechnique(SampleState technique)
{
	switch (technique)
	{
	case Effect::SampleState::Point:
		m_pTechnique = m_PointTechnique;
		break;
	case Effect::SampleState::Linear:
		m_pTechnique = m_LinearTechnique;
		break;
	case Effect::SampleState::Anisotropic:
		m_pTechnique = m_AnisotropicTechnique;
		break;
	default:
		break;
	}
}

void OpaqueEffect::SetDiffuseMap(const dae::Texture* pDiffuseTexture) const
{
	if (m_pDiffuseMapVariable)
	{
		const HRESULT result = m_pDiffuseMapVariable->SetResource(pDiffuseTexture->GetShaderResourceView());

		if (FAILED(result))
		{
			std::wcout << L"Setting the diffuse resource failed\n";
		}
	}
}

void OpaqueEffect::SetNormalMap(const dae::Texture* pNormalMapTexture) const
{
	if (m_pNormalMapVariable)
	{
		const HRESULT result = m_pNormalMapVariable->SetResource(pNormalMapTexture->GetShaderResourceView());

		if (FAILED(result))
		{
			std::wcout << L"Setting the normal resource failed\n";
		}
	}
}

void OpaqueEffect::SetSpecularMap(const dae::Texture* pSpecularMapTexture) const
{
	if (m_pSpecularMapVariable)
	{
		const HRESULT result = m_pSpecularMapVariable->SetResource(pSpecularMapTexture->GetShaderResourceView());

		if (FAILED(result))
		{
			std::wcout << L"Setting the specular resource failed\n";
		}
	}
}

void OpaqueEffect::SetGlossinessMap(const dae::Texture* pGlossMapTexture) const
{
	if (m_pGlossinessMapVariable)
	{
		const HRESULT result = m_pGlossinessMapVariable->SetResource(pGlossMapTexture->GetShaderResourceView());

		if (FAILED(result))
		{
			std::wcout << L"Setting the glossiness resource failed\n";
		}
	}
}

void OpaqueEffect::SetMatricis(const dae::Matrix& worldProjectionMatrix, const dae::Matrix& worldMatrix, const dae::Vector3& cameraOrigin)
{
	m_pWorldViewProjectionMatrix->SetMatrix(reinterpret_cast<const float*>(&worldProjectionMatrix));
	m_pWorldMatrix->SetMatrix(reinterpret_cast<const float*>(&worldMatrix));
	m_pCameraPosition->AsVector()->SetFloatVector(reinterpret_cast<const float*>(&cameraOrigin));
}

ID3DX11EffectTechnique* OpaqueEffect::GetTechnique() const
{
	return m_pTechnique;
}

ID3D11InputLayout* OpaqueEffect::GetInputLayout() const
{
	return m_pInputLayout;
}

TransparentEffect::TransparentEffect(ID3DX11Effect* pEffect, ID3D11Device* pDevice, std::vector<dae::Vertex_In> vertices, std::vector<uint32_t> indices):
	m_pEffect{pEffect}
{
	m_PointTechnique = m_pEffect->GetTechniqueByName("PointTechnique");
	m_LinearTechnique = m_pEffect->GetTechniqueByName("LinearTechnique");
	m_AnisotropicTechnique = m_pEffect->GetTechniqueByName("AnisotropicTechnique");

	m_pTechnique = m_PointTechnique;

	if (!m_pTechnique->IsValid())
	{
		std::cout << "Failed!";
	}

	// Create Vertex Layout
	static constexpr uint32_t numElements{ 4 };
	D3D11_INPUT_ELEMENT_DESC vertexDesc[numElements]{};

	vertexDesc[0].SemanticName = "Position";
	vertexDesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[0].AlignedByteOffset = 0;
	vertexDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[1].SemanticName = "TEXCOORD";
	vertexDesc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	vertexDesc[1].AlignedByteOffset = 12;
	vertexDesc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[2].SemanticName = "Normal";
	vertexDesc[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[2].AlignedByteOffset = 20;
	vertexDesc[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	vertexDesc[3].SemanticName = "Tangent";
	vertexDesc[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	vertexDesc[3].AlignedByteOffset = 32;
	vertexDesc[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;

	// Create Input Layout
	D3DX11_PASS_DESC passDesc{};
	m_pTechnique->GetPassByIndex(0)->GetDesc(&passDesc);
	HRESULT result = pDevice->CreateInputLayout(
		vertexDesc,
		numElements,
		passDesc.pIAInputSignature,
		passDesc.IAInputSignatureSize,
		&m_pInputLayout);

	m_pWorldViewProjectionMatrix = m_pEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
	if (!m_pWorldViewProjectionMatrix->IsValid())
	{
		std::wcout << L"WorldViewProjection matrix not valid" << std::endl;
	}

	m_pDiffuseMapVariable = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();

	if (!m_pDiffuseMapVariable->IsValid())
	{
		std::wcout << L"DiffuseMap variable not valid\n";
	}

	if (FAILED(result))
	{
		return;
	}
}

TransparentEffect::~TransparentEffect()
{
}

void TransparentEffect::SetTechnique(SampleState technique)
{
	switch (technique)
	{
	case Effect::SampleState::Point:
		m_pTechnique = m_PointTechnique;
		break;
	case Effect::SampleState::Linear:
		m_pTechnique = m_LinearTechnique;
		break;
	case Effect::SampleState::Anisotropic:
		m_pTechnique = m_AnisotropicTechnique;
		break;
	default:
		break;
	}
}

void TransparentEffect::SetDiffuseMap(const dae::Texture* pDiffuseTexture) const
{
	if (m_pDiffuseMapVariable)
	{
		const HRESULT result = m_pDiffuseMapVariable->SetResource(pDiffuseTexture->GetShaderResourceView());

		if (FAILED(result))
		{
			std::wcout << L"Setting the diffuse resource failed\n";
		}
	}
}

void TransparentEffect::SetNormalMap(const dae::Texture* pNormalMapTexture) const
{
	return;
}

void TransparentEffect::SetSpecularMap(const dae::Texture* pSpecularMapTexture) const
{
	return;
}

void TransparentEffect::SetGlossinessMap(const dae::Texture* pGlossMapTexture) const
{
	return;
}

void TransparentEffect::SetMatricis(const dae::Matrix& worldProjectionMatrix, const dae::Matrix& worldMatrix, const dae::Vector3& cameraOrigin)
{
	m_pWorldViewProjectionMatrix->SetMatrix(reinterpret_cast<const float*>(&worldProjectionMatrix));
}

ID3DX11EffectTechnique* TransparentEffect::GetTechnique() const
{
	return m_pTechnique;
}

ID3D11InputLayout* TransparentEffect::GetInputLayout() const
{
	return m_pInputLayout;
}
