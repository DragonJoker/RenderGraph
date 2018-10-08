/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/Attachment.hpp"

namespace crg
{
	Attachment Attachment::createInput( std::string const & name
		, ashes::TextureView const & view )
	{
		return
		{
			name,
			ashes::AttachmentLoadOp::eDontCare,
			ashes::AttachmentStoreOp::eDontCare,
			ashes::AttachmentLoadOp::eDontCare,
			ashes::AttachmentStoreOp::eDontCare,
			view
		};
	}

	Attachment Attachment::createColour( std::string const & name
		, ashes::AttachmentLoadOp loadOp
		, ashes::AttachmentStoreOp storeOp
		, ashes::TextureView const & view )
	{
		return
		{
			name,
			loadOp,
			storeOp,
			ashes::AttachmentLoadOp::eDontCare,
			ashes::AttachmentStoreOp::eDontCare,
			view
		};
	}

	Attachment Attachment::createDepthStencil( std::string const & name
		, ashes::AttachmentLoadOp loadOp
		, ashes::AttachmentStoreOp storeOp
		, ashes::AttachmentLoadOp stencilLoadOp
		, ashes::AttachmentStoreOp stencilStoreOp
		, ashes::TextureView const & view )
	{
		return
		{
			name,
			loadOp,
			storeOp,
			stencilLoadOp,
			stencilStoreOp,
			view
		};
	}

	bool operator==( Attachment const & lhs, Attachment const & rhs )
	{
		return lhs.name == rhs.name
			&& lhs.loadOp == rhs.loadOp
			&& lhs.storeOp == rhs.storeOp
			&& lhs.stencilLoadOp == rhs.stencilLoadOp
			&& lhs.stencilStoreOp == rhs.stencilStoreOp
			&& &lhs.view == &rhs.view;
	}
}
