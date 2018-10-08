#include "Utils/Config/PlatformConfig.hpp"

#if defined( Utils_PlatformLinux )

#include "Utils/Align/Aligned.hpp"

#include "Utils/Log/Logger.hpp"
#include "Exception/Assertion.hpp"

#	if Utils_CompilerVersion >= 50000
#		ifndef _ISOC11_SOURCE
#			define _ISOC11_SOURCE
#		endif
#		include <cstdlib>
#		define CU_ALIGNED_ALLOC( m, a, s )\
	REQUIRE( ( s % a ) == 0 && cuT( "size is not an integral multiple of alignment" ) );\
	m = aligned_alloc( a, s )
#	else
#		include <cstdlib>
#		define CU_ALIGNED_ALLOC( m, a, s )\
	int error = posix_memalign( &m, a, s );\
	if ( error )\
	{\
		if ( error == EINVAL )\
		{\
			Logger::logError( makeStringStream() << cuT( "Aligned allocation failed, alignment of " ) << a << cuT( " is not a power of two times sizeof( void * )" ) );\
		}\
		else if ( error == ENOMEM )\
		{\
			Logger::logError( makeStringStream() << cuT( "Aligned allocation failed, no memory available" ) );\
		}\
		m = nullptr;\
	}
#	endif
#	define CU_ALIGNED_FREE( m )\
	free( m )

namespace utils
{
	void * alignedAlloc( size_t p_alignment, size_t p_size )
	{
		void * mem = nullptr;
		CU_ALIGNED_ALLOC( mem, p_alignment, p_size );
		return mem;
	}

	void alignedFree( void * p_memory )
	{
		CU_ALIGNED_FREE( p_memory );
	}
}

#endif
