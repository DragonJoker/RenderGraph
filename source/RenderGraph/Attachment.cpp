/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/Attachment.hpp"

#include <cassert>

namespace crg
{
	Attachment::Attachment( FlagKind flags
		, std::string name
		, ImageViewId view
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp )
		: name{ std::move( name ) }
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
	{
		assert( !isSampled()
			|| ( ( this->loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE )
				&& ( this->storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE )
				&& ( this->stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE )
				&& ( this->stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE ) ) );
	}

	Attachment Attachment::createSampled( std::string const & name
		, ImageViewId view )
	{
		return
		{
			FlagKind( Flag::Sampled ),
			name,
			view,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
		};
	}

	Attachment Attachment::createColour( std::string const & name
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, ImageViewId view )
	{
		return
		{
			FlagKind( Flag::None ),
			name,
			view,
			loadOp,
			storeOp,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
		};
	}

	Attachment Attachment::createDepthStencil( std::string const & name
		, VkAttachmentLoadOp loadOp
		, VkAttachmentStoreOp storeOp
		, VkAttachmentLoadOp stencilLoadOp
		, VkAttachmentStoreOp stencilStoreOp
		, ImageViewId view )
	{
		return
		{
			( ( ( loadOp != VK_ATTACHMENT_LOAD_OP_DONT_CARE )
					|| ( storeOp != VK_ATTACHMENT_STORE_OP_DONT_CARE ) )
				? FlagKind( Flag::Depth )
				: FlagKind( Flag::None ) ),
			name,
			view,
			loadOp,
			storeOp,
			stencilLoadOp,
			stencilStoreOp,
		};
	}

	bool operator==( Attachment const & lhs, Attachment const & rhs )
	{
		return lhs.flags == rhs.flags
			&& lhs.name == rhs.name
			&& lhs.view == rhs.view
			&& lhs.loadOp == rhs.loadOp
			&& lhs.storeOp == rhs.storeOp
			&& lhs.stencilLoadOp == rhs.stencilLoadOp
			&& lhs.stencilStoreOp == rhs.stencilStoreOp;
	}
}
