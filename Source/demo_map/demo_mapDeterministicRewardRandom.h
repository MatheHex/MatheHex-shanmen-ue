#pragma once
#include "CoreMinimal.h"

/** Existing reward xorshift64* stream, extracted unchanged for native products.
 * Not a global RNG: callers give each registered source/channel its own seed.
 * RangeInclusive is intended for bounded tables, not the full uint64 interval.
 */
struct Fdemo_mapDeterministicRewardRandom
{
	explicit Fdemo_mapDeterministicRewardRandom(uint64 Seed)
		: State(Seed == 0 ? 0x9E3779B97F4A7C15ull : Seed) {}
	uint64 Next()
	{
		uint64 X = State; X ^= X >> 12; X ^= X << 25; X ^= X >> 27;
		State = X; return X * 2685821657736338717ull;
	}
	uint64 RangeInclusive(uint64 Min, uint64 Max)
	{
		return Min >= Max ? Min : Min + (Next() % (Max - Min + 1));
	}
private:
	uint64 State;
};
