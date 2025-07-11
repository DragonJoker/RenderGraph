/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "FrameGraphStructs.hpp"

#include <assert.h>

namespace crg
{
	CRG_API std::string_view getName( PixelFormat v );
	CRG_API std::string_view getName( FilterMode v );
	CRG_API std::string_view getName( MipmapMode v );
	CRG_API std::string_view getName( WrapMode v );

	CRG_API bool operator==( Buffer const & lhs, Buffer const & rhs );

	CRG_API ImageCreateFlags getImageCreateFlags( ImageId const & image )noexcept;
	CRG_API ImageCreateFlags getImageCreateFlags( ImageViewId const & image )noexcept;
	CRG_API Extent3D const & getExtent( ImageId const & image )noexcept;
	CRG_API Extent3D const & getExtent( ImageViewId const & image )noexcept;
	CRG_API Extent3D getMipExtent( ImageViewId const & image )noexcept;
	CRG_API PixelFormat getFormat( ImageId const & image )noexcept;
	CRG_API PixelFormat getFormat( ImageViewId const & image )noexcept;
	CRG_API ImageType getImageType( ImageId const & image )noexcept;
	CRG_API ImageType getImageType( ImageViewId const & image )noexcept;
	CRG_API ImageViewType getImageViewType( ImageViewId const & image )noexcept;
	CRG_API uint32_t getMipLevels( ImageId const & image )noexcept;
	CRG_API uint32_t getMipLevels( ImageViewId const & image )noexcept;
	CRG_API uint32_t getArrayLayers( ImageId const & image )noexcept;
	CRG_API uint32_t getArrayLayers( ImageViewId const & image )noexcept;
	CRG_API ImageAspectFlags getAspectFlags( ImageViewId const & image )noexcept;
	CRG_API ImageSubresourceRange const & getSubresourceRange( ImageViewId const & format )noexcept;
	CRG_API AccessFlags getAccessMask( ImageLayout layout )noexcept;
	CRG_API PipelineStageFlags getStageMask( ImageLayout layout )noexcept;
	CRG_API PipelineState getPipelineState( PipelineStageFlags flags )noexcept;
	CRG_API LayoutState makeLayoutState( ImageLayout layout )noexcept;
	CRG_API ImageAspectFlags getAspectMask( PixelFormat format )noexcept;
	CRG_API LayoutState const & addSubresourceRangeLayout( LayerLayoutStates & ranges
		, ImageSubresourceRange const & range
		, LayoutState const & newLayout );
	CRG_API LayoutState getSubresourceRangeLayout( LayerLayoutStates const & ranges
		, ImageSubresourceRange const & range );
	CRG_API ImageSubresourceRange getVirtualRange( ImageId const & image
		, ImageViewType viewType
		, ImageSubresourceRange const & range )noexcept;
	CRG_API bool match( ImageViewId const & lhs, ImageViewId const & rhs )noexcept;
	CRG_API bool match( Buffer const & lhs, Buffer const & rhs )noexcept;
	CRG_API ImageViewId const & resolveView( ImageViewId const & view
		, uint32_t passIndex );

	CRG_API void convert( SemaphoreWaitArray const & toWait
		, std::vector< VkSemaphore > & semaphores
		, std::vector< VkPipelineStageFlags > & dstStageMasks );
	CRG_API std::vector< VkClearValue > convert( std::vector< ClearValue > const & v );
	CRG_API VkQueryPool createQueryPool( GraphContext & context
		, std::string const & name
		, uint32_t passesCount );

	CRG_API ClearColorValue getClearColorValue( ClearValue const & v );
	CRG_API ClearDepthStencilValue getClearDepthStencilValue( ClearValue const & v );

	constexpr VkFormat convert( PixelFormat v )noexcept
	{
		return VkFormat( v );
	}

	constexpr PixelFormat convert( VkFormat v )noexcept
	{
		return PixelFormat( v );
	}

	constexpr bool isDepthFormat( PixelFormat fmt )noexcept
	{
		return fmt == PixelFormat::eD16_UNORM
			|| fmt == PixelFormat::eX8_D24_UNORM
			|| fmt == PixelFormat::eD32_SFLOAT
			|| fmt == PixelFormat::eD16_UNORM_S8_UINT
			|| fmt == PixelFormat::eD24_UNORM_S8_UINT
			|| fmt == PixelFormat::eD32_SFLOAT_S8_UINT;
	}

	constexpr bool isStencilFormat( PixelFormat fmt )noexcept
	{
		return fmt == PixelFormat::eS8_UINT
			|| fmt == PixelFormat::eD16_UNORM_S8_UINT
			|| fmt == PixelFormat::eD24_UNORM_S8_UINT
			|| fmt == PixelFormat::eD32_SFLOAT_S8_UINT;
	}

	constexpr bool isColourFormat( PixelFormat fmt )noexcept
	{
		return !isDepthFormat( fmt ) && !isStencilFormat( fmt );
	}

	constexpr bool isDepthStencilFormat( PixelFormat fmt )noexcept
	{
		return isDepthFormat( fmt ) && isStencilFormat( fmt );
	}

	constexpr bool isDepthOrStencilFormat( PixelFormat fmt )noexcept
	{
		return isDepthFormat( fmt ) || isStencilFormat( fmt );
	}

	constexpr VkImageType convert( ImageType v )noexcept
	{
		return VkImageType( v );
	}

	constexpr ImageType convert( VkImageType  v )noexcept
	{
		return ImageType( v );
	}

	constexpr VkSampleCountFlagBits convert( SampleCount v )noexcept
	{
		return VkSampleCountFlagBits( v );
	}

	constexpr SampleCount convert( VkSampleCountFlagBits v )noexcept
	{
		return SampleCount( v );
	}

	constexpr VkImageTiling convert( ImageTiling v )noexcept
	{
		return VkImageTiling( v );
	}

	constexpr ImageTiling convert( VkImageTiling v )noexcept
	{
		return ImageTiling( v );
	}

	constexpr VkImageViewType convert( ImageViewType v )noexcept
	{
		return VkImageViewType( v );
	}

	constexpr ImageViewType convert( VkImageViewType  v )noexcept
	{
		return ImageViewType( v );
	}

	constexpr VkImageLayout convert( ImageLayout v )noexcept
	{
		return VkImageLayout( v );
	}

	constexpr ImageLayout convert( VkImageLayout  v )noexcept
	{
		return ImageLayout( v );
	}

	constexpr VkFilter convert( FilterMode v )noexcept
	{
		return VkFilter( v );
	}

	constexpr FilterMode convert( VkFilter  v )noexcept
	{
		return FilterMode( v );
	}

	constexpr VkSamplerMipmapMode convert( MipmapMode v )noexcept
	{
		return VkSamplerMipmapMode( v );
	}

	constexpr MipmapMode convert( VkSamplerMipmapMode v )noexcept
	{
		return MipmapMode( v );
	}

	constexpr VkSamplerAddressMode convert( WrapMode v )noexcept
	{
		return VkSamplerAddressMode( v );
	}

	constexpr WrapMode convert( VkSamplerAddressMode v )noexcept
	{
		return WrapMode( v );
	}

	constexpr VkAttachmentLoadOp convert( AttachmentLoadOp v )noexcept
	{
		return VkAttachmentLoadOp( v );
	}

	constexpr AttachmentLoadOp convert( VkAttachmentLoadOp v )noexcept
	{
		return AttachmentLoadOp( v );
	}

	constexpr VkBlendFactor convert( BlendFactor v )noexcept
	{
		return VkBlendFactor( v );
	}

	constexpr BlendFactor convert( VkBlendFactor v )noexcept
	{
		return BlendFactor( v );
	}

	constexpr VkBlendOp convert( BlendOp v )noexcept
	{
		return VkBlendOp( v );
	}

	constexpr BlendOp convert( VkBlendOp v )noexcept
	{
		return BlendOp( v );
	}

	constexpr VkAttachmentStoreOp convert( AttachmentStoreOp v )noexcept
	{
		return VkAttachmentStoreOp( v );
	}

	constexpr AttachmentStoreOp convert( VkAttachmentStoreOp v )noexcept
	{
		return AttachmentStoreOp( v );
	}

	constexpr VkImageCreateFlags getImageCreateFlags( ImageCreateFlags v )noexcept
	{
		return VkImageCreateFlags( v );
	}

	constexpr ImageCreateFlags getImageCreateFlags( VkImageCreateFlags v )noexcept
	{
		return ImageCreateFlags( v );
	}

	constexpr VkImageUsageFlags getImageUsageFlags( ImageUsageFlags v )noexcept
	{
		return VkImageUsageFlags( v );
	}

	constexpr ImageUsageFlags getImageUsageFlags( VkImageUsageFlags v )noexcept
	{
		return ImageUsageFlags( v );
	}

	constexpr VkImageViewCreateFlags getImageViewCreateFlags( ImageViewCreateFlags v )noexcept
	{
		return VkImageViewCreateFlags( v );
	}

	constexpr ImageViewCreateFlags getImageViewCreateFlags( VkImageViewCreateFlags v )noexcept
	{
		return ImageViewCreateFlags( v );
	}

	constexpr VkImageAspectFlags getImageAspectFlags( ImageAspectFlags v )noexcept
	{
		return VkImageAspectFlags( v );
	}

	constexpr ImageAspectFlags getImageAspectFlags( VkImageAspectFlags v )noexcept
	{
		return ImageAspectFlags( v );
	}

	constexpr VkPipelineStageFlags getPipelineStageFlags( PipelineStageFlags v )noexcept
	{
		return VkPipelineStageFlags( v );
	}

	constexpr PipelineStageFlags getPipelineStageFlags( VkPipelineStageFlags v )noexcept
	{
		return PipelineStageFlags( v );
	}

	constexpr VkAccessFlags getAccessFlags( AccessFlags v )noexcept
	{
		return VkAccessFlags( v );
	}

	constexpr AccessFlags getAccessFlags( VkAccessFlags v )noexcept
	{
		return AccessFlags( v );
	}

	constexpr VkColorComponentFlags getColorComponentFlags( ColorComponentFlags v )noexcept
	{
		return VkColorComponentFlags( v );
	}

	constexpr ColorComponentFlags getColorComponentFlags( VkColorComponentFlags v )noexcept
	{
		return ColorComponentFlags( v );
	}

	constexpr VkExtent2D convert( Extent2D const & v )noexcept
	{
		return std::bit_cast< VkExtent2D >( v );
	}

	constexpr Extent2D convert( VkExtent2D const & v )noexcept
	{
		return std::bit_cast< Extent2D >( v );
	}

	constexpr VkOffset2D convert( Offset2D const & v )noexcept
	{
		return std::bit_cast< VkOffset2D >( v );
	}

	constexpr Offset2D convert( VkOffset2D const & v )noexcept
	{
		return std::bit_cast< Offset2D >( v );
	}

	constexpr VkRect2D convert( Rect2D const & v )noexcept
	{
		return std::bit_cast< VkRect2D >( v );
	}

	constexpr Rect2D convert( VkRect2D const & v )noexcept
	{
		return std::bit_cast< Rect2D >( v );
	}

	constexpr VkExtent3D convert( Extent3D const & v )noexcept
	{
		return std::bit_cast< VkExtent3D >( v );
	}

	constexpr Extent3D convert( VkExtent3D const & v )noexcept
	{
		return std::bit_cast< Extent3D >( v );
	}

	constexpr VkOffset3D convert( Offset3D const & v )noexcept
	{
		return std::bit_cast< VkOffset3D >( v );
	}

	constexpr Offset3D convert( VkOffset3D const & v )noexcept
	{
		return std::bit_cast< Offset3D >( v );
	}

	constexpr VkImageSubresourceRange convert( ImageSubresourceRange const & v )noexcept
	{
		return std::bit_cast< VkImageSubresourceRange >( v );
	}

	constexpr ImageSubresourceRange convert( VkImageSubresourceRange const & v )noexcept
	{
		return std::bit_cast< ImageSubresourceRange >( v );
	}

	constexpr VkPipelineColorBlendAttachmentState convert( PipelineColorBlendAttachmentState const & v )noexcept
	{
		return std::bit_cast< VkPipelineColorBlendAttachmentState >( v );
	}

	constexpr PipelineColorBlendAttachmentState convert( VkPipelineColorBlendAttachmentState const & v )noexcept
	{
		return std::bit_cast< PipelineColorBlendAttachmentState >( v );
	}

	constexpr VkClearDepthStencilValue convert( ClearDepthStencilValue const & v )noexcept
	{
		return std::bit_cast< VkClearDepthStencilValue >( v );
	}

	constexpr VkClearColorValue convert( ClearColorValue const & v )noexcept
	{
		if ( v.isFloat32() )
		{
			VkClearColorValue result;
			result.float32[0] = v.float32()[0];
			result.float32[1] = v.float32()[1];
			result.float32[2] = v.float32()[2];
			result.float32[3] = v.float32()[3];
			return result;
		}

		if ( v.isInt32() )
		{
			VkClearColorValue result;
			result.int32[0] = v.int32()[0];
			result.int32[1] = v.int32()[1];
			result.int32[2] = v.int32()[2];
			result.int32[3] = v.int32()[3];
			return result;
		}


		VkClearColorValue result;
		result.uint32[0] = v.uint32()[0];
		result.uint32[1] = v.uint32()[1];
		result.uint32[2] = v.uint32()[2];
		result.uint32[3] = v.uint32()[3];
		return result;
	}

	constexpr VkClearValue convert( ClearValue const & v )noexcept
	{
		if ( v.isColor() )
		{
			VkClearValue result;
			result.color = convert( v.color() );
			return result;
		}

		VkClearValue result;
		result.depthStencil = convert( v.depthStencil() );
		return result;
	}

	constexpr VkImageViewCreateInfo convert( ImageViewCreateInfo const & v )noexcept
	{
		return VkImageViewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, nullptr
			, getImageViewCreateFlags( v.flags ), VkImage{}, convert( v.viewType ), convert( v.format )
			, { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY }
		, convert( v.subresourceRange ) };
	}

	constexpr ImageViewCreateInfo convert( VkImageViewCreateInfo const & v )noexcept
	{
		return ImageViewCreateInfo{ getImageViewCreateFlags( v.flags ), convert( v.viewType ), convert( v.format )
			, convert( v.subresourceRange ) };
	}

	constexpr VkImageCreateInfo convert( ImageCreateInfo const & v )noexcept
	{
		return VkImageCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, nullptr
			, getImageCreateFlags( v.flags ), convert( v.imageType ), convert( v.format )
			, convert( v.extent ), v.mipLevels, v.arrayLayers, convert( v.samples )
			, convert( v.tiling ), getImageUsageFlags( v.usage )
			, VK_SHARING_MODE_EXCLUSIVE, 0u, nullptr
			, VK_IMAGE_LAYOUT_UNDEFINED };
	}

	constexpr ImageCreateInfo convert( VkImageCreateInfo const & v )noexcept
	{
		return ImageCreateInfo{ getImageCreateFlags( v.flags ), convert( v.imageType ), convert( v.format )
			, convert( v.extent ), v.mipLevels, v.arrayLayers, convert( v.samples )
			, convert( v.tiling ), getImageUsageFlags( v.usage ) };
	}

	inline VkImageSubresourceLayers getSubresourceLayers( ImageSubresourceRange const & range
		, uint32_t layerCount )
	{
		return VkImageSubresourceLayers{ getImageAspectFlags( range.aspectMask )
			, range.baseMipLevel
			, range.baseArrayLayer
			, layerCount };
	}

	inline VkImageSubresourceLayers getSubresourceLayers( ImageSubresourceRange const & range )
	{
		return getSubresourceLayers( range, range.layerCount );
	}

	inline VkImageSubresourceLayers getSubresourceLayer( ImageSubresourceRange const & range )
	{
		return getSubresourceLayers( range, 1u );
	}
}
