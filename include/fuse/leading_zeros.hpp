#pragma once

#include <cstdint>

#include <intrin.h>

#include <xmmintrin.h>
#include <mmintrin.h>
#include <pmmintrin.h>
#include <smmintrin.h>

#define FUSE_BSR_INVALID (0xFFFFFFFF)

namespace fuse
{

	inline uint32_t leading_zeros(uint64_t x)
	{

		union
		{
			uint64_t ui64;
			uint32_t ui32[2];
		} t = { x };

		unsigned long bsrHigh;
		unsigned long bsrLow;

		unsigned char okHigh = _BitScanReverse(&bsrHigh, t.ui32[1]);
		unsigned char okLow  = _BitScanReverse(&bsrLow, t.ui32[0]);

		unsigned int lz;

		if (okHigh)
		{
			lz = 31 - bsrHigh;
		}
		else if (okLow)
		{
			lz = 63 - bsrLow;
		}
		else
		{
			lz = 64;
		}

		return lz;

	}

	inline uint32_t most_significant_one(uint64_t x)
	{

		union
		{
			uint64_t ui64;
			uint32_t ui32[2];
		} t = { x };

		unsigned long bsrHigh;
		unsigned long bsrLow;

		unsigned char okHigh = _BitScanReverse(&bsrHigh, t.ui32[1]);
		unsigned char okLow  = _BitScanReverse(&bsrLow, t.ui32[0]);

		unsigned int bsr;

		if (okHigh)
		{
			bsr = bsrHigh + 32;
		}
		else if (okLow)
		{
			bsr = bsrLow;
		}
		else
		{
			bsr = FUSE_BSR_INVALID;
		}

		return bsr;

	}

}