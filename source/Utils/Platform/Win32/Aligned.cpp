#include "Utils/Config/PlatformConfig.hpp"

#if defined( Utils_PlatformWin32 )

#include "Utils/Align/Aligned.hpp"

#include "Utils/Log/Logger.hpp"
#include "Utils/Exception/Assertion.hpp"

#	include <malloc.h>
#	define CU_ALIGNED_ALLOC( m, a, s )\
	m = _aligned_malloc( s, a )
#	define CU_ALIGNED_FREE( m )\
	_aligned_free( m )

namespace utils
{
	void * alignedAlloc( size_t p_alignment, size_t p_size )
	{
		void * mem;
		CU_ALIGNED_ALLOC( mem, p_alignment, p_size );
		return mem;
	}

	void alignedFree( void * p_memory )
	{
		CU_ALIGNED_FREE( p_memory );
	}
}

#endif
