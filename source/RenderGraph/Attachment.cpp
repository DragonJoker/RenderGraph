/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/Attachment.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/ImageViewData.hpp"
#include "RenderGraph/WriteDescriptorSet.hpp"

#include <cassert>
#include <cstring>

namespace crg
{
	namespace attach
	{
		static bool match( VkImageSubresourceRange const & lhsRange
			, VkImageSubresourceRange const & rhsRange )
		{
			return ( ( lhsRange.aspectMask & rhsRange.aspectMask ) != 0 )
				&& lhsRange.baseArrayLayer == rhsRange.baseArrayLayer
				&& lhsRange.layerCount == rhsRange.layerCount
				&& lhsRange.baseMipLevel == rhsRange.baseMipLevel
				&& lhsRange.levelCount == rhsRange.levelCount;
		}

		static bool match( ImageId const & image
			, VkImageViewType lhsType
			, VkImageViewType rhsType
			, VkImageSubresourceRange const & lhsRange
			, VkImageSubresourceRange const & rhsRange )
		{
			auto result = lhsType == rhsType;

			if ( !result )
			{
				result = match( getVirtualRange( image, lhsType, lhsRange )
					, getVirtualRange( image, rhsType, rhsRange ) );
			}
			else
			{
				result = match( lhsRange, rhsRange );
			}

			return result;
		}

		static bool match( ImageId const & image
			, VkImageViewCreateInfo const & lhsInfo
			, VkImageViewCreateInfo const & rhsInfo )
		{
			return lhsInfo.flags == rhsInfo.flags
				&& lhsInfo.format == rhsInfo.format
				&& match( image
					, lhsInfo.viewType, rhsInfo.viewType
					, lhsInfo.subresourceRange, rhsInfo.subresourceRange );
		}

		static VkAccessFlags getAccessMask( VkImageLayout layout )
		{
			VkAccessFlags result{ 0u };

			switch ( layout )
			{
			case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
				result |= VK_ACCESS_MEMORY_READ_BIT;
				break;
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				result |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
				result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
				break;
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				result |= VK_ACCESS_SHADER_READ_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				result |= VK_ACCESS_TRANSFER_READ_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				result |= VK_ACCESS_TRANSFER_WRITE_BIT;
				break;
			case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
				result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
				result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;
#ifdef VK_NV_shading_rate_image
			case VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV:
				result |= VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV;
				break;
#endif
#ifdef VK_EXT_fragment_density_map
			case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
				result |= VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT;
				break;
#endif
			default:
				break;
			}

			return result;
		}

		static VkPipelineStageFlags getStageMask( VkImageLayout layout )
		{
			VkPipelineStageFlags result{ 0u };

			switch ( layout )
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				result |= VK_PIPELINE_STAGE_HOST_BIT;
				break;
			case VK_IMAGE_LAYOUT_GENERAL:
				result |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
				break;
			case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
				result |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
				break;
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
			case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
				result |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				break;
			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				result |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				break;
#ifdef VK_EXT_fragment_density_map
			case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
#endif
			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				result |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				break;
			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				result |= VK_PIPELINE_STAGE_TRANSFER_BIT;
				break;
#ifdef VK_NV_shading_rate_image
			case VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV:
				result |= VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV;
				break;
#endif
			default:
				break;
			}

			return result;
		}
	}

	static bool operator==( VkClearValue const & lhs
		, VkClearValue const & rhs )
	{
		return std::memcmp( &lhs, &rhs, sizeof( VkClearValue ) ) == 0;
	}

	static bool operator==( VkPipelineColorBlendAttachmentState const & lhs
		, VkPipelineColorBlendAttachmentState const & rhs )
	{
		return lhs.blendEnable == rhs.blendEnable
			&& lhs.srcColorBlendFactor == rhs.srcColorBlendFactor
			&& lhs.dstColorBlendFactor == rhs.dstColorBlendFactor
			&& lhs.colorBlendOp == rhs.colorBlendOp
			&& lhs.srcAlphaBlendFactor == rhs.srcAlphaBlendFactor
			&& lhs.dstAlphaBlendFactor == rhs.dstAlphaBlendFactor
			&& lhs.alphaBlendOp == rhs.alphaBlendOp
			&& lhs.colorWriteMask == rhs.colorWriteMask;
	}

	bool isDepthFormat( VkFormat fmt )
	{
		return fmt == VK_FORMAT_D16_UNORM
			|| fmt == VK_FORMAT_X8_D24_UNORM_PACK32
			|| fmt == VK_FORMAT_D32_SFLOAT
			|| fmt == VK_FORMAT_D16_UNORM_S8_UINT
			|| fmt == VK_FORMAT_D24_UNORM_S8_UINT
			|| fmt == VK_FORMAT_D32_SFLOAT_S8_UINT;
	}

	bool isStencilFormat( VkFormat fmt )
	{
		return fmt == VK_FORMAT_S8_UINT
			|| fmt == VK_FORMAT_D16_UNORM_S8_UINT
			|| fmt == VK_FORMAT_D24_UNORM_S8_UINT
			|| fmt == VK_FORMAT_D32_SFLOAT_S8_UINT;
	}

	bool isColourFormat( VkFormat fmt )
	{
		return !isDepthFormat( fmt ) && !isStencilFormat( fmt );
	}

	bool isDepthStencilFormat( VkFormat fmt )
	{
		return isDepthFormat( fmt ) && isStencilFormat( fmt );
	}

	VkImageSubresourceRange getVirtualRange( ImageId const & image
		, VkImageViewType viewType
		, VkImageSubresourceRange const & range )
	{
		auto result = range;

		if ( viewType == VK_IMAGE_VIEW_TYPE_3D
			&& ( range.levelCount == 1u
				|| range.levelCount == image.data->info.mipLevels ) )
		{
			result.baseArrayLayer = 0u;
			result.layerCount = getExtent( image ).depth >> range.baseMipLevel;
		}

		return result;
	}

	bool match( ImageViewData const & lhs, ImageViewData const & rhs )
	{
		return lhs.image.id == rhs.image.id
			&& attach::match( lhs.image
				, lhs.info
				, rhs.info );
	}

	//*********************************************************************************************

	BufferAttachment::BufferAttachment()
		: buffer{ nullptr, std::string{} }
		, view{ nullptr }
		, range{}
		, flags{}
	{
	}

	BufferAttachment::BufferAttachment( Buffer buffer )
		: buffer{ std::move( buffer ) }
		, view{ nullptr }
		, range{}
		, flags{}
	{
	}

	BufferAttachment::BufferAttachment( FlagKind flags
		, Buffer buffer
		, VkDeviceSize offset
		, VkDeviceSize range )
		: buffer{ std::move( buffer ) }
		, view{ nullptr }
		, range{ offset, range }
		, flags{ flags }
	{
	}

	BufferAttachment::BufferAttachment( FlagKind flags
		, Buffer buffer
		, VkBufferView view
		, VkDeviceSize offset
		, VkDeviceSize range )
		: buffer{ std::move( buffer ) }
		, view{ view }
		, range{ offset, range }
		, flags{ flags }
	{
	}

	WriteDescriptorSet BufferAttachment::getWrite( uint32_t binding
		, uint32_t count )const
	{
		WriteDescriptorSet result{ binding
			, 0u
			, count
			, getDescriptorType() };

		if ( isView() )
		{
			result.bufferViewInfo.push_back( VkDescriptorBufferInfo{ buffer.buffer, range.offset, range.size } );
			result.texelBufferView.push_back( view );
		}
		else
		{
			result.bufferInfo.push_back( VkDescriptorBufferInfo{ buffer.buffer, range.offset, range.size } );
		}

		return result;
	}

	VkDescriptorType BufferAttachment::getDescriptorType()const
	{
		if ( isUniformView() )
		{
			return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		}

		if ( isStorageView() )
		{
			return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		}

		if ( isUniform() )
		{
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		}

		return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	}

	VkAccessFlags BufferAttachment::getAccessMask( bool isInput
		, bool isOutput )const
	{
		VkAccessFlags result{ 0u };

		if ( isStorage() )
		{
			if ( isInput )
			{
				result |= VK_ACCESS_SHADER_READ_BIT;
			}

			if ( isOutput )
			{
				result |= VK_ACCESS_SHADER_WRITE_BIT;
			}
		}
		else
		{
			result |= VK_ACCESS_SHADER_READ_BIT;
		}

		return result;
	}

	VkPipelineStageFlags BufferAttachment::getPipelineStageFlags( bool isCompute )const
	{
		VkPipelineStageFlags result{ 0u };

		if ( isCompute )
		{
			result |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		}
		else
		{
			result |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}

		return result;
	}

	//*********************************************************************************************

	ImageAttachment::ImageAttachment( ImageViewId view
		, ImageAttachment const & origin )
		: views{ { view } }
		, loadOp{ origin.loadOp }
		, storeOp{ origin.storeOp }
		, stencilLoadOp{ origin.stencilLoadOp }
		, stencilStoreOp{ origin.stencilStoreOp }
		, initialLayout{ origin.initialLayout }
		, samplerDesc{ origin.samplerDesc }
		, clearValue{ origin.clearValue }
		, blendState{ origin.blendState }
		, transitionLayout{ origin.transitionLayout }
		, flags{ origin.flags }
	{
	}

	ImageAttachment::ImageAttachment()
		: views{}
		, loadOp{}
		, storeOp{}
		, stencilLoadOp{}
		, stencilStoreOp{}
		, initialLayout{}
		, samplerDesc{}
		, clearValue{}
		, blendState{}
		, transitionLayout{}
		, flags{}
	{
	}

	ImageAttachment::ImageAttachment( ImageViewId view )
		: views{ 1u, view }
		, loadOp{}
		, storeOp{}
		, stencilLoadOp{}
		, stencilStoreOp{}
		, initialLayout{}
		, samplerDesc{}
		, clearValue{}
		, blendState{}
		, transitionLayout{}
		, flags{}
	{
	}

	ImageAttachment::ImageAttachment( FlagKind flags
		, ImageViewIdArray views
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, VkImageLayout initialLayout
		, VkImageLayout finalLayout
		, SamplerDesc samplerDesc
		, VkClearValue clearValue
		, VkPipelineColorBlendAttachmentState blendState
		, VkImageLayout transitionLayout )
		: views{ std::move( views ) }
		, loadOp{ loadOp }
		, storeOp{ storeOp }
		, stencilLoadOp{ stencilLoadOp }
		, stencilStoreOp{ stencilStoreOp }
		, initialLayout{ initialLayout }
		, finalLayout{ finalLayout }
		, samplerDesc{ std::move( samplerDesc ) }
		, clearValue{ std::move( clearValue ) }
		, blendState{ std::move( blendState ) }
		, transitionLayout{ transitionLayout }
		, flags{ FlagKind( flags
			| ( loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR
				? FlagKind( Flag::Clearing )
				: FlagKind( Flag::None ) )
			| ( stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR
				? FlagKind( Flag::Clearing )
				: FlagKind( Flag::None ) ) ) }
	{
		assert( ( ( view().data->info.subresourceRange.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT ) != 0
			&& isColourFormat( view().data->info.format ) )
			|| ( ( view().data->info.subresourceRange.aspectMask & ( VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT ) ) != 0
				&& isDepthStencilFormat( view().data->info.format ) )
			|| ( ( view().data->info.subresourceRange.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT ) != 0
				&& isDepthFormat( view().data->info.format ) )
			|| ( ( view().data->info.subresourceRange.aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT ) != 0
				&& isStencilFormat( view().data->info.format ) ) );
		assert( ( !isSampledView() && !isTransitionView() && !isTransferView() )
			|| ( ( this->loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE )
				&& ( this->storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE )
				&& ( this->stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE )
				&& ( this->stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE ) ) );
	}

	ImageViewId ImageAttachment::view( uint32_t index )const
	{
		return views.size() == 1u
			? views.front()
			: views[index];
	}

	VkDescriptorType ImageAttachment::getDescriptorType()const
	{
		if ( isStorageView() )
		{
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		}

		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	}

	VkImageLayout ImageAttachment::getImageLayout( bool separateDepthStencilLayouts
		, bool isInput
		, bool isOutput )const
	{
		if ( isSampledView() )
		{
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		if ( isTransitionView() )
		{
			return transitionLayout;
		}

		if ( isStorageView() )
		{
			return VK_IMAGE_LAYOUT_GENERAL;
		}

		if ( isTransferView() )
		{
			if ( isInput )
			{
				return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			}

			if ( isOutput )
			{
				return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			}
		}

		if ( isColourAttach() )
		{
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}

#if VK_KHR_separate_depth_stencil_layouts
		if ( separateDepthStencilLayouts )
		{
			if ( isOutput )
			{
				if ( isDepthStencilAttach() )
				{
					return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				}

				if ( isDepthAttach() )
				{
					return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
				}
			}

			if ( isInput )
			{
				if ( isDepthAttach() )
				{
					return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
				}

				if ( isDepthStencilAttach() )
				{
					return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				}
			}

			if ( isStencilOutputAttach() )
			{
				return VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
			}

			if ( isStencilInputAttach() )
			{
				return VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
			}
		}
		else
#endif
		{
			if ( isOutput )
			{
				if ( isDepthAttach()
					|| isStencilOutputAttach() )
				{
					return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				}
			}

			if ( isInput )
			{
				if ( isDepthAttach()
					|| isStencilInputAttach() )
				{
					return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				}
			}
		}

		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	VkAccessFlags ImageAttachment::getAccessMask( bool isInput
		, bool isOutput )const
	{
		VkAccessFlags result{ 0u };

		if ( isSampledView() )
		{
			result |= VK_ACCESS_SHADER_READ_BIT;
		}
		else if ( isTransitionView() )
		{
			result |= attach::getAccessMask( transitionLayout );
		}
		else if ( isStorageView() )
		{
			if ( isInput )
			{
				result |= VK_ACCESS_SHADER_READ_BIT;
			}

			if ( isOutput )
			{
				result |= VK_ACCESS_SHADER_WRITE_BIT;
			}
		}
		else if ( isTransferView() )
		{
			if ( isInput )
			{
				result |= VK_ACCESS_TRANSFER_READ_BIT;
			}

			if ( isOutput )
			{
				result |= VK_ACCESS_TRANSFER_WRITE_BIT;
			}
		}
		else if ( isDepthAttach() || isStencilAttach() )
		{
			if ( isInput )
			{
				result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			}

			if ( isOutput )
			{
				result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			}
		}
		else
		{
			if ( isInput )
			{
				result |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			}

			if ( isOutput )
			{
				result |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			}
		}

		return result;
	}

	VkPipelineStageFlags ImageAttachment::getPipelineStageFlags( bool isCompute )const
	{
		VkPipelineStageFlags result{ 0u };

		if ( isSampledView() )
		{
			result |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if ( isTransitionView() )
		{
			result |= attach::getStageMask( transitionLayout );
		}
		else if ( isStorageView() )
		{
			if ( isCompute )
			{
				result |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			}
			else
			{
				result |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
		}
		else if ( isTransferView() )
		{
			result |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if ( isDepthAttach() || isStencilAttach() )
		{
			result |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		}
		else if ( !isTransitionView() )
		{
			result |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}

		return result;
	}

	//*********************************************************************************************

	Attachment::Attachment( ImageViewId view
		, Attachment const & origin )
		: pass{ origin.pass }
		, binding{ origin.binding }
		, name{ origin.name + view.data->name }
		, image{ origin.image }
		, buffer{ origin.buffer }
		, flags{ origin.flags }
	{
	}

	Attachment::Attachment( ImageViewId view )
		: binding{}
		, name{}
		, image{ view }
		, flags{}
	{
	}

	Attachment::Attachment( Buffer buffer )
		: binding{}
		, name{}
		, buffer{ buffer }
		, flags{}
	{
	}

	Attachment::Attachment( FlagKind flags
		, FramePass & pass
		, uint32_t binding
		, uint32_t count
		, std::string name
		, ImageAttachment::FlagKind imageFlags
		, ImageViewIdArray views
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, VkImageLayout initialLayout
		, VkImageLayout finalLayout
		, SamplerDesc samplerDesc
		, VkClearValue clearValue
		, VkPipelineColorBlendAttachmentState blendState
		, VkImageLayout transitionLayout )
		: pass{ &pass }
		, binding{ binding }
		, count{ count }
		, name{ std::move( name ) }
		, image{ imageFlags
			, std::move( views )
			, loadOp
			, storeOp
			, stencilLoadOp
			, stencilStoreOp
			, initialLayout
			, finalLayout
			, std::move( samplerDesc )
			, std::move( clearValue )
			, std::move( blendState )
			, transitionLayout }
		, flags{ FlagKind( flags
			| FlagKind( Flag::Image )
			| ( loadOp == VK_ATTACHMENT_LOAD_OP_LOAD
				? FlagKind( Flag::Input )
				: FlagKind( Flag::None ) )
			| ( storeOp == VK_ATTACHMENT_STORE_OP_STORE
				? FlagKind( Flag::Output )
				: FlagKind( Flag::None ) )
			| ( stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD
				? FlagKind( Flag::Input )
				: FlagKind( Flag::None ) )
			| ( stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE
				? FlagKind( Flag::Output )
				: FlagKind( Flag::None ) ) ) }
	{
	}

	Attachment::Attachment( FlagKind flags
		, FramePass & pass
		, uint32_t binding
		, uint32_t count
		, std::string name
		, BufferAttachment::FlagKind bufferFlags
		, Buffer buffer
		, VkDeviceSize offset
		, VkDeviceSize range )
		: pass{ &pass }
		, binding{ binding }
		, count{ count }
		, name{ std::move( name ) }
		, buffer{ bufferFlags, std::move( buffer ), offset, range }
		, flags{ FlagKind( flags
			| FlagKind( Flag::Buffer ) ) }
	{
	}

	Attachment::Attachment( FlagKind flags
		, FramePass & pass
		, uint32_t binding
		, uint32_t count
		, std::string name
		, BufferAttachment::FlagKind bufferFlags
		, Buffer buffer
		, VkBufferView view
		, VkDeviceSize offset
		, VkDeviceSize range )
		: pass{ &pass }
		, count{ count }
		, name{ std::move( name ) }
		, buffer{ bufferFlags, std::move( buffer ), view, offset, range }
		, flags{ FlagKind( flags
			| FlagKind( Flag::Buffer ) ) }
	{
	}

	ImageViewId Attachment::view( uint32_t index )const
	{
		return isImage()
			? image.view( index )
			: ImageViewId{};
	}

	VkImageLayout Attachment::getImageLayout( bool separateDepthStencilLayouts )const
	{
		assert( isImage() );
		return image.getImageLayout( separateDepthStencilLayouts
			, isInput()
			, isOutput() );
	}

	VkDescriptorType Attachment::getDescriptorType()const
	{
		if ( isImage() )
		{
			return image.getDescriptorType();
		}

		return buffer.getDescriptorType();
	}

	WriteDescriptorSet Attachment::getBufferWrite()const
	{
		assert( isBuffer() );
		return buffer.getWrite( binding, count );
	}

	VkAccessFlags Attachment::getAccessMask()const
	{
		if ( isImage() )
		{
			return image.getAccessMask( isInput()
				, isOutput() );
		}

		return buffer.getAccessMask( isInput()
			, isOutput() );
	}

	VkPipelineStageFlags Attachment::getPipelineStageFlags( bool isCompute )const
	{
		if ( isImage() )
		{
			return image.getPipelineStageFlags( isCompute );
		}

		return buffer.getPipelineStageFlags( isCompute );
	}

	//*********************************************************************************************

	bool operator==( SamplerDesc const & lhs
		, SamplerDesc const & rhs )
	{
		return lhs.magFilter == rhs.magFilter
			&& lhs.minFilter == rhs.minFilter
			&& lhs.mipmapMode == rhs.mipmapMode
			&& lhs.addressModeU == rhs.addressModeU
			&& lhs.addressModeV == rhs.addressModeV
			&& lhs.addressModeW == rhs.addressModeW
			&& lhs.mipLodBias == rhs.mipLodBias
			&& lhs.minLod == rhs.minLod
			&& lhs.maxLod == rhs.maxLod;
	}

	bool operator==( ImageAttachment const & lhs
		, ImageAttachment const & rhs )
	{
		return lhs.flags == rhs.flags
			&& lhs.views == rhs.views
			&& lhs.loadOp == rhs.loadOp
			&& lhs.storeOp == rhs.storeOp
			&& lhs.stencilLoadOp == rhs.stencilLoadOp
			&& lhs.stencilStoreOp == rhs.stencilStoreOp
			&& lhs.samplerDesc == rhs.samplerDesc
			&& lhs.clearValue == rhs.clearValue
			&& lhs.blendState == rhs.blendState;
	}

	bool operator!=( ImageAttachment const & lhs
		, ImageAttachment const & rhs )
	{
		return !( lhs == rhs );
	}

	bool operator==( Attachment const & lhs
		, Attachment const & rhs )
	{
		return lhs.pass == rhs.pass
			&& lhs.flags == rhs.flags
			&& lhs.image == rhs.image;
	}

	bool operator!=( Attachment const & lhs
		, Attachment const & rhs )
	{
		return !( lhs == rhs );
	}

	//*********************************************************************************************
}
