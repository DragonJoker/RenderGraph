#include "Window.hpp"

#include <Core/Renderer.hpp>

#include <Core/Connection.hpp>
#include <Core/Device.hpp>
#include <Core/PlatformWindowHandle.hpp>
#include <Utils/DynamicLibrary.hpp>

#include <cmath>
#include <iostream>

#if ASHES_WIN32
#	include <Windows.h>
#	include <cstdio>
#	include <cstring>
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <direct.h>
#	include <Shlobj.h>
#	include <windows.h>
#elif ASHES_XLIB
#	include <X11/X.h>
#	include <X11/Xlib.h>
#	include <sys/stat.h>
#	include <unistd.h>
#	include <dirent.h>
#	include <pwd.h>
#endif

namespace test
{
#if ASHES_WIN32
	struct AppInstance::InstImpl
	{
		HINSTANCE h_instance;  // Windows Instance
		HWND h_wnd;            // window handle
	};
#elif ASHES_XCB
	struct AppInstance::InstImpl
	{
		xcb_connection_t *xcb_connection;
		xcb_screen_t *xcb_screen;
		xcb_window_t xcb_window;
	};
#elif ASHES_XLIB
	struct AppInstance::InstImpl
	{
		Display *xlib_display;
		Window xlib_window;
	};
#elif ASHES_ANDROID  // TODO
	struct AppInstance::InstImpl
	{
		ANativeWindow *window;
	};
#endif

	namespace details
	{
		template< typename T >
		size_t constexpr getArraySize( T const * const a )
		{
			return sizeof( a ) / sizeof( a[0] );
		}

		//---------------------------Win32---------------------------
#if ASHES_WIN32

	// Returns nonzero if the console is used only for this process. Will return
	// zero if another process (such as cmd.exe) is also attached.
		static int consoleIsExclusive( void )
		{
			DWORD pids[2];
			DWORD num_pids = ::GetConsoleProcessList( pids, DWORD( getArraySize( pids ) ) );
			return num_pids <= 1;
		}

	// Enlarges the console window to have a large scrollback size.
		static void consoleEnlarge( char const * const name )
		{
			const HANDLE console_handle = ::GetStdHandle( STD_OUTPUT_HANDLE );

			// make the console window bigger
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			COORD buffer_size;
			if ( ::GetConsoleScreenBufferInfo( console_handle, &csbi ) )
			{
				buffer_size.X = csbi.dwSize.X + 30;
				buffer_size.Y = 20000;
				::SetConsoleScreenBufferSize( console_handle, buffer_size );
			}

			SMALL_RECT r;
			r.Left = r.Top = 0;
			r.Right = csbi.dwSize.X - 1 + 30;
			r.Bottom = 50;
			::SetConsoleWindowInfo( console_handle, true, &r );

			// change the console window title
			::SetConsoleTitleA( name );
		}

		// MS-Windows event handling function:
		LRESULT CALLBACK wndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
		{
			return ( ::DefWindowProcA( hWnd, uMsg, wParam, lParam ) );
		}

		static void createWindow( char const * const name
			, AppInstance & inst )
		{
			inst.pimpl->h_instance = GetModuleHandle( NULL );

			WNDCLASSEXA win_class;

			// Initialize the window class structure:
			win_class.cbSize = sizeof( WNDCLASSEX );
			win_class.style = CS_HREDRAW | CS_VREDRAW;
			win_class.lpfnWndProc = wndProc;
			win_class.cbClsExtra = 0;
			win_class.cbWndExtra = 0;
			win_class.hInstance = inst.pimpl->h_instance;
			win_class.hIcon = LoadIcon( NULL, IDI_APPLICATION );
			win_class.hCursor = LoadCursor( NULL, IDC_ARROW );
			win_class.hbrBackground = ( HBRUSH )::GetStockObject( WHITE_BRUSH );
			win_class.lpszMenuName = NULL;
			win_class.lpszClassName = name;
			win_class.hInstance = inst.pimpl->h_instance;
			win_class.hIconSm = LoadIcon( NULL, IDI_WINLOGO );
			// Register window class:
			if ( !::RegisterClassExA( &win_class ) )
			{
				// It didn't work, so try to give a useful error:
				printf( "Failed to register the window class!\n" );
				fflush( stdout );
				exit( 1 );
			}
			// Create window with the registered class:
			RECT wr = { 0, 0, LONG( inst.width ), LONG( inst.height ) };
			::AdjustWindowRect( &wr, WS_OVERLAPPEDWINDOW, FALSE );
			inst.pimpl->h_wnd = ::CreateWindowExA( 0,
				name,       // class name
				name,       // app name
									  //WS_VISIBLE | WS_SYSMENU |
				WS_OVERLAPPEDWINDOW,  // window style
				100, 100,             // x/y coords
				wr.right - wr.left,   // width
				wr.bottom - wr.top,   // height
				NULL,                 // handle to parent
				NULL,                 // handle to menu
				inst.pimpl->h_instance,      // hInstance
				NULL );                // no extra parameters
			if ( !inst.pimpl->h_wnd )
			{
				// It didn't work, so try to give a useful error:
				printf( "Failed to create a window!\n" );
				fflush( stdout );
				exit( 1 );
			}
		}

		static void createDevice( AppInstance & inst )
		{
			auto connection = inst.instance->createConnection( 0u
				, ashes::WindowHandle{ std::make_unique< ashes::IMswWindowHandle >( inst.pimpl->h_instance, inst.pimpl->h_wnd ) } );
			inst.device = inst.instance->createDevice( std::move( connection ) );
		}

		static void destroyWindow( AppInstance & inst )
		{
			::DestroyWindow( inst.pimpl->h_wnd );
			inst.pimpl.reset();

			if ( consoleIsExclusive() )
			{
				::Sleep( INFINITE );
			}
		}
#endif //ASHES_WIN32
	//-----------------------------------------------------------

	//----------------------------XCB----------------------------

#if ASHES_XCB
		static void createWindow( char const * const name
			, AppInstance & inst )
		{
			//--Init Connection--
			const xcb_setup_t *setup;
			xcb_screen_iterator_t iter;
			int scr;

			// API guarantees non-null xcb_connection
			inst.pimpl->xcb_connection = xcb_connect( NULL, &scr );
			int conn_error = xcb_connection_has_error( inst.pimpl->xcb_connection );
			if ( conn_error )
			{
				fprintf( stderr, "XCB failed to connect to the X server due to error:%d.\n", conn_error );
				fflush( stderr );
				inst.pimpl->xcb_connection = NULL;
			}

			setup = xcb_get_setup( inst.pimpl->xcb_connection );
			iter = xcb_setup_roots_iterator( setup );
			while ( scr-- > 0 )
			{
				xcb_screen_next( &iter );
			}

			inst.pimpl->xcb_screen = iter.data;
			//-------------------

			inst.pimpl->xcb_window = xcb_generate_id( inst.pimpl->xcb_connection );
			xcb_create_window( inst.pimpl->xcb_connection, XCB_COPY_FROM_PARENT, inst.pimpl->xcb_window,
				inst.pimpl->xcb_screen->root, 0, 0, inst.width, inst.height, 0,
				XCB_WINDOW_CLASS_INPUT_OUTPUT, inst.pimpl->xcb_screen->root_visual,
				0, NULL );

			xcb_intern_atom_cookie_t cookie = xcb_intern_atom( inst.pimpl->xcb_connection, 1, 12, "WM_PROTOCOLS" );
			xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply( inst.pimpl->xcb_connection, cookie, 0 );
			free( reply );
		}

		static void createDevice( AppInstance & inst )
		{
			if ( !inst.pimpl->xcb_connection )
			{
				return;
			}

			auto connection = inst.instance->createConnection( 0u
				, ashes::WindowHandle{ std::make_unique< ashes::IXcbWindowHandle >( inst.pimpl->xcb_connection, inst.pimpl->xcb_window ) } );
			inst.device = inst.instance->createDevice( std::move( connection ) );
		}

		static void destroyWindow( AppInstance & inst )
		{
			if ( !inst.pimpl->xcb_connection )
			{
				return; // Nothing to destroy
			}

			xcb_destroy_window( inst.pimpl->xcb_connection, inst.pimpl->xcb_window );
			xcb_disconnect( inst.pimpl->xcb_connection );
			inst.pimpl.reset();
		}
		//VK_USE_PLATFORM_XCB_KHR
		//-----------------------------------------------------------

		//----------------------------XLib---------------------------
#elif ASHES_XLIB

		static void createWindow( char const * const name
			, AppInstance & inst )
		{
			long visualMask = VisualScreenMask;
			int numberOfVisuals;

			inst.pimpl->xlib_display = XOpenDisplay( NULL );
			if ( inst.pimpl->xlib_display == NULL )
			{
				printf( "XLib failed to connect to the X server.\nExiting ...\n" );
				fflush( stdout );
				exit( 1 );
			}

			XVisualInfo vInfoTemplate = {};
			vInfoTemplate.screen = DefaultScreen( inst.pimpl->xlib_display );
			XVisualInfo *visualInfo = XGetVisualInfo( inst.pimpl->xlib_display, visualMask,
				&vInfoTemplate, &numberOfVisuals );
			inst.pimpl->xlib_window = XCreateWindow(
				inst.pimpl->xlib_display, RootWindow( inst.pimpl->xlib_display, vInfoTemplate.screen ), 0, 0,
				inst.width, inst.height, 0, visualInfo->depth, InputOutput,
				visualInfo->visual, 0, NULL );

			XSync( inst.pimpl->xlib_display, false );
		}

		static void createDevice( AppInstance & inst )
		{
			auto connection = inst.instance->createConnection( 0u
				, ashes::WindowHandle{ std::make_unique< ashes::IXWindowHandle >( inst.pimpl->xlib_window, inst.pimpl->xlib_display ) } );
			inst.device = inst.instance->createDevice( std::move( connection ) );
		}

		static void destroyWindow( AppInstance & inst )
		{
			XDestroyWindow( inst.pimpl->xlib_display, inst.pimpl->xlib_window );
			XCloseDisplay( inst.pimpl->xlib_display );
			inst.pimpl.reset();
		}
#endif //VK_USE_PLATFORM_XLIB_KHR
	//-----------------------------------------------------------

		std::string getParentPath( std::string const & path );

#if ASHES_WIN32

		static char constexpr PathSeparator = '\\';

		namespace
		{
			template< typename DirectoryFuncType, typename FileFuncType >
			bool traverseFolder( std::string const & folderPath
				, DirectoryFuncType directoryFunction
				, FileFuncType fileFunction )
			{
				assert( !folderPath.empty() );
				bool result = false;
				WIN32_FIND_DATAA findData;
				HANDLE handle = ::FindFirstFileA( ( folderPath + PathSeparator + "*.*" ).c_str(), &findData );

				if ( handle != INVALID_HANDLE_VALUE )
				{
					result = true;
					std::string name = findData.cFileName;

					if ( name != "." && name != ".." )
					{
						if ( ( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == FILE_ATTRIBUTE_DIRECTORY )
						{
							result = directoryFunction( folderPath + PathSeparator + name );
						}
						else
						{
							fileFunction( folderPath + PathSeparator + name );
						}
					}

					while ( result && ::FindNextFileA( handle, &findData ) == TRUE )
					{
						if ( findData.cFileName != name )
						{
							name = findData.cFileName;

							if ( name != "." && name != ".." )
							{
								if ( ( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) == FILE_ATTRIBUTE_DIRECTORY )
								{
									result = directoryFunction( folderPath + PathSeparator + name );
								}
								else
								{
									fileFunction( folderPath + PathSeparator + name );
								}
							}
						}
					}

					::FindClose( handle );
				}

				return result;
			}
		}

		std::string getBinFolder()
		{
			std::string pathReturn;
			char path[FILENAME_MAX];
			DWORD result = ::GetModuleFileNameA( nullptr
				, path
				, sizeof( path ) );

			if ( result != 0 )
			{
				pathReturn = path;
			}

			pathReturn = getParentPath( pathReturn );
			return pathReturn;
		}

		bool listFolderFiles( std::string const & folderPath
			, std::vector< std::string > & files
			, bool recursive )
		{
			struct FileFunction
			{
				explicit FileFunction( std::vector< std::string > & files )
					: m_files( files )
				{
				}
				void operator()( std::string const & path )
				{
					m_files.push_back( path );
				}
				std::vector< std::string > & m_files;
			};

			if ( recursive )
			{
				struct DirectoryFunction
				{
					explicit DirectoryFunction( std::vector< std::string > & files )
						: m_files( files )
					{
					}
					bool operator()( std::string const & path )
					{
						return traverseFolder( path
							, DirectoryFunction( m_files )
							, FileFunction( m_files ) );
					}
					std::vector< std::string > & m_files;
				};

				return traverseFolder( folderPath
					, DirectoryFunction( files )
					, FileFunction( files ) );
			}
			else
			{
				struct DirectoryFunction
				{
					DirectoryFunction()
					{
					}
					bool operator()( std::string const & path )
					{
						return true;
					}
				};

				return traverseFolder( folderPath
					, DirectoryFunction()
					, FileFunction( files ) );
			}
		}

#else

		static char constexpr PathSeparator = '/';

		namespace
		{
			template< typename DirectoryFuncType, typename FileFuncType >
			bool traverseFolder( std::string const & folderPath
				, DirectoryFuncType directoryFunction
				, FileFuncType fileFunction )
			{
				assert( !folderPath.empty() );
				bool result = false;
				DIR * dir;

				if ( ( dir = opendir( folderPath.c_str() ) ) == nullptr )
				{
					switch ( errno )
					{
					case EACCES:
						std::cerr << "Can't open dir : Permission denied - Directory : " << folderPath << std::endl;
						break;

					case EBADF:
						std::cerr << "Can't open dir : Invalid file descriptor - Directory : " << folderPath << std::endl;
						break;

					case EMFILE:
						std::cerr << "Can't open dir : Too many file descriptor in use - Directory : " << folderPath << std::endl;
						break;

					case ENFILE:
						std::cerr << "Can't open dir : Too many files currently open - Directory : " << folderPath << std::endl;
						break;

					case ENOENT:
						std::cerr << "Can't open dir : Directory doesn't exist - Directory : " << folderPath << std::endl;
						break;

					case ENOMEM:
						std::cerr << "Can't open dir : Insufficient memory - Directory : " << folderPath << std::endl;
						break;

					case ENOTDIR:
						std::cerr << "Can't open dir : <name> is not a directory - Directory : " << folderPath << std::endl;
						break;

					default:
						std::cerr << "Can't open dir : Unknown error - Directory : " << folderPath << std::endl;
						break;
					}

					result = false;
				}
				else
				{
					result = true;
					dirent * dirent;

					while ( result && ( dirent = readdir( dir ) ) != nullptr )
					{
						std::string name = dirent->d_name;

						if ( name != "." && name != ".." )
						{
							if ( dirent->d_type == DT_DIR )
							{
								result = directoryFunction( folderPath + PathSeparator + name );
							}
							else
							{
								fileFunction( folderPath + PathSeparator + name );
							}
						}
					}

					closedir( dir );
				}

				return result;
			}
		}

		std::string getBinFolder()
		{
			std::string pathReturn;
			char path[FILENAME_MAX];
			char buffer[32];
			sprintf( buffer, "/proc/%d/exe", getpid() );
			int bytes = std::min< std::size_t >( readlink( buffer
				, path
				, sizeof( path ) )
				, sizeof( path ) - 1 );

			if ( bytes > 0 )
			{
				path[bytes] = '\0';
				pathReturn = path;
			}

			pathReturn = getParentPath( pathReturn );
			return pathReturn;
		}

		bool listFolderFiles( std::string const & folderPath
			, std::vector< std::string > & files
			, bool recursive )
		{
			struct FileFunction
			{
				explicit FileFunction( std::vector< std::string > & files )
					: m_files( files )
				{
				}
				void operator()( std::string const & path )
				{
					m_files.push_back( path );
				}

				std::vector< std::string > & m_files;
			};

			if ( recursive )
			{
				struct DirectoryFunction
				{
					explicit DirectoryFunction( std::vector< std::string > & files )
						: m_files( files )
					{
					}
					bool operator()( std::string const & path )
					{
						return traverseFolder( path
							, DirectoryFunction( m_files )
							, FileFunction( m_files ) );
					}
					std::vector< std::string > & m_files;
				};

				return traverseFolder( folderPath
					, DirectoryFunction( files )
					, FileFunction( files ) );
			}
			else
			{
				struct DirectoryFunction
				{
					DirectoryFunction()
					{
					}
					bool operator()( std::string const & path )
					{
						return true;
					}
				};

				return traverseFolder( folderPath
					, DirectoryFunction()
					, FileFunction( files ) );
			}
		}

#endif

		std::string getParentPath( std::string const & path )
		{
			return path.substr( 0, path.find_last_of( PathSeparator ) );
		}

		std::vector< RendererPlugin > listPlugins( RendererFactory & factory )
		{
			std::vector< std::string > files;
			std::vector< RendererPlugin > result;

			if ( listFolderFiles( getBinFolder(), files, false ) )
			{
				for ( auto file : files )
				{
					if ( file.find( ".dll" ) != std::string::npos
						|| file.find( ".so" ) != std::string::npos )
						try
					{
						ashes::DynamicLibrary lib{ file };
						result.emplace_back( std::move( lib )
							, factory );
					}
					catch ( std::exception & exc )
					{
						std::cerr << exc.what() << std::endl;
					}
				}
			}

			return result;
		}
	}

	void RendererFactory::registerType( Key const & key, Creator creator )
	{
		m_registered[key] = creator;
	}

	void RendererFactory::clear()
	{
		m_registered.clear();
	}

	RendererFactory::ObjPtr RendererFactory::create( Key const & key
		, ashes::Renderer::Configuration const & config )const
	{
		ObjPtr result;
		auto it = m_registered.find( key );

		if ( it != m_registered.end() )
		{
			result = it->second( config );
		}
		else
		{
			static std::string const Error = "Unknown object type: ";
			std::cerr << Error << "[" << key << "]" << std::endl;
			throw std::runtime_error{ Error };
		}

		return result;
	}

	RendererPlugin::RendererPlugin( ashes::DynamicLibrary && library
		, RendererFactory & factory )
		: m_library{ std::move( library ) }
		, m_creator{ nullptr }
	{
		if ( !m_library.getFunction( "createRenderer", m_creator ) )
		{
			throw std::runtime_error{ "Not a renderer plugin" };
		}

		NamerFunction shortNamer;

		if ( !m_library.getFunction( "getShortName", shortNamer ) )
		{
			throw std::runtime_error{ "Not a renderer plugin" };
		}

		NamerFunction fullNamer;

		if ( !m_library.getFunction( "getFullName", fullNamer ) )
		{
			throw std::runtime_error{ "Not a renderer plugin" };
		}

		m_shortName = shortNamer();
		m_fullName = fullNamer();

		auto creator = m_creator;
		factory.registerType( m_shortName, [creator]( ashes::Renderer::Configuration const & configuration )
			{
				return ashes::RendererPtr{ creator( configuration ) };
			} );
	}

	ashes::RendererPtr RendererPlugin::create( ashes::Renderer::Configuration const & configuration )
	{
		return ashes::RendererPtr{ m_creator( configuration ) };
	}

	void createWindow( char const * const name
		, AppInstance & inst )
	{
		inst.width = 1920;
		inst.height = 1080;
		inst.pimpl = std::make_shared< AppInstance::InstImpl >();
		details::createWindow( name, inst );
		inst.plugins = details::listPlugins( inst.factory );

		ashes::Renderer::Configuration config
		{
			name,
			name,
			false,
		};
		inst.instance = inst.factory.create( "gl4", config );

		details::createDevice( inst );
		inst.swapChain = inst.device->createSwapChain( { uint32_t( inst.width ), uint32_t( inst.height ) } );
	}

	void destroyWindow( AppInstance & inst )
	{
		inst.swapChain.reset();
		inst.device.reset();
		inst.instance.reset();
		inst.factory.clear();
		inst.plugins.clear();
		details::destroyWindow( inst );
	}
}
