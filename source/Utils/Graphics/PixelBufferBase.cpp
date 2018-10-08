#include "Utils/Graphics/PixelBuffer.hpp"

namespace utils
{
	PxBufferBase::PxBufferBase( Size const & p_size, ashes::Format p_format )
		: m_pixelFormat( p_format )
		, m_size( p_size )
		, m_buffer( 0 )
	{
	}

	PxBufferBase::PxBufferBase( PxBufferBase const & p_pixelBuffer )
		: m_pixelFormat( p_pixelBuffer.m_pixelFormat )
		, m_size( p_pixelBuffer.m_size )
		, m_buffer( 0 )
	{
	}

	PxBufferBase::~PxBufferBase()
	{
	}

	PxBufferBase & PxBufferBase::operator=( PxBufferBase const & p_pixelBuffer )
	{
		clear();
		m_size = p_pixelBuffer.m_size;
		m_pixelFormat = p_pixelBuffer.m_pixelFormat;
		initialise( p_pixelBuffer.m_buffer.data(), p_pixelBuffer.m_pixelFormat );
		return * this;
	}

	void PxBufferBase::clear()
	{
		m_buffer.clear();
	}

	void PxBufferBase::initialise( uint8_t const * p_buffer, ashes::Format p_bufferFormat )
	{
		uint8_t bpp = PF::getBytesPerPixel( format() );
		uint32_t newSize = count() * bpp;
		m_buffer.resize( newSize );

		if ( p_buffer == nullptr )
		{
			memset( m_buffer.data(), 0, newSize );
		}
		else
		{
			if ( p_bufferFormat == m_pixelFormat )
			{
				memcpy( m_buffer.data(), p_buffer, newSize );
			}
			else
			{
				PF::convertBuffer( p_bufferFormat
					, p_buffer
					, count() * PF::getBytesPerPixel( p_bufferFormat )
					, format()
					, m_buffer.data()
					, size() );
			}
		}
	}

	void PxBufferBase::initialise( Size const & p_size )
	{
		m_size = p_size;
		initialise( nullptr, ashes::Format::eR8G8B8A8_UNORM );
	}

	void PxBufferBase::swap( PxBufferBase & p_pixelBuffer )
	{
		std::swap( m_size, p_pixelBuffer.m_size );
		std::swap( m_pixelFormat, p_pixelBuffer.m_pixelFormat );
		std::swap( m_buffer, p_pixelBuffer.m_buffer );
	}

	void PxBufferBase::flip()
	{
		uint32_t fwidth = getWidth() * PF::getBytesPerPixel( m_pixelFormat );
		uint32_t fheight = getHeight();
		uint32_t hheight = fheight / 2;
		uint8_t * bufferTop = &m_buffer[0];
		uint8_t * bufferBot = &m_buffer[( fheight - 1 ) * fwidth - 1];
		std::vector< uint8_t > buffer( fwidth );

		for ( uint32_t i = 0; i < hheight; i++ )
		{
			auto topLine = bufferTop;
			auto botLine = bufferBot;
			std::memcpy( buffer.data(), bufferTop, fwidth );
			std::memcpy( bufferTop, bufferBot, fwidth );
			std::memcpy( bufferBot, buffer.data(), fwidth );
			bufferTop += fwidth;
			bufferBot -= fwidth;
		}
	}

	PxBufferBaseSPtr PxBufferBase::create( Size const & p_size, ashes::Format p_eWantedFormat, uint8_t const * p_buffer, ashes::Format p_eBufferFormat )
	{
		PxBufferBaseSPtr result;

		switch ( p_eWantedFormat )
		{
		case ashes::Format::eR8_UNORM:
			result = std::make_shared< PxBuffer< ashes::Format::eR8_UNORM > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eR32_SFLOAT:
			result = std::make_shared< PxBuffer< ashes::Format::eR32_SFLOAT > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eR8G8_UNORM:
			result = std::make_shared< PxBuffer< ashes::Format::eR8G8_UNORM > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eR32G32_SFLOAT:
			result = std::make_shared< PxBuffer< ashes::Format::eR32G32_SFLOAT > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eA1R5G5B5_UNORM_PACK16:
			result = std::make_shared< PxBuffer< ashes::Format::eA1R5G5B5_UNORM_PACK16 > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eR5G6B5_UNORM_PACK16:
			result = std::make_shared< PxBuffer< ashes::Format::eR5G6B5_UNORM_PACK16 > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eR8G8B8_UNORM:
			result = std::make_shared< PxBuffer< ashes::Format::eR8G8B8_UNORM > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eB8G8R8_UNORM:
			result = std::make_shared< PxBuffer< ashes::Format::eB8G8R8_UNORM > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eR8G8B8_SRGB:
			result = std::make_shared< PxBuffer< ashes::Format::eR8G8B8_SRGB > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eB8G8R8_SRGB:
			result = std::make_shared< PxBuffer< ashes::Format::eB8G8R8_SRGB > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eR8G8B8A8_UNORM:
			result = std::make_shared< PxBuffer< ashes::Format::eR8G8B8A8_UNORM > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eB8G8R8A8_UNORM:
			result = std::make_shared< PxBuffer< ashes::Format::eB8G8R8A8_UNORM > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eR8G8B8A8_SRGB:
			result = std::make_shared< PxBuffer< ashes::Format::eR8G8B8A8_SRGB > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eB8G8R8A8_SRGB:
			result = std::make_shared< PxBuffer< ashes::Format::eB8G8R8A8_SRGB > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eR16G16B16_SFLOAT:
			result = std::make_shared< PxBuffer< ashes::Format::eR16G16B16_SFLOAT > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eR16G16B16A16_SFLOAT:
			result = std::make_shared< PxBuffer< ashes::Format::eR16G16B16A16_SFLOAT > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eR32G32B32_SFLOAT:
			result = std::make_shared< PxBuffer< ashes::Format::eR32G32B32_SFLOAT > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eR32G32B32A32_SFLOAT:
			result = std::make_shared< PxBuffer< ashes::Format::eR32G32B32A32_SFLOAT > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eD16_UNORM:
			result = std::make_shared< PxBuffer< ashes::Format::eD16_UNORM > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eD24_UNORM_S8_UINT:
			result = std::make_shared< PxBuffer< ashes::Format::eD24_UNORM_S8_UINT > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eD32_SFLOAT:
			result = std::make_shared< PxBuffer< ashes::Format::eD32_SFLOAT > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eD32_SFLOAT_S8_UINT:
			result = std::make_shared< PxBuffer< ashes::Format::eD32_SFLOAT_S8_UINT > >( p_size, p_buffer, p_eBufferFormat );
			break;

		case ashes::Format::eS8_UINT:
			result = std::make_shared< PxBuffer< ashes::Format::eS8_UINT > >( p_size, p_buffer, p_eBufferFormat );
			break;
		}

		return result;
	}
}
