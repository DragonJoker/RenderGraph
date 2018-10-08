#include "Utils/Config/PlatformConfig.hpp"

#if defined( Utils_PlatformAndroid )

#	include "Utils/Align/Aligned.hpp"
#	include "Utils/Exception/Assertion.hpp"
#	include <cstdlib>

namespace utils
{
	void * alignedAlloc( size_t p_alignment, size_t p_size )
	{
		REQUIRE( ( p_size % p_alignment ) == 0 && cuT( "size is not an integral multiple of alignment" ) );
		return malloc( p_size );
	}

	void alignedFree( void * p_memory )
	{
		free( p_memory );
	}
}

#endif
