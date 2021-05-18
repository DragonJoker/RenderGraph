/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/Attachment.hpp"

#include <cassert>
#include <cstring>

namespace crg
{
	Attachment::Attachment()
	{
	}

	Attachment::Attachment( FlagKind flags
		, ImageViewData viewData
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, VkImageLayout initialLayout
		, VkImageLayout finalLayout
		, VkFilter filter
		, VkClearValue clearValue
		, VkPipelineColorBlendAttachmentState blendState )
		: viewData{ std::move( viewData ) }
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
		, finalLayout{ finalLayout }
		, filter{ filter }
		, clearValue{ std::move( clearValue ) }
		, blendState{ std::move( blendState ) }
	{
		assert( !isSampled()
			|| ( ( this->loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE )
				&& ( this->storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE )
				&& ( this->stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE )
				&& ( this->stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE ) ) );
	}

	Attachment Attachment::createSampled( ImageViewData viewData
		, VkImageLayout initialLayout
		, VkFilter filter )
	{
		return { FlagKind( Flag::Sampled )
			, viewData
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, initialLayout
			, filter
			, {}
			, {} };
	}

	Attachment Attachment::createColour( ImageViewData viewData
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkImageLayout initialLayout
		, VkImageLayout finalLayout
		, VkClearValue clearValue
		, VkPipelineColorBlendAttachmentState blendState )
	{
		return { FlagKind( Flag::None )
			, viewData
			, loadOp
			, storeOp
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, finalLayout
			, {}
			, std::move( clearValue )
			, std::move( blendState ) };
	}

	Attachment Attachment::createDepthStencil( ImageViewData viewData
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, VkImageLayout initialLayout
		, VkImageLayout finalLayout
		, VkClearValue clearValue )
	{
		return { FlagKind( Flag::Depth )
			, viewData
			, loadOp
			, storeOp
			, stencilLoadOp
			, stencilStoreOp
			, initialLayout
			, finalLayout
			, {}
			, std::move( clearValue )
			, {} };
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
		return lhs.flags == rhs.flags
			&& lhs.viewData == rhs.viewData
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
