/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/Attachment.hpp"

namespace crg
{
	Attachment Attachment::createSampled( std::string const & name
		, ImageViewId view )
	{
		return
		{
			name,
			view,
			true,
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
			name,
			view,
			false,
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
			name,
			view,
			false,
			loadOp,
			storeOp,
			stencilLoadOp,
			stencilStoreOp,
		};
	}

	bool operator==( Attachment const & lhs, Attachment const & rhs )
	{
		return lhs.name == rhs.name
			&& lhs.view == rhs.view
			&& lhs.isSampled == rhs.isSampled
			&& lhs.loadOp == rhs.loadOp
			&& lhs.storeOp == rhs.storeOp
			&& lhs.stencilLoadOp == rhs.stencilLoadOp
			&& lhs.stencilStoreOp == rhs.stencilStoreOp;
	}
}
