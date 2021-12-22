/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "GraphContext.hpp"
#include "FrameGraph.hpp"
#include "RunnablePass.hpp"

namespace crg
{
	/**
	*\brief
	*	Tells how the texture coordinates from the vertex buffer are built.
	*/
	struct Texcoord
	{
		/**
		*\brief
		*	Tells if the U coordinate of UV must be inverted, thus mirroring vertically the resulting image.
		*/
		bool invertU{ false };
		/**
		*\brief
		*	Tells if the U coordinate of UV must be inverted, thus mirroring horizontally the resulting image.
		*/
		bool invertV{ false };
	};

	class RunnableGraph
	{
	public:
		/**
		*\param inputTransitions
		*	Transitions for which the pass is the destination.
		*\param outputTransitions
		*	Transitions for which the pass is the source.
		*\param transitions
		*	All transitions.
		*/
		CRG_API RunnableGraph( FrameGraph & graph
			, FramePassDependencies inputTransitions
			, FramePassDependencies outputTransitions
			, AttachmentTransitions transitions
			, GraphNodePtrArray nodes
			, RootNode rootNode
			, GraphContext & context );
		CRG_API ~RunnableGraph();

		CRG_API void record();

		CRG_API VkImage createImage( ImageId const & image );
		CRG_API VkImageView createImageView( ImageViewId const & view );

		CRG_API SemaphoreWaitArray run( VkQueue queue );
		CRG_API SemaphoreWaitArray run( SemaphoreWait toWait
			, VkQueue queue );
		CRG_API SemaphoreWaitArray run( SemaphoreWaitArray const & toWait
			, VkQueue queue );
		CRG_API ImageViewId createView( ImageViewData const & view );
		CRG_API VertexBuffer const & createQuadTriVertexBuffer( bool texCoords
			, Texcoord const & config );
		CRG_API VkSampler createSampler( SamplerDesc const & samplerDesc );

		CRG_API LayoutState getCurrentLayout( FramePass const & pass
			, uint32_t passIndex
			, ImageViewId view )const;
		CRG_API LayoutState updateCurrentLayout( FramePass const & pass
			, uint32_t passIndex
			, ImageViewId view
			, LayoutState newLayout );
		CRG_API LayoutState getOutputLayout( FramePass const & pass
			, ImageViewId view
			, bool isCompute )const;
		CRG_API AccessState getCurrentAccessState( FramePass const & pass
			, uint32_t passIndex
			, Buffer const & buffer )const;
		CRG_API AccessState updateCurrentAccessState( FramePass const & pass
			, uint32_t passIndex
			, Buffer const & buffer
			, AccessState newState );
		CRG_API AccessState getOutputAccessState( FramePass const & pass
			, Buffer const & buffer
			, bool isCompute )const;
		CRG_API RunnablePass::LayoutTransition getTransition( FramePass const & pass
			, ImageViewId const & view )const;

		CRG_API void memoryBarrier( RecordContext & context
			, VkCommandBuffer commandBuffer
			, ImageViewId const & view
			, VkImageLayout initialLayout
			, LayoutState const & wantedState );
		CRG_API void memoryBarrier( RecordContext & context
			, VkCommandBuffer commandBuffer
			, ImageId const & image
			, VkImageSubresourceRange const & subresourceRange
			, VkImageLayout initialLayout
			, LayoutState const & wantedState );
		CRG_API void memoryBarrier( RecordContext & context
			, VkCommandBuffer commandBuffer
			, ImageId const & image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & subresourceRange
			, VkImageLayout initialLayout
			, LayoutState const & wantedState );

		CRG_API void memoryBarrier( RecordContext & context
			, VkCommandBuffer commandBuffer
			, VkBuffer buffer
			, BufferSubresourceRange const & subresourceRange
			, VkAccessFlags initialMask
			, VkPipelineStageFlags initialStage
			, AccessState const & wantedState );

		ConstGraphAdjacentNode getGraph()const
		{
			return &m_rootNode;
		}
		
		std::string const & getName()const
		{
			return m_graph.getName();
		}

		AttachmentTransitions const & getTransitions()const
		{
			return m_transitions;
		}

	private:
		struct RemainingPasses
		{
			uint32_t count;
			ViewLayoutRanges views;
			BufferLayoutRanges buffers;
		};

		void doRegisterImages( crg::FramePass const & pass
			, LayoutStateMap & images );
		void doRegisterBuffers( crg::FramePass const & pass
			, AccessStateMap & buffers );
		void doCreateImages();
		void doCreateImageViews();

	private:
		FrameGraph & m_graph;
		GraphContext & m_context;
		FramePassDependencies m_inputTransitions;
		FramePassDependencies m_outputTransitions;
		AttachmentTransitions m_transitions;
		GraphNodePtrArray m_nodes;
		RootNode m_rootNode;
		std::vector< RunnablePassPtr > m_passes;
		std::map< ImageId, VkImage > m_images;
		std::map< ImageViewId, VkImageView > m_imageViews;
		std::vector< LayoutStateMap > m_viewsLayouts;
		std::vector< AccessStateMap > m_buffersLayouts;
		std::map< FramePass const *, RemainingPasses > m_passesLayouts;
		uint32_t m_maxPassCount{ 1u };
		std::vector< uint32_t > m_state;
	};
}
