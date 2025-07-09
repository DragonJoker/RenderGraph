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
		, VkDeviceSize range
		, AccessState access )
		: buffer{ std::move( buffer ) }
		, range{ offset, range }
		, flags{ flags }
		, wantedAccess{ std::move( access ) }
	{
	}

	BufferAttachment::BufferAttachment( FlagKind flags
		, Buffer buffer
		, VkBufferView view
		, VkDeviceSize offset
		, VkDeviceSize range
		, AccessState access )
		: buffer{ std::move( buffer ) }
		, view{ view }
		, range{ offset, range }
		, flags{ flags }
		, wantedAccess{ std::move( access ) }
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

	AccessFlags BufferAttachment::getAccessMask( bool isInput
		, bool isOutput )const
	{
		AccessFlags result{ 0u };

		if ( isTransition() )
		{
			result = wantedAccess.access;
		}
		else if ( isStorage() )
		{
			if ( isInput )
			{
				result |= AccessFlags::eShaderRead;
			}

			if ( isOutput )
			{
				result |= AccessFlags::eShaderWrite;
			}
		}
		else if ( isTransfer() )
		{
			if ( isInput )
			{
				result |= AccessFlags::eTransferRead;
			}

			if ( isOutput )
			{
				result |= AccessFlags::eTransferWrite;
			}
		}
		else
		{
			result |= AccessFlags::eShaderRead;
		}

		return result;
	}

	PipelineStageFlags BufferAttachment::getPipelineStageFlags( bool isCompute )const
	{
		PipelineStageFlags result{ 0u };

		if ( isTransition() )
		{
			result = wantedAccess.pipelineStage;
		}
		else if ( isTransfer() )
		{
			result |= PipelineStageFlags::eTransfer;
		}
		else if ( isCompute )
		{
			result |= PipelineStageFlags::eComputeShader;
		}
		else
		{
			result |= PipelineStageFlags::eFragmentShader;
		}

		return result;
	}

	//*********************************************************************************************

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
		, ImageLayout wantedLayout )
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
			&& isColourFormat( convert( view().data->info.format ) ) )
			|| ( ( view().data->info.subresourceRange.aspectMask & ( VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT ) ) != 0
				&& isDepthStencilFormat( convert( view().data->info.format ) ) )
			|| ( ( view().data->info.subresourceRange.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT ) != 0
				&& isDepthFormat( convert( view().data->info.format ) ) )
			|| ( ( view().data->info.subresourceRange.aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT ) != 0
				&& isStencilFormat( convert( view().data->info.format ) ) ) );
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

	ImageLayout ImageAttachment::getImageLayout( bool separateDepthStencilLayouts
		, bool isInput
		, bool isOutput )const
	{
		auto result = ImageLayout::eReadOnly;

		if ( isSampledView() )
		{
			result = ImageLayout::eShaderReadOnly;
		}
		else if ( isTransitionView() )
		{
			result = wantedLayout;
		}
		else if ( isStorageView() )
		{
			result = ImageLayout::eGeneral;
		}
		else if ( isTransferView() )
		{
			if ( isOutput )
			{
				result = ImageLayout::eTransferDst;
			}
			else if ( isInput )
			{
				result = ImageLayout::eTransferSrc;
			}
		}
		else if ( isColourAttach() )
		{
			result = ImageLayout::eColorAttachment;
		}
#if VK_KHR_separate_depth_stencil_layouts
		else if ( separateDepthStencilLayouts )
		{
			if ( isDepthStencilAttach() )
			{
				if ( isOutput )
				{
					result = ImageLayout::eDepthStencilAttachment;
				}
				else if ( isInput )
				{
					result = ImageLayout::eDepthStencilReadOnly;
				}
			}
			else if ( isStencilAttach() )
			{
				if ( isOutput && isStencilOutputAttach() )
				{
					result = ImageLayout::eStencilAttachment;
				}
				else if ( isInput && isStencilInputAttach() )
				{
					result = ImageLayout::eStencilReadOnly;
				}
			}
			else if ( isDepthAttach() )
			{
				if ( isOutput )
				{
					result = ImageLayout::eDepthAttachment;
				}
				else if ( isInput )
				{
					result = ImageLayout::eDepthReadOnly;
				}
			}
		}
		else
#endif
		{
			if ( isOutput
				&& ( isDepthAttach() || isStencilOutputAttach() ) )
			{
				result = ImageLayout::eDepthStencilAttachment;
			}
			else if ( isInput
				&& ( isDepthAttach() || isStencilInputAttach() ) )
			{
				result = ImageLayout::eDepthStencilReadOnly;
			}
		}

		return result;
	}

	AccessFlags ImageAttachment::getAccessMask( bool isInput
		, bool isOutput )const
	{
		AccessFlags result{ 0u };

		if ( isSampledView() )
		{
			result |= AccessFlags::eShaderRead;
		}
		else if ( isTransitionView() )
		{
			result |= crg::getAccessMask( wantedLayout );
		}
		else if ( isStorageView() )
		{
			if ( isInput )
			{
				result |= AccessFlags::eShaderRead;
			}

			if ( isOutput )
			{
				result |= AccessFlags::eShaderWrite;
			}
		}
		else if ( isTransferView() )
		{
			if ( isInput )
			{
				result |= AccessFlags::eTransferRead;
			}

			if ( isOutput )
			{
				result |= AccessFlags::eTransferWrite;
			}
		}
		else if ( isDepthAttach() || isStencilAttach() )
		{
			if ( isInput )
			{
				result |= AccessFlags::eDepthStencilAttachmentRead;
			}

			if ( isOutput )
			{
				result |= AccessFlags::eDepthStencilAttachmentWrite;
			}
		}
		else
		{
			if ( isInput )
			{
				result |= AccessFlags::eColorAttachmentRead;
			}

			if ( isOutput )
			{
				result |= AccessFlags::eColorAttachmentWrite;
			}
		}

		return result;
	}

	PipelineStageFlags ImageAttachment::getPipelineStageFlags( bool isCompute )const
	{
		PipelineStageFlags result{ 0u };

		if ( isSampledView() )
		{
			result |= PipelineStageFlags::eFragmentShader;
		}
		else if ( isTransitionView() )
		{
			result |= getStageMask( wantedLayout );
		}
		else if ( isStorageView() )
		{
			if ( isCompute )
			{
				result |= PipelineStageFlags::eComputeShader;
			}
			else
			{
				result |= PipelineStageFlags::eFragmentShader;
			}
		}
		else if ( isTransferView() )
		{
			result |= PipelineStageFlags::eTransfer;
		}
		else if ( isDepthAttach() || isStencilAttach() )
		{
			result |= PipelineStageFlags::eLateFragmentTests;
		}
		else
		{
			result |= PipelineStageFlags::eColorAttachmentOutput;
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
		, flags{ FlagKind( Attachment::Flag::Image ) }
	{
	}

	Attachment::Attachment( Buffer buffer )
		: bufferAttach{ std::move( buffer ) }
		, flags{ FlagKind( Attachment::Flag::Buffer ) }
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
		, ImageLayout wantedLayout )
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
		, VkDeviceSize range
		, AccessState wantedAccess )
		: pass{ &pass }
		, binding{ binding }
		, name{ std::move( name ) }
		, bufferAttach{ bufferFlags, std::move( buffer ), offset, range, std::move( wantedAccess ) }
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
		, VkDeviceSize range
		, AccessState wantedAccess )
		: pass{ &pass }
		, binding{ binding }
		, name{ std::move( name ) }
		, bufferAttach{ bufferFlags, std::move( buffer ), view, offset, range, std::move( wantedAccess ) }
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

	ImageLayout Attachment::getImageLayout( bool separateDepthStencilLayouts )const
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

	AccessFlags Attachment::getAccessMask()const
	{
		if ( isImage() )
		{
			return imageAttach.getAccessMask( isInput()
				, isOutput() );
		}

		return bufferAttach.getAccessMask( isInput()
			, isOutput() );
	}

	PipelineStageFlags Attachment::getPipelineStageFlags( bool isCompute )const
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
