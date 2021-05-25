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
		CRG_API FramePass( std::string const & name
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
		/**
		*\brief
		*	Tells if, for given view, this pass directly depends on given pass.
		*\param[in] pass
		*	The pass to test.
		*\param[in] view
		*	The view.
		*/
		CRG_API bool dependsOn( FramePass const & pass
			, ImageViewId const & view )const;
		/**
		*\brief
		*	Tells if this pass directly depends on given pass.
		*\param[in] pass
		*	The pass to test.
		*/
		CRG_API bool dependsOn( FramePass const & pass )const;
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
		CRG_API void addUniformBuffer( VkBuffer buffer
			, uint32_t binding
			, VkDeviceSize offset
			, VkDeviceSize range );
		/**
		*\brief
		*	Creates a storage buffer attachment.
		*/
		CRG_API void addStorageBuffer( VkBuffer buffer
			, uint32_t binding
			, VkDeviceSize offset
			, VkDeviceSize range );
		/**
		*\brief
		*	Creates a uniform texel buffer view attachment.
		*/
		CRG_API void addUniformBufferView( VkBuffer buffer
			, VkBufferView view
			, uint32_t binding
			, VkDeviceSize offset
			, VkDeviceSize range );
		/**
		*\brief
		*	Creates a storage texel buffer view attachment.
		*/
		CRG_API void addStorageBufferView( VkBuffer buffer
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
		CRG_API void addSampledView( ImageViewId view
			, uint32_t binding
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			, SamplerDesc samplerDesc = SamplerDesc{} );
		/**
		*\brief
		*	Creates a storage image attachment.
		*/
		CRG_API void addStorageView( ImageViewId view
			, uint32_t binding
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_GENERAL );
		/**
		*\brief
		*	Creates an transfer input attachment.
		*/
		CRG_API void addTransferInputView( ImageViewId view
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );
		/**
		*\brief
		*	Creates an transfer output attachment.
		*/
		CRG_API void addTransferOutputView( ImageViewId view
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
		/**
		*\brief
		*	Creates a colour attachment.
		*/
		CRG_API void addColourView( std::string const & name
			, ImageViewId view
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkClearValue clearValue = {}
			, VkPipelineColorBlendAttachmentState blendState = DefaultBlendState );
		/**
		*\brief
		*	Creates a depth and/or stencil output attachment.
		*/
		CRG_API void addDepthStencilView( std::string const & name
			, ImageViewId view
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkAttachmentLoadOp stencilLoadOp
			, VkAttachmentStoreOp stencilStoreOp
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkClearValue clearValue = {} );
		/**
		*\brief
		*	Creates an input colour attachment.
		*/
		void addInputColourView( ImageViewId view )
		{
			return addColourView( "InColour"
				, std::move( view )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_IMAGE_LAYOUT_UNDEFINED );
		}
		/**
		*\brief
		*	Creates an in/out colour attachment.
		*/
		void addInOutColourView( ImageViewId view
			, VkPipelineColorBlendAttachmentState blendState = DefaultBlendState )
		{
			addColourView( "InOutColour"
				, std::move( view )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_IMAGE_LAYOUT_UNDEFINED
				, {}
				, std::move( blendState ) );
		}
		/**
		*\brief
		*	Creates an output colour attachment.
		*/
		void addOutputColourView( ImageViewId view
			, VkClearValue clearValue = {} )
		{
			addColourView( "OutColour"
				, std::move( view )
				, VK_ATTACHMENT_LOAD_OP_CLEAR
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_IMAGE_LAYOUT_UNDEFINED
				, std::move( clearValue ) );
		}
		/**
		*\brief
		*	Creates an input depth attachment.
		*/
		void addInputDepthView( ImageViewId view )
		{
			addDepthStencilView( "InDepth"
				, std::move( view )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_IMAGE_LAYOUT_UNDEFINED );
		}
		/**
		*\brief
		*	Creates an in/out depth attachment.
		*/
		void addInOutDepthView( ImageViewId view )
		{
			addDepthStencilView( "InOutDepth"
				, std::move( view )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_IMAGE_LAYOUT_UNDEFINED
				, {} );
		}
		/**
		*\brief
		*	Creates an output depth attachment.
		*/
		void addOutputDepthView( ImageViewId view
			, VkClearValue clearValue = {} )
		{
			addDepthStencilView( "OutDepth"
				, std::move( view )
				, VK_ATTACHMENT_LOAD_OP_CLEAR
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_IMAGE_LAYOUT_UNDEFINED
				, std::move( clearValue ) );
		}
		/**
		*\brief
		*	Creates an input depth and stencil attachment.
		*/
		void addInputDepthStencilView( ImageViewId view )
		{
			addDepthStencilView( "InDepthStencil"
				, std::move( view )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_IMAGE_LAYOUT_UNDEFINED );
		}
		/**
		*\brief
		*	Creates an in/out depth and stencil attachment.
		*/
		void addInOutDepthStencilView( ImageViewId view )
		{
			addDepthStencilView( "InOutDepthStencil"
				, std::move( view )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_IMAGE_LAYOUT_UNDEFINED
				, {} );
		}
		/**
		*\brief
		*	Creates an output depth and stencil attachment.
		*/
		void addOutputDepthStencilView( ImageViewId view
			, VkClearValue clearValue = {} )
		{
			addDepthStencilView( "OutDepthStencil"
				, std::move( view )
				, VK_ATTACHMENT_LOAD_OP_CLEAR
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_ATTACHMENT_LOAD_OP_CLEAR
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_IMAGE_LAYOUT_UNDEFINED
				, std::move( clearValue ) );
		}
		/**
		*\brief
		*	Creates an input stencil attachment.
		*/
		void addInputStencilView( ImageViewId view )
		{
			addDepthStencilView( "InStencil"
				, std::move( view )
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_IMAGE_LAYOUT_UNDEFINED );
		}
		/**
		*\brief
		*	Creates an in/out stencil attachment.
		*/
		void addInOutStencilView( ImageViewId view )
		{
			addDepthStencilView( "InOutStencil"
				, std::move( view )
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_IMAGE_LAYOUT_UNDEFINED
				, {} );
		}
		/**
		*\brief
		*	Creates an output stencil attachment.
		*/
		void addOutputStencilView( ImageViewId view
			, VkClearValue clearValue = {} )
		{
			addDepthStencilView( "OutStencil"
				, std::move( view )
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_CLEAR
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_IMAGE_LAYOUT_UNDEFINED
				, std::move( clearValue ) );
		}
		/**@}*/
		/**
		*\name
		*	Graph compilation.
		*/
		/**@[*/
		CRG_API RunnablePassPtr createRunnable( GraphContext const & context
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
