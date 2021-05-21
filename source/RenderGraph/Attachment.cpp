/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/Attachment.hpp"

#include <cassert>
#include <cstring>

namespace crg
{
	namespace
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
	}

	Attachment::Attachment( ImageViewId view )
		: view{ view }
	{
	}

	Attachment::Attachment( FlagKind flags
		, FramePass & pass
		, std::string name
		, ImageViewId view
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, VkImageLayout initialLayout
		, uint32_t binding
		, VkFilter filter
		, VkClearValue clearValue
		, VkPipelineColorBlendAttachmentState blendState )
		: pass{ &pass }
		, name{ std::move( name ) }
		, view{ std::move( view ) }
		, loadOp{ loadOp }
		, storeOp{ storeOp }
		, stencilLoadOp{ stencilLoadOp }
		, stencilStoreOp{ stencilStoreOp }
		, flags{ FlagKind( flags
			| ( loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR
				? FlagKind( Flag::Clearing )
				: FlagKind( Flag::None ) )
			| ( loadOp == VK_ATTACHMENT_LOAD_OP_LOAD
				? FlagKind( Flag::Input )
				: FlagKind( Flag::None ) )
			| ( storeOp == VK_ATTACHMENT_STORE_OP_STORE
				? FlagKind( Flag::Output )
				: FlagKind( Flag::None ) )
			| ( stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR
				? FlagKind( Flag::Clearing )
				: FlagKind( Flag::None ) )
			| ( stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD
				? FlagKind( Flag::Input )
				: FlagKind( Flag::None ) )
			| ( stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE
				? FlagKind( Flag::Output )
				: FlagKind( Flag::None ) ) ) }
		, initialLayout{ initialLayout }
		, filter{ filter }
		, binding{ binding }
		, clearValue{ std::move( clearValue ) }
		, blendState{ std::move( blendState ) }
	{
		assert( ( ( view.data->info.subresourceRange.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT ) != 0
				&& isColourFormat( view.data->info.format ) )
			|| ( ( view.data->info.subresourceRange.aspectMask & ( VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT ) ) != 0
				&& isDepthStencilFormat( view.data->info.format ) )
			|| ( ( view.data->info.subresourceRange.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT ) != 0
				&& isDepthFormat( view.data->info.format ) )
			|| ( ( view.data->info.subresourceRange.aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT ) != 0
				&& isStencilFormat( view.data->info.format ) ) );
		assert( !isSampled()
			|| ( ( this->loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE )
				&& ( this->storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE )
				&& ( this->stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE )
				&& ( this->stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE ) ) );
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

	bool operator==( Attachment const & lhs
		, Attachment const & rhs )
	{
		return lhs.pass == rhs.pass
			&& lhs.flags == rhs.flags
			&& lhs.view == rhs.view
			&& lhs.loadOp == rhs.loadOp
			&& lhs.storeOp == rhs.storeOp
			&& lhs.stencilLoadOp == rhs.stencilLoadOp
			&& lhs.stencilStoreOp == rhs.stencilStoreOp
			&& lhs.filter == rhs.filter
			&& lhs.clearValue == rhs.clearValue
			&& lhs.blendState == rhs.blendState;
	}

	bool operator!=( Attachment const & lhs
		, Attachment const & rhs )
	{
		return !( lhs == rhs );
	}
}
