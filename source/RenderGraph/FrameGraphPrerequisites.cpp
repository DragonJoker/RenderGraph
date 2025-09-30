#include "RenderGraph/FrameGraphPrerequisites.hpp"
#include "RenderGraph/BufferData.hpp"
#include "RenderGraph/BufferViewData.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/ImageViewData.hpp"

#include <cassert>

namespace crg
{
	namespace fgph
	{
		static bool match( ImageSubresourceRange const & lhsRange
			, ImageSubresourceRange const & rhsRange )noexcept
		{
			return ( ( lhsRange.aspectMask & rhsRange.aspectMask ) != ImageAspectFlags::eNone )
				&& lhsRange.baseArrayLayer == rhsRange.baseArrayLayer
				&& lhsRange.layerCount == rhsRange.layerCount
				&& lhsRange.baseMipLevel == rhsRange.baseMipLevel
				&& lhsRange.levelCount == rhsRange.levelCount;
		}

		static bool match( ImageId const & image
			, ImageViewType lhsType
			, ImageViewType rhsType
			, ImageSubresourceRange const & lhsRange
			, ImageSubresourceRange const & rhsRange )noexcept
		{
			return match( getVirtualRange( image, lhsType, lhsRange )
				, getVirtualRange( image, rhsType, rhsRange ) );
		}

		static bool match( ImageId const & image
			, ImageViewCreateInfo const & lhsInfo
			, ImageViewCreateInfo const & rhsInfo )noexcept
		{
			return lhsInfo.flags == rhsInfo.flags
				&& lhsInfo.format == rhsInfo.format
				&& match( image
					, lhsInfo.viewType, rhsInfo.viewType
					, lhsInfo.subresourceRange, rhsInfo.subresourceRange );
		}

		static bool match( ImageViewData const & lhs, ImageViewData const & rhs )noexcept
		{
			return lhs.image.id == rhs.image.id
				&& match( lhs.image, lhs.info, rhs.info );
		}
	}

	//*********************************************************************************************

	std::string_view getName( PixelFormat format )
	{
		switch ( format )
		{
		case PixelFormat::eR4G4_UNORM:
			return "rg8";
		case PixelFormat::eR4G4B4A4_UNORM:
			return "rgba16";
		case PixelFormat::eB4G4R4A4_UNORM:
			return "rgba16s";
		case PixelFormat::eR5G6B5_UNORM:
			return "rgb565";
		case PixelFormat::eB5G6R5_UNORM:
			return "bgr565";
		case PixelFormat::eR5G5B5A1_UNORM:
			return "rgba5551";
		case PixelFormat::eB5G5R5A1_UNORM:
			return "bgra5551";
		case PixelFormat::eA1R5G5B5_UNORM:
			return "argb1555";
		case PixelFormat::eR8_UNORM:
			return "r8";
		case PixelFormat::eR8_SNORM:
			return "r8s";
		case PixelFormat::eR8_USCALED:
			return "r8us";
		case PixelFormat::eR8_SSCALED:
			return "r8ss";
		case PixelFormat::eR8_UINT:
			return "r8ui";
		case PixelFormat::eR8_SINT:
			return "r8si";
		case PixelFormat::eR8_SRGB:
			return "r8srgb";
		case PixelFormat::eR8G8_UNORM:
			return "rg16";
		case PixelFormat::eR8G8_SNORM:
			return "rg16s";
		case PixelFormat::eR8G8_USCALED:
			return "rg16us";
		case PixelFormat::eR8G8_SSCALED:
			return "rg16ss";
		case PixelFormat::eR8G8_UINT:
			return "rg16ui";
		case PixelFormat::eR8G8_SINT:
			return "rg16si";
		case PixelFormat::eR8G8_SRGB:
			return "rg16srgb";
		case PixelFormat::eR8G8B8_UNORM:
			return "rgb24";
		case PixelFormat::eR8G8B8_SNORM:
			return "rgb24s";
		case PixelFormat::eR8G8B8_USCALED:
			return "rgb24us";
		case PixelFormat::eR8G8B8_SSCALED:
			return "rgb24ss";
		case PixelFormat::eR8G8B8_UINT:
			return "rgb24ui";
		case PixelFormat::eR8G8B8_SINT:
			return "rgb24si";
		case PixelFormat::eR8G8B8_SRGB:
			return "rgb24srgb";
		case PixelFormat::eB8G8R8_UNORM:
			return "bgr24";
		case PixelFormat::eB8G8R8_SNORM:
			return "bgr24s";
		case PixelFormat::eB8G8R8_USCALED:
			return "bgr24us";
		case PixelFormat::eB8G8R8_SSCALED:
			return "bgr24ss";
		case PixelFormat::eB8G8R8_UINT:
			return "bgr24ui";
		case PixelFormat::eB8G8R8_SINT:
			return "bgr24si";
		case PixelFormat::eB8G8R8_SRGB:
			return "bgr24srgb";
		case PixelFormat::eR8G8B8A8_UNORM:
			return "rgba32";
		case PixelFormat::eR8G8B8A8_SNORM:
			return "rgba32s";
		case PixelFormat::eR8G8B8A8_USCALED:
			return "rgba32us";
		case PixelFormat::eR8G8B8A8_SSCALED:
			return "rgba32ss";
		case PixelFormat::eR8G8B8A8_UINT:
			return "rgba32ui";
		case PixelFormat::eR8G8B8A8_SINT:
			return "rgba32si";
		case PixelFormat::eR8G8B8A8_SRGB:
			return "rgba32srgb";
		case PixelFormat::eB8G8R8A8_UNORM:
			return "bgra32";
		case PixelFormat::eB8G8R8A8_SNORM:
			return "bgra32s";
		case PixelFormat::eB8G8R8A8_USCALED:
			return "bgra32us";
		case PixelFormat::eB8G8R8A8_SSCALED:
			return "bgra32ss";
		case PixelFormat::eB8G8R8A8_UINT:
			return "bgra32ui";
		case PixelFormat::eB8G8R8A8_SINT:
			return "bgra32si";
		case PixelFormat::eB8G8R8A8_SRGB:
			return "bgra32srgb";
		case PixelFormat::eA8B8G8R8_UNORM:
			return "abgr32";
		case PixelFormat::eA8B8G8R8_SNORM:
			return "abgr32s";
		case PixelFormat::eA8B8G8R8_USCALED:
			return "abgr32us";
		case PixelFormat::eA8B8G8R8_SSCALED:
			return "abgr32ss";
		case PixelFormat::eA8B8G8R8_UINT:
			return "abgr32ui";
		case PixelFormat::eA8B8G8R8_SINT:
			return "abgr32si";
		case PixelFormat::eA8B8G8R8_SRGB:
			return "abgr32srgb";
		case PixelFormat::eA2R10G10B10_UNORM:
			return "argb2101010";
		case PixelFormat::eA2R10G10B10_SNORM:
			return "argb2101010s";
		case PixelFormat::eA2R10G10B10_USCALED:
			return "argb2101010us";
		case PixelFormat::eA2R10G10B10_SSCALED:
			return "argb2101010ss";
		case PixelFormat::eA2R10G10B10_UINT:
			return "argb2101010ui";
		case PixelFormat::eA2R10G10B10_SINT:
			return "argb2101010si";
		case PixelFormat::eA2B10G10R10_UNORM:
			return "abgr2101010";
		case PixelFormat::eA2B10G10R10_SNORM:
			return "abgr2101010s";
		case PixelFormat::eA2B10G10R10_USCALED:
			return "abgr2101010us";
		case PixelFormat::eA2B10G10R10_SSCALED:
			return "abgr2101010ss";
		case PixelFormat::eA2B10G10R10_UINT:
			return "abgr2101010ui";
		case PixelFormat::eA2B10G10R10_SINT:
			return "abgr2101010si";
		case PixelFormat::eR16_UNORM:
			return "r16";
		case PixelFormat::eR16_SNORM:
			return "rg16s";
		case PixelFormat::eR16_USCALED:
			return "rg16us";
		case PixelFormat::eR16_SSCALED:
			return "rg16ss";
		case PixelFormat::eR16_UINT:
			return "rg16ui";
		case PixelFormat::eR16_SINT:
			return "rg16si";
		case PixelFormat::eR16_SFLOAT:
			return "rg16f";
		case PixelFormat::eR16G16_UNORM:
			return "rg32";
		case PixelFormat::eR16G16_SNORM:
			return "rg32s";
		case PixelFormat::eR16G16_USCALED:
			return "rg32us";
		case PixelFormat::eR16G16_SSCALED:
			return "rg32ss";
		case PixelFormat::eR16G16_UINT:
			return "rg32ui";
		case PixelFormat::eR16G16_SINT:
			return "rg32si";
		case PixelFormat::eR16G16_SFLOAT:
			return "rg32f";
		case PixelFormat::eR16G16B16_UNORM:
			return "rgb48";
		case PixelFormat::eR16G16B16_SNORM:
			return "rgb48s";
		case PixelFormat::eR16G16B16_USCALED:
			return "rgb48us";
		case PixelFormat::eR16G16B16_SSCALED:
			return "rgb48ss";
		case PixelFormat::eR16G16B16_UINT:
			return "rgb48ui";
		case PixelFormat::eR16G16B16_SINT:
			return "rgb48si";
		case PixelFormat::eR16G16B16_SFLOAT:
			return "rgb48f";
		case PixelFormat::eR16G16B16A16_UNORM:
			return "rgba64";
		case PixelFormat::eR16G16B16A16_SNORM:
			return "rgba64s";
		case PixelFormat::eR16G16B16A16_USCALED:
			return "rgba64us";
		case PixelFormat::eR16G16B16A16_SSCALED:
			return "rgba64ss";
		case PixelFormat::eR16G16B16A16_UINT:
			return "rgba64ui";
		case PixelFormat::eR16G16B16A16_SINT:
			return "rgba64si";
		case PixelFormat::eR16G16B16A16_SFLOAT:
			return "rgba64f";
		case PixelFormat::eR32_UINT:
			return "r32ui";
		case PixelFormat::eR32_SINT:
			return "r32si";
		case PixelFormat::eR32_SFLOAT:
			return "r32f";
		case PixelFormat::eR32G32_UINT:
			return "rg64ui";
		case PixelFormat::eR32G32_SINT:
			return "rg64si";
		case PixelFormat::eR32G32_SFLOAT:
			return "rg64f";
		case PixelFormat::eR32G32B32_UINT:
			return "rgb96ui";
		case PixelFormat::eR32G32B32_SINT:
			return "rgb96si";
		case PixelFormat::eR32G32B32_SFLOAT:
			return "rgb96f";
		case PixelFormat::eR32G32B32A32_UINT:
			return "rgba128ui";
		case PixelFormat::eR32G32B32A32_SINT:
			return "rgba128si";
		case PixelFormat::eR32G32B32A32_SFLOAT:
			return "rgba128f";
		case PixelFormat::eR64_UINT:
			return "r64ui";
		case PixelFormat::eR64_SINT:
			return "r64si";
		case PixelFormat::eR64_SFLOAT:
			return "r64f";
		case PixelFormat::eR64G64_UINT:
			return "rg128ui";
		case PixelFormat::eR64G64_SINT:
			return "rg128si";
		case PixelFormat::eR64G64_SFLOAT:
			return "rg128f";
		case PixelFormat::eR64G64B64_UINT:
			return "rgb192ui";
		case PixelFormat::eR64G64B64_SINT:
			return "rgb192si";
		case PixelFormat::eR64G64B64_SFLOAT:
			return "rgb192f";
		case PixelFormat::eR64G64B64A64_UINT:
			return "rgba256ui";
		case PixelFormat::eR64G64B64A64_SINT:
			return "rgba256si";
		case PixelFormat::eR64G64B64A64_SFLOAT:
			return "rgba256f";
		case PixelFormat::eB10G11R11_UFLOAT:
			return "bgr32f";
		case PixelFormat::eE5B9G9R9_UFLOAT:
			return "ebgr32f";
		case PixelFormat::eD16_UNORM:
			return "depth16";
		case PixelFormat::eX8_D24_UNORM:
			return "depth24";
		case PixelFormat::eD32_SFLOAT:
			return "depth32f";
		case PixelFormat::eS8_UINT:
			return "stencil8";
		case PixelFormat::eD16_UNORM_S8_UINT:
			return "depth16s8";
		case PixelFormat::eD24_UNORM_S8_UINT:
			return "depth24s8";
		case PixelFormat::eD32_SFLOAT_S8_UINT:
			return "depth32fs8";
		case PixelFormat::eBC1_RGB_UNORM_BLOCK:
			return "bc1_rgb";
		case PixelFormat::eBC1_RGB_SRGB_BLOCK:
			return "bc1_srgb";
		case PixelFormat::eBC1_RGBA_UNORM_BLOCK:
			return "bc1_rgba";
		case PixelFormat::eBC1_RGBA_SRGB_BLOCK:
			return "bc1_rgba_srgb";
		case PixelFormat::eBC2_UNORM_BLOCK:
			return "bc2_rgba";
		case PixelFormat::eBC2_SRGB_BLOCK:
			return "bc2_rgba_srgb";
		case PixelFormat::eBC3_UNORM_BLOCK:
			return "bc3_rgba";
		case PixelFormat::eBC3_SRGB_BLOCK:
			return "bc3_rgba_srgb";
		case PixelFormat::eBC4_UNORM_BLOCK:
			return "bc4_r";
		case PixelFormat::eBC4_SNORM_BLOCK:
			return "bc4_r_s";
		case PixelFormat::eBC5_UNORM_BLOCK:
			return "bc5_rg";
		case PixelFormat::eBC5_SNORM_BLOCK:
			return "bc5_rg_s";
		case PixelFormat::eBC6H_UFLOAT_BLOCK:
			return "bc6h";
		case PixelFormat::eBC6H_SFLOAT_BLOCK:
			return "bc6h_s";
		case PixelFormat::eBC7_UNORM_BLOCK:
			return "bc7";
		case PixelFormat::eBC7_SRGB_BLOCK:
			return "bc7_srgb";
		case PixelFormat::eETC2_R8G8B8_UNORM_BLOCK:
			return "etc2_rgb";
		case PixelFormat::eETC2_R8G8B8_SRGB_BLOCK:
			return "etc2_rgb_srgb";
		case PixelFormat::eETC2_R8G8B8A1_UNORM_BLOCK:
			return "etc2_rgba1";
		case PixelFormat::eETC2_R8G8B8A1_SRGB_BLOCK:
			return "etc2_rgba1_srgb";
		case PixelFormat::eETC2_R8G8B8A8_UNORM_BLOCK:
			return "etc2_rgba";
		case PixelFormat::eETC2_R8G8B8A8_SRGB_BLOCK:
			return "etc2_rgba_srgb";
		case PixelFormat::eEAC_R11_UNORM_BLOCK:
			return "eac_r";
		case PixelFormat::eEAC_R11_SNORM_BLOCK:
			return "eac_r_s";
		case PixelFormat::eEAC_R11G11_UNORM_BLOCK:
			return "eac_rg";
		case PixelFormat::eEAC_R11G11_SNORM_BLOCK:
			return "eac_rg_s";
		case PixelFormat::eASTC_4x4_UNORM_BLOCK:
			return "astc_4x4";
		case PixelFormat::eASTC_4x4_SRGB_BLOCK:
			return "astc_4x4_srgb";
		case PixelFormat::eASTC_5x4_UNORM_BLOCK:
			return "astc_5x4";
		case PixelFormat::eASTC_5x4_SRGB_BLOCK:
			return "astc_5x4_srgb";
		case PixelFormat::eASTC_5x5_UNORM_BLOCK:
			return "astc_5x5";
		case PixelFormat::eASTC_5x5_SRGB_BLOCK:
			return "astc_5x5_srgb";
		case PixelFormat::eASTC_6x5_UNORM_BLOCK:
			return "astc_6x5";
		case PixelFormat::eASTC_6x5_SRGB_BLOCK:
			return "astc_6x5_srgb";
		case PixelFormat::eASTC_6x6_UNORM_BLOCK:
			return "astc_6x6";
		case PixelFormat::eASTC_6x6_SRGB_BLOCK:
			return "astc_6x6_srgb";
		case PixelFormat::eASTC_8x5_UNORM_BLOCK:
			return "astc_8x5";
		case PixelFormat::eASTC_8x5_SRGB_BLOCK:
			return "astc_8x5_srgb";
		case PixelFormat::eASTC_8x6_UNORM_BLOCK:
			return "astc_8x6";
		case PixelFormat::eASTC_8x6_SRGB_BLOCK:
			return "astc_8x6_srgb";
		case PixelFormat::eASTC_8x8_UNORM_BLOCK:
			return "astc_8x8";
		case PixelFormat::eASTC_8x8_SRGB_BLOCK:
			return "astc_8x8_srgb";
		case PixelFormat::eASTC_10x5_UNORM_BLOCK:
			return "astc_10x5";
		case PixelFormat::eASTC_10x5_SRGB_BLOCK:
			return "astc_10x5_srgb";
		case PixelFormat::eASTC_10x6_UNORM_BLOCK:
			return "astc_10x6";
		case PixelFormat::eASTC_10x6_SRGB_BLOCK:
			return "astc_10x6_srgb";
		case PixelFormat::eASTC_10x8_UNORM_BLOCK:
			return "astc_10x8";
		case PixelFormat::eASTC_10x8_SRGB_BLOCK:
			return "astc_10x8_srgb";
		case PixelFormat::eASTC_10x10_UNORM_BLOCK:
			return "astc_10x10";
		case PixelFormat::eASTC_10x10_SRGB_BLOCK:
			return "astc_10x10_srgb";
		case PixelFormat::eASTC_12x10_UNORM_BLOCK:
			return "astc_12x10";
		case PixelFormat::eASTC_12x10_SRGB_BLOCK:
			return "astc_12x10_srgb";
		case PixelFormat::eASTC_12x12_UNORM_BLOCK:
			return "astc_12x12";
		case PixelFormat::eASTC_12x12_SRGB_BLOCK:
			return "astc_12x12_srgb";
		default:
			return "undefined";
		}
	}

	//*********************************************************************************************

	std::string_view getName( FilterMode v )
	{
		if ( v == FilterMode::eLinear )
			return "linear";
		return "nearest";
	}

	//*********************************************************************************************

	std::string_view getName( MipmapMode v )
	{
		if ( v == MipmapMode::eLinear )
			return "linear";
		return "nearest";
	}

	//*********************************************************************************************

	std::string_view getName( WrapMode v )
	{
		switch ( v )
		{
		case WrapMode::eMirroredRepeat:
			return "mirrored_repeat";
		case WrapMode::eClampToEdge:
			return "clamp_to_edge";
		case WrapMode::eClampToBorder:
			return "clamp_to_border";
		case WrapMode::eMirrorClampToEdge:
			return "mirrored_clamp_to_edge";
		default:
			return "repeat";
		}
	}

	//*********************************************************************************************

	ImageCreateFlags getImageCreateFlags( ImageId const & image )noexcept
	{
		return image.data->info.flags;
	}

	ImageCreateFlags getImageCreateFlags( ImageViewId const & image )noexcept
	{
		return getImageCreateFlags( image.data->image );
	}

	Extent3D const & getExtent( ImageId const & image )noexcept
	{
		return image.data->info.extent;
	}

	Extent3D const & getExtent( ImageViewId const & image )noexcept
	{
		return getExtent( image.data->image );
	}

	DeviceSize getSize( BufferId const & buffer )noexcept
	{
		return buffer.data->info.size;
	}

	DeviceSize getSize( BufferViewId const & buffer )noexcept
	{
		return buffer.data->info.subresourceRange.size;
	}

	Extent3D getMipExtent( ImageViewId const & image )noexcept
	{
		auto result = getExtent( image.data->image );
		result.width >>= getSubresourceRange( image ).baseMipLevel;
		result.height >>= getSubresourceRange( image ).baseMipLevel;
		result.depth >>= getSubresourceRange( image ).baseMipLevel;
		return result;
	}

	PixelFormat getFormat( ImageId const & image )noexcept
	{
		return image.data->info.format;
	}

	PixelFormat getFormat( ImageViewId const & image )noexcept
	{
		return image.data->info.format;
	}

	ImageType getImageType( ImageId const & image )noexcept
	{
		return image.data->info.imageType;
	}

	ImageType getImageType( ImageViewId const & image )noexcept
	{
		return getImageType( image.data->image );
	}

	ImageViewType getImageViewType( ImageViewId const & image )noexcept
	{
		return image.data->info.viewType;
	}

	uint32_t getMipLevels( ImageId const & image )noexcept
	{
		return image.data->info.mipLevels;
	}

	uint32_t getMipLevels( ImageViewId const & image )noexcept
	{
		return getSubresourceRange( image ).levelCount;
	}

	uint32_t getArrayLayers( ImageId const & image )noexcept
	{
		return image.data->info.arrayLayers;
	}

	uint32_t getArrayLayers( ImageViewId const & image )noexcept
	{
		return getSubresourceRange( image ).layerCount;
	}

	ImageAspectFlags getAspectFlags( ImageViewId const & image )noexcept
	{
		return getSubresourceRange( image ).aspectMask;
	}

	ImageSubresourceRange const & getSubresourceRange( ImageViewId const & image )noexcept
	{
		return image.data->info.subresourceRange;
	}

	BufferSubresourceRange const & getSubresourceRange( BufferViewId const & buffer )noexcept
	{
		return buffer.data->info.subresourceRange;
	}

	AccessFlags getAccessMask( ImageLayout layout )noexcept
	{
		AccessFlags result{ 0u };

		switch ( layout )
		{
		case ImageLayout::ePresentSrc:
		case ImageLayout::eSharedPresent:
			result |= AccessFlags::eMemoryRead;
			break;
		case ImageLayout::eColorAttachment:
			result |= AccessFlags::eColorAttachmentWrite;
			break;
		case ImageLayout::eDepthStencilAttachment:
			result |= AccessFlags::eDepthStencilAttachmentWrite;
			break;
		case ImageLayout::eDepthStencilReadOnly:
			result |= AccessFlags::eDepthStencilAttachmentRead;
			break;
		case ImageLayout::eShaderReadOnly:
			result |= AccessFlags::eShaderRead;
			break;
		case ImageLayout::eTransferSrc:
			result |= AccessFlags::eTransferRead;
			break;
		case ImageLayout::eTransferDst:
			result |= AccessFlags::eTransferWrite;
			break;
		case ImageLayout::eDepthReadOnlyStencilAttachment:
		case ImageLayout::eDepthAttachmentStencilReadOnly:
			result |= AccessFlags::eDepthStencilAttachmentRead;
			result |= AccessFlags::eDepthStencilAttachmentWrite;
			break;
#ifdef VK_NV_shading_rate_image
		case ImageLayout::eFragmentShadingRateAttachment:
			result |= AccessFlags::eFragmentShadingRateAttachmentRead;
			break;
#endif
#ifdef VK_EXT_fragment_density_map
		case ImageLayout::eFragmentDensityMap:
			result |= AccessFlags::eFragmentDensityMapRead;
			break;
#endif
		default:
			break;
		}

		return result;
	}

	PipelineState getPipelineState( PipelineStageFlags flags )noexcept
	{
		AccessFlags result{ 0u };

		if ( checkFlag( flags, PipelineStageFlags::eBottomOfPipe ) )
		{
			result |= AccessFlags::eMemoryRead;
		}

		if ( checkFlag( flags, PipelineStageFlags::eColorAttachmentOutput ) )
		{
			result |= AccessFlags::eColorAttachmentWrite | AccessFlags::eColorAttachmentRead;
		}

		if ( checkFlag( flags, PipelineStageFlags::eLateFragmentTests ) )
		{
			result |= AccessFlags::eDepthStencilAttachmentWrite;
			result |= AccessFlags::eDepthStencilAttachmentRead;
		}

		if ( checkFlag( flags, PipelineStageFlags::eFragmentShader ) )
		{
			result |= AccessFlags::eShaderRead;
		}

		if ( checkFlag( flags, PipelineStageFlags::eTransfer ) )
		{
			result |= AccessFlags::eTransferRead;
			result |= AccessFlags::eTransferWrite;
		}

		if ( checkFlag( flags, PipelineStageFlags::eFragmentShadingRateAttachment ) )
		{
			result |= AccessFlags::eFragmentShadingRateAttachmentRead;
		}

		if ( checkFlag( flags, PipelineStageFlags::eComputeShader ) )
		{
			result |= AccessFlags::eShaderRead;
		}

		return { result, flags };
	}

	LayoutState makeLayoutState( ImageLayout layout )noexcept
	{
		return { layout
			, getAccessMask( layout )
			, getStageMask( layout ) };
	}

	PipelineStageFlags getStageMask( ImageLayout layout )noexcept
	{
		PipelineStageFlags result{ 0u };

		switch ( layout )
		{
		case ImageLayout::eUndefined:
			result |= PipelineStageFlags::eHost;
			break;
		case ImageLayout::eGeneral:
			result |= PipelineStageFlags::eBottomOfPipe;
			break;
		case ImageLayout::ePresentSrc:
		case ImageLayout::eSharedPresent:
			result |= PipelineStageFlags::eBottomOfPipe;
			break;
		case ImageLayout::eDepthStencilReadOnly:
		case ImageLayout::eDepthReadOnlyStencilAttachment:
		case ImageLayout::eDepthAttachmentStencilReadOnly:
		case ImageLayout::eDepthStencilAttachment:
			result |= PipelineStageFlags::eLateFragmentTests;
			break;
		case ImageLayout::eColorAttachment:
			result |= PipelineStageFlags::eColorAttachmentOutput;
			break;
#ifdef VK_EXT_fragment_density_map
		case ImageLayout::eFragmentDensityMap:
#endif
		case ImageLayout::eShaderReadOnly:
			result |= PipelineStageFlags::eFragmentShader;
			break;
		case ImageLayout::eTransferSrc:
		case ImageLayout::eTransferDst:
			result |= PipelineStageFlags::eTransfer;
			break;
#ifdef VK_NV_shading_rate_image
		case ImageLayout::eFragmentShadingRateAttachment:
			result |= PipelineStageFlags::eFragmentShadingRateAttachment;
			break;
#endif
		default:
			break;
		}

		return result;
	}

	ImageAspectFlags getAspectMask( PixelFormat format )noexcept
	{
		if ( isDepthStencilFormat( format ) )
			return ImageAspectFlags::eDepth | ImageAspectFlags::eStencil;
		if ( isDepthFormat( format ) )
			return ImageAspectFlags::eDepth;
		if ( isStencilFormat( format ) )
			return ImageAspectFlags::eStencil;
		return ImageAspectFlags::eColor;
	}

	LayoutState const & addSubresourceRangeLayout( LayerLayoutStates & ranges
		, ImageSubresourceRange const & range
		, LayoutState const & newLayout )
	{
		for ( uint32_t layerIdx = 0u; layerIdx < range.layerCount; ++layerIdx )
		{
			auto & layers = ranges.try_emplace( range.baseArrayLayer + layerIdx ).first->second;

			for ( uint32_t levelIdx = 0u; levelIdx < range.levelCount; ++levelIdx )
			{
				layers.insert_or_assign( range.baseMipLevel + levelIdx, newLayout );
			}
		}

		return newLayout;
	}

	static void gatherSubresourceRangeLayoutMips( LayerLayoutStates const & ranges
		, ImageSubresourceRange const & range
		, MipLayoutStates const & layers
		, std::map< ImageLayout, LayoutState > & states )
	{
		for ( uint32_t levelIdx = 0u; levelIdx < range.levelCount; ++levelIdx )
		{
			if ( auto it = layers.find( range.baseMipLevel + levelIdx );
				it != layers.end() )
			{
				auto state = it->second;
				auto [rit, res] = states.emplace( state.layout, state );

				if ( !res )
				{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
					rit->second.state.access |= state.state.access;
#pragma GCC diagnostic pop
				}
			}
		}
	}

	LayoutState getSubresourceRangeLayout( LayerLayoutStates const & ranges
		, ImageSubresourceRange const & range )
	{
		std::map< ImageLayout, LayoutState > states;

		for ( uint32_t layerIdx = 0u; layerIdx < range.layerCount; ++layerIdx )
		{
			if ( auto layerIt = ranges.find( range.baseArrayLayer + layerIdx );
				layerIt != ranges.end() )
			{
				auto & layers = layerIt->second;
				gatherSubresourceRangeLayoutMips( ranges, range, layers, states );
			}
		}

		if ( states.empty() )
		{
			return { ImageLayout::eUndefined
				, getAccessMask( ImageLayout::eUndefined )
				, getStageMask( ImageLayout::eUndefined ) };
		}

		return states.begin()->second;
	}

	ImageSubresourceRange getVirtualRange( ImageId const & image
		, ImageViewType viewType
		, ImageSubresourceRange const & range )noexcept
	{
		ImageSubresourceRange result = range;

		if ( viewType == ImageViewType::e3D
			&& ( range.levelCount == 1u
				|| range.levelCount == image.data->info.mipLevels ) )
		{
			result.baseArrayLayer = 0u;
			result.layerCount = getExtent( image ).depth >> range.baseMipLevel;
		}

		return result;
	}

	bool match( ImageViewId const & lhs, ImageViewId const & rhs )noexcept
	{
		return fgph::match( *lhs.data, *rhs.data );
	}

	bool match( BufferViewId const & lhs, BufferViewId const & rhs )noexcept
	{
		return lhs == rhs;
	}

	ImageViewId const & resolveView( ImageViewId const & view
		, uint32_t passIndex )
	{
		return view.data->source.empty()
			? view
			: view.data->source[passIndex];
	}

	BufferViewId const & resolveView( BufferViewId const & view
		, uint32_t passIndex )
	{
		return view.data->source.empty()
			? view
			: view.data->source[passIndex];
	}

	ClearColorValue getClearColorValue( ClearValue const & v )
	{
		if ( v.isColor() )
			return v.color();
		static ClearColorValue dummy{};
		return dummy;
	}

	ClearDepthStencilValue getClearDepthStencilValue( ClearValue const & v )
	{
		if ( v.isDepthStencil() )
			return v.depthStencil();
		static ClearDepthStencilValue dummy{};
		return dummy;
	}

	//*********************************************************************************************
}
