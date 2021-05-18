/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/Attachment.hpp"

#include <functional>
#include <optional>

namespace crg
{
	using RunnablePassCreator = std::function< RunnablePassPtr( FramePass const &
		, GraphContext const &
		, RunnableGraph & ) >;

	struct FramePass
	{
		/**
		*\name
		*	Construction.
		*/
		/**@[*/
		FramePass( std::string const & name
			, RunnablePassCreator runnableCreator );
		FramePass( std::string const & name
			, AttachmentArray const & sampled
			, AttachmentArray const & colourInOuts
			, RunnablePassCreator runnableCreator );
		FramePass( std::string const & name
			, AttachmentArray const & sampled
			, AttachmentArray const & colourInOuts
			, std::optional< Attachment > const & depthStencilInOut
			, RunnablePassCreator runnableCreator );
		/**@}*/
		/**
		*\name
		*	Attachments adding.
		*/
		/**@[*/
		/**
		*\brief
		*	Creates a sampled image attachment.
		*/
		Attachment createSampled( ImageViewData viewData
			, VkImageLayout initialLayout
			, VkFilter filter );
		/**
		*\brief
		*	Creates a colour attachment.
		*/
		Attachment createColour( ImageViewData viewData
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkClearValue clearValue = {}
			, VkPipelineColorBlendAttachmentState blendState = DefaultBlendState );
		/**
		*\brief
		*	Creates a depth and/or stencil output attachment.
		*/
		Attachment createDepthStencil( ImageViewData viewData
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkAttachmentLoadOp stencilLoadOp
			, VkAttachmentStoreOp stencilStoreOp
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkClearValue clearValue = {} );
		/**
		*\brief
		*	Creates an input colour attachment.
		*/
		Attachment createInputColour( ImageViewData viewData
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			return createColour( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, initialLayout
				, finalLayout );
		}
		/**
		*\brief
		*	Creates an in/out colour attachment.
		*/
		Attachment createInOutColour( ImageViewData viewData
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout
			, VkPipelineColorBlendAttachmentState blendState = DefaultBlendState )
		{
			return createColour( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, initialLayout
				, finalLayout
				, {}
				, std::move( blendState ) );
		}
		/**
		*\brief
		*	Creates an output colour attachment.
		*/
		Attachment createOutputColour( ImageViewData viewData
			, VkImageLayout finalLayout
			, VkClearValue clearValue = {} )
		{
			return createColour( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_CLEAR
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_IMAGE_LAYOUT_UNDEFINED
				, finalLayout
				, std::move( clearValue ) );
		}
		/**
		*\brief
		*	Creates an input depth attachment.
		*/
		Attachment createInputDepth( ImageViewData viewData
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			return createDepthStencil( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, initialLayout
				, finalLayout );
		}
		/**
		*\brief
		*	Creates an in/out depth attachment.
		*/
		Attachment createInOutDepth( ImageViewData viewData
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			return createDepthStencil( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, initialLayout
				, finalLayout
				, {} );
		}
		/**
		*\brief
		*	Creates an output depth attachment.
		*/
		Attachment createOutputDepth( ImageViewData viewData
			, VkImageLayout finalLayout
			, VkClearValue clearValue = {} )
		{
			return createDepthStencil( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_CLEAR
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_IMAGE_LAYOUT_UNDEFINED
				, finalLayout
				, std::move( clearValue ) );
		}
		/**
		*\brief
		*	Creates an input depth and stencil attachment.
		*/
		Attachment createInputDepthStencil( ImageViewData viewData
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			return createDepthStencil( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, initialLayout
				, finalLayout );
		}
		/**
		*\brief
		*	Creates an in/out depth and stencil attachment.
		*/
		Attachment createInOutDepthStencil( ImageViewData viewData
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			return createDepthStencil( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, initialLayout
				, finalLayout
				, {} );
		}
		/**
		*\brief
		*	Creates an output depth and stencil attachment.
		*/
		Attachment createOutputDepthStencil( ImageViewData viewData
			, VkImageLayout finalLayout
			, VkClearValue clearValue = {} )
		{
			return createDepthStencil( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_CLEAR
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_ATTACHMENT_LOAD_OP_CLEAR
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_IMAGE_LAYOUT_UNDEFINED
				, finalLayout
				, std::move( clearValue ) );
		}
		/**
		*\brief
		*	Creates an input stencil attachment.
		*/
		Attachment createInputStencil( ImageViewData viewData
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			return createDepthStencil( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, initialLayout
				, finalLayout );
		}
		/**
		*\brief
		*	Creates an in/out stencil attachment.
		*/
		Attachment createInOutStencil( ImageViewData viewData
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			return createDepthStencil( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, initialLayout
				, finalLayout
				, {} );
		}
		/**
		*\brief
		*	Creates an output stencil attachment.
		*/
		Attachment createOutputStencil( ImageViewData viewData
			, VkImageLayout finalLayout
			, VkClearValue clearValue = {} )
		{
			return createDepthStencil( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_CLEAR
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_IMAGE_LAYOUT_UNDEFINED
				, finalLayout
				, std::move( clearValue ) );
		}
		/**@}*/
		/**
		*\name
		*	Graph compilation.
		*/
		/**@[*/
		RunnablePassPtr createRunnable( GraphContext const & context
			, RunnableGraph & graph )const;
		/**@}*/

		std::string name;
		AttachmentArray sampled;
		AttachmentArray colourInOuts;
		std::optional< Attachment > depthStencilInOut;
		RunnablePassCreator runnableCreator;
	};
}
