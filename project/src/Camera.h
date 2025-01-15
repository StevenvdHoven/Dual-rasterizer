#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <iostream>
#include "Math.h"


#include "Vector3.h"
#include "Matrix.h"
#include "Timer.h"


namespace dae
{
	class Camera
	{
	public:
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle) :
			origin{ _origin },
			fovAngle{ _fovAngle }
		{
		}


		Vector3 origin{};
		float fovAngle{ 45.f };
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };
		const float speed{ 5 };
		const float rotationSpeed{ 100 };

		Vector3 forward{ Vector3::UnitZ };
		Vector3 up{ Vector3::UnitY };
		Vector3 right{ Vector3::UnitX };

		float totalPitch{};
		float totalYaw{};
		float aspectRatio{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		Matrix ProjectionMatrix;

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = { 0.f,0.f,0.f }, float aspectratio = 0)
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;
			aspectRatio = aspectratio;

			CalculateProjectionMatrix();
		}

        void CalculateViewMatrix()
        {
            // Constants
            const float toDegreeScale{ M_PI / 180.0f };

            // Create translation matrix based on the current origin
            const Matrix translationMatrix = Matrix::CreateTranslation(origin);

            // Create rotation matrix using yaw and pitch
            const Matrix rotationMatrix = Matrix::CreateRotation(totalYaw * toDegreeScale, totalPitch * toDegreeScale, 0.0f);

            // Combine translation and rotation into the view matrix
            const Matrix combinedMatrix = rotationMatrix * translationMatrix;

            // Update view matrix and inverse view matrix
            viewMatrix = combinedMatrix;
            invViewMatrix = Matrix::Inverse(combinedMatrix);
        }

        void CalculateProjectionMatrix()
        {
            // Projection Matrix setup with a field of view (fov), aspect ratio, near plane and far plane
            ProjectionMatrix = Matrix::CreatePerspectiveFovLH(fov, aspectRatio, 1.0f, 100.0f);
        }

        void Update(const dae::Timer* pTimer)
        {
            const float deltaTime = pTimer->GetElapsed();

            // Handle keyboard input
            const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);
            Vector3 movement;

            // Forward/Backward movement (W/S keys)
            if (pKeyboardState[SDL_GetScancodeFromKey(SDLK_w)] || pKeyboardState[SDL_GetScancodeFromKey(SDLK_s)])
            {
                movement += forward * (pKeyboardState[SDL_GetScancodeFromKey(SDLK_w)] ? 1.0f : -1.0f);
            }

            // Left/Right movement (A/D keys)
            if (pKeyboardState[SDL_GetScancodeFromKey(SDLK_a)] || pKeyboardState[SDL_GetScancodeFromKey(SDLK_d)])
            {
                movement += right * (pKeyboardState[SDL_GetScancodeFromKey(SDLK_d)] ? 1.0f : -1.0f);
            }

            // Handle mouse input for camera rotation
            int mouseX{}, mouseY{};
            const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

            Vector3 direction{ static_cast<float>(mouseX), 0.0f, static_cast<float>(-mouseY) };
            if (mouseState & SDL_BUTTON_LEFT)
            {
                movement = direction;  // Override movement when left-click is pressed
            }
            else if (mouseState & 4)  // Right-click for rotation
            {
                direction.Normalize();

                // Apply yaw and pitch rotation
                if (mouseX != 0) totalYaw -= direction.z * rotationSpeed * deltaTime;
                if (mouseY != 0) totalPitch += direction.x * rotationSpeed * deltaTime;

                // Optional: clamp totalPitch to avoid full vertical rotation (camera flip)
                totalPitch = std::clamp(totalPitch, -89.0f, 89.0f);  // Prevent camera flip
            }

            // Update the camera position based on keyboard input
            origin += movement * speed * deltaTime;

            // Recalculate the view matrix based on updated position and rotation
            CalculateViewMatrix();
        }

	};
}
