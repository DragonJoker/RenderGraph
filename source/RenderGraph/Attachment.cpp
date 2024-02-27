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
	//*********************************************************************************************

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

	//*********************************************************************************************

	BufferAttachment::BufferAttachment( Buffer buffer )
		: buffer{ std::move( buffer ) }
	{
	}

	BufferAttachment::BufferAttachment( FlagKind flags
		, Buffer buffer
		, VkDeviceSize offset
		, VkDeviceSize range )
		: buffer{ std::move( buffer ) }
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
		, uint32_t count
		, uint32_t index )const
	{
		WriteDescriptorSet result{ binding
			, 0u
			, count
			, getDescriptorType() };

		if ( isView() )
		{
			result.bufferViewInfo.push_back( VkDescriptorBufferInfo{ buffer.buffer( index ), range.offset, range.size } );
			result.texelBufferView.push_back( view );
		}
		else
		{
			result.bufferInfo.push_back( VkDescriptorBufferInfo{ buffer.buffer( index ), range.offset, range.size } );
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
		, samplerDesc{ origin.samplerDesc }
		, clearValue{ origin.clearValue }
		, blendState{ origin.blendState }
		, wantedLayout{ origin.wantedLayout }
		, flags{ origin.flags }
	{
	}

	ImageAttachment::ImageAttachment( ImageViewId view )
		: views{ 1u, view }
	{
	}

	ImageAttachment::ImageAttachment( FlagKind flags
		, ImageViewIdArray views
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, SamplerDesc samplerDesc
		, VkClearValue clearValue
		, VkPipelineColorBlendAttachmentState blendState
		, VkImageLayout wantedLayout )
		: views{ std::move( views ) }
		, loadOp{ loadOp }
		, storeOp{ storeOp }
		, stencilLoadOp{ stencilLoadOp }
		, stencilStoreOp{ stencilStoreOp }
		, samplerDesc{ std::move( samplerDesc ) }
		, clearValue{ std::move( clearValue ) }
		, blendState{ std::move( blendState ) }
		, wantedLayout{ wantedLayout }
		, flags{ flags }
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
			return wantedLayout;
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
			if ( isOutput
				&& ( isDepthAttach() || isStencilOutputAttach() ) )
			{
				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}

			if ( isInput
				&& ( isDepthAttach() || isStencilInputAttach() ) )
			{
				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
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
			result |= crg::getAccessMask( wantedLayout );
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
			result |= getStageMask( wantedLayout );
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
		else
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
		, imageAttach{ origin.imageAttach }
		, bufferAttach{ origin.bufferAttach }
		, flags{ origin.flags }
	{
	}

	Attachment::Attachment( ImageViewId view )
		: imageAttach{ std::move( view ) }
	{
	}

	Attachment::Attachment( Buffer buffer )
		: bufferAttach{ std::move( buffer ) }
	{
	}

	Attachment::Attachment( FlagKind flags
		, FramePass & pass
		, uint32_t binding
		, std::string name
		, ImageAttachment::FlagKind imageFlags
		, ImageViewIdArray views
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, SamplerDesc samplerDesc
		, VkClearValue clearValue
		, VkPipelineColorBlendAttachmentState blendState
		, VkImageLayout wantedLayout )
		: pass{ &pass }
		, binding{ binding }
		, name{ std::move( name ) }
		, imageAttach{ imageFlags
			, std::move( views )
			, loadOp
			, storeOp
			, stencilLoadOp
			, stencilStoreOp
			, std::move( samplerDesc )
			, std::move( clearValue )
			, std::move( blendState )
			, wantedLayout }
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
		, std::string name
		, BufferAttachment::FlagKind bufferFlags
		, Buffer buffer
		, VkDeviceSize offset
		, VkDeviceSize range )
		: pass{ &pass }
		, binding{ binding }
		, name{ std::move( name ) }
		, bufferAttach{ bufferFlags, std::move( buffer ), offset, range }
		, flags{ FlagKind( flags
			| FlagKind( Flag::Buffer ) ) }
	{
	}

	Attachment::Attachment( FlagKind flags
		, FramePass & pass
		, uint32_t binding
		, std::string name
		, BufferAttachment::FlagKind bufferFlags
		, Buffer buffer
		, VkBufferView view
		, VkDeviceSize offset
		, VkDeviceSize range )
		: pass{ &pass }
		, binding{ binding }
		, name{ std::move( name ) }
		, bufferAttach{ bufferFlags, std::move( buffer ), view, offset, range }
		, flags{ FlagKind( flags
			| FlagKind( Flag::Buffer ) ) }
	{
	}

	uint32_t Attachment::getViewCount()const
	{
		return isImage()
			? imageAttach.getViewCount()
			: uint32_t{};
	}

	uint32_t Attachment::getBufferCount()const
	{
		return isBuffer()
			? bufferAttach.getBufferCount()
			: uint32_t{};
	}

	ImageViewId Attachment::view( uint32_t index )const
	{
		return isImage()
			? imageAttach.view( index )
			: ImageViewId{};
	}

	VkBuffer Attachment::buffer( uint32_t index )const
	{
		return isBuffer()
			? bufferAttach.buffer.buffer( index )
			: VkBuffer{};
	}

	VkImageLayout Attachment::getImageLayout( bool separateDepthStencilLayouts )const
	{
		assert( isImage() );
		return imageAttach.getImageLayout( separateDepthStencilLayouts
			, isInput()
			, isOutput() );
	}

	VkDescriptorType Attachment::getDescriptorType()const
	{
		if ( isImage() )
		{
			return imageAttach.getDescriptorType();
		}

		return bufferAttach.getDescriptorType();
	}

	WriteDescriptorSet Attachment::getBufferWrite( uint32_t index )const
	{
		assert( isBuffer() );
		return bufferAttach.getWrite( binding, 1u, index );
	}

	VkAccessFlags Attachment::getAccessMask()const
	{
		if ( isImage() )
		{
			return imageAttach.getAccessMask( isInput()
				, isOutput() );
		}

		return bufferAttach.getAccessMask( isInput()
			, isOutput() );
	}

	VkPipelineStageFlags Attachment::getPipelineStageFlags( bool isCompute )const
	{
		if ( isImage() )
		{
			return imageAttach.getPipelineStageFlags( isCompute );
		}

		return bufferAttach.getPipelineStageFlags( isCompute );
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

	bool operator==( BufferAttachment const & lhs
		, BufferAttachment const & rhs )
	{
		return lhs.flags == rhs.flags
			&& lhs.buffer == rhs.buffer
			&& lhs.view == rhs.view
			&& lhs.range.offset == rhs.range.offset
			&& lhs.range.size == rhs.range.size;
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

	bool operator==( Attachment const & lhs
		, Attachment const & rhs )
	{
		return lhs.pass == rhs.pass
			&& lhs.flags == rhs.flags
			&& lhs.imageAttach == rhs.imageAttach
			&& lhs.bufferAttach == rhs.bufferAttach;
	}

	//*********************************************************************************************
}
