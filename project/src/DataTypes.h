#pragma once
#include "Maths.h"
#include <vector>

namespace dae
{
	struct Plane
	{
		Vector3 normal = { 0.f, 1.f, 0.f };

		// distance from origin to the nearest point in the plane
		float  distance = 0.f;
	};

	struct Vertex_PosCol
	{
		dae::Vector3 position{ };
		dae::Vector2 uv{ };
		dae::Vector3 normal{ };
		dae::Vector3 tangent{ };
	};

	struct Frustum
	{
		Plane topFace;
		Plane bottomFace;

		Plane rightFace;
		Plane leftFace;

		Plane farFace;
		Plane nearFace;

		bool IsInsideFrustum(const Vector4& transformedPosition) const
		{
			if (transformedPosition.x < -1.0f || transformedPosition.x > 1.0f ||
				transformedPosition.y < -1.0f || transformedPosition.y > 1.0f ||
				transformedPosition.z < 0.0f || transformedPosition.z > 1.0f)
			{
				return false; // Outside frustum
			}

			// Loop through all six planes of the frustum
			for (const Plane& plane : { nearFace, farFace,
										leftFace, rightFace,
										topFace, bottomFace })
			{
				// Compute the distance of the point from the plane
				float distance = Vector3::Dot(plane.normal, Vector3(transformedPosition.x,
					transformedPosition.y,
					transformedPosition.z)) + plane.distance;

				 //If the point is outside any plane, it's outside the frustum
				if (distance < 0.0f)
				{
					return false;
				}
			}

			// The point is inside all planes of the frustum
			return true;

		}
	};

	struct Vertex_In
	{
		dae::Vector3 position{ };
		dae::Vector2 uv{ };
		dae::Vector3 normal{ };
		dae::Vector3 tangent{ };
	};

	struct Vertex_Out
	{
		Vector4 position{};
		Vector2 uv{};
		Vector3 normal{};
		Vector3 tangent{};
		Vector3 viewDirection{};
	};

	enum class PrimitiveTopology
	{
		TriangleList,
		TriangleStrip
	};

	enum RenderMethod
	{
		DirectX,
		Software
	};

	enum CullMode
	{
		Back,
		Front,
		None
	};

	enum ShadingMode
	{
		Combined,
		Observed,
		Diffuse,
		Specular
	};

	struct Triangle
	{
		std::vector<Vertex_Out> vertices;
	};
}