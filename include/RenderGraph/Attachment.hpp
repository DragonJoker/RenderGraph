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
	*	A sampled image, or an input/output colour/depth/stencil/depthstencil attachment.
	*/
	struct Attachment
	{
		/**
		*\brief
		*	Creates a sampled image attachment.
		*/
		static Attachment createSampled( std::string const & name
			, ImageViewId view );
		/**
		*\brief
		*	Creates a colour attachment.
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
		/**
		*\brief
		*	Creates an input colour attachment.
		*/
		static inline Attachment createInputColour( std::string const & name
			, ImageViewId view )
		{
			return createColour( name
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, view );
		}
		/**
		*\brief
		*	Creates an in/out colour attachment.
		*/
		static inline Attachment createInOutColour( std::string const & name
			, ImageViewId view )
		{
			return createColour( name
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, view );
		}
		/**
		*\brief
		*	Creates an output colour attachment.
		*/
		static inline Attachment createOutputColour( std::string const & name
			, ImageViewId view )
		{
			return createColour( name
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_STORE
				, view );
		}
		/**
		*\brief
		*	Creates an input depth attachment.
		*/
		static inline Attachment createInputDepth( std::string const & name
			, ImageViewId view )
		{
			return createDepthStencil( name
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, view );
		}
		/**
		*\brief
		*	Creates an in/out depth attachment.
		*/
		static inline Attachment createInOutDepth( std::string const & name
			, ImageViewId view )
		{
			return createDepthStencil( name
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, view );
		}
		/**
		*\brief
		*	Creates an output depth attachment.
		*/
		static inline Attachment createOutputDepth( std::string const & name
			, ImageViewId view )
		{
			return createDepthStencil( name
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, view );
		}
		/**
		*\brief
		*	Creates an input depth and stencil attachment.
		*/
		static inline Attachment createInputDepthStencil( std::string const & name
			, ImageViewId view )
		{
			return createDepthStencil( name
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, view );
		}
		/**
		*\brief
		*	Creates an in/out depth and stencil attachment.
		*/
		static inline Attachment createInOutDepthStencil( std::string const & name
			, ImageViewId view )
		{
			return createDepthStencil( name
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, view );
		}
		/**
		*\brief
		*	Creates an output depth and stencil attachment.
		*/
		static inline Attachment createOutputDepthStencil( std::string const & name
			, ImageViewId view )
		{
			return createDepthStencil( name
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_STORE
				, view );
		}
		/**
		*\brief
		*	Creates an input stencil attachment.
		*/
		static inline Attachment createInputStencil( std::string const & name
			, ImageViewId view )
		{
			return createDepthStencil( name
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, view );
		}
		/**
		*\brief
		*	Creates an in/out stencil attachment.
		*/
		static inline Attachment createInOutStencil( std::string const & name
			, ImageViewId view )
		{
			return createDepthStencil( name
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, view );
		}
		/**
		*\brief
		*	Creates an output stencil attachment.
		*/
		static inline Attachment createOutputStencil( std::string const & name
			, ImageViewId view )
		{
			return createDepthStencil( name
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_STORE
				, view );
		}

		std::string name;
		ImageViewId view;
		bool isSampled;
		VkAttachmentLoadOp loadOp;
		VkAttachmentStoreOp storeOp;
		VkAttachmentLoadOp stencilLoadOp;
		VkAttachmentStoreOp stencilStoreOp;
	};
	bool operator==( Attachment const & lhs, Attachment const & rhs );
}
