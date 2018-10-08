#include "Utils/Miscellaneous/Utils.hpp"

#include <thread>

namespace utils
{
	namespace System
	{
		void sleep( uint32_t p_uiTime )
		{
			std::this_thread::sleep_for( Milliseconds( p_uiTime ) );
		}
	}
}
