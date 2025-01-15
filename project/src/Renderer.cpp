#include "Mesh.h"
#include "Effect.h"
#include "Renderer.h"
#include "Utils.h"
#include "DataTypes.h"
#include <algorithm>
#include <execution>


namespace dae {

	Renderer::Renderer(SDL_Window* pWindow) :
		m_pWindow(pWindow)
	{
		//Initialize
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

		//Create Buffers
		m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
		m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
		m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

		m_pDepthBufferPixels = new float[m_Width * m_Height];
		std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

		//Initialize DirectX pipeline
		const HRESULT result = InitializeDirectX();
		if (result == S_OK)
		{
			m_IsInitialized = true;

			std::vector<Vertex_In> vertices;
			std::vector<uint32_t> indices;

			Utils::ParseOBJ("resources/vehicle.obj", vertices, indices);

			std::string texturePaths[4]
			{
				"resources/vehicle_diffuse.png",
				"resources/vehicle_normal.png",
				"resources/vehicle_gloss.png",
				"resources/vehicle_specular.png",
			};

			m_pMesh = new Mesh
			{
				m_pDevice,
				vertices,
				indices,
				texturePaths
			};

			m_pMesh->WorldMatrix = Matrix::CreateTranslation({ 0,0,0 });

			std::cout << "DirectX is initialized and ready!\n";
		}
		else
		{
			std::cout << "DirectX initialization failed!\n";
		}

		m_Camera = new Camera{};
		m_Camera->Initialize(45.f, { 0,0,-100.f }, static_cast<float>(m_Width) / static_cast<float>(m_Height));

	}

	Renderer::~Renderer()
	{
		delete m_pMesh;
		delete m_Camera;
		m_pRenderTargetView->Release();
		m_pRenderTargetBuffer->Release();
		m_pDethStencilView->Release();
		m_pDepthStencilBuffer->Release();
		m_pSwapChain->Release();

		if (m_pDeviceContext)
		{
			m_pDeviceContext->ClearState();
			m_pDeviceContext->Flush();
			m_pDeviceContext->Release();
		}

		m_pDevice->Release();

	}

	void Renderer::Update(const dae::Timer* pTimer)
	{
		m_Camera->Update(pTimer);
	}


	void Renderer::Render() const
	{
		if (m_RenderMethod == DirectX)
		{
			Render_DirectX();
		}
		else
		{
			Render_Software();
		}
	}

	void Renderer::Render_DirectX() const
	{
		if (!m_IsInitialized)
			return;

		constexpr float color[4] = { 0.f,0.f,0.3f,1.f };
		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, color);
		m_pDeviceContext->ClearDepthStencilView(m_pDethStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

		m_pMesh->Render_DirectX(m_pDeviceContext, m_Camera);


		m_pSwapChain->Present(0, 0);
	}

	void Renderer::Render_Software() const
	{
		SDL_LockSurface(m_pBackBuffer);

		dae::Matrix view{ m_Camera->invViewMatrix };
		dae::Matrix proj{ m_Camera->ProjectionMatrix };



		dae::Frustum frustum;
		ExtractFrustumPlanes(view * proj, frustum);

		for (int px{ 0 }; px < m_Width; ++px)
		{
			for (int py{ 0 }; py < m_Height; ++py)
			{
				ColorRGB finalColor{ .25f, .25f, .25f };

				finalColor.MaxToOne();
				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}
		}

		RenderMesh(m_pMesh);

		for (int px{ 0 }; px < m_Width; ++px)
		{
			for (int py{ 0 }; py < m_Height; ++py)
			{
				m_pDepthBufferPixels[px + (py * m_Width)] = FLT_MAX;
			}
		}
	

		SDL_UnlockSurface(m_pBackBuffer);
		SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
		SDL_UpdateWindowSurface(m_pWindow);
	}

	void Renderer::ToggleRenderMethod()
	{
		if (m_RenderMethod == DirectX)
		{
			m_RenderMethod = Software;
		}
		else
		{
			m_RenderMethod = DirectX;
		}
	}

	void Renderer::RenderMesh(Mesh* mesh) const
	{
		std::vector<uint32_t> indices(3);
			std::vector<dae::Triangle> triangles{};
		
			dae::Matrix view{ m_Camera->invViewMatrix };
			dae::Matrix proj{ m_Camera->ProjectionMatrix };
			const dae::Matrix worldViewProjectionMatrix{ mesh->WorldMatrix * view * proj };
		
			int increment = (mesh->m_PrimitiveTopology == dae::PrimitiveTopology::TriangleList) ? 3 : 1;
			for (int indicesIndex = 0; indicesIndex < mesh->m_Indices.size() - 2; indicesIndex += increment)
			{
				bool isOdd = (indicesIndex % 2) != 0 && mesh->m_PrimitiveTopology == dae::PrimitiveTopology::TriangleStrip;
		
				// Calculate indices for current triangle
				indices = {
					mesh->m_Indices[indicesIndex],
					mesh->m_Indices[indicesIndex + (isOdd ? 2 : 1)],
					mesh->m_Indices[indicesIndex + (isOdd ? 1 : 2)],
				};
		
				// Skip degenerate triangles
				if (indices[0] == indices[1] || indices[1] == indices[2] || indices[0] == indices[2]) continue;
		
				// Prepare vertices
				std::vector<dae::Vertex_Out> out;
				std::vector<dae::Vertex_In> in{ mesh->m_Vertices[indices[0]], mesh->m_Vertices[indices[1]], mesh->m_Vertices[indices[2]] };
		
				// Transform vertices and cull
				bool culling{ false };
				
		
				VertexTransformationFunction(in,out,culling,mesh->WorldMatrix);
				if (culling || out.size() != 3) continue;
		
				triangles.emplace_back(dae::Triangle{ out });
			}
		
			std::for_each(std::execution::par_unseq, triangles.begin(), triangles.end(), [&](dae::Triangle triangle)
				{
					RenderTriangle(triangle.vertices,mesh);
				});
	}

	void Renderer::RenderTriangle(const std::vector<Vertex_Out>& vertices_ndc, const Mesh* pMesh) const
	{
		const std::vector<float> xPositions{ vertices_ndc[0].position.x,vertices_ndc[1].position.x,vertices_ndc[2].position.x };
		const std::vector<float> yPositions{ vertices_ndc[0].position.y,vertices_ndc[1].position.y,vertices_ndc[2].position.y };

		float minX{ *std::min_element(xPositions.begin(), xPositions.end()) };
		float minY{ *std::min_element(yPositions.begin(), yPositions.end()) };
		minX = std::floor(std::max(0.f, minX));
		minY = std::floor(std::max(0.f, minY));

		float maxX{ *std::max_element(xPositions.begin(), xPositions.end()) };
		float maxY{ *std::max_element(yPositions.begin(), yPositions.end()) };
		maxX = std::ceil(std::min((float)m_Width, maxX));
		maxY = std::ceil(std::min((float)m_Height, maxY));

		const Vector3 v0{ vertices_ndc[1].position - vertices_ndc[0].position };
		const Vector3 v1{ vertices_ndc[2].position - vertices_ndc[0].position };
		const float area{ std::fabs(Vector3::Cross(v0,v1).Magnitude()) * 0.5f };
		const Vector3 up{ 0,0,1 };

		float weights[3]{};

		for (int px = static_cast<int>(minX); px < static_cast<int>(maxX); ++px)
		{
			for (int py = static_cast<int>(minY); py < static_cast<int>(maxY); ++py)
			{
				uint32_t pixelIndex = px + py * m_Width;
				PixelTriangleTest(pixelIndex, pMesh, vertices_ndc, up, area, minX, minY, maxX, maxY, weights);
			}
		}
	}

	void Renderer::PixelTriangleTest(uint32_t pixelIndex, const Mesh* pMesh, const std::vector<Vertex_Out>& vertices_ndc, const Vector3& up, const float& area, float minX, float minY, float maxX, float maxY, float* weights) const
	{
		if (pixelIndex == -1) return;

		ColorRGB finalColor{ .25f,.25f,.25f };

		const int px{ (int)pixelIndex % m_Width };
		const int py{ (int)pixelIndex / m_Width };
		const float pixel_x{ px + 0.5f };
		const float pixel_y{ py + 0.5f };

		const Vector3& point{ pixel_x,pixel_y,1 };

		for (int index{ 0 }; index < 3; ++index)
		{
			const int otherIndex{ index == 2 ? 0 : index + 1 };

			const Vector3 c{ point - vertices_ndc[index].position };
			const Vector3 a{ vertices_ndc[otherIndex].position - vertices_ndc[index].position };
			const Vector3 cross{ Vector3::Cross(a,c) };
			const float dotResult{ Vector3::Dot(cross,up) };
			if (dotResult < 0)
			{
				return;
			}
			const float weight{ ((Vector2::Cross(a.GetXY(), c.GetXY()) * .5f) / area) };

			const int weightIndex{ index == 0 ? 2 : index - 1 };
			weights[weightIndex] = weight;
		}

		float depth{ ((weights[0] / vertices_ndc[0].position.z) + (weights[1] / vertices_ndc[1].position.z) + (weights[2] / vertices_ndc[2].position.z)) };
		depth = 1.f / depth;


		if (m_pDepthBufferPixels[pixelIndex] < depth || (depth > 1 || depth < 0)) return;
		m_pDepthBufferPixels[pixelIndex] = depth;

		float w_interpolated{ 0 };
		Vector2 caculated_uv{};

		for (int index{ 0 }; index < vertices_ndc.size(); ++index)
		{
			caculated_uv += (vertices_ndc[index].uv / vertices_ndc[index].position.w) * weights[index];
			w_interpolated += (1.f / vertices_ndc[index].position.w) * weights[index];
		}

		w_interpolated = 1.f / w_interpolated;
		caculated_uv *= w_interpolated;


		Vector3 normal{};
		Vector3 tangent{};
		Vector3 position{};

		for (int index{ 0 }; index < vertices_ndc.size(); ++index)
		{
			normal += vertices_ndc[index].normal * weights[index];
			tangent += vertices_ndc[index].tangent * weights[index];
			position += vertices_ndc[index].position * weights[index];
		}

		const ColorRGB sampledColor{ m_pMesh->GetDiffuseMap()->Sample(caculated_uv) };
		const Vector3 viewDirection{ (position - m_Camera->origin).Normalized() };
		const Vertex_Out pixelVertex{ {},caculated_uv,normal,tangent };

		finalColor = PixelShading(pixelVertex,viewDirection,sampledColor,pMesh);

		finalColor.MaxToOne();

		m_pBackBufferPixels[pixelIndex] = SDL_MapRGB(m_pBackBuffer->format,
			static_cast<uint8_t>(finalColor.r * 255),
			static_cast<uint8_t>(finalColor.g * 255),
			static_cast<uint8_t>(finalColor.b * 255));

	}

	ColorRGB Renderer::PixelShading(const Vertex_Out& vertex,const Vector3& vieDirection, const ColorRGB& sampledColor, const Mesh* pMesh) const
	{
		Vector3 normal{ vertex.normal };
		const Vector3 binormal{ Vector3::Cross(vertex.normal, vertex.tangent) };
		const Matrix tangentSpaceAxis{ vertex.tangent, binormal, vertex.normal, Vector3::Zero };
		const ColorRGB sampledNormal{ pMesh->GetNormalMap()->Sample(vertex.uv) };

		Vector3 caculatedNormal{ sampledNormal.r, sampledNormal.g, sampledNormal.b };
		caculatedNormal /= 255.f;
		caculatedNormal = 2.f * caculatedNormal - Vector3{ 1.f, 1.f, 1.f };

		Vector3 transformedNormal{ tangentSpaceAxis.TransformVector(caculatedNormal) };
		transformedNormal.Normalize();
		normal = transformedNormal;


		const float observedArea{ std::max(Vector3::Dot(normal, m_InvLightDirection), 0.f) };
		const ColorRGB lambert{ GetDiffuse(sampledColor) };
		const ColorRGB specular{ GetSpecular(vertex,normal,vieDirection,pMesh) };
		return ((lambert * observedArea) + specular) / 255.f;
	}

	void Renderer::ExtractFrustumPlanes(const Matrix& viewProjectionMatrix, Frustum& frustum) const
	{
		// Access the matrix rows
		const Vector4& row0 = viewProjectionMatrix[0]; // x-axis
		const Vector4& row1 = viewProjectionMatrix[1]; // y-axis
		const Vector4& row2 = viewProjectionMatrix[2]; // z-axis
		const Vector4& row3 = viewProjectionMatrix[3]; // translation

		// Extract frustum planes from the view-projection matrix
		frustum.nearFace = Plane{
			Vector3(row3.x + row2.x, row3.y + row2.y, row3.z + row2.z), // Normal
			row3.w + row2.w // Distance
		};

		frustum.farFace = Plane{
			Vector3(row3.x - row2.x, row3.y - row2.y, row3.z - row2.z), // Normal
			row3.w - row2.w // Distance
		};

		frustum.leftFace = Plane{
			Vector3(row3.x + row0.x, row3.y + row0.y, row3.z + row0.z), // Normal
			row3.w + row0.w // Distance
		};

		frustum.rightFace = Plane{
			Vector3(row3.x - row0.x, row3.y - row0.y, row3.z - row0.z), // Normal
			row3.w - row0.w // Distance
		};

		frustum.topFace = Plane{
			Vector3(row3.x - row1.x, row3.y - row1.y, row3.z - row1.z), // Normal
			row3.w - row1.w // Distance
		};

		frustum.bottomFace = Plane{
			Vector3(row3.x + row1.x, row3.y + row1.y, row3.z + row1.z), // Normal
			row3.w + row1.w // Distance
		};

		// Normalize the planes to ensure unit-length normals
		for (Plane* plane : { &frustum.nearFace, &frustum.farFace, &frustum.leftFace,
							  &frustum.rightFace, &frustum.topFace, &frustum.bottomFace })
		{
			float magnitude = plane->normal.Magnitude();
			plane->normal /= magnitude;
			plane->distance /= magnitude;
		}
	}

	void Renderer::VertexTransformationFunction(const std::vector<Vertex_In>& vertices_in, std::vector<Vertex_Out>& vertices_out, bool& culling, const Matrix& worldMatrix) const
	{
		const float aspectRatio{ (float)m_Width / m_Height };
		const Matrix projM{ m_Camera->ProjectionMatrix };

		const Matrix m{ worldMatrix * m_Camera->invViewMatrix * projM };

		Frustum frustum;
		ExtractFrustumPlanes(m, frustum);

		for (const auto& vertex : vertices_in)
		{
			// Transform to clip space
			Vector4 transformedPosition = m.TransformPoint(Vector4{ vertex.position, vertex.position.z });

			// Perform perspective division
			const float devision{ std::fmax(0.0001f, transformedPosition.w) };
			transformedPosition.x /= devision;
			transformedPosition.y /= devision;
			transformedPosition.z /= devision;

			if (!frustum.IsInsideFrustum(transformedPosition))
			{
				culling = true;
			}

			// Convert to screen space
			Vertex_Out newVertex;
			newVertex.position.x = ((transformedPosition.x + 1) / 2) * m_Width;  // Screen space X
			newVertex.position.y = ((1 - transformedPosition.y) / 2) * m_Height; // Screen space Y
			newVertex.position.z = transformedPosition.z;                       // NDC Depth (0 to 1)
			newVertex.position.w = transformedPosition.w;

			// Pass additional attributes (UVs, color, etc.)
			newVertex.uv = vertex.uv;
			newVertex.normal = worldMatrix.TransformVector(vertex.normal);
			newVertex.tangent = worldMatrix.TransformVector(vertex.tangent);


			vertices_out.emplace_back(newVertex);
		}
	}

	ColorRGB Renderer::GetDiffuse(const ColorRGB& sampledColor) const
	{
		return ((m_KD * sampledColor) / M_PI);
	}

	ColorRGB Renderer::GetSpecular(const Vertex_Out& vertex, const Vector3& normal,const Vector3& viewDirection, const Mesh* pMesh) const
	{
		const float glossiness{ std::fmax(0.0f, pMesh->GetGlossinessMap()->Sample(vertex.uv).r)};
		ColorRGB specularColor{ pMesh->GetSpecularMap()->Sample(vertex.uv) * m_Shininess};

		const Vector3 reflect{ (m_InvLightDirection - 2.f * Vector3::Dot(m_InvLightDirection, normal) * normal).Normalized() };
		const float angle{ std::fmax(0.0f, Vector3::Dot(reflect, viewDirection.Normalized())) };
		const float specReflection{ m_KS * powf(angle, glossiness) };

		return specularColor * specReflection;
	}

	HRESULT Renderer::InitializeDirectX()
	{
		//1. Create Device & DeviceContext
		//======

		D3D_FEATURE_LEVEL featureLevel{ D3D_FEATURE_LEVEL_11_1 };
		uint32_t createDeviceFlags{ 0 };
#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags, &featureLevel,
			1, D3D11_SDK_VERSION, &m_pDevice, nullptr, &m_pDeviceContext);

		// Create Swap Chain
		IDXGIFactory1* pDxgiFactory{};
		result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pDxgiFactory));
		if (FAILED(result))
			return result;

		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		swapChainDesc.BufferDesc.Width = m_Width;
		swapChainDesc.BufferDesc.Height = m_Height;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;

		SDL_SysWMinfo sysWMInfo{};
		SDL_GetVersion(&sysWMInfo.version);
		SDL_GetWindowWMInfo(m_pWindow, &sysWMInfo);
		swapChainDesc.OutputWindow = sysWMInfo.info.win.window;

		result = pDxgiFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
		if (FAILED(result))
			return result;

		pDxgiFactory->Release();

		D3D11_TEXTURE2D_DESC depthStencilDesc{};
		depthStencilDesc.Width = m_Width;
		depthStencilDesc.Height = m_Height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		depthStencilViewDesc.Format = depthStencilDesc.Format;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		result = m_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, &m_pDepthStencilBuffer);
		if (FAILED(result))
			return result;

		result = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDethStencilView);
		if (FAILED(result))
			return result;

		result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer));
		if (FAILED(result))
			return result;

		result = m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView);
		if (FAILED(result))
			return result;

		m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDethStencilView);

		D3D11_VIEWPORT viewport{};
		viewport.Width = static_cast<float>(m_Width);
		viewport.Height = static_cast<float>(m_Height);
		viewport.TopLeftX = 0.f;
		viewport.TopLeftY = 0.f;
		viewport.MinDepth = 0.f;
		viewport.MaxDepth = 1.f;
		m_pDeviceContext->RSSetViewports(1, &viewport);

		return result;
	}

}
