#include "pch.h"

#if defined(_DEBUG)
#include "vld.h"
#endif

#undef main
#include "Renderer.h"

using namespace dae;

void ShutDown(SDL_Window* pWindow)
{
	SDL_DestroyWindow(pWindow);
	SDL_Quit();
}

int main(int argc, char* args[])
{
	//Unreferenced parameters
	(void)argc;
	(void)args;

	//Create window + surfaces
	SDL_Init(SDL_INIT_VIDEO);

	const uint32_t width = 640;
	const uint32_t height = 480;

	SDL_Window* pWindow = SDL_CreateWindow(
		"Dual-Rasterizor - Steven van den Hoven Class: GD11E",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width, height, 0);

	if (!pWindow)
		return 1;

	//Initialize "framework"
	const auto pTimer = new Timer();
	const auto pRenderer = new Renderer(pWindow);

	//Start loop
	bool printFPS{ false };

	pTimer->Start();
	float printTimer = 0.f;
	bool isLooping = true;
	while (isLooping)
	{
		//--------- Get input events ---------
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				isLooping = false;
				break;
			case SDL_KEYUP:
				//Test for a key
				//if (e.key.keysym.scancode == SDL_SCANCODE_X)
				if (e.key.keysym.scancode == SDL_SCANCODE_F1)
				{
					pRenderer->ToggleRenderMethod();
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F2)
				{
					pRenderer->ToggleVehicleRotation();
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F3)
				{
					pRenderer->ToggleFireEffect();
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F4)
				{
					pRenderer->CycleSamplerState();
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F5)
				{
					pRenderer->CycleShadingMode();
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F6)
				{
					pRenderer->ToggleNormalMap();
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F7)
				{
					pRenderer->ToggleDepthBuffer();
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F9)
				{
					pRenderer->CycleCullMode();
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F8)
				{
					pRenderer->ToggleBoundingBox();
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F10)
				{
					pRenderer->ToggleUniformClearColor();
				}
				else if (e.key.keysym.scancode == SDL_SCANCODE_F11)
				{
					HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
					SetConsoleTextAttribute(hConsole, 6);
					printFPS = !printFPS;
					if (printFPS)
					{
						std::cout << "Print FPS = ON**" << std::endl;
					}
					else
					{
						std::cout << "Print FPS = OFF**" << std::endl;
					}
					SetConsoleTextAttribute(hConsole, 15);
				}
				break;
			default: ;
			}
		}

		//--------- Update ---------
		pRenderer->Update(pTimer);

		//--------- Render ---------
		pRenderer->Render();

		//--------- Timer ---------
		pTimer->Update();
		printTimer += pTimer->GetElapsed();
		if (printTimer >= 1.f)
		{
			printTimer = 0.f;
			if(printFPS) std::cout << "dFPS: " << pTimer->GetdFPS() << std::endl;
		}
	}
	pTimer->Stop();

	//Shutdown "framework"
	delete pRenderer;
	delete pTimer;

	ShutDown(pWindow);
	return 0;
}