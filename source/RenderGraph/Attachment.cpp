/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/Attachment.hpp"
#include "RenderGraph/WriteDescriptorSet.hpp"

#include <cassert>
#include <cstring>

namespace crg
{
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

	//*********************************************************************************************

	BufferAttachment::BufferAttachment()
		: buffer{ VK_NULL_HANDLE }
		, view{ VK_NULL_HANDLE }
		, offset{}
		, range{}
		, flags{}
	{
	}

	BufferAttachment::BufferAttachment( FlagKind flags
		, VkBuffer buffer
		, VkDeviceSize offset
		, VkDeviceSize range )
		: buffer{ buffer }
		, view{ VK_NULL_HANDLE }
		, offset{ offset }
		, range{ range }
		, flags{ flags }
	{
	}

	BufferAttachment::BufferAttachment( FlagKind flags
		, VkBuffer buffer
		, VkBufferView view
		, VkDeviceSize offset
		, VkDeviceSize range )
		: buffer{ buffer }
		, view{ view }
		, offset{ offset }
		, range{ range }
		, flags{ flags }
	{
	}

	WriteDescriptorSet BufferAttachment::getWrite( uint32_t binding )const
	{
		WriteDescriptorSet result{ binding
			, 0u
			, 1u
			, getDescriptorType() };

		if ( isView() )
		{
			result.bufferViewInfo.push_back( VkDescriptorBufferInfo{ buffer, offset, range } );
			result.texelBufferView.push_back( view );
		}
		else
		{
			result.bufferInfo.push_back( VkDescriptorBufferInfo{ buffer, offset, range } );
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

	//*********************************************************************************************

	ImageAttachment::ImageAttachment( ImageViewId view
		, ImageAttachment const & origin )
		: name{ origin.name + view.data->name }
		, views{ { view } }
		, loadOp{ origin.loadOp }
		, storeOp{ origin.storeOp }
		, stencilLoadOp{ origin.stencilLoadOp }
		, stencilStoreOp{ origin.stencilStoreOp }
		, initialLayout{ origin.initialLayout }
		, samplerDesc{ origin.samplerDesc }
		, clearValue{ origin.clearValue }
		, blendState{ origin.blendState }
		, flags{ origin.flags }
	{
	}

	ImageAttachment::ImageAttachment()
		: name{}
		, views{}
		, loadOp{}
		, storeOp{}
		, stencilLoadOp{}
		, stencilStoreOp{}
		, initialLayout{}
		, samplerDesc{}
		, clearValue{}
		, blendState{}
		, flags{}
	{
	}

	ImageAttachment::ImageAttachment( ImageViewId view )
		: name{}
		, views{ 1u, view }
		, loadOp{}
		, storeOp{}
		, stencilLoadOp{}
		, stencilStoreOp{}
		, initialLayout{}
		, samplerDesc{}
		, clearValue{}
		, blendState{}
		, flags{}
	{
	}

	ImageAttachment::ImageAttachment( FlagKind flags
		, std::string name
		, ImageViewIdArray views
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, VkImageLayout initialLayout
		, SamplerDesc samplerDesc
		, VkClearValue clearValue
		, VkPipelineColorBlendAttachmentState blendState )
		: name{ std::move( name ) }
		, views{ std::move( views ) }
		, loadOp{ loadOp }
		, storeOp{ storeOp }
		, stencilLoadOp{ stencilLoadOp }
		, stencilStoreOp{ stencilStoreOp }
		, initialLayout{ initialLayout }
		, samplerDesc{ std::move( samplerDesc ) }
		, clearValue{ std::move( clearValue ) }
		, blendState{ std::move( blendState ) }
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
		assert( !isSampled()
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
		if ( isStorage() )
		{
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		}

		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	}

	//*********************************************************************************************

	Attachment::Attachment( ImageViewId view
		, Attachment const & origin )
		: pass{ origin.pass }
		, binding{ origin.binding }
		, image{ origin.image }
		, buffer{ origin.buffer }
		, flags{ origin.flags }
	{
	}

	Attachment::Attachment( ImageViewId view )
		: image{ view }
	{
	}

	Attachment::Attachment( FlagKind flags
		, FramePass & pass
		, uint32_t binding
		, ImageAttachment::FlagKind imageFlags
		, std::string name
		, ImageViewIdArray views
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, VkImageLayout initialLayout
		, SamplerDesc samplerDesc
		, VkClearValue clearValue
		, VkPipelineColorBlendAttachmentState blendState )
		: pass{ &pass }
		, binding{ binding }
		, image{ imageFlags
			, std::move( name )
			, std::move( views )
			, loadOp
			, storeOp
			, stencilLoadOp
			, stencilStoreOp
			, initialLayout
			, std::move( samplerDesc )
			, std::move( clearValue )
			, std::move( blendState ) }
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
		, BufferAttachment::FlagKind bufferFlags
		, VkBuffer buffer
		, VkDeviceSize offset
		, VkDeviceSize range )
		: pass{ &pass }
		, binding{ binding }
		, buffer{ bufferFlags, buffer, offset, range }
		, flags{ FlagKind( flags
			| FlagKind( Flag::Buffer ) ) }
	{
	}

	Attachment::Attachment( FlagKind flags
		, FramePass & pass
		, uint32_t binding
		, BufferAttachment::FlagKind bufferFlags
		, VkBuffer buffer
		, VkBufferView view
		, VkDeviceSize offset
		, VkDeviceSize range )
		: pass{ &pass }
		, binding{ binding }
		, buffer{ bufferFlags, buffer, view, offset, range }
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
		if ( isSampled() )
		{
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		if ( isStorage() )
		{
			return VK_IMAGE_LAYOUT_GENERAL;
		}

		if ( isTransferInput() )
		{
			return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		}

		if ( isTransferOutput() )
		{
			return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		}

		if ( isColourInput()
			|| isColourOutput() )
		{
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}

#if VK_KHR_separate_depth_stencil_layouts
		if ( separateDepthStencilLayouts )
		{
			if ( isDepthStencilOutput() )
			{
				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}

			if ( isDepthStencilInput() )
			{
				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}

			if ( isDepthOutput() )
			{
				return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			}

			if ( isDepthInput() )
			{
				return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
			}

			if ( isStencilOutput() )
			{
				return VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
			}

			if ( isStencilInput() )
			{
				return VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
			}
		}
		else
#endif
		{
			if ( isDepthOutput()
				|| isStencilOutput() )
			{
				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}

			if ( isDepthInput()
				|| isStencilInput() )
			{
				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}
		}

		return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
		return buffer.getWrite( binding );
	}

	//*********************************************************************************************

	bool operator==( VkClearValue const & lhs
		, VkClearValue const & rhs )
	{
		return std::memcmp( &lhs, &rhs, sizeof( VkClearValue ) ) == 0;
	}

	bool operator==( VkPipelineColorBlendAttachmentState const & lhs
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
