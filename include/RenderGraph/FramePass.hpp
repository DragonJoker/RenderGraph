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
		, GraphContext &
		, RunnableGraph & ) >;

	struct FramePass
	{
	protected:
		friend struct FramePassGroup;
		/**
		*\name
		*	Construction.
		*/
		/**@[*/
		CRG_API FramePass( FramePassGroup const & group
			, FrameGraph & graph
			, uint32_t id
			, std::string const & name
			, RunnablePassCreator runnableCreator );
		/**@}*/

	public:
		/**
		*\name
		*	Dependencies.
		*/
		/**@[*/
		void addDependency( FramePass const & pass )
		{
			passDepends.push_back( &pass );
		}

		void addDependencies( FramePassArray const & passes )
		{
			passDepends.insert( passDepends.end()
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
		*	Tells if, for given buffer, this pass directly depends on given pass.
		*\param[in] pass
		*	The pass to test.
		*\param[in] view
		*	The view.
		*/
		CRG_API bool dependsOn( FramePass const & pass
			, Buffer const & buffer )const;
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
		CRG_API void addUniformBuffer( Buffer buffer
			, uint32_t binding
			, VkDeviceSize offset
			, VkDeviceSize range );
		/**
		*\brief
		*	Creates a storage buffer attachment.
		*/
		CRG_API void addInputStorageBuffer( Buffer buffer
			, uint32_t binding
			, VkDeviceSize offset
			, VkDeviceSize range );
		/**
		*\brief
		*	Creates a storage buffer attachment.
		*/
		CRG_API void addOutputStorageBuffer( Buffer buffer
			, uint32_t binding
			, VkDeviceSize offset
			, VkDeviceSize range );
		/**
		*\brief
		*	Creates a storage buffer attachment.
		*/
		CRG_API void addInOutStorageBuffer( Buffer buffer
			, uint32_t binding
			, VkDeviceSize offset
			, VkDeviceSize range );
		/**
		*\brief
		*	Creates a uniform texel buffer view attachment.
		*/
		CRG_API void addUniformBufferView( Buffer buffer
			, VkBufferView view
			, uint32_t binding
			, VkDeviceSize offset
			, VkDeviceSize range );
		/**
		*\brief
		*	Creates a storage texel buffer view attachment.
		*/
		CRG_API void addStorageBufferView( Buffer buffer
			, VkBufferView view
			, uint32_t binding
			, VkDeviceSize offset
			, VkDeviceSize range );
		/**@}*/
		/**
		*\name
		*	Image view split/merge.
		*/
		/**@[*/
		/**
		*\brief
		*	Creates a view which represents the given views merging.
		*/
		CRG_API ImageViewId mergeViews( ImageViewIdArray const & views
			, bool mergeMipLevels = true
			, bool mergeArrayLayers = true );
		/**@}*/
		/**
		*\name
		*	Image single-pass attachments.
		*/
		/**@[*/
		/**
		*\brief
		*	Creates a sampled image attachment.
		*/
		CRG_API void addSampledView( ImageViewId view
			, uint32_t binding
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, SamplerDesc samplerDesc = SamplerDesc{} );
		/**
		*\brief
		*	Creates an implicit image attachment.
		*\remarks
		*	This image will only be transitioned to wanted layout at pass start, without being actually used.
		*	It will also be used to compute dependencies, and is considered an input, in that goal.
		*/
		CRG_API void addImplicitColourView( ImageViewId view
			, VkImageLayout wantedLayout );
		/**
		*\brief
		*	Creates an implicit image attachment.
		*\remarks
		*	This image will only be transitioned to wanted layout at pass start, without being actually used.
		*	It will also be used to compute dependencies, and is considered an input, in that goal.
		*/
		CRG_API void addImplicitDepthView( ImageViewId view
			, VkImageLayout wantedLayout );
		/**
		*\brief
		*	Creates an implicit image attachment.
		*\remarks
		*	This image will only be transitioned to wanted layout at pass start, without being actually used.
		*	It will also be used to compute dependencies, and is considered an input, in that goal.
		*/
		CRG_API void addImplicitDepthStencilView( ImageViewId view
			, VkImageLayout wantedLayout );
		/**
		*\brief
		*	Creates a storage image attachment.
		*/
		CRG_API void addInputStorageView( ImageViewId view
			, uint32_t binding
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED );
		/**
		*\brief
		*	Creates a storage image attachment.
		*/
		CRG_API void addOutputStorageView( ImageViewId view
			, uint32_t binding
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED );
		/**
		*\brief
		*	Creates a storage image attachment.
		*/
		CRG_API void addInOutStorageView( ImageViewId view
			, uint32_t binding
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED );
		/**
		*\brief
		*	Creates an transfer input attachment.
		*/
		CRG_API void addTransferInputView( ImageViewId view
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED );
		/**
		*\brief
		*	Creates an transfer output attachment.
		*/
		CRG_API void addTransferOutputView( ImageViewId view
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED );
		/**
		*\brief
		*	Creates an transfer input/output attachment.
		*/
		CRG_API void addTransferInOutView( ImageViewId view
			, crg::Attachment::Flag flag = {} );
		/**
		*\brief
		*	Creates a colour attachment.
		*/
		CRG_API void addColourView( std::string const & name
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
		CRG_API void addDepthView( std::string const & name
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
		*	Creates a depth and/or stencil output attachment.
		*/
		CRG_API void addStencilView( std::string const & name
			, ImageAttachment::FlagKind stencilFlags
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
		*	Creates a depth and/or stencil output attachment.
		*/
		CRG_API void addDepthStencilView( std::string const & name
			, ImageAttachment::FlagKind stencilFlags
			, ImageViewId view
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkAttachmentLoadOp stencilLoadOp
			, VkAttachmentStoreOp stencilStoreOp
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkClearValue clearValue = {} );
		/**@}*/
		/**
		*\name
		*	Image multi-pass attachments.
		*/
		/**@[*/
		/**
		*\brief
		*	Creates a sampled image attachment.
		*/
		CRG_API void addSampledView( ImageViewIdArray view
			, uint32_t binding
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, SamplerDesc samplerDesc = SamplerDesc{} );
		/**
		*\brief
		*	Creates a sampled image array attachment.
		*/
		CRG_API void addSampledViews( ImageViewIdArray views
			, uint32_t binding
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, SamplerDesc samplerDesc = SamplerDesc{} );
		/**
		*\brief
		*	Creates an implicit image attachment.
		*\remarks
		*	This image will only be transitioned to wanted layout at pass start, without being actually used.
		*	It will also be used to compute dependencies, and is considered an input, in that goal.
		*/
		CRG_API void addImplicitColourView( ImageViewIdArray view
			, VkImageLayout wantedLayout );
		/**
		*\brief
		*	Creates implicit image array attachment.
		*\remarks
		*	This image will only be transitioned to wanted layout at pass start, without being actually used.
		*	It will also be used to compute dependencies, and is considered an input, in that goal.
		*/
		CRG_API void addImplicitColourViews( ImageViewIdArray view
			, VkImageLayout wantedLayout );
		/**
		*\brief
		*	Creates an implicit image attachment.
		*\remarks
		*	This image will only be transitioned to wanted layout at pass start, without being actually used.
		*	It will also be used to compute dependencies, and is considered an input, in that goal.
		*/
		CRG_API void addImplicitDepthView( ImageViewIdArray view
			, VkImageLayout wantedLayout );
		/**
		*\brief
		*	Creates implicit image array attachment.
		*\remarks
		*	This image will only be transitioned to wanted layout at pass start, without being actually used.
		*	It will also be used to compute dependencies, and is considered an input, in that goal.
		*/
		CRG_API void addImplicitDepthViews( ImageViewIdArray view
			, VkImageLayout wantedLayout );
		/**
		*\brief
		*	Creates an implicit image attachment.
		*\remarks
		*	This image will only be transitioned to wanted layout at pass start, without being actually used.
		*	It will also be used to compute dependencies, and is considered an input, in that goal.
		*/
		CRG_API void addImplicitDepthStencilView( ImageViewIdArray view
			, VkImageLayout wantedLayout );
		/**
		*\brief
		*	Creates implicit image array attachment.
		*\remarks
		*	This image will only be transitioned to wanted layout at pass start, without being actually used.
		*	It will also be used to compute dependencies, and is considered an input, in that goal.
		*/
		CRG_API void addImplicitDepthStencilViews( ImageViewIdArray view
			, VkImageLayout wantedLayout );
		/**
		*\brief
		*	Creates a storage image attachment.
		*/
		CRG_API void addInputStorageView( ImageViewIdArray view
			, uint32_t binding
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED );
		/**
		*\brief
		*	Creates a storage image array attachment.
		*/
		CRG_API void addInputStorageViews( ImageViewIdArray view
			, uint32_t binding
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED );
		/**
		*\brief
		*	Creates a storage image attachment.
		*/
		CRG_API void addOutputStorageView( ImageViewIdArray view
			, uint32_t binding
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED );
		/**
		*\brief
		*	Creates a storage image array attachment.
		*/
		CRG_API void addOutputStorageViews( ImageViewIdArray view
			, uint32_t binding
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED );
		/**
		*\brief
		*	Creates a storage image attachment.
		*/
		CRG_API void addInOutStorageView( ImageViewIdArray view
			, uint32_t binding
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED );
		/**
		*\brief
		*	Creates a storage image array attachment.
		*/
		CRG_API void addInOutStorageViews( ImageViewIdArray view
			, uint32_t binding
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED );
		/**
		*\brief
		*	Creates an transfer input attachment.
		*/
		CRG_API void addTransferInputView( ImageViewIdArray view
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED );
		/**
		*\brief
		*	Creates an transfer output attachment.
		*/
		CRG_API void addTransferOutputView( ImageViewIdArray view
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED );
		/**
		*\brief
		*	Creates an transfer input/output attachment.
		*/
		CRG_API void addTransferInOutView( ImageViewIdArray view
			, crg::Attachment::Flag flag = {} );
		/**
		*\brief
		*	Creates a colour attachment.
		*/
		CRG_API void addColourView( std::string const & name
			, ImageViewIdArray view
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
		CRG_API void addDepthView( std::string const & name
			, ImageViewIdArray view
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkAttachmentLoadOp stencilLoadOp
			, VkAttachmentStoreOp stencilStoreOp
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkClearValue clearValue = {} );
		/**
		*\brief
		*	Creates a depth and/or stencil output attachment.
		*/
		CRG_API void addStencilView( std::string const & name
			, ImageAttachment::FlagKind stencilFlags
			, ImageViewIdArray view
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkAttachmentLoadOp stencilLoadOp
			, VkAttachmentStoreOp stencilStoreOp
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkClearValue clearValue = {} );
		/**
		*\brief
		*	Creates a depth and/or stencil output attachment.
		*/
		CRG_API void addDepthStencilView( std::string const & name
			, ImageAttachment::FlagKind stencilFlags
			, ImageViewIdArray view
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkAttachmentLoadOp stencilLoadOp
			, VkAttachmentStoreOp stencilStoreOp
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkClearValue clearValue = {} );
		/**@}*/
		/**
		*\name
		*	Image specified attachments.
		*/
		/**@[*/
		/**
		*\brief
		*	Creates an input colour attachment.
		*/
		template< typename ImageViewT >
		void addInputColourView( ImageViewT view
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED )
		{
			return addColourView( "Ic"
				, std::move( view )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, initialLayout
				, VK_IMAGE_LAYOUT_UNDEFINED );
		}
		/**
		*\brief
		*	Creates an in/out colour attachment.
		*/
		template< typename ImageViewT >
		void addInOutColourView( ImageViewT view
			, VkPipelineColorBlendAttachmentState blendState = DefaultBlendState
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED )
		{
			addColourView( "IOc"
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
		template< typename ImageViewT >
		void addOutputColourView( ImageViewT view
			, VkClearValue clearValue = {}
			, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED )
		{
			addColourView( "Oc"
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
		template< typename ImageViewT >
		void addInputDepthView( ImageViewT view
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED )
		{
			addDepthView( "Id"
				, std::move( view )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, initialLayout
				, VK_IMAGE_LAYOUT_UNDEFINED );
		}
		/**
		*\brief
		*	Creates an in/out depth attachment.
		*/
		template< typename ImageViewT >
		void addInOutDepthView( ImageViewT view
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED )
		{
			addDepthView( "IOd"
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
		template< typename ImageViewT >
		void addOutputDepthView( ImageViewT view
			, VkClearValue clearValue = {}
			, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED )
		{
			addDepthView( "Od"
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
		template< typename ImageViewT >
		void addInputDepthStencilView( ImageViewT view
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED )
		{
			addDepthStencilView( "Ids"
				, ImageAttachment::FlagKind( ImageAttachment::Flag::StencilInput )
				, std::move( view )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, initialLayout
				, VK_IMAGE_LAYOUT_UNDEFINED );
		}
		/**
		*\brief
		*	Creates an in/out depth and stencil attachment.
		*/
		template< typename ImageViewT >
		void addInOutDepthStencilView( ImageViewT view
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED )
		{
			addDepthStencilView( "IOds"
				, ImageAttachment::FlagKind( ImageAttachment::Flag::StencilOutput ) | ImageAttachment::FlagKind( ImageAttachment::Flag::StencilInput )
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
		template< typename ImageViewT >
		void addOutputDepthStencilView( ImageViewT view
			, VkClearValue clearValue = {}
			, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED )
		{
			addDepthStencilView( "Ods"
				, ImageAttachment::FlagKind( ImageAttachment::Flag::StencilOutput )
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
		template< typename ImageViewT >
		void addInputStencilView( ImageViewT view
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED )
		{
			addStencilView( "Is"
				, ImageAttachment::FlagKind( ImageAttachment::Flag::StencilInput )
				, std::move( view )
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, initialLayout
				, VK_IMAGE_LAYOUT_UNDEFINED );
		}
		/**
		*\brief
		*	Creates an in/out stencil attachment.
		*/
		template< typename ImageViewT >
		void addInOutStencilView( ImageViewT view
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED )
		{
			addStencilView( "IOs"
				, ImageAttachment::FlagKind( ImageAttachment::Flag::StencilOutput ) | ImageAttachment::FlagKind( ImageAttachment::Flag::StencilInput )
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
		template< typename ImageViewT >
		void addOutputStencilView( ImageViewT view
			, VkClearValue clearValue = {}
			, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED )
		{
			addStencilView( "Os"
				, ImageAttachment::FlagKind( ImageAttachment::Flag::StencilOutput )
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
		CRG_API RunnablePassPtr createRunnable( GraphContext & context
			, RunnableGraph & graph )const;
		/**@}*/

		FramePassGroup const & group;
		FrameGraph & graph;
		uint32_t id;
		std::string name;
		AttachmentArray images;
		AttachmentArray buffers;
		RunnablePassCreator runnableCreator;
		FramePassArray passDepends;
	};
}
