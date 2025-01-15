#pragma once
#include "pch.h"
#include "Camera.h"
#include "Utils.h"
#include "Texture.h"
#include "DataTypes.h"
#include "DataTypes.h"

class ID3D11Device;
class Effect;
struct Matrix;


class Mesh final
{
public:
	Mesh(ID3D11Device* pDevice, std::vector<dae::Vertex_In> vertices, std::vector<uint32_t> indices, std::string* texturesPaths);
	~Mesh();

	void Render_DirectX(ID3D11DeviceContext* pDeviceContext, dae::Camera* camera);
	//void Render_Software(float aspectRatio, float width, float height, dae::Camera* pCamera, const dae::Frustum& frustum, SDL_Surface* pBackBuffer, uint32_t* pBackBufferPixels, float* pDepthBufferArr);


	dae::Texture* GetDiffuseMap() const;
	dae::Texture* GetNormalMap() const;
	dae::Texture* GetSpecularMap() const;
	dae::Texture* GetGlossinessMap() const;

	dae::Matrix WorldMatrix;
	dae::PrimitiveTopology m_PrimitiveTopology{ dae::PrimitiveTopology::TriangleList };

	std::vector<dae::Vertex_In> m_Vertices{};
	std::vector<uint32_t> m_Indices{};
private:
	//Software
	//void RenderTriangle(float width, float height, dae::Camera* pCamera, const std::vector<dae::Vertex_Out>& vertices_ndc, SDL_Surface* pBackBuffer, uint32_t* pBackBufferPixels, float* pDepthBufferArr);
	//void PixelTriangleTest(uint32_t pixelIndex, float width, float height, dae::Camera* pCamera, const std::vector<dae::Vertex_Out>& vertices_ndc, const dae::Vector3& up, SDL_Surface* pBackBuffer, uint32_t* pBackBufferPixels, float* pDepthBufferArr, const float& area, float minX, float minY, float maxX, float maxY, float* weights);
	//dae::ColorRGB PixelShading(const dae::Vertex_Out& vertex, const dae::Vector3& viewDirection, const dae::ColorRGB& sampledColor);

	//void VertexTransformationFunction(float aspectRatio, float width, float height, const dae::Frustum& frustum, const std::vector<dae::Vertex_In>& vertices_in, std::vector<dae::Vertex_Out>& vertices_out, bool& culling, const dae::Matrix& worldViewProjection) const;

	

	//dae::ColorRGB GetDiffuse(const dae::ColorRGB& sampledColor);
	//dae::ColorRGB GetSpecular(const dae::Vertex_Out& vertex, const dae::Vector3& normal, dae::Vector3 viewDirection);


	// DirectX
	Effect* m_pEffect;

	ID3D11Buffer* m_pVertexBuffer;
	ID3D11Buffer* m_pIndexBuffer;

	std::unique_ptr<dae::Texture> m_pDiffuseMap;
	std::unique_ptr<dae::Texture> m_pNormalMap;
	std::unique_ptr<dae::Texture> m_pSpecularMap;
	std::unique_ptr<dae::Texture> m_pGlossinessMap;

	uint32_t m_NumIndices;

	
};

