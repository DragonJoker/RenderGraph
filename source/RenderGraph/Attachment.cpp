/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/Attachment.hpp"

#include <cassert>

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
		, VkImageLayout finalLayout )
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
	{
		assert( !isSampled()
			|| ( ( this->loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE )
				&& ( this->storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE )
				&& ( this->stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE )
				&& ( this->stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE ) ) );
	}

	Attachment Attachment::createSampled( ImageViewData viewData
		, VkImageLayout initialLayout )
	{
		return { FlagKind( Flag::Sampled )
			, viewData
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, initialLayout };
	}

	Attachment Attachment::createColour( ImageViewData viewData
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkImageLayout initialLayout
		, VkImageLayout finalLayout )
	{
		return { FlagKind( Flag::None )
			, viewData
			, loadOp
			, storeOp
			, VK_ATTACHMENT_LOAD_OP_DONT_CARE
			, VK_ATTACHMENT_STORE_OP_DONT_CARE
			, initialLayout
			, finalLayout };
	}

	Attachment Attachment::createDepthStencil( ImageViewData viewData
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, VkImageLayout initialLayout
		, VkImageLayout finalLayout )
	{
		return { FlagKind( Flag::Depth )
			, viewData
			, loadOp
			, storeOp
			, stencilLoadOp
			, stencilStoreOp
			, initialLayout
			, finalLayout };
	}

	bool operator==( Attachment const & lhs
		, Attachment const & rhs )
	{
		return lhs.flags == rhs.flags
			&& lhs.viewData == rhs.viewData
			&& lhs.loadOp == rhs.loadOp
			&& lhs.storeOp == rhs.storeOp
			&& lhs.stencilLoadOp == rhs.stencilLoadOp
			&& lhs.stencilStoreOp == rhs.stencilStoreOp;
	}

	bool operator!=( Attachment const & lhs
		, Attachment const & rhs )
	{
		return !( lhs == rhs );
	}
}
