/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraphPrerequisites.hpp"

#include <RenderPass/AttachmentDescription.hpp>

namespace crg
{
	/**
	*\brief
	*	An input or output attachment.
	*/
	struct Attachment
	{
		/**
		*\brief
		*	Creates an input attachment.
		*/
		static Attachment createInput( std::string const & name
			, ashes::TextureView const & view );
		/**
		*\brief
		*	Creates a colour output attachment.
		*/
		static Attachment createColour( std::string const & name
			, ashes::AttachmentLoadOp loadOp
			, ashes::AttachmentStoreOp storeOp
			, ashes::TextureView const & view );
		/**
		*\brief
		*	Creates a depth and/or stencil output attachment.
		*/
		static Attachment createDepthStencil( std::string const & name
			, ashes::AttachmentLoadOp loadOp
			, ashes::AttachmentStoreOp storeOp
			, ashes::AttachmentLoadOp stencilLoadOp
			, ashes::AttachmentStoreOp stencilStoreOp
			, ashes::TextureView const & view );

		std::string const name;
		ashes::AttachmentLoadOp const loadOp;
		ashes::AttachmentStoreOp const storeOp;
		ashes::AttachmentLoadOp const stencilLoadOp;
		ashes::AttachmentStoreOp const stencilStoreOp;
		ashes::TextureView const & view;
	};
	bool operator==( Attachment const & lhs, Attachment const & rhs );
}
