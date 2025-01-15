#pragma once
#include "pch.h"

// DirectX headers
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;
class Effect;
class Mesh;
class Camera;

namespace dae
{
	enum RenderMethod
	{
		DirectX,
		Software
	};

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(const Timer* pTimer);
		void Render() const;

		void Render_DirectX() const;
		void Render_Software() const;

		void ToggleRenderMethod();

	private:
		//Software
		void RenderMesh(Mesh* mesh) const;
		void RenderTriangle(const std::vector<Vertex_Out>& vertices_ndc, const Mesh* pMesh) const;
		void PixelTriangleTest(uint32_t pixelIndex, const Mesh* pMesh, const std::vector<Vertex_Out>& vertices_ndc, const Vector3& up, const float& area, float minX, float minY, float maxX, float maxY, float* weights) const;

		ColorRGB PixelShading(const Vertex_Out& vertex, const Vector3& vieDirection, const ColorRGB& sampledColor, const Mesh* pMesh) const;
		void ExtractFrustumPlanes(const dae::Matrix& viewProjectionMatrix, dae::Frustum& frustum) const;

		void VertexTransformationFunction(const std::vector<Vertex_In>& vertices_in, std::vector<Vertex_Out>& vertices_out, bool& culling, const Matrix& worldMatrix) const;

		ColorRGB GetDiffuse(const ColorRGB& sampledColor) const;
		ColorRGB GetSpecular(const Vertex_Out& vertex, const Vector3& normal, const Vector3& viewDirection, const Mesh* pMesh) const;

		void RotateMesh(float elapsedSec);

		SDL_Window* m_pWindow{};

		int m_Width{};
		int m_Height{};

		bool m_IsInitialized{ false };
		RenderMethod m_RenderMethod{RenderMethod::Software};

		//DIRECTX
		HRESULT InitializeDirectX();

		ID3D11Device* m_pDevice{ nullptr };
		ID3D11DeviceContext* m_pDeviceContext{ nullptr };
		IDXGISwapChain* m_pSwapChain{ nullptr };
		ID3D11Texture2D* m_pDepthStencilBuffer{ nullptr };
		ID3D11DepthStencilView* m_pDethStencilView{ nullptr };

		ID3D11Resource* m_pRenderTargetBuffer{ nullptr };
		ID3D11RenderTargetView* m_pRenderTargetView{ nullptr };


		//Software
		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{ nullptr };

		const dae::Vector3 m_InvLightDirection{ -0.577f, 0.577f, -0.577f };
		const float m_KS{ .5f };
		const float m_LightIntensity{ 7.f };
		const float m_Shininess{ 25.f };

		Mesh* m_pMesh;
		Camera* m_Camera;
		//...
	};
}
