/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/Attachment.hpp"
#include "RenderGraph/WriteDescriptorSet.hpp"

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
		/**@}*/
		/**
		*\name
		*	Dependencies.
		*/
		/**@[*/
		void addDependency( FramePass const & pass )
		{
			depends.push_back( &pass );
		}

		void addDependencies( FramePassArray const & passes )
		{
			depends.insert( depends.end()
				, passes.begin()
				, passes.end() );
		}

		bool dependsOn( FramePass const & pass )const;
		bool directDependsOn( FramePass const & pass )const;
		/**@}*/
		/**
		*\name
		*	Buffer attachments.
		*/
		/**@[*/
		/**
		*\brief
		*	Creates a uniform buffer attachment.
		*/
		void addUniformBuffer( VkBuffer buffer
			, uint32_t binding
			, VkDeviceSize offset
			, VkDeviceSize range );
		/**
		*\brief
		*	Creates a storage buffer attachment.
		*/
		void addStorageBuffer( VkBuffer buffer
			, uint32_t binding
			, VkDeviceSize offset
			, VkDeviceSize range );
		/**
		*\brief
		*	Creates a uniform texel buffer view attachment.
		*/
		void addUniformBufferView( VkBuffer buffer
			, VkBufferView view
			, uint32_t binding
			, VkDeviceSize offset
			, VkDeviceSize range );
		/**
		*\brief
		*	Creates a storage texel buffer view attachment.
		*/
		void addStorageBufferView( VkBuffer buffer
			, VkBufferView view
			, uint32_t binding
			, VkDeviceSize offset
			, VkDeviceSize range );
		/**@}*/
		/**
		*\name
		*	Image attachments.
		*/
		/**@[*/
		/**
		*\brief
		*	Creates a sampled image attachment.
		*/
		void addSampledView( std::string name
			, ImageViewId view
			, VkImageLayout initialLayout
			, uint32_t binding
			, VkFilter filter );
		/**
		*\brief
		*	Creates a storage image attachment.
		*/
		void addStorageView( std::string name
			, ImageViewId view
			, VkImageLayout initialLayout
			, uint32_t binding );
		/**
		*\brief
		*	Creates a colour attachment.
		*/
		void addColourView( std::string name
			, ImageViewId view
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
		void addDepthStencilView( std::string name
			, ImageViewId view
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkAttachmentLoadOp stencilLoadOp
			, VkAttachmentStoreOp stencilStoreOp
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkClearValue clearValue = {} );
		/**
		*\brief
		*	Creates an transfer input attachment.
		*/
		void addTransferInputView( std::string name
			, ImageViewId view );
		/**
		*\brief
		*	Creates an transfer output attachment.
		*/
		void addTransferOutputView( std::string name
			, ImageViewId view );
		/**
		*\brief
		*	Creates an input colour attachment.
		*/
		void addInputColourView( std::string name
			, ImageViewId view
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			return addColourView( std::move( name )
				, std::move( view )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, initialLayout
				, finalLayout );
		}
		/**
		*\brief
		*	Creates an in/out colour attachment.
		*/
		void addInOutColourView( std::string name
			, ImageViewId view
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout
			, VkPipelineColorBlendAttachmentState blendState = DefaultBlendState )
		{
			addColourView( std::move( name )
				, std::move( view )
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
		void addOutputColourView( std::string name
			, ImageViewId view
			, VkImageLayout finalLayout
			, VkClearValue clearValue = {} )
		{
			addColourView( std::move( name )
				, std::move( view )
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
		void addInputDepthView( std::string name
			, ImageViewId view
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			addDepthStencilView( std::move( name )
				, std::move( view )
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
		void addInOutDepthView( std::string name
			, ImageViewId view
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			addDepthStencilView( std::move( name )
				, std::move( view )
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
		void addOutputDepthView( std::string name
			, ImageViewId view
			, VkImageLayout finalLayout
			, VkClearValue clearValue = {} )
		{
			addDepthStencilView( std::move( name )
				, std::move( view )
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
		void addInputDepthStencilView( std::string name
			, ImageViewId view
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			addDepthStencilView( std::move( name )
				, std::move( view )
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
		void addInOutDepthStencilView( std::string name
			, ImageViewId view
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			addDepthStencilView( std::move( name )
				, std::move( view )
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
		void addOutputDepthStencilView( std::string name
			, ImageViewId view
			, VkImageLayout finalLayout
			, VkClearValue clearValue = {} )
		{
			addDepthStencilView( std::move( name )
				, std::move( view )
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
		void addInputStencilView( std::string name
			, ImageViewId view
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			addDepthStencilView( std::move( name )
				, std::move( view )
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
		void addInOutStencilView( std::string name
			, ImageViewId view
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			addDepthStencilView( std::move( name )
				, std::move( view )
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
		void addOutputStencilView( std::string name
			, ImageViewId view
			, VkImageLayout finalLayout
			, VkClearValue clearValue = {} )
		{
			addDepthStencilView( std::move( name )
				, std::move( view )
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
		AttachmentArray storage;
		AttachmentArray colourInOuts;
		std::optional< Attachment > depthStencilInOut;
		AttachmentArray transferInOuts;
		WriteDescriptorSetArray buffers;
		WriteDescriptorSetArray bufferViews;
		RunnablePassCreator runnableCreator;
		FramePassArray depends;
	};
}
