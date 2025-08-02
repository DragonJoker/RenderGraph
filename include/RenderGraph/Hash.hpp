/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

namespace crg
{
	template< typename T >
	inline size_t hashCombine( size_t hash
		, T const & rhs )
	{
		const uint64_t kMul = 0x9ddfea08eb382d69ULL;
		auto seed = hash;

		std::hash< T > hasher;
		uint64_t a = ( hasher( rhs ) ^ seed ) * kMul;
		a ^= ( a >> 47 );

		uint64_t b = ( seed ^ a ) * kMul;
		b ^= ( b >> 47 );

#pragma warning( push )
#pragma warning( disable: 4068 )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
		hash = static_cast< std::size_t >( b * kMul );
#pragma GCC diagnostic pop
#pragma clang diagnostic pop
#pragma warning( pop )
		return hash;
	}

}
