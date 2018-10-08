#include "Utils/Config/PlatformConfig.hpp"

#if defined( Utils_PlatformAndroid )

#include "Utils/Data/Path.hpp"
#include "Utils/Graphics/Image.hpp"
#include "Utils/Graphics/Rectangle.hpp"
#include "Utils/Log/Logger.hpp"
#include "Utils/Miscellaneous/StringUtils.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace utils
{
	namespace
	{
		PxBufferBaseSPtr doLoad8BitsPerChannel( Path const & p_path )
		{
			PxBufferBaseSPtr result;
			int x = 0;
			int y = 0;
			int n = 0;
			uint8_t * data = stbi_load( string::stringCast< char >( p_path ).c_str()
				, &x
				, &y
				, &n
				, 0 );

			if ( data )
			{
				ashes::Format format;

				switch ( n )
				{
				case 1:
					format = ashes::Format::eR8_UNORM;
					break;

				case 2:
					format = ashes::Format::eR8G8_UNORM;
					break;

				case 3:
					format = ashes::Format::eR8G8B8_UNORM;
					break;

				case 4:
					format = ashes::Format::eR8G8B8A8_UNORM;
					break;
				}

				result = PxBufferBase::create( Size( uint32_t( x ), uint32_t( y ) )
					, format
					, data
					, format );
				stbi_image_free( data );
			}

			return result;
		}

		PxBufferBaseSPtr doLoad32BitsPerChannel( Path const & p_path )
		{
			PxBufferBaseSPtr result;
			int x = 0;
			int y = 0;
			int n = 0;
			float * data = stbi_loadf( string::stringCast< char >( p_path ).c_str()
				, &x
				, &y
				, &n
				, 0 );

			if ( data )
			{
				ashes::Format format;

				switch ( n )
				{
				case 1:
					format = ashes::Format::eR32_SFLOAT;
					break;

				case 2:
					format = ashes::Format::eR32G32_SFLOAT;
					break;

				case 3:
					format = ashes::Format::eR32G32B32_SFLOAT;
					break;

				case 4:
					format = ashes::Format::eR32G32B32A32_SFLOAT;
					break;
				}

				result = PxBufferBase::create( Size( uint32_t( x ), uint32_t( y ) )
					, format
					, reinterpret_cast< uint8_t * >( data )
					, format );
				stbi_image_free( data );
			}

			return result;
		}
	}

	//************************************************************************************************

	Image::BinaryLoader::BinaryLoader()
	{
	}

	bool Image::BinaryLoader::operator()( Image & p_image, Path const & p_path )
	{
		if ( p_path.empty() )
		{
			LOADER_ERROR( "Can't load image : path is empty" );
		}

		p_image.m_buffer.reset();
		auto extension = string::upperCase( p_path.getExtension() );

		if ( extension.find( cuT( "hdr" ) ) != String::npos )
		{
			p_image.m_buffer = doLoad32BitsPerChannel( p_path );
		}
		else
		{
			p_image.m_buffer = doLoad8BitsPerChannel( p_path );
		}

		return p_image.m_buffer != nullptr;
	}

	//************************************************************************************************

	Image::BinaryWriter::BinaryWriter()
	{
	}

	bool Image::BinaryWriter::operator()( Image const & p_image, Path const & p_path )
	{
		bool result = false;

		return result;
	}

	//************************************************************************************************

	Image & Image::resample( Size const & p_size )
	{
		return *this;
	}

	void Image::initialiseImageLib()
	{
	}

	void Image::cleanupImageLib()
	{
	}

	//************************************************************************************************
}

#endif
