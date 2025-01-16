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


Mesh::Mesh(ID3D11Device* pDevice, std::vector<dae::Vertex_In> vertices, std::vector<uint32_t> indices, std::string* texturesPaths):
	m_Vertices{vertices},
	m_Indices{indices}
{
	const std::wstring assetFile{ L"resources/PosCol3D.fx" };
	auto pEffect{ Effect::LoadEffect(pDevice,assetFile) };
	m_pEffect = new Effect{ pEffect ,pDevice,vertices,indices };

	m_pDiffuseMap = dae::Texture::LoadFromFile(texturesPaths[0], pDevice);
	m_pEffect->SetDiffuseMap(m_pDiffuseMap.get());

	if (!texturesPaths[1].empty())
	{
		m_pNormalMap = dae::Texture::LoadFromFile(texturesPaths[1], pDevice);
		m_pEffect->SetNormalMap(m_pNormalMap.get());
	}

	if (!texturesPaths[2].empty())
	{
		m_pSpecularMap = dae::Texture::LoadFromFile(texturesPaths[2], pDevice);
		m_pEffect->SetSpecularMap(m_pSpecularMap.get());
	}

	if (!texturesPaths[3].empty())
	{
		m_pGlossinessMap = dae::Texture::LoadFromFile(texturesPaths[3], pDevice);
		m_pEffect->SetGlossinessMap(m_pGlossinessMap.get());
	}

	// Create vertex Buffer
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(dae::Vertex_In) * static_cast<uint32_t>(vertices.size());
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = vertices.data();

	HRESULT result = pDevice->CreateBuffer(&bd, &initData, &m_pVertexBuffer);
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

	m_pEffect->SetMatrix(worldViewProjectionMatrix, WorldMatrix, camera->origin);

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

//void Mesh::Render_Software(float aspectRatio, float width, float height, dae::Camera* pCamera, const dae::Frustum& frustum, SDL_Surface* pBackBuffer, uint32_t* pBackBufferPixels, float* pDepthBufferArr)
//{
//	std::vector<uint32_t> indices(3);
//	std::vector<dae::Triangle> triangles{};
//
//	dae::Matrix view{ pCamera->invViewMatrix };
//	dae::Matrix proj{ pCamera->ProjectionMatrix };
//	const dae::Matrix worldViewProjectionMatrix{ WorldMatrix * view * proj };
//
//	int increment = (m_PrimitiveTopology == dae::PrimitiveTopology::TriangleList) ? 3 : 1;
//	for (int indicesIndex = 0; indicesIndex < m_Indices.size() - 2; indicesIndex += increment)
//	{
//		bool isOdd = (indicesIndex % 2) != 0 && m_PrimitiveTopology == dae::PrimitiveTopology::TriangleStrip;
//
//		// Calculate indices for current triangle
//		indices = {
//			m_Indices[indicesIndex],
//			m_Indices[indicesIndex + (isOdd ? 2 : 1)],
//			m_Indices[indicesIndex + (isOdd ? 1 : 2)],
//		};
//
//		// Skip degenerate triangles
//		if (indices[0] == indices[1] || indices[1] == indices[2] || indices[0] == indices[2]) continue;
//
//		// Prepare vertices
//		std::vector<dae::Vertex_Out> out;
//		std::vector<dae::Vertex_In> in{ m_Vertices[indices[0]], m_Vertices[indices[1]], m_Vertices[indices[2]] };
//
//		// Transform vertices and cull
//		bool culling{ false };
//		
//
//		VertexTransformationFunction(aspectRatio, width, height, frustum, in, out, culling, worldViewProjectionMatrix);
//		if (culling || out.size() != 3) continue;
//
//		triangles.emplace_back(dae::Triangle{ out });
//	}
//
//	std::for_each( triangles.begin(), triangles.end(), [&, width, height, pCamera](dae::Triangle triangle)
//		{
//			RenderTriangle(width, height, pCamera, triangle.vertices,pBackBuffer,pBackBufferPixels,pDepthBufferArr);
//		});
//}

void Mesh::SetSamplerState(Effect::SampleState samplerState)
{
	m_pEffect->SetTechnique(samplerState);
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

//void Mesh::RenderTriangle(float width, float height, dae::Camera* pCamera, const std::vector<dae::Vertex_Out>& vertices_ndc, SDL_Surface* pBackBuffer, uint32_t* pBackBufferPixels, float* pDepthBufferArr)
//{
//	const std::vector<float> xPositions{ vertices_ndc[0].position.x,vertices_ndc[1].position.x,vertices_ndc[2].position.x };
//	const std::vector<float> yPositions{ vertices_ndc[0].position.y,vertices_ndc[1].position.y,vertices_ndc[2].position.y };
//
//	float minX{ *std::min_element(xPositions.begin(), xPositions.end()) };
//	float minY{ *std::min_element(yPositions.begin(), yPositions.end()) };
//	minX = std::floor(std::max(0.f, minX));
//	minY = std::floor(std::max(0.f, minY));
//
//	float maxX{ *std::max_element(xPositions.begin(), xPositions.end()) };
//	float maxY{ *std::max_element(yPositions.begin(), yPositions.end()) };
//	maxX = std::ceil(std::min((float)width, maxX));
//	maxY = std::ceil(std::min((float)height, maxY));
//
//	const dae::Vector3 v0{ vertices_ndc[1].position - vertices_ndc[0].position };
//	const dae::Vector3 v1{ vertices_ndc[2].position - vertices_ndc[0].position };
//	const float area{ std::fabs(dae::Vector3::Cross(v0,v1).Magnitude()) * 0.5f };
//	const dae::Vector3 up{ 0,0,1 };
//
//	float weights[3]{};
//
//	for (int px = static_cast<int>(minX); px < static_cast<int>(maxX); ++px)
//	{
//		for (int py = static_cast<int>(minY); py < static_cast<int>(maxY); ++py)
//		{
//			uint32_t pixelIndex = px + py * width;
//			PixelTriangleTest(pixelIndex, width, height, pCamera, vertices_ndc, up,pBackBuffer,pBackBufferPixels,pDepthBufferArr, area, minX, minY, maxX, maxY, weights);
//		}
//	}
//}
//
//dae::ColorRGB Mesh::PixelShading(const dae::Vertex_Out& vertex,const dae::Vector3& viewDirection, const dae::ColorRGB& sampledColor)
//{
//	dae::Vector3 normal{ vertex.normal };
//	const dae::Vector3 binormal{ dae::Vector3::Cross(vertex.normal, vertex.tangent) };
//	const dae::Matrix tangentSpaceAxis{ vertex.tangent, binormal, vertex.normal, dae::Vector3::Zero };
//	const dae::ColorRGB sampledNormal{ m_pNormalMap->Sample(vertex.uv) };
//
//	dae::Vector3 caculatedNormal{ sampledNormal.r, sampledNormal.g, sampledNormal.b };
//	caculatedNormal /= 255.f;
//	caculatedNormal = 2.f * caculatedNormal - dae::Vector3{ 1.f, 1.f, 1.f };
//
//	dae::Vector3 transformedNormal{ tangentSpaceAxis.TransformVector(caculatedNormal) };
//	transformedNormal.Normalize();
//	normal = transformedNormal;
//
//	const float observedArea{ std::max(dae::Vector3::Dot(normal, m_InvLightDirection), 0.f) };
//	const dae::ColorRGB lambert{ GetDiffuse(sampledColor) };
//	const dae::ColorRGB specular{ GetSpecular(vertex,normal,viewDirection) };
//
//	return ((lambert * observedArea) + specular) / 255.f;
//
//}
//
//void Mesh::PixelTriangleTest(uint32_t pixelIndex, float width, float height, dae::Camera* pCamera, const std::vector<dae::Vertex_Out>& vertices_ndc, const dae::Vector3& up, SDL_Surface* pBackBuffer, uint32_t* pBackBufferPixels, float* pDepthBufferArr, const float& area, float minX, float minY, float maxX, float maxY, float* weights)
//{
//	if (pixelIndex == -1) return;
//
//	dae::ColorRGB finalColor{ .25f,.25f,.25f };
//
//	const int px{ static_cast<int>(pixelIndex) % static_cast<int>(width) };
//	const int py{ static_cast<int>(pixelIndex) / static_cast<int>(height) };
//	const float pixel_x{ px + 0.5f };
//	const float pixel_y{ py + 0.5f };
//
//	const dae::Vector3& point{ pixel_x,pixel_y,1 };
//
//	for (int index{ 0 }; index < 3; ++index)
//	{
//		const int otherIndex{ index == 2 ? 0 : index + 1 };
//
//		const dae::Vector3 c{ point - vertices_ndc[index].position };
//		const dae::Vector3 a{ vertices_ndc[otherIndex].position - vertices_ndc[index].position };
//		const dae::Vector3 cross{ dae::Vector3::Cross(a, c) };
//
//		// Normalize the up vector
//		const dae::Vector3 normalizedUp = up.Normalized();
//
//		const float dotResult{ dae::Vector3::Dot(cross, normalizedUp) };
//
//		// Use a small tolerance for near-zero dot products
//		const float epsilon = 1e-6f;
//		if (dotResult < -epsilon)
//		{
//			return;
//		}
//
//		const float weight{ ((dae::Vector2::Cross(a.GetXY(), c.GetXY()) * 0.5f) / area) };
//
//		const int weightIndex{ index == 0 ? 2 : index - 1 };
//		weights[weightIndex] = weight;
//	}
//
//	float depth{ ((weights[0] / vertices_ndc[0].position.z) + (weights[1] / vertices_ndc[1].position.z) + (weights[2] / vertices_ndc[2].position.z)) };
//	depth = 1.f / depth;
//
//
//	if (pDepthBufferArr[pixelIndex] < depth || (depth > 1 || depth < 0)) return;
//	pDepthBufferArr[pixelIndex] = depth;
//
//	float w_interpolated{ 0 };
//	dae::Vector2 caculated_uv{};
//
//	for (int index{ 0 }; index < vertices_ndc.size(); ++index)
//	{
//		caculated_uv += (vertices_ndc[index].uv / vertices_ndc[index].position.w) * weights[index];
//		w_interpolated += (1.f / vertices_ndc[index].position.w) * weights[index];
//	}
//
//	w_interpolated = 1.f / w_interpolated;
//	caculated_uv *= w_interpolated;
//
//	dae::Vector3 normal{};
//	dae::Vector3 tangent{};
//	dae::Vector3 position{};
//
//	for (int index{ 0 }; index < vertices_ndc.size(); ++index)
//	{
//		normal += vertices_ndc[index].normal * weights[index];
//		tangent += vertices_ndc[index].tangent * weights[index];
//		position += vertices_ndc[index].position * weights[index];
//	}
//
//	const dae::ColorRGB sampledColor{ m_pDiffuseMap->Sample(caculated_uv) };
//	const dae::Vector3 viewDirection{ (position - pCamera->origin).Normalized() };
//	const dae::Vertex_Out pixelVertex{ {},caculated_uv,normal,tangent };
//
//	finalColor = PixelShading(pixelVertex,viewDirection,sampledColor);
//
//
//
//	finalColor.MaxToOne();
//
//	pBackBufferPixels[pixelIndex] = SDL_MapRGB(pBackBuffer->format,
//		static_cast<uint8_t>(finalColor.r * 255),
//		static_cast<uint8_t>(finalColor.g * 255),
//		static_cast<uint8_t>(finalColor.b * 255));
//
//}
//
//void Mesh::VertexTransformationFunction(float aspectRatio, float width, float height, const dae::Frustum& frustum, const std::vector<dae::Vertex_In>& vertices_in, std::vector<dae::Vertex_Out>& vertices_out, bool& culling, const dae::Matrix& worldViewProjection) const
//{
//	for (const auto& vertex : vertices_in)
//	{
//		dae::Vector4 transformedPosition = worldViewProjection.TransformPoint(dae::Vector4{ vertex.position, 1.0f });
//
//		// Frustum culling check (in clip space, before perspective division)
//		if (!frustum.IsInsideFrustum(transformedPosition))
//		{
//			culling = true;
//		}
//
//		// Perform perspective division to get NDC
//		const float w = std::fmax(0.0001f, transformedPosition.w); // Avoid division by zero
//		dae::Vector4 ndcPosition{
//			transformedPosition.x / w,
//			transformedPosition.y / w,
//			transformedPosition.z / w,
//			w
//		};
//
//		// Convert to screen space
//		dae::Vertex_Out newVertex;
//		newVertex.position.x = ((ndcPosition.x + 1) / 2) * width;  // Screen space X
//		newVertex.position.y = ((1 - ndcPosition.y) / 2) * height; // Screen space Y
//		newVertex.position.z = ndcPosition.z;                      // Depth (0 to 1)
//		newVertex.position.w = ndcPosition.w;                      // Store original w for later use
//
//		// Pass additional attributes (UVs, normals, tangents, etc.)
//		newVertex.uv = vertex.uv;
//		newVertex.normal = WorldMatrix.TransformVector(vertex.normal).Normalized(); // Transform and normalize
//		newVertex.tangent = WorldMatrix.TransformVector(vertex.tangent).Normalized(); // Transform and normalize
//
//		vertices_out.emplace_back(newVertex);
//	}
//}
//
//dae::ColorRGB Mesh::GetDiffuse(const dae::ColorRGB& sampledColor)
//{
//	return ((m_KD * sampledColor) / M_PI);
//}
//
//dae::ColorRGB Mesh::GetSpecular(const dae::Vertex_Out& vertex, const dae::Vector3& normal,dae::Vector3 viewDirection)
//{
//	const float glossiness{ std::fmax(0.0f, m_pGlossinessMap->Sample(vertex.uv).r) };
//	dae::ColorRGB specularColor{ m_pSpecularMap->Sample(vertex.uv) * m_Shininess };
//
//	const dae::Vector3 reflect{ (m_InvLightDirection - 2.f * dae::Vector3::Dot(m_InvLightDirection, normal) * normal).Normalized() };
//	const float angle{ std::fmax(0.0f, dae::Vector3::Dot(reflect,viewDirection.Normalized())) };
//	const float specReflection{ m_KS * powf(angle, glossiness) };
//
//	return specularColor * specReflection;
//}
