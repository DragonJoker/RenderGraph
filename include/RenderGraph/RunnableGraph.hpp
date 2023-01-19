/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "GraphContext.hpp"
#include "FrameGraph.hpp"
#include "RunnableLayoutsCache.hpp"
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

	template<>
	struct DefaultValueGetterT< Texcoord >
	{
		static Texcoord get()
		{
			static Texcoord const result{ false, false };
			return result;
		}
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
			, FramePassArray passes
			, FramePassDependencies inputTransitions
			, FramePassDependencies outputTransitions
			, AttachmentTransitions transitions
			, GraphNodePtrArray nodes
			, RootNode rootNode
			, GraphContext & context );

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

		VkCommandPool getCommandPool()const
		{
			return m_commandPool.object;
		}

		VkQueryPool getTimerQueryPool()const
		{
			return m_timerQueries.object;
		}

		uint32_t & getTimerQueryOffset()
		{
			return m_timerQueryOffset;
		}

	private:
		FrameGraph & m_graph;
		GraphContext & m_context;
		std::unique_ptr< RunnableLayoutsCache > m_layouts;
		FramePassDependencies m_inputTransitions;
		FramePassDependencies m_outputTransitions;
		AttachmentTransitions m_transitions;
		GraphNodePtrArray m_nodes;
		RootNode m_rootNode;
		ContextObjectT< VkQueryPool > m_timerQueries;
		uint32_t m_timerQueryOffset{};
		ContextObjectT< VkCommandPool > m_commandPool;
		std::vector< RunnablePassPtr > m_passes;
		uint32_t m_maxPassCount{ 1u };
		RecordContext::GraphIndexMap m_states;
	};
}
