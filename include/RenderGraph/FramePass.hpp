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
		/**@{*/
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
		/**@{*/
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
			, ImageViewId const & view
			, PassDependencyCache & cache )const;
		/**
		*\brief
		*	Tells if, for given buffer, this pass directly depends on given pass.
		*\param[in] pass
		*	The pass to test.
		*\param[in] view
		*	The view.
		*/
		CRG_API bool dependsOn( FramePass const & pass
			, Buffer const & buffer
			, PassDependencyCache & cache )const;
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
		/**@{*/
		/**
		*\brief
		*	Creates an implicit buffer attachment.
		*\remarks
		*	This buffer will only be used to compute dependencies, and is considered an input, in that goal.
		*/
		CRG_API void addImplicitBuffer( Buffer buffer
			, DeviceSize offset
			, DeviceSize range
			, AccessState wantedAccess );
		/**
		*\brief
		*	Creates a uniform buffer attachment.
		*/
		CRG_API void addUniformBuffer( Buffer buffer
			, uint32_t binding
			, DeviceSize offset
			, DeviceSize range );
		/**
		*\brief
		*	Creates an input storage buffer attachment.
		*/
		CRG_API void addInputStorageBuffer( Buffer buffer
			, uint32_t binding
			, DeviceSize offset
			, DeviceSize range );
		/**
		*\brief
		*	Creates an output storage buffer attachment.
		*/
		CRG_API void addOutputStorageBuffer( Buffer buffer
			, uint32_t binding
			, DeviceSize offset
			, DeviceSize range );
		/**
		*\brief
		*	Creates a storage buffer attachment that will be cleared a the beginning of the pass.
		*/
		CRG_API void addClearableOutputStorageBuffer( Buffer buffer
			, uint32_t binding
			, DeviceSize offset
			, DeviceSize range );
		/**
		*\brief
		*	Creates an input/output storage buffer attachment.
		*/
		CRG_API void addInOutStorageBuffer( Buffer buffer
			, uint32_t binding
			, DeviceSize offset
			, DeviceSize range );
		/**
		*\brief
		*	Creates an implicit buffer view attachment.
		*\remarks
		*	This buffer will only be used to compute dependencies, and is considered an input, in that goal.
		*/
		CRG_API void addImplicitBufferView( Buffer buffer
			, VkBufferView view
			, DeviceSize offset
			, DeviceSize range
			, AccessState wantedAccess );
		/**
		*\brief
		*	Creates a uniform texel buffer view attachment.
		*/
		CRG_API void addUniformBufferView( Buffer buffer
			, VkBufferView view
			, uint32_t binding
			, DeviceSize offset
			, DeviceSize range );
		/**
		*\brief
		*	Creates an input  storage texel buffer view attachment.
		*/
		CRG_API void addInputStorageBufferView( Buffer buffer
			, VkBufferView view
			, uint32_t binding
			, DeviceSize offset
			, DeviceSize range );
		/**
		*\brief
		*	Creates an output storage texel buffer view attachment.
		*/
		CRG_API void addOutputStorageBufferView( Buffer buffer
			, VkBufferView view
			, uint32_t binding
			, DeviceSize offset
			, DeviceSize range );
		/**
		*\brief
		*	Creates a storage texel buffer attachment that will be cleared a the beginning of the pass.
		*/
		CRG_API void addClearableOutputStorageBufferView( Buffer buffer
			, VkBufferView view
			, uint32_t binding
			, DeviceSize offset
			, DeviceSize range );
		/**
		*\brief
		*	Creates an input/output storage texel buffer view attachment.
		*/
		CRG_API void addInOutStorageBufferView( Buffer buffer
			, VkBufferView view
			, uint32_t binding
			, DeviceSize offset
			, DeviceSize range );
		/**
		*\brief
		*	Creates a transfer input buffer attachment.
		*/
		CRG_API void addTransferInputBuffer( Buffer buffer
			, DeviceSize offset
			, DeviceSize range );
		/**
		*\brief
		*	Creates a transfer output buffer attachment.
		*/
		CRG_API void addTransferOutputBuffer( Buffer buffer
			, DeviceSize offset
			, DeviceSize range );
		/**
		*\brief
		*	Creates a transfer input/output buffer attachment.
		*/
		CRG_API void addTransferInOutBuffer( Buffer buffer
			, DeviceSize offset
			, DeviceSize range
			, Attachment::Flag flag = {} );
		/**@}*/
		/**
		*\name
		*	Image view split/merge.
		*/
		/**@{*/
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
		*	Image multi-pass attachments.
		*/
		/**@{*/
		/**
		*\brief
		*	Creates a sampled image attachment.
		*/
		CRG_API void addSampledView( ImageViewIdArray view
			, uint32_t binding
			, SamplerDesc samplerDesc = SamplerDesc{} );
		/**
		*\brief
		*	Creates an implicit image attachment.
		*\remarks
		*	This image will only be transitioned to wanted layout at pass start, without being actually used.
		*	It will also be used to compute dependencies, and is considered an input, in that goal.
		*/
		CRG_API void addImplicitColourView( ImageViewIdArray view
			, ImageLayout wantedLayout );
		/**
		*\brief
		*	Creates an implicit image attachment.
		*\remarks
		*	This image will only be transitioned to wanted layout at pass start, without being actually used.
		*	It will also be used to compute dependencies, and is considered an input, in that goal.
		*/
		CRG_API void addImplicitDepthView( ImageViewIdArray view
			, ImageLayout wantedLayout );
		/**
		*\brief
		*	Creates an implicit image attachment.
		*\remarks
		*	This image will only be transitioned to wanted layout at pass start, without being actually used.
		*	It will also be used to compute dependencies, and is considered an input, in that goal.
		*/
		CRG_API void addImplicitDepthStencilView( ImageViewIdArray view
			, ImageLayout wantedLayout );
		/**
		*\brief
		*	Creates a storage image attachment.
		*/
		CRG_API void addInputStorageView( ImageViewIdArray view
			, uint32_t binding );
		/**
		*\brief
		*	Creates a storage image attachment.
		*/
		CRG_API void addOutputStorageView( ImageViewIdArray view
			, uint32_t binding );
		/**
		*\brief
		*	Creates a storage image attachment.
		*/
		CRG_API void addClearableOutputStorageView( ImageViewIdArray view
			, uint32_t binding );
		/**
		*\brief
		*	Creates a storage image attachment.
		*/
		CRG_API void addInOutStorageView( ImageViewIdArray view
			, uint32_t binding );
		/**
		*\brief
		*	Creates an transfer input attachment.
		*/
		CRG_API void addTransferInputView( ImageViewIdArray view );
		/**
		*\brief
		*	Creates an transfer output attachment.
		*/
		CRG_API void addTransferOutputView( ImageViewIdArray view );
		/**
		*\brief
		*	Creates an transfer input/output attachment.
		*/
		CRG_API void addTransferInOutView( ImageViewIdArray view
			, Attachment::Flag flag = {} );
		/**@}*/
		/**
		*\name
		*	Image single-pass attachments.
		*/
		/**@{*/
		/**
		*\brief
		*	Creates a sampled image attachment.
		*/
		inline void addSampledView( ImageViewId view
			, uint32_t binding
			, SamplerDesc samplerDesc = SamplerDesc{} )
		{
			addSampledView( ImageViewIdArray{ view }
				, binding
				, std::move( samplerDesc ) );
		}
		/**
		*\brief
		*	Creates an implicit image attachment.
		*\remarks
		*	This image will only be transitioned to wanted layout at pass start, without being actually used.
		*	It will also be used to compute dependencies, and is considered an input, in that goal.
		*/
		inline void addImplicitColourView( ImageViewId view
			, ImageLayout wantedLayout )
		{
			addImplicitColourView( ImageViewIdArray{ view }
				, wantedLayout );
		}
		/**
		*\brief
		*	Creates an implicit image attachment.
		*\remarks
		*	This image will only be transitioned to wanted layout at pass start, without being actually used.
		*	It will also be used to compute dependencies, and is considered an input, in that goal.
		*/
		inline void addImplicitDepthView( ImageViewId view
			, ImageLayout wantedLayout )
		{
			addImplicitDepthView( ImageViewIdArray{ view }
				, wantedLayout );
		}
		/**
		*\brief
		*	Creates an implicit image attachment.
		*\remarks
		*	This image will only be transitioned to wanted layout at pass start, without being actually used.
		*	It will also be used to compute dependencies, and is considered an input, in that goal.
		*/
		inline void addImplicitDepthStencilView( ImageViewId view
			, ImageLayout wantedLayout )
		{
			addImplicitDepthStencilView( ImageViewIdArray{ view }
				, wantedLayout );
		}
		/**
		*\brief
		*	Creates a storage image attachment.
		*/
		inline void addInputStorageView( ImageViewId view
			, uint32_t binding )
		{
			addInputStorageView( ImageViewIdArray{ view }
				, binding );
		}
		/**
		*\brief
		*	Creates a storage image attachment.
		*/
		inline void addOutputStorageView( ImageViewId view
			, uint32_t binding )
		{
			addOutputStorageView( ImageViewIdArray{ view }
				, binding );
		}
		/**
		*\brief
		*	Creates a storage image attachment.
		*/
		inline void addClearableOutputStorageView( ImageViewId view
			, uint32_t binding )
		{
			addClearableOutputStorageView( ImageViewIdArray{ view }
				, binding );
		}
		/**
		*\brief
		*	Creates a storage image attachment.
		*/
		inline void addInOutStorageView( ImageViewId view
			, uint32_t binding )
		{
			addInOutStorageView( ImageViewIdArray{ view }
				, binding );
		}
		/**
		*\brief
		*	Creates an transfer input attachment.
		*/
		inline void addTransferInputView( ImageViewId view )
		{
			addTransferInputView( ImageViewIdArray{ view } );
		}
		/**
		*\brief
		*	Creates an transfer output attachment.
		*/
		inline void addTransferOutputView( ImageViewId view )
		{
			addTransferOutputView( ImageViewIdArray{ view } );
		}
		/**
		*\brief
		*	Creates an transfer input/output attachment.
		*/
		inline void addTransferInOutView( ImageViewId view
			, Attachment::Flag flag = {} )
		{
			addTransferInOutView( ImageViewIdArray{ view }
				, flag );
		}
		/**@}*/
		/**
		*\name
		*	Image specified attachments.
		*/
		/**@{*/
		/**
		*\brief
		*	Creates an input colour attachment.
		*/
		template< typename ImageViewT >
		void addInputColourView( ImageViewT view )
		{
			addColourView( "Ic"
				, Attachment::FlagKind( Attachment::Flag::Input )
				, std::move( view )
				, AttachmentLoadOp::eLoad
				, AttachmentStoreOp::eDontCare
				, ImageLayout::eColorAttachment );
		}
		/**
		*\brief
		*	Creates an in/out colour attachment.
		*/
		template< typename ImageViewT >
		void addInOutColourView( ImageViewT view
			, PipelineColorBlendAttachmentState blendState = DefaultBlendState )
		{
			addColourView( "IOc"
				, Attachment::FlagKind( Attachment::Flag::InOut )
				, std::move( view )
				, AttachmentLoadOp::eLoad
				, AttachmentStoreOp::eStore
				, ImageLayout::eColorAttachment
				, ClearColorValue{}
				, std::move( blendState ) );
		}
		/**
		*\brief
		*	Creates an output colour attachment.
		*/
		template< typename ImageViewT >
		void addOutputColourView( ImageViewT view
			, ClearColorValue clearValue = ClearColorValue{} )
		{
			addColourView( "Oc"
				, Attachment::FlagKind( Attachment::Flag::Output )
				, std::move( view )
				, AttachmentLoadOp::eClear
				, AttachmentStoreOp::eStore
				, ImageLayout::eColorAttachment
				, std::move( clearValue ) );
		}
		/**
		*\brief
		*	Creates an input depth attachment.
		*/
		template< typename ImageViewT >
		void addInputDepthView( ImageViewT view )
		{
			addDepthView( "Id"
				, Attachment::FlagKind( Attachment::Flag::Input )
				, std::move( view )
				, AttachmentLoadOp::eLoad
				, AttachmentStoreOp::eDontCare
				, AttachmentLoadOp::eDontCare
				, AttachmentStoreOp::eDontCare
				, ImageLayout::eDepthStencilAttachment
				, ClearDepthStencilValue{} );
		}
		/**
		*\brief
		*	Creates an in/out depth attachment.
		*/
		template< typename ImageViewT >
		void addInOutDepthView( ImageViewT view )
		{
			addDepthView( "IOd"
				, Attachment::FlagKind( Attachment::Flag::InOut )
				, std::move( view )
				, AttachmentLoadOp::eLoad
				, AttachmentStoreOp::eStore
				, AttachmentLoadOp::eDontCare
				, AttachmentStoreOp::eDontCare
				, ImageLayout::eDepthStencilAttachment
				, ClearDepthStencilValue{} );
		}
		/**
		*\brief
		*	Creates an output depth attachment.
		*/
		template< typename ImageViewT >
		void addOutputDepthView( ImageViewT view
			, ClearDepthStencilValue clearValue = ClearDepthStencilValue{} )
		{
			addDepthView( "Od"
				, Attachment::FlagKind( Attachment::Flag::Output )
				, std::move( view )
				, AttachmentLoadOp::eClear
				, AttachmentStoreOp::eStore
				, AttachmentLoadOp::eDontCare
				, AttachmentStoreOp::eDontCare
				, ImageLayout::eDepthStencilAttachment
				, std::move( clearValue ) );
		}
		/**
		*\brief
		*	Creates an input depth and stencil attachment.
		*/
		template< typename ImageViewT >
		void addInputDepthStencilView( ImageViewT view )
		{
			addDepthStencilView( "Ids"
				, Attachment::FlagKind( Attachment::Flag::Input )
				, ImageAttachment::FlagKind( ImageAttachment::Flag::StencilInput )
				, std::move( view )
				, AttachmentLoadOp::eLoad
				, AttachmentStoreOp::eDontCare
				, AttachmentLoadOp::eLoad
				, AttachmentStoreOp::eDontCare
				, ImageLayout::eDepthStencilAttachment
				, ClearDepthStencilValue{} );
		}
		/**
		*\brief
		*	Creates an in/out depth and stencil attachment.
		*/
		template< typename ImageViewT >
		void addInOutDepthStencilView( ImageViewT view )
		{
			addDepthStencilView( "IOds"
				, Attachment::FlagKind( Attachment::Flag::InOut )
				, ImageAttachment::FlagKind( ImageAttachment::Flag::StencilInOut )
				, std::move( view )
				, AttachmentLoadOp::eLoad
				, AttachmentStoreOp::eStore
				, AttachmentLoadOp::eLoad
				, AttachmentStoreOp::eStore
				, ImageLayout::eDepthStencilAttachment
				, ClearDepthStencilValue{} );
		}
		/**
		*\brief
		*	Creates an output depth and stencil attachment.
		*/
		template< typename ImageViewT >
		void addOutputDepthStencilView( ImageViewT view
			, ClearDepthStencilValue clearValue = ClearDepthStencilValue{} )
		{
			addDepthStencilView( "Ods"
				, Attachment::FlagKind( Attachment::Flag::Output )
				, ImageAttachment::FlagKind( ImageAttachment::Flag::StencilOutput )
				, std::move( view )
				, AttachmentLoadOp::eClear
				, AttachmentStoreOp::eStore
				, AttachmentLoadOp::eClear
				, AttachmentStoreOp::eStore
				, ImageLayout::eDepthStencilAttachment
				, std::move( clearValue ) );
		}
		/**
		*\brief
		*	Creates an input stencil attachment.
		*/
		template< typename ImageViewT >
		void addInputStencilView( ImageViewT view )
		{
			addStencilView( "Is"
				, Attachment::FlagKind( Attachment::Flag::Input )
				, ImageAttachment::FlagKind( ImageAttachment::Flag::StencilInput )
				, std::move( view )
				, AttachmentLoadOp::eDontCare
				, AttachmentStoreOp::eDontCare
				, AttachmentLoadOp::eLoad
				, AttachmentStoreOp::eDontCare
				, ImageLayout::eDepthStencilAttachment
				, ClearDepthStencilValue{} );
		}
		/**
		*\brief
		*	Creates an in/out stencil attachment.
		*/
		template< typename ImageViewT >
		void addInOutStencilView( ImageViewT view )
		{
			addStencilView( "IOs"
				, Attachment::FlagKind( Attachment::Flag::InOut )
				, ImageAttachment::FlagKind( ImageAttachment::Flag::StencilInOut )
				, std::move( view )
				, AttachmentLoadOp::eDontCare
				, AttachmentStoreOp::eDontCare
				, AttachmentLoadOp::eLoad
				, AttachmentStoreOp::eStore
				, ImageLayout::eDepthStencilAttachment
				, ClearDepthStencilValue{} );
		}
		/**
		*\brief
		*	Creates an output stencil attachment.
		*/
		template< typename ImageViewT >
		void addOutputStencilView( ImageViewT view
			, ClearDepthStencilValue clearValue = ClearDepthStencilValue{} )
		{
			addStencilView( "Os"
				, Attachment::FlagKind( Attachment::Flag::Output )
				, ImageAttachment::FlagKind( ImageAttachment::Flag::StencilOutput )
				, std::move( view )
				, AttachmentLoadOp::eDontCare
				, AttachmentStoreOp::eDontCare
				, AttachmentLoadOp::eClear
				, AttachmentStoreOp::eStore
				, ImageLayout::eDepthStencilAttachment
				, std::move( clearValue ) );
		}
		/**@}*/
		/**
		*\name
		*	Graph compilation.
		*/
		/**@{*/
		CRG_API RunnablePassPtr createRunnable( GraphContext & context
			, RunnableGraph & graph )const;
		/**@}*/

		CRG_API std::string getFullName()const;
		CRG_API std::string getGroupName()const;

		std::string const & getName()const
		{
			return m_name;
		}

		FramePassGroup const & group;
		FrameGraph & graph;
		uint32_t id;
		AttachmentArray images;
		AttachmentArray buffers;
		RunnablePassCreator runnableCreator;
		FramePassArray passDepends;

	private:
		CRG_API void addColourView( std::string const & name
			, Attachment::FlagKind flags
			, ImageViewIdArray view
			, AttachmentLoadOp loadOp
			, AttachmentStoreOp storeOp
			, ImageLayout wantedLayout = ImageLayout::eUndefined
			, ClearColorValue clearValue = ClearColorValue{}
			, PipelineColorBlendAttachmentState blendState = DefaultBlendState );
		CRG_API void addDepthView( std::string const & name
			, Attachment::FlagKind flags
			, ImageViewIdArray view
			, AttachmentLoadOp loadOp
			, AttachmentStoreOp storeOp
			, AttachmentLoadOp stencilLoadOp
			, AttachmentStoreOp stencilStoreOp
			, ImageLayout wantedLayout = ImageLayout::eUndefined
			, ClearDepthStencilValue clearValue = ClearDepthStencilValue{} );
		CRG_API void addStencilView( std::string const & name
			, Attachment::FlagKind flags
			, ImageAttachment::FlagKind stencilFlags
			, ImageViewIdArray view
			, AttachmentLoadOp loadOp
			, AttachmentStoreOp storeOp
			, AttachmentLoadOp stencilLoadOp
			, AttachmentStoreOp stencilStoreOp
			, ImageLayout wantedLayout = ImageLayout::eUndefined
			, ClearDepthStencilValue clearValue = ClearDepthStencilValue{} );
		CRG_API void addDepthStencilView( std::string const & name
			, Attachment::FlagKind flags
			, ImageAttachment::FlagKind stencilFlags
			, ImageViewIdArray view
			, AttachmentLoadOp loadOp
			, AttachmentStoreOp storeOp
			, AttachmentLoadOp stencilLoadOp
			, AttachmentStoreOp stencilStoreOp
			, ImageLayout wantedLayout = ImageLayout::eUndefined
			, ClearDepthStencilValue clearValue = ClearDepthStencilValue{} );

		void addColourView( std::string const & name
			, Attachment::FlagKind flags
			, ImageViewId view
			, AttachmentLoadOp loadOp
			, AttachmentStoreOp storeOp
			, ImageLayout wantedLayout = ImageLayout::eUndefined
			, ClearColorValue clearValue = ClearColorValue{}
			, PipelineColorBlendAttachmentState blendState = DefaultBlendState )
		{
			addColourView( name
				, flags
				, ImageViewIdArray{ view }
				, loadOp
				, storeOp
				, wantedLayout
				, clearValue
				, std::move( blendState ) );
		}

		void addDepthView( std::string const & name
			, Attachment::FlagKind flags
			, ImageViewId view
			, AttachmentLoadOp loadOp
			, AttachmentStoreOp storeOp
			, AttachmentLoadOp stencilLoadOp
			, AttachmentStoreOp stencilStoreOp
			, ImageLayout wantedLayout = ImageLayout::eUndefined
			, ClearDepthStencilValue clearValue = ClearDepthStencilValue{} )
		{
			addDepthView( name
				, flags
				, ImageViewIdArray{ view }
				, loadOp
				, storeOp
				, stencilLoadOp
				, stencilStoreOp
				, wantedLayout
				, clearValue );
		}

		void addStencilView( std::string const & name
			, Attachment::FlagKind flags
			, ImageAttachment::FlagKind stencilFlags
			, ImageViewId view
			, AttachmentLoadOp loadOp
			, AttachmentStoreOp storeOp
			, AttachmentLoadOp stencilLoadOp
			, AttachmentStoreOp stencilStoreOp
			, ImageLayout wantedLayout = ImageLayout::eUndefined
			, ClearDepthStencilValue clearValue = ClearDepthStencilValue{} )
		{
			addStencilView( name
				, flags
				, stencilFlags
				, ImageViewIdArray{ view }
				, loadOp
				, storeOp
				, stencilLoadOp
				, stencilStoreOp
				, wantedLayout
				, clearValue );
		}

		void addDepthStencilView( std::string const & name
			, Attachment::FlagKind flags
			, ImageAttachment::FlagKind stencilFlags
			, ImageViewId view
			, AttachmentLoadOp loadOp
			, AttachmentStoreOp storeOp
			, AttachmentLoadOp stencilLoadOp
			, AttachmentStoreOp stencilStoreOp
			, ImageLayout wantedLayout = ImageLayout::eUndefined
			, ClearDepthStencilValue clearValue = ClearDepthStencilValue{} )
		{
			addDepthStencilView( name
				, flags
				, stencilFlags
				, ImageViewIdArray{ view }
				, loadOp
				, storeOp
				, stencilLoadOp
				, stencilStoreOp
				, wantedLayout
				, clearValue );
		}

	private:
		std::string m_name;
	};
}
