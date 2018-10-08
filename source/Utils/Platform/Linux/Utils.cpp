#include "Utils/Config/PlatformConfig.hpp"

#if defined( Utils_PlatformLinux )

#include "Utils/Miscellaneous/Utils.hpp"
#include "Utils/Graphics/Size.hpp"

#include <X11/Xlib.h>
#if Utils_HasXinerama
#	include <X11/extensions/Xinerama.h>
#endif

#define cpuid( func, ax, bx, cx, dx )\
	__asm__ __volatile__ ( "cpuid":\
	"=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) : "a" (func));

namespace utils
{
	namespace System
	{
#	if Utils_HasXinerama

		bool getScreenSize( uint32_t p_screen, utils::Size & p_size )
		{
			bool result = false;
			auto display = XOpenDisplay( nullptr );

			if ( !display )
			{
				Logger::logError( "Failed to open default display." );
			}
			else
			{
				auto screenIndex = DefaultScreen( display );
				int dummy1, dummy2;

				if ( XineramaQueryExtension( display, &dummy1, &dummy2 ) )
				{
					if ( XineramaIsActive( display ) )
					{
						int heads = 0;
						XineramaScreenInfo * screenInfo = XineramaQueryScreens( display, &heads );

						if ( heads > 0 && screenIndex < heads )
						{
							p_size =
							{
								uint32_t( screenInfo[screenIndex].width ),
								uint32_t( screenInfo[screenIndex].height )
							};
							result = true;
						}
						else
						{
							std::cout << "XineramaQueryScreens says there aren't any" << std::endl;
						}

						XFree( screenInfo );
					}
					else
					{
						std::cout << "Xinerama not active" << std::endl;
					}
				}
				else
				{
					std::cout << "No Xinerama extension" << std::endl;
				}

				if ( !result )
				{
					auto screen = ScreenOfDisplay( display, screenIndex );

					if ( !screen )
					{
						Logger::logError( "Failed to obtain the default screen of given display." );
					}
					else
					{
						p_size =
						{
							uint32_t( screen->width ),
							uint32_t( screen->height )
						};
						result = true;
					}
				}

				XCloseDisplay( display );
			}

			return result;
		}

#	else

		bool getScreenSize( uint32_t p_screen, utils::Size & p_size )
		{
			bool result = false;
			auto display = XOpenDisplay( nullptr );

			if ( !display )
			{
				Logger::logError( "Failed to open default display." );
			}
			else
			{
				auto screenIndex = DefaultScreen( display );
				auto screen = ScreenOfDisplay( display, screenIndex );

				if ( !screen )
				{
					Logger::logError( "Failed to obtain the default screen of given display." );
				}
				else
				{
					p_size =
					{
						uint32_t( screen->width ),
						uint32_t( screen->height )
					};
					result = true;
				}
			}

			XCloseDisplay( display );
			return result;
		}

#	endif

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

