/*
See LICENSE file in root folder
*/
#ifndef ___Utils_COMPILER_CONFIG_H___
#define ___Utils_COMPILER_CONFIG_H___

#undef Utils_HasAlignas

#if defined( _MSC_VER )

#	define Utils_CompilerMSVC
#	define Utils_CompilerVersion _MSC_VER
#	if !defined( VLD_AVAILABLE )
#		define _CRTDBG_MAP_ALLOC
#		include <stdlib.h>
#		include <crtdbg.h>
#	endif
#	if Utils_CompilerVersion < 1900
#		error "Your compiler is too old, consider an upgrade."
#	endif

#elif defined( __clang__ )

#	define Utils_CompilerCLANG
#	define Utils_CompilerGNUC
#	if ( ! __has_feature( cxx_alignas ) || !__has_feature( cxx_nullptr ) || !__has_feature( cxx_defaulted_functions ) || !__has_feature( cxx_deleted_functions ) || !__has_feature( cxx_variadic_templates ) )
#		error "Your compiler is too old, consider an upgrade."
#	endif

#elif defined( __GNUG__ )

#	define Utils_CompilerGNUC
#	define Utils_CompilerVersion (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#	if Utils_CompilerVersion < 40900
#		error "Your compiler is too old, consider an upgrade."
#	endif

#elif defined( __BORLANDC__ )

#	define Utils_CompilerBORLAND
#	define Utils_CompilerVersion __BORLANDC__
#	warning "Theoretically supported compiler, but untested yet"
#	if Utils_CompilerVersion <= 0x621
#		error "Your compiler is too old, consider an upgrade."
#	endif

#else
#	error "Yet unsupported compiler"
#endif

#if defined( Utils_CompilerMSVC )
#	define Utils_SharedLibPrefix cuT( "")
#else
#	define Utils_SharedLibPrefix cuT( "lib")
#endif

#endif
