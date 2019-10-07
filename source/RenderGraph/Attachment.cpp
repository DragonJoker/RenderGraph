/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/Attachment.hpp"

namespace crg
{
	Attachment Attachment::createInput( std::string const & name
		, ImageViewId view )
	{
		return
		{
			name,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			view,
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
			loadOp,
			storeOp,
			VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			VK_ATTACHMENT_STORE_OP_DONT_CARE,
			view,
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
			loadOp,
			storeOp,
			stencilLoadOp,
			stencilStoreOp,
			view,
		};
	}

	bool operator==( Attachment const & lhs, Attachment const & rhs )
	{
		return lhs.name == rhs.name
			&& lhs.loadOp == rhs.loadOp
			&& lhs.storeOp == rhs.storeOp
			&& lhs.stencilLoadOp == rhs.stencilLoadOp
			&& lhs.stencilStoreOp == rhs.stencilStoreOp
			&& lhs.view == rhs.view;
	}

	bool operator==( AttachmentArray const & lhs, AttachmentArray const & rhs )
	{
		auto result = lhs.size() == rhs.size();

		for ( size_t i = 0; result && i < lhs.size(); ++i )
		{
			result = lhs[i] == rhs[i];
		}

		return result;
	}
}
