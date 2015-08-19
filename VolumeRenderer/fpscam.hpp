
///////////////////////////////////////////////////////////////////////////////////////
// Camera Utilities
///////////////////////////////////////////////////////////////////////////////////////
namespace fpscam {

#define mat4_type			glm::detail::tmat4x4<T/*, glm::highp - This is for a higher version of GLM.*/>
#define vec4_type			glm::detail::tvec4<T/*, glm::highp*/>
#define vec3_type			glm::detail::tvec3<T/*, glm::highp*/>

	template<typename T>
	void MoveForward(T amount, bool inParallelWithGround, mat4_type &viewMatrix)
	{
		if (inParallelWithGround)
		{
			vec4_type moveVec = vec4_type(viewMatrix[0][2], viewMatrix[1][2], 0, 0);

			// if looking straight down or up such that you can't move forward, do something smart
			if( glm::dot(moveVec, moveVec) < T(FLT_EPSILON) )
				moveVec = -vec4_type(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1], 0);

			viewMatrix[3] += viewMatrix * (glm::normalize(moveVec) * amount);
		}
		else
		{
			viewMatrix[3].z += amount;
		}
	}

	template<typename T>
	void StrafeRight(T amount, bool inParallelWithGround, mat4_type & viewMatrix)
	{
		if (inParallelWithGround)
		{
			vec4_type moveVec = -vec4_type(viewMatrix[0][0], viewMatrix[1][0], 0, 0);

			// if rolled 90 degrees or equivalent such that you can't move right, do something smart
			if( glm::dot(moveVec, moveVec) < T(FLT_EPSILON) )
				moveVec = -vec4_type(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1], 0);

			viewMatrix[3] += viewMatrix * (glm::normalize(moveVec) * amount);
		}
		else
		{
			viewMatrix[3].x += amount;
		}
	}

	template<typename T>
	void MoveUp(T amount, bool inWorldUp, mat4_type & viewMatrix)
	{
		vec4_type moveVec = inWorldUp ? -amount * viewMatrix[2] : vec4_type(0, amount, 0, 0);
		viewMatrix[3] += moveVec;
	}

	template<typename T>
	void LookRight(T degrees, bool rotateAboutWorldUp, mat4_type & viewMatrix)
	{
		vec4_type rotVec = rotateAboutWorldUp ? viewMatrix[2] : vec4_type(0, 0, 1, 0);
		viewMatrix = glm::rotate(mat4_type(1), degrees, vec3_type(rotVec)) * viewMatrix;
	}

	template<typename T>
	void LookUp(T degrees, bool inAllowUpsideDown, mat4_type & viewMatrix)
	{
		// camera's up vector in world space.
		vec3_type up = vec3_type(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);

		mat4_type rotMat = glm::rotate(mat4_type(1), -degrees, vec3_type(1, 0, 0));

		// the result of looking up, which could lead to an upside down view.
		mat4_type rotatedViewMatrix = rotMat * viewMatrix;

		if ( !inAllowUpsideDown && rotatedViewMatrix[2][1] < 0)
		{
			// Result of lookup would be upside down.
			// Recalculate the rotation angle so that the camera does not flip upside down.

			// up vector projected on the the world XY plane
			vec3_type projectedUp = glm::normalize(vec3_type(up.x, up.y, 0));;

			// camera's right vector in world space coords.
			vec3_type right = vec3_type(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);

			// complicated math..
			vec3_type cross = glm::cross(vec3_type(0, 0, 1), right);
			T sign = glm::sign(up.z) * ((glm::dot(cross, up) > 0) ? 1 : -1);

			// Rebuild rotatedViewMatrix with a better 'degrees'
			degrees = glm::degrees(glm::angle(up, projectedUp));

			rotMat = glm::rotate(mat4_type(1), sign*degrees, vec3_type(1,0,0));
			rotatedViewMatrix = rotMat * viewMatrix;
		}

		viewMatrix = rotatedViewMatrix;
	}

	template<typename T>
	void OrbitRight(const vec3_type & inOrigin, T inDegree, mat4_type & inOutViewMatrix)
	{
		mat4_type cameraMat = glm::affineInverse(inOutViewMatrix);
		mat4_type translateMat = glm::translate(mat4_type(1), inOrigin);
		mat4_type rotMat = glm::rotate(mat4_type(1), inDegree, vec3_type(0, 0, 1));
		cameraMat = translateMat * rotMat * glm::affineInverse(translateMat) * cameraMat;

		inOutViewMatrix = glm::affineInverse(cameraMat);
	}

	template<typename T>
	void OrbitUp(const vec3_type & inOrigin, T inDegree, mat4_type & inOutViewMatrix)
	{
		mat4_type cameraMat = glm::affineInverse(inOutViewMatrix);
		mat4_type translateMat = glm::translate(mat4_type(1), inOrigin);
		mat4_type rotMat = glm::rotate(mat4_type(1), inDegree, vec3_type(cameraMat[0]));
		cameraMat = translateMat * rotMat * glm::affineInverse(translateMat) * cameraMat;

		inOutViewMatrix = glm::affineInverse(cameraMat);
	}
}; // end namespace fpscam

