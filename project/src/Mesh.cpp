#include "Mesh.h"
#include "Effect.h"
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>
#include "Texture.h"
#include "Utils.h"
#include <algorithm>
#include <execution>
#include "Camera.h"
#include <iostream>


Mesh::Mesh(ID3D11Device* pDevice, std::vector<dae::Vertex_In> vertices, std::vector<uint32_t> indices, std::string* texturesPaths, bool onlyDiffuse, Effect* pEffect):
	m_Vertices{vertices},
	m_Indices{indices},
	m_pEffect{pEffect}
{
	HRESULT result;

	

	m_pDiffuseMap = dae::Texture::LoadFromFile(texturesPaths[0], pDevice);
	m_pEffect->SetDiffuseMap(m_pDiffuseMap.get());

	if (!onlyDiffuse && !texturesPaths[1].empty())
	{
		m_pNormalMap = dae::Texture::LoadFromFile(texturesPaths[1], pDevice);
		m_pEffect->SetNormalMap(m_pNormalMap.get());
	}

	if (!onlyDiffuse && !texturesPaths[2].empty())
	{
		m_pSpecularMap = dae::Texture::LoadFromFile(texturesPaths[2], pDevice);
		m_pEffect->SetSpecularMap(m_pSpecularMap.get());
	}

	if (!onlyDiffuse && !texturesPaths[3].empty())
	{
		m_pGlossinessMap = dae::Texture::LoadFromFile(texturesPaths[3], pDevice);
		m_pEffect->SetGlossinessMap(m_pGlossinessMap.get());
	}

	D3D11_RASTERIZER_DESC rasterizerDescBack{};
	ZeroMemory(&rasterizerDescBack, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDescBack.CullMode = D3D11_CULL_BACK;
	rasterizerDescBack.FillMode = D3D11_FILL_SOLID;
	rasterizerDescBack.DepthClipEnable = true;

	result = pDevice->CreateRasterizerState(&rasterizerDescBack, &m_RasterizerStateBack);
	if (FAILED(result))
		return;

	D3D11_RASTERIZER_DESC rasterizerDescFront{};
	ZeroMemory(&rasterizerDescFront, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDescFront.CullMode = D3D11_CULL_FRONT;
	rasterizerDescFront.FillMode = D3D11_FILL_SOLID;
	rasterizerDescFront.DepthClipEnable = true;

	result = pDevice->CreateRasterizerState(&rasterizerDescFront, &m_RasterizerStateFront);
	if (FAILED(result))
		return;

	D3D11_RASTERIZER_DESC rasterizerDescNone{};
	ZeroMemory(&rasterizerDescNone, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerDescNone.CullMode = D3D11_CULL_NONE;
	rasterizerDescNone.FillMode = D3D11_FILL_SOLID;
	rasterizerDescNone.DepthClipEnable = true;

	result = pDevice->CreateRasterizerState(&rasterizerDescNone, &m_RasterizerStateNone);
	if (FAILED(result))
		return;

	m_CurrentRastizerState = m_RasterizerStateBack;

	

	// Create vertex Buffer
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(dae::Vertex_In) * static_cast<uint32_t>(vertices.size());
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = vertices.data();

	result = pDevice->CreateBuffer(&bd, &initData, &m_pVertexBuffer);
	if (FAILED(result))
		return;

	// Create index buffer
	m_NumIndices = static_cast<uint32_t>(indices.size());
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(uint32_t) * m_NumIndices;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	initData.pSysMem = indices.data();
	result = pDevice->CreateBuffer(&bd, &initData, &m_pIndexBuffer);

	if (FAILED(result))
		return;
}

Mesh::~Mesh()
{
	if (m_pVertexBuffer) m_pVertexBuffer->Release();
	if (m_pIndexBuffer) m_pIndexBuffer->Release();
	delete m_pEffect;
}

void Mesh::Render_DirectX(ID3D11DeviceContext* pDeviceContext, dae::Camera* camera)
{
	pDeviceContext->RSSetState(m_CurrentRastizerState);

	//1. Set Primitive Topology
	pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//2. Set Input Layout
	pDeviceContext->IASetInputLayout(m_pEffect->GetInputLayout());

	//3. Set VertexBuffer
	constexpr UINT stride{ sizeof(dae::Vertex_In) };
	constexpr UINT offset{ 0 };
	pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	dae::Matrix view{ camera->invViewMatrix };
	dae::Matrix proj{ camera->ProjectionMatrix };
	const dae::Matrix worldViewProjectionMatrix{ WorldMatrix * view * proj };

	m_pEffect->SetMatricis(worldViewProjectionMatrix, WorldMatrix, camera->origin);

	//4. Set IndexBuffer
	pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	pDeviceContext->GenerateMips(m_pDiffuseMap->GetShaderResourceView());

	//5. Draw
	D3DX11_TECHNIQUE_DESC techDesc{};
	m_pEffect->GetTechnique()->GetDesc(&techDesc);
	for (UINT p{ 0 }; p < techDesc.Passes; ++p)
	{
		m_pEffect->GetTechnique()->GetPassByIndex(p)->Apply(0, pDeviceContext);
		pDeviceContext->DrawIndexed(m_NumIndices, 0, 0);
	}
}



void Mesh::SetSamplerState(Effect::SampleState samplerState)
{
	m_pEffect->SetTechnique(samplerState);
}

void Mesh::SetCullingMode(dae::CullMode cullmode)
{
	switch (cullmode)
	{
	case dae::Back:
		m_CurrentRastizerState = m_RasterizerStateBack;
		break;
	case dae::Front:
		m_CurrentRastizerState = m_RasterizerStateFront;
		break;
	case dae::None:
		m_CurrentRastizerState = m_RasterizerStateNone;
		break;
	default:
		break;
	}
}

dae::Texture* Mesh::GetDiffuseMap() const
{
	return m_pDiffuseMap.get();
}

dae::Texture* Mesh::GetNormalMap() const
{
	return m_pNormalMap.get();
}

dae::Texture* Mesh::GetSpecularMap() const
{
	return m_pSpecularMap.get();
}

dae::Texture* Mesh::GetGlossinessMap() const
{
	return m_pGlossinessMap.get();
}

