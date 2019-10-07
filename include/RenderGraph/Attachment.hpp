/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "ImageViewData.hpp"

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
			, ImageViewId view );
		/**
		*\brief
		*	Creates a colour output attachment.
		*/
		static Attachment createColour( std::string const & name
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, ImageViewId view );
		/**
		*\brief
		*	Creates a depth and/or stencil output attachment.
		*/
		static Attachment createDepthStencil( std::string const & name
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkAttachmentLoadOp stencilLoadOp
			, VkAttachmentStoreOp stencilStoreOp
			, ImageViewId view );

		std::string name;
		VkAttachmentLoadOp loadOp;
		VkAttachmentStoreOp storeOp;
		VkAttachmentLoadOp stencilLoadOp;
		VkAttachmentStoreOp stencilStoreOp;
		ImageViewId view;
	};
	bool operator==( Attachment const & lhs, Attachment const & rhs );
	bool operator==( AttachmentArray const & lhs, AttachmentArray const & rhs );
}
