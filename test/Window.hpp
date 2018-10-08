#include <AshesPrerequisites.hpp>

#include <Core/Device.hpp>
#include <Core/Renderer.hpp>
#include <Core/SwapChain.hpp>
#include <Utils/DynamicLibrary.hpp>

#include <iostream>
#include <stdexcept>

namespace test
{
	class RendererFactory
	{
	protected:
		using Key = std::string;
		using PtrType = ashes::RendererPtr;
		using Creator = std::function<ashes::RendererPtr( const ashes::Renderer::Configuration& ) >;
		using ObjPtr = PtrType;
		using ObjMap = std::map< Key, Creator >;

	public:
		void registerType( Key const & key, Creator creator );
		void clear();
		ObjPtr create( Key const & key
			, ashes::Renderer::Configuration const & config )const;

	private:
		ObjMap m_registered;
	};

	class RendererPlugin
	{
		using CreatorFunction = ashes::Renderer *( *)( ashes::Renderer::Configuration const & );
		using NamerFunction = char const *( *)( );

	public:
		RendererPlugin( ashes::DynamicLibrary && library
			, RendererFactory & factory );
		ashes::RendererPtr create( ashes::Renderer::Configuration const & configuration );

	private:
		ashes::DynamicLibrary m_library;
		CreatorFunction m_creator;
		std::string m_shortName;
		std::string m_fullName;
	};

	struct AppInstance
	{
		RendererFactory factory;
		std::vector< RendererPlugin > plugins;
		ashes::RendererPtr instance;
		ashes::DevicePtr device;
		ashes::SwapChainPtr swapChain;
		uint32_t width, height;
		struct InstImpl;
		std::shared_ptr< InstImpl > pimpl;
	};

	void createWindow( char const * const name
		, AppInstance & inst );
	void destroyWindow( AppInstance & inst );
}

#define testSuiteBegin( name, inst )\
	auto result = EXIT_SUCCESS;\
	test::createWindow( "TestAttachment", inst );\
	try\
	{\

#define testSuiteEnd()\
	}\
	catch ( std::exception & exc )\
	{\
		std::cout << "Test " << exc.what() << std::endl;\
		result = EXIT_FAILURE;\
	}\
	catch ( ... )\
	{\
		std::cout << "Test failed : Unhandled exception." << std::endl;\
		result = EXIT_FAILURE;\
	}\
	test::destroyWindow( inst );\
	return result;

#define testBegin( name )\
	std::string testName = name;\
	uint32_t errCount = 0u;\
	try\
	{\

#define testEnd()\
	}\
	catch ( std::exception & exc )\
	{\
		throw std::runtime_error{ testName + " Failed : " + exc.what() };\
	}\
	catch ( ... )\
	{\
		throw std::runtime_error{ testName + " Failed : Unknown unhandled exception" };\
	}

#define check( x )\
	try\
	{\
		if ( !( x ) )\
		{\
			throw std::runtime_error{ #x##" failed" };\
		}\
	}\
	catch ( ... )\
	{\
		throw std::runtime_error{ #x##" failed" };\
	}

#define checkEqual( x, y )\
	try\
	{\
		if ( ( x ) != ( y ) )\
		{\
			throw std::runtime_error{ #x##" == "##y##" failed" };\
		}\
	}\
	catch ( ... )\
	{\
		throw std::runtime_error{ #x##" == "##y##" failed" };\
	}

#define checkNotEqual( x, y )\
	try\
	{\
		if ( ( x ) == ( y ) )\
		{\
			throw std::runtime_error{ #x##" != "##y##" failed" };\
		}\
	}\
	catch ( ... )\
	{\
		throw std::runtime_error{ #x##" != "##y##" failed" };\
	}

#define checkThrow( x )\
	try\
	{\
		( x ); \
		throw std::runtime_error{ #x##" failed" };\
	}\
	catch ( ... )\
	{\
	}

#define checkNoThrow( x )\
	try\
	{\
		( x ); \
	}\
	catch ( ... )\
	{\
		throw std::runtime_error{ #x##" failed" };\
	}
