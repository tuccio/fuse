#pragma once

namespace fuse
{

	/* Absolute value */

	inline float absolute(float a)
	{
		*(((int*)&a)) &= 0x7FFFFFFF;
		return a;
	}

	inline int absolute(int a)
	{
		// http://graphics.stanford.edu/~seander/bithacks.html#IntegerAbs
		const int mask = a >> (sizeof(int) * CHAR_BIT - 1);
		return (a + mask) ^ mask;
	}

	/* Square root / Inverse */

	inline float inverse_square_root(float x)
	{
		float halfx = .5f * x;
		unsigned int i = *((unsigned int*)&x);
		i = 0x5f3759dfu - (i >> 1);
		float yi = *((float*)&i);
		float y0 = yi * (1.5f - (halfx * yi * yi));
		return y0;
	}

	inline float square_root(float x)
	{
		return x * inverse_square_root(x);
	}

}