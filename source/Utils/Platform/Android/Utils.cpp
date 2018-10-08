#include "Utils/Config/PlatformConfig.hpp"

#if defined( Utils_PlatformAndroid )

#include "Utils/Miscellaneous/Utils.hpp"
#include "Utils/Graphics/Size.hpp"

#pragma GCC diagnostic ignored "-Wpedantic"
#include <EGL/egl.h>

namespace utils
{
	namespace System
	{
		bool getScreenSize( uint32_t p_screen, utils::Size & p_size )
		{
			bool result = false;
			auto display = eglgetDisplay( EGLNativeDisplayType( p_screen ) );

			if ( !display )
			{
				Logger::logError( "Failed to open display." );
			}
			else
			{
				auto surface = eglgetCurrentSurface( EGL_READ );

				if ( !surface )
				{
					Logger::logError( "Failed to open display's surface." );
				}
				else
				{
					int width;
					int height;
					eglQuerySurface( display, surface, EGL_WIDTH, &width );
					eglQuerySurface( display, surface, EGL_HEIGHT, &height );
					p_size = 
					{
						uint32_t( width ),
						uint32_t( height )
					};
					result = true;
				}
			}

			return result;
		}

		String getLastErrorText()
		{
			String strReturn;
			int error = errno;
			char * szError = nullptr;

			if ( error != 0 && ( szError = strerror( error ) ) != nullptr )
			{
				strReturn = string::toString( error ) + cuT( " (" ) + string::stringCast< xchar >( szError ) + cuT( ")" );
				string::replace( strReturn, cuT( "\n" ), cuT( "" ) );
			}

			return strReturn;
		}
	}

	void getLocaltime( std::tm * p_tm, time_t const * p_pTime )
	{
		*p_tm = *localtime( p_pTime );
	}
}

#endif
