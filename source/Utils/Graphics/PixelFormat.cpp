#include "Utils/Graphics/PixelFormat.hpp"
#include "Utils/Graphics/PixelBuffer.hpp"

namespace utils
{
	namespace PF
	{
		ashes::Format getPFWithoutAlpha( ashes::Format p_format )
		{
			ashes::Format result = ashes::Format::eRange;

			switch ( p_format )
			{
			case ashes::Format::eR8G8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8G8_UNORM >::NoAlphaPF;
				break;

			case ashes::Format::eR32G32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32G32_SFLOAT >::NoAlphaPF;
				break;

			case ashes::Format::eA1R5G5B5_UNORM_PACK16:
				result = PixelDefinitions< ashes::Format::eA1R5G5B5_UNORM_PACK16 >::NoAlphaPF;
				break;

			case ashes::Format::eR8G8B8A8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8G8B8A8_UNORM >::NoAlphaPF;
				break;

			case ashes::Format::eB8G8R8A8_UNORM:
				result = PixelDefinitions< ashes::Format::eB8G8R8A8_UNORM >::NoAlphaPF;
				break;

			case ashes::Format::eR8G8B8A8_SRGB:
				result = PixelDefinitions< ashes::Format::eR8G8B8A8_UNORM >::NoAlphaPF;
				break;

			case ashes::Format::eB8G8R8A8_SRGB:
				result = PixelDefinitions< ashes::Format::eB8G8R8A8_UNORM >::NoAlphaPF;
				break;

			case ashes::Format::eR32G32B32A32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32G32B32A32_SFLOAT >::NoAlphaPF;
				break;

			default:
				result = p_format;
				break;
			}

			return result;
		}

		bool hasAlpha( ashes::Format p_format )
		{
			bool result = false;

			switch ( p_format )
			{
			case ashes::Format::eR8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8_UNORM >::Alpha;
				break;

			case ashes::Format::eR16_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR16_SFLOAT >::Alpha;
				break;

			case ashes::Format::eR32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32_SFLOAT >::Alpha;
				break;

			case ashes::Format::eR8G8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8G8_UNORM >::Alpha;
				break;

			case ashes::Format::eR32G32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32G32_SFLOAT >::Alpha;
				break;

			case ashes::Format::eA1R5G5B5_UNORM_PACK16:
				result = PixelDefinitions< ashes::Format::eA1R5G5B5_UNORM_PACK16 >::Alpha;
				break;

			case ashes::Format::eR5G6B5_UNORM_PACK16:
				result = PixelDefinitions< ashes::Format::eR5G6B5_UNORM_PACK16 >::Alpha;
				break;

			case ashes::Format::eR8G8B8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8G8B8_UNORM >::Alpha;
				break;

			case ashes::Format::eB8G8R8_UNORM:
				result = PixelDefinitions< ashes::Format::eB8G8R8_UNORM >::Alpha;
				break;

			case ashes::Format::eR8G8B8_SRGB:
				result = PixelDefinitions< ashes::Format::eR8G8B8_SRGB >::Alpha;
				break;

			case ashes::Format::eB8G8R8_SRGB:
				result = PixelDefinitions< ashes::Format::eB8G8R8_SRGB >::Alpha;
				break;

			case ashes::Format::eR8G8B8A8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8G8B8A8_UNORM >::Alpha;
				break;

			case ashes::Format::eB8G8R8A8_UNORM:
				result = PixelDefinitions< ashes::Format::eB8G8R8A8_UNORM >::Alpha;
				break;

			case ashes::Format::eR8G8B8A8_SRGB:
				result = PixelDefinitions< ashes::Format::eR8G8B8A8_SRGB >::Alpha;
				break;

			case ashes::Format::eB8G8R8A8_SRGB:
				result = PixelDefinitions< ashes::Format::eB8G8R8A8_SRGB >::Alpha;
				break;

			case ashes::Format::eR16G16B16_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR16G16B16_SFLOAT >::Alpha;
				break;

			case ashes::Format::eR16G16B16A16_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR16G16B16A16_SFLOAT >::Alpha;
				break;

			case ashes::Format::eR32G32B32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32G32B32_SFLOAT >::Alpha;
				break;

			case ashes::Format::eR32G32B32A32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32G32B32A32_SFLOAT >::Alpha;
				break;

			case ashes::Format::eD16_UNORM:
				result = PixelDefinitions< ashes::Format::eD16_UNORM >::Alpha;
				break;

			case ashes::Format::eD24_UNORM_S8_UINT:
				result = PixelDefinitions< ashes::Format::eD24_UNORM_S8_UINT >::Alpha;
				break;

			case ashes::Format::eD32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eD32_SFLOAT >::Alpha;
				break;

			case ashes::Format::eD32_SFLOAT_S8_UINT:
				result = PixelDefinitions< ashes::Format::eD32_SFLOAT_S8_UINT >::Alpha;
				break;

			case ashes::Format::eS8_UINT:
				result = PixelDefinitions< ashes::Format::eS8_UINT >::Alpha;
				break;

			default:
				result = false;
				break;
			}

			return result;
		}

		bool isCompressed( ashes::Format p_format )
		{
			bool result = false;

			switch ( p_format )
			{
			case ashes::Format::eR8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8_UNORM >::Compressed;
				break;

			case ashes::Format::eR16_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR16_SFLOAT >::Compressed;
				break;

			case ashes::Format::eR32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32_SFLOAT >::Compressed;
				break;

			case ashes::Format::eR8G8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8G8_UNORM >::Compressed;
				break;

			case ashes::Format::eR32G32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32G32_SFLOAT >::Compressed;
				break;

			case ashes::Format::eA1R5G5B5_UNORM_PACK16:
				result = PixelDefinitions< ashes::Format::eA1R5G5B5_UNORM_PACK16 >::Compressed;
				break;

			case ashes::Format::eR5G6B5_UNORM_PACK16:
				result = PixelDefinitions< ashes::Format::eR5G6B5_UNORM_PACK16 >::Compressed;
				break;

			case ashes::Format::eR8G8B8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8G8B8_UNORM >::Compressed;
				break;

			case ashes::Format::eB8G8R8_UNORM:
				result = PixelDefinitions< ashes::Format::eB8G8R8_UNORM >::Compressed;
				break;

			case ashes::Format::eR8G8B8_SRGB:
				result = PixelDefinitions< ashes::Format::eR8G8B8_SRGB >::Compressed;
				break;

			case ashes::Format::eB8G8R8_SRGB:
				result = PixelDefinitions< ashes::Format::eB8G8R8_SRGB >::Compressed;
				break;

			case ashes::Format::eR8G8B8A8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8G8B8A8_UNORM >::Compressed;
				break;

			case ashes::Format::eB8G8R8A8_UNORM:
				result = PixelDefinitions< ashes::Format::eB8G8R8A8_UNORM >::Compressed;
				break;

			case ashes::Format::eR8G8B8A8_SRGB:
				result = PixelDefinitions< ashes::Format::eR8G8B8A8_SRGB >::Compressed;
				break;

			case ashes::Format::eB8G8R8A8_SRGB:
				result = PixelDefinitions< ashes::Format::eB8G8R8A8_SRGB >::Compressed;
				break;

			case ashes::Format::eR16G16B16_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR16G16B16_SFLOAT >::Compressed;
				break;

			case ashes::Format::eR16G16B16A16_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR16G16B16A16_SFLOAT >::Compressed;
				break;

			case ashes::Format::eR32G32B32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32G32B32_SFLOAT >::Compressed;
				break;

			case ashes::Format::eR32G32B32A32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32G32B32A32_SFLOAT >::Compressed;
				break;

			case ashes::Format::eD16_UNORM:
				result = PixelDefinitions< ashes::Format::eD16_UNORM >::Compressed;
				break;

			case ashes::Format::eD24_UNORM_S8_UINT:
				result = PixelDefinitions< ashes::Format::eD24_UNORM_S8_UINT >::Compressed;
				break;

			case ashes::Format::eD32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eD32_SFLOAT >::Compressed;
				break;

			case ashes::Format::eD32_SFLOAT_S8_UINT:
				result = PixelDefinitions< ashes::Format::eD32_SFLOAT_S8_UINT >::Compressed;
				break;

			case ashes::Format::eS8_UINT:
				result = PixelDefinitions< ashes::Format::eS8_UINT >::Compressed;
				break;

			default:
				result = false;
				break;
			}

			return result;
		}

		PxBufferBaseSPtr extractAlpha( PxBufferBaseSPtr & p_pSrc )
		{
			PxBufferBaseSPtr result;

			if ( hasAlpha( p_pSrc->format() ) )
			{
				result = PxBufferBase::create( p_pSrc->dimensions(), ashes::Format::eR8G8_UNORM, p_pSrc->constPtr(), p_pSrc->format() );
				uint8_t * pBuffer = result->ptr();

				for ( uint32_t i = 0; i < result->size(); i += 2 )
				{
					pBuffer[0] = pBuffer[1];
					pBuffer++;
					pBuffer++;
				}

				result = PxBufferBase::create( p_pSrc->dimensions(), ashes::Format::eR8_UNORM, result->constPtr(), result->format() );
				p_pSrc = PxBufferBase::create( p_pSrc->dimensions(), getPFWithoutAlpha( p_pSrc->format() ), p_pSrc->constPtr(), p_pSrc->format() );
			}

			return result;
		}

		void reduceToAlpha( PxBufferBaseSPtr & p_pSrc )
		{
			if ( hasAlpha( p_pSrc->format() ) )
			{
				if ( p_pSrc->format() != ashes::Format::eR8G8_UNORM )
				{
					p_pSrc = PxBufferBase::create( p_pSrc->dimensions(), ashes::Format::eR8G8_UNORM, p_pSrc->constPtr(), p_pSrc->format() );
				}

				uint8_t * pBuffer = p_pSrc->ptr();

				for ( uint32_t i = 0; i < p_pSrc->size(); i += 2 )
				{
					pBuffer[0] = pBuffer[1];
					pBuffer++;
					pBuffer++;
				}
			}

			p_pSrc = PxBufferBase::create( p_pSrc->dimensions(), ashes::Format::eR8_UNORM, p_pSrc->constPtr(), p_pSrc->format() );
		}

		uint8_t getBytesPerPixel( ashes::Format p_format )
		{
			uint8_t result = 0;

			switch ( p_format )
			{
			case ashes::Format::eR8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8_UNORM >::Size;
				break;

			case ashes::Format::eR16_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR16_SFLOAT >::Size;
				break;

			case ashes::Format::eR32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32_SFLOAT >::Size;
				break;

			case ashes::Format::eR8G8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8G8_UNORM >::Size;
				break;

			case ashes::Format::eR32G32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32G32_SFLOAT >::Size;
				break;

			case ashes::Format::eA1R5G5B5_UNORM_PACK16:
				result = PixelDefinitions< ashes::Format::eA1R5G5B5_UNORM_PACK16 >::Size;
				break;

			case ashes::Format::eR5G6B5_UNORM_PACK16:
				result = PixelDefinitions< ashes::Format::eR5G6B5_UNORM_PACK16 >::Size;
				break;

			case ashes::Format::eR8G8B8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8G8B8_UNORM >::Size;
				break;

			case ashes::Format::eB8G8R8_UNORM:
				result = PixelDefinitions< ashes::Format::eB8G8R8_UNORM >::Size;
				break;

			case ashes::Format::eR8G8B8_SRGB:
				result = PixelDefinitions< ashes::Format::eR8G8B8_SRGB >::Size;
				break;

			case ashes::Format::eB8G8R8_SRGB:
				result = PixelDefinitions< ashes::Format::eB8G8R8_SRGB >::Size;
				break;

			case ashes::Format::eR8G8B8A8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8G8B8A8_UNORM >::Size;
				break;

			case ashes::Format::eB8G8R8A8_UNORM:
				result = PixelDefinitions< ashes::Format::eB8G8R8A8_UNORM >::Size;
				break;

			case ashes::Format::eR8G8B8A8_SRGB:
				result = PixelDefinitions< ashes::Format::eR8G8B8A8_SRGB >::Size;
				break;

			case ashes::Format::eB8G8R8A8_SRGB:
				result = PixelDefinitions< ashes::Format::eB8G8R8A8_SRGB >::Size;
				break;

			case ashes::Format::eR16G16B16_SFLOAT:
				result = PixelDefinitions<	ashes::Format::eR16G16B16_SFLOAT >::Size;
				break;

			case ashes::Format::eR16G16B16A16_SFLOAT:
				result = PixelDefinitions<	ashes::Format::eR16G16B16A16_SFLOAT >::Size;
				break;

			case ashes::Format::eR32G32B32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32G32B32_SFLOAT >::Size;
				break;

			case ashes::Format::eR32G32B32A32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32G32B32A32_SFLOAT >::Size;
				break;

			case ashes::Format::eD16_UNORM:
				result = PixelDefinitions< ashes::Format::eD16_UNORM >::Size;
				break;

			case ashes::Format::eD24_UNORM_S8_UINT:
				result = PixelDefinitions< ashes::Format::eD24_UNORM_S8_UINT >::Size;
				break;

			case ashes::Format::eD32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eD32_SFLOAT >::Size;
				break;

			case ashes::Format::eD32_SFLOAT_S8_UINT:
				result = PixelDefinitions< ashes::Format::eD32_SFLOAT_S8_UINT >::Size;
				break;

			case ashes::Format::eS8_UINT:
				result = PixelDefinitions< ashes::Format::eS8_UINT >::Size;
				break;

			default:
				FAILURE( "Unsupported pixel format" );
				break;
			}

			return result;
		}

		uint8_t getComponentsCount( ashes::Format p_format )
		{
			uint8_t result = 0;

			switch ( p_format )
			{
			case ashes::Format::eR8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8_UNORM >::Count;
				break;

			case ashes::Format::eR32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32_SFLOAT >::Count;
				break;

			case ashes::Format::eR16_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR16_SFLOAT >::Count;
				break;

			case ashes::Format::eR8G8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8G8_UNORM >::Count;
				break;

			case ashes::Format::eR32G32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32G32_SFLOAT >::Count;
				break;

			case ashes::Format::eA1R5G5B5_UNORM_PACK16:
				result = PixelDefinitions< ashes::Format::eA1R5G5B5_UNORM_PACK16 >::Count;
				break;

			case ashes::Format::eR5G6B5_UNORM_PACK16:
				result = PixelDefinitions< ashes::Format::eR5G6B5_UNORM_PACK16 >::Count;
				break;

			case ashes::Format::eR8G8B8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8G8B8_UNORM >::Count;
				break;

			case ashes::Format::eB8G8R8_UNORM:
				result = PixelDefinitions< ashes::Format::eB8G8R8_UNORM >::Count;
				break;

			case ashes::Format::eR8G8B8_SRGB:
				result = PixelDefinitions< ashes::Format::eR8G8B8_SRGB >::Count;
				break;

			case ashes::Format::eB8G8R8_SRGB:
				result = PixelDefinitions< ashes::Format::eB8G8R8_SRGB >::Count;
				break;

			case ashes::Format::eR8G8B8A8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8G8B8A8_UNORM >::Count;
				break;

			case ashes::Format::eB8G8R8A8_UNORM:
				result = PixelDefinitions< ashes::Format::eB8G8R8A8_UNORM >::Count;
				break;

			case ashes::Format::eR8G8B8A8_SRGB:
				result = PixelDefinitions< ashes::Format::eR8G8B8A8_SRGB >::Count;
				break;

			case ashes::Format::eB8G8R8A8_SRGB:
				result = PixelDefinitions< ashes::Format::eB8G8R8A8_SRGB >::Count;
				break;

			case ashes::Format::eR16G16B16_SFLOAT:
				result = PixelDefinitions<	ashes::Format::eR16G16B16_SFLOAT >::Count;
				break;

			case ashes::Format::eR16G16B16A16_SFLOAT:
				result = PixelDefinitions<	ashes::Format::eR16G16B16A16_SFLOAT >::Count;
				break;

			case ashes::Format::eR32G32B32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32G32B32_SFLOAT >::Count;
				break;

			case ashes::Format::eR32G32B32A32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32G32B32A32_SFLOAT >::Count;
				break;

			case ashes::Format::eD16_UNORM:
				result = PixelDefinitions< ashes::Format::eD16_UNORM >::Count;
				break;

			case ashes::Format::eD24_UNORM_S8_UINT:
				result = PixelDefinitions< ashes::Format::eD24_UNORM_S8_UINT >::Count;
				break;

			case ashes::Format::eD32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eD32_SFLOAT >::Count;
				break;

			case ashes::Format::eD32_SFLOAT_S8_UINT:
				result = PixelDefinitions< ashes::Format::eD32_SFLOAT_S8_UINT >::Count;
				break;

			case ashes::Format::eS8_UINT:
				result = PixelDefinitions< ashes::Format::eS8_UINT >::Count;
				break;

			default:
				FAILURE( "Unsupported pixel format" );
				break;
			}

			return result;
		}

		void convertPixel( ashes::Format p_eSrcFmt, uint8_t const *& p_pSrc, ashes::Format p_eDestFmt, uint8_t *& p_pDest )
		{
			switch ( p_eSrcFmt )
			{
			case ashes::Format::eR8_UNORM:
				PixelDefinitions< ashes::Format::eR8_UNORM >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eR16_SFLOAT:
				PixelDefinitions< ashes::Format::eR16_SFLOAT >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eR32_SFLOAT:
				PixelDefinitions< ashes::Format::eR32_SFLOAT >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eR8G8_UNORM:
				PixelDefinitions< ashes::Format::eR8G8_UNORM >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eR32G32_SFLOAT:
				PixelDefinitions< ashes::Format::eR32G32_SFLOAT >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eA1R5G5B5_UNORM_PACK16:
				PixelDefinitions< ashes::Format::eA1R5G5B5_UNORM_PACK16 >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eR5G6B5_UNORM_PACK16:
				PixelDefinitions< ashes::Format::eR5G6B5_UNORM_PACK16 >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eR8G8B8_UNORM:
				PixelDefinitions< ashes::Format::eR8G8B8_UNORM >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eB8G8R8_UNORM:
				PixelDefinitions< ashes::Format::eB8G8R8_UNORM >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eR8G8B8_SRGB:
				PixelDefinitions< ashes::Format::eR8G8B8_SRGB >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eB8G8R8_SRGB:
				PixelDefinitions< ashes::Format::eB8G8R8_SRGB >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eR8G8B8A8_UNORM:
				PixelDefinitions< ashes::Format::eR8G8B8A8_UNORM >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eB8G8R8A8_UNORM:
				PixelDefinitions< ashes::Format::eB8G8R8A8_UNORM >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eR8G8B8A8_SRGB:
				PixelDefinitions< ashes::Format::eR8G8B8A8_SRGB >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eB8G8R8A8_SRGB:
				PixelDefinitions< ashes::Format::eB8G8R8A8_SRGB >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eR16G16B16_SFLOAT:
				PixelDefinitions< ashes::Format::eR16G16B16_SFLOAT >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eR16G16B16A16_SFLOAT:
				PixelDefinitions< ashes::Format::eR16G16B16A16_SFLOAT >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eR32G32B32_SFLOAT:
				PixelDefinitions< ashes::Format::eR32G32B32_SFLOAT >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eR32G32B32A32_SFLOAT:
				PixelDefinitions< ashes::Format::eR32G32B32A32_SFLOAT >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eD16_UNORM:
				PixelDefinitions< ashes::Format::eD16_UNORM >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eD24_UNORM_S8_UINT:
				PixelDefinitions< ashes::Format::eD24_UNORM_S8_UINT >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eD32_SFLOAT:
				PixelDefinitions< ashes::Format::eD32_SFLOAT >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eD32_SFLOAT_S8_UINT:
				PixelDefinitions< ashes::Format::eD32_SFLOAT_S8_UINT >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			case ashes::Format::eS8_UINT:
				PixelDefinitions< ashes::Format::eS8_UINT >::convert( p_pSrc, p_pDest, p_eDestFmt );
				break;

			default:
				FAILURE( "Unsupported pixel format" );
				break;
			}
		}

		void convertBuffer( ashes::Format p_eSrcFormat, uint8_t const * p_pSrcBuffer, uint32_t p_uiSrcSize, ashes::Format p_eDstFormat, uint8_t * p_pDstBuffer, uint32_t p_uiDstSize )
		{
			switch ( p_eSrcFormat )
			{
			case ashes::Format::eR8_UNORM:
				PixelDefinitions< ashes::Format::eR8_UNORM >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eR16_SFLOAT:
				PixelDefinitions< ashes::Format::eR16_SFLOAT >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eR32_SFLOAT:
				PixelDefinitions< ashes::Format::eR32_SFLOAT >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eR8G8_UNORM:
				PixelDefinitions< ashes::Format::eR8G8_UNORM >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eR32G32_SFLOAT:
				PixelDefinitions< ashes::Format::eR32G32_SFLOAT >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eA1R5G5B5_UNORM_PACK16:
				PixelDefinitions< ashes::Format::eA1R5G5B5_UNORM_PACK16 >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eR5G6B5_UNORM_PACK16:
				PixelDefinitions< ashes::Format::eR5G6B5_UNORM_PACK16 >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eR8G8B8_UNORM:
				PixelDefinitions< ashes::Format::eR8G8B8_UNORM >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eB8G8R8_UNORM:
				PixelDefinitions< ashes::Format::eB8G8R8_UNORM >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eR8G8B8_SRGB:
				PixelDefinitions< ashes::Format::eR8G8B8_SRGB >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eB8G8R8_SRGB:
				PixelDefinitions< ashes::Format::eB8G8R8_SRGB >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eR8G8B8A8_UNORM:
				PixelDefinitions< ashes::Format::eR8G8B8A8_UNORM >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eB8G8R8A8_UNORM:
				PixelDefinitions< ashes::Format::eB8G8R8A8_UNORM >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eR8G8B8A8_SRGB:
				PixelDefinitions< ashes::Format::eR8G8B8A8_SRGB >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eB8G8R8A8_SRGB:
				PixelDefinitions< ashes::Format::eB8G8R8A8_SRGB >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eR16G16B16_SFLOAT:
				PixelDefinitions< ashes::Format::eR16G16B16_SFLOAT >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eR16G16B16A16_SFLOAT:
				PixelDefinitions< ashes::Format::eR16G16B16A16_SFLOAT >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eR32G32B32_SFLOAT:
				PixelDefinitions< ashes::Format::eR32G32B32_SFLOAT >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eR32G32B32A32_SFLOAT:
				PixelDefinitions< ashes::Format::eR32G32B32A32_SFLOAT >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eD16_UNORM:
				PixelDefinitions< ashes::Format::eD16_UNORM >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eD24_UNORM_S8_UINT:
				PixelDefinitions< ashes::Format::eD24_UNORM_S8_UINT >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eD32_SFLOAT:
				PixelDefinitions< ashes::Format::eD32_SFLOAT >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eD32_SFLOAT_S8_UINT:
				PixelDefinitions< ashes::Format::eD32_SFLOAT_S8_UINT >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			case ashes::Format::eS8_UINT:
				PixelDefinitions< ashes::Format::eS8_UINT >::convert( p_pSrcBuffer, p_uiSrcSize, p_eDstFormat, p_pDstBuffer, p_uiDstSize );
				break;

			default:
				FAILURE( "Unsupported pixel format" );
				break;
			}
		}

		ashes::Format getFormatByName( String const & p_strFormat )
		{
			ashes::Format result = ashes::Format::eRange;

			for ( int i = 0; i < int( result ); ++i )
			{
				switch ( ashes::Format( i ) )
				{
				case ashes::Format::eR8_UNORM:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eR8_UNORM >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eR16_SFLOAT:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eR16_SFLOAT >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eR32_SFLOAT:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eR32_SFLOAT >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eR8G8_UNORM:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eR8G8_UNORM >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eR32G32_SFLOAT:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eR32G32_SFLOAT >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eA1R5G5B5_UNORM_PACK16:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eA1R5G5B5_UNORM_PACK16 >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eR5G6B5_UNORM_PACK16:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eR5G6B5_UNORM_PACK16 >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eR8G8B8_UNORM:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eR8G8B8_UNORM >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eB8G8R8_UNORM:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eB8G8R8_UNORM >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eR8G8B8_SRGB:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eR8G8B8_SRGB >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eB8G8R8_SRGB:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eB8G8R8_SRGB >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eR8G8B8A8_UNORM:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eR8G8B8A8_UNORM >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eB8G8R8A8_UNORM:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eB8G8R8A8_UNORM >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eR8G8B8A8_SRGB:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eR8G8B8A8_SRGB >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eB8G8R8A8_SRGB:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eB8G8R8A8_SRGB >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eR16G16B16_SFLOAT:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eR16G16B16_SFLOAT >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eR16G16B16A16_SFLOAT:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eR16G16B16A16_SFLOAT >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eR32G32B32_SFLOAT:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eR32G32B32_SFLOAT >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eR32G32B32A32_SFLOAT:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eR32G32B32A32_SFLOAT >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eD16_UNORM:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eD16_UNORM >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eD24_UNORM_S8_UINT:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eD24_UNORM_S8_UINT >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eD32_SFLOAT:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eD32_SFLOAT >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eD32_SFLOAT_S8_UINT:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eD32_SFLOAT_S8_UINT >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				case ashes::Format::eS8_UINT:
					result = ( p_strFormat == PixelDefinitions< ashes::Format::eS8_UINT >::toStr() ? ashes::Format( i ) : ashes::Format::eRange );
					break;

				default:
					FAILURE( "Unsupported pixel format" );
					break;
				}
			}

			return result;
		}

		String getFormatName( ashes::Format p_format )
		{
			String result;

			switch ( p_format )
			{
			case ashes::Format::eR8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8_UNORM >::toStr();
				break;

			case ashes::Format::eR16_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32_SFLOAT >::toStr();
				break;

			case ashes::Format::eR32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32_SFLOAT >::toStr();
				break;

			case ashes::Format::eR8G8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8G8_UNORM >::toStr();
				break;

			case ashes::Format::eR32G32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32G32_SFLOAT >::toStr();
				break;

			case ashes::Format::eA1R5G5B5_UNORM_PACK16:
				result = PixelDefinitions< ashes::Format::eA1R5G5B5_UNORM_PACK16 >::toStr();
				break;

			case ashes::Format::eR5G6B5_UNORM_PACK16:
				result = PixelDefinitions< ashes::Format::eR5G6B5_UNORM_PACK16 >::toStr();
				break;

			case ashes::Format::eR8G8B8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8G8B8_UNORM >::toStr();
				break;

			case ashes::Format::eB8G8R8_UNORM:
				result = PixelDefinitions< ashes::Format::eB8G8R8_UNORM >::toStr();
				break;

			case ashes::Format::eR8G8B8_SRGB:
				result = PixelDefinitions< ashes::Format::eR8G8B8_SRGB >::toStr();
				break;

			case ashes::Format::eB8G8R8_SRGB:
				result = PixelDefinitions< ashes::Format::eB8G8R8_SRGB >::toStr();
				break;

			case ashes::Format::eR8G8B8A8_UNORM:
				result = PixelDefinitions< ashes::Format::eR8G8B8A8_UNORM >::toStr();
				break;

			case ashes::Format::eB8G8R8A8_UNORM:
				result = PixelDefinitions< ashes::Format::eB8G8R8A8_UNORM >::toStr();
				break;

			case ashes::Format::eR8G8B8A8_SRGB:
				result = PixelDefinitions< ashes::Format::eR8G8B8A8_SRGB >::toStr();
				break;

			case ashes::Format::eB8G8R8A8_SRGB:
				result = PixelDefinitions< ashes::Format::eB8G8R8A8_SRGB >::toStr();
				break;

			case ashes::Format::eR16G16B16_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR16G16B16_SFLOAT >::toStr();
				break;

			case ashes::Format::eR16G16B16A16_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR16G16B16A16_SFLOAT >::toStr();
				break;

			case ashes::Format::eR32G32B32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32G32B32_SFLOAT >::toStr();
				break;

			case ashes::Format::eR32G32B32A32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eR32G32B32A32_SFLOAT >::toStr();
				break;

			case ashes::Format::eD16_UNORM:
				result = PixelDefinitions< ashes::Format::eD16_UNORM >::toStr();
				break;

			case ashes::Format::eD24_UNORM_S8_UINT:
				result = PixelDefinitions< ashes::Format::eD24_UNORM_S8_UINT >::toStr();
				break;

			case ashes::Format::eD32_SFLOAT:
				result = PixelDefinitions< ashes::Format::eD32_SFLOAT >::toStr();
				break;

			case ashes::Format::eD32_SFLOAT_S8_UINT:
				result = PixelDefinitions< ashes::Format::eD32_SFLOAT_S8_UINT >::toStr();
				break;

			case ashes::Format::eS8_UINT:
				result = PixelDefinitions< ashes::Format::eS8_UINT >::toStr();
				break;

			default:
				FAILURE( "Unsupported pixel format" );
				break;
			}

			return result;
		}
	}
}
