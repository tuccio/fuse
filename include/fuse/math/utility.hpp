#pragma once

namespace fuse
{

	template <int ... N>
	struct sequence {};

	template <int M, int ... N>
	struct sequence_factory :
		sequence_factory<M - 1, M - 1, N ...> {};

	template <int ... N>
	struct sequence_factory<0, N ...>
	{
		typedef sequence<N ...> type;
	};

	template <int N>
	inline typename sequence_factory<N>::type make_sequence(void)
	{
		return sequence_factory<N>::type();
	}
	
}