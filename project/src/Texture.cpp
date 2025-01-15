#include "Texture.h"
#include <iostream>
#include <SDL_image.h>
#include "Math.h"

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface, ID3D11Device* pDevice) :
		m_pResource{ nullptr },
		m_pShaderResourceView{ nullptr },
		m_pSurface{ pSurface },
		m_pSurfacePixels{ static_cast<uint32_t*>(pSurface->pixels) }
	{
		DXGI_FORMAT format{ DXGI_FORMAT_R8G8B8A8_UNORM };
		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = pSurface->w;
		desc.Height = pSurface->h;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = pSurface->pixels;
		initData.SysMemPitch = static_cast<UINT>(pSurface->pitch);
		initData.SysMemSlicePitch = static_cast<UINT>(pSurface->h * pSurface->pitch);

		HRESULT hr = pDevice->CreateTexture2D(&desc, &initData, &m_pResource);

		if (FAILED(hr)) std::wcout << L"Loading texture failed!" << std::endl;

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVdesc{};
		SRVdesc.Format = format;
		SRVdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVdesc.Texture2D.MipLevels = 1;

		hr = pDevice->CreateShaderResourceView(m_pResource, &SRVdesc, &m_pShaderResourceView);

		if (FAILED(hr)) std::wcout << L"ShaderResource creation failed!" << std::endl;
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}

		m_pShaderResourceView->Release();
		m_pResource->Release();
	}

	std::unique_ptr<Texture> Texture::LoadFromFile(const std::string& path, ID3D11Device* pDevice)
	{
		SDL_Surface* pSurface = IMG_Load(path.c_str());

		if (pSurface == nullptr)
		{
			return nullptr;
		}

		return std::unique_ptr<Texture>(new Texture(pSurface, pDevice));
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		float x = uv.x > 1.f ? 1.f : uv.x;
		float y = uv.y > 1.f ? 1.f : uv.y;
		//return ColorRGB{ x,y,0 };
		//TODO
		//Sample the correct texel for the given uv
		int xPixel{ (int)(x * m_pSurface->w) };
		int yPixel{ (int)(y * m_pSurface->h) };

		Uint8 r{ 0 };
		Uint8 g{ 0 };
		Uint8 b{ 0 };

		SDL_GetRGB(m_pSurfacePixels[xPixel + (yPixel * m_pSurface->w)], m_pSurface->format, &r, &g, &b);
		return ColorRGB{ static_cast<float>(r), static_cast<float>(g), static_cast<float>(b) };

	}

	ID3D11ShaderResourceView* Texture::GetShaderResourceView() const
	{
		return m_pShaderResourceView;
	}
}
