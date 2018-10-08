/*
See LICENSE file in root folder
*/
#ifndef ___Utils_PLATFORM_CONFIG_H___
#define ___Utils_PLATFORM_CONFIG_H___

#if defined( ANDROID )
#	define Utils_PlatformAndroid
#elif defined( __linux__ )
#	define Utils_PlatformLinux
#elif defined( _WIN32 )
#	define Utils_PlatformWin32
#endif

#if defined( Utils_PlatformWin32 )
#	define Utils_SharedLibExt cuT( "dll")
#	define dlerror() ::getLastError()
#else
#	define Utils_SharedLibExt cuT( "so")
#	define CU_API
#endif

#if !defined( Utils_PlatformAndroid ) && !defined( Utils_PlatformLinux ) && !defined( Utils_PlatformWin32 )
#	error "Yet unsupported OS"
#endif

#endif
