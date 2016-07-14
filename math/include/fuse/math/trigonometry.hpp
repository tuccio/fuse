#pragma once

#include "constants.hpp"

namespace fuse
{

	inline float to_radians(float x)
	{
		return x * 0.01745329251f;
	}

	inline float to_degrees(float x)
	{
		return x * 57.2957795131f;
	}

}