#pragma once
#include "pch.h"
#include "Camera.h"
#include "Utils.h"
#include "Texture.h"
#include "DataTypes.h"
#include "Effect.h"


struct ID3D11Device;
struct Matrix;


class Mesh final
{
public:
	Mesh(ID3D11Device* pDevice, std::vector<dae::Vertex_In> vertices, std::vector<uint32_t> indices, std::string* texturesPaths, bool onlyDiffuse, Effect* pEffect);
	~Mesh();

	void Render_DirectX(ID3D11DeviceContext* pDeviceContext, dae::Camera* camera);
	//void Render_Software(float aspectRatio, float width, float height, dae::Camera* pCamera, const dae::Frustum& frustum, SDL_Surface* pBackBuffer, uint32_t* pBackBufferPixels, float* pDepthBufferArr);

	void SetSamplerState(Effect::SampleState samplerState);
	void SetCullingMode(dae::CullMode cullmode);

	dae::Texture* GetDiffuseMap() const;
	dae::Texture* GetNormalMap() const;
	dae::Texture* GetSpecularMap() const;
	dae::Texture* GetGlossinessMap() const;

	dae::Matrix WorldMatrix;
	dae::PrimitiveTopology m_PrimitiveTopology{ dae::PrimitiveTopology::TriangleList };

	std::vector<dae::Vertex_In> m_Vertices{};
	std::vector<uint32_t> m_Indices{};
private:

	// DirectX
	Effect* m_pEffect;

	ID3D11Buffer* m_pVertexBuffer;
	ID3D11Buffer* m_pIndexBuffer;

	std::unique_ptr<dae::Texture> m_pDiffuseMap;
	std::unique_ptr<dae::Texture> m_pNormalMap;
	std::unique_ptr<dae::Texture> m_pSpecularMap;
	std::unique_ptr<dae::Texture> m_pGlossinessMap;

	ID3D11RasterizerState* m_RasterizerStateBack;
	ID3D11RasterizerState* m_RasterizerStateFront;
	ID3D11RasterizerState* m_RasterizerStateNone;
	ID3D11RasterizerState* m_CurrentRastizerState;

	uint32_t m_NumIndices;

};

