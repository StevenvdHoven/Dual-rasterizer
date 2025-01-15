#pragma once
#include <d3d11.h>
#include <memory>
#include <SDL_surface.h>
#include <string>
#include "ColorRGB.h"

namespace dae
{
	struct Vector2;

	class Texture
	{
	public:
		Texture(SDL_Surface* pSurface, ID3D11Device* pDevice);
		~Texture();

		static std::unique_ptr<Texture> LoadFromFile(const std::string& path, ID3D11Device* pDevice);
		ColorRGB Sample(const Vector2& uv) const;

		ID3D11ShaderResourceView* GetShaderResourceView() const;

	private:

		ID3D11Texture2D* m_pResource;
		ID3D11ShaderResourceView* m_pShaderResourceView;

		SDL_Surface* m_pSurface;
		uint32_t* m_pSurfacePixels;
	};
}
