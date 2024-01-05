/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "GraphContext.hpp"
#include "FrameGraph.hpp"
#include "ResourceHandler.hpp"
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
			, FramePassDependencies inputTransitions
			, FramePassDependencies outputTransitions
			, AttachmentTransitions transitions
			, GraphNodePtrArray nodes
			, RootNode rootNode
			, GraphContext & context );
		CRG_API ~RunnableGraph()noexcept;

		CRG_API void record();

		CRG_API SemaphoreWaitArray run( VkQueue queue );
		CRG_API SemaphoreWaitArray run( SemaphoreWait toWait
			, VkQueue queue );
		CRG_API SemaphoreWaitArray run( SemaphoreWaitArray const & toWait
			, VkQueue queue );

		CRG_API ImageViewId createView( ImageViewData const & view );
		CRG_API VkImage createImage( ImageId const & image );
		CRG_API VkImageView createImageView( ImageViewId const & view );
		CRG_API VkSampler createSampler( SamplerDesc const & samplerDesc );
		CRG_API VertexBuffer const & createQuadTriVertexBuffer( bool texCoords
			, Texcoord const & config );

		CRG_API LayoutState getCurrentLayoutState( RecordContext & context
			, ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange range )const;
		CRG_API LayoutState getCurrentLayoutState( RecordContext & context
			, ImageViewId view )const;
		CRG_API LayoutState getNextLayoutState( RecordContext & context
			, crg::RunnablePass const & runnable
			, ImageViewId view )const;
		CRG_API LayoutState getOutputLayoutState( ImageViewId view )const;

		ConstGraphAdjacentNode getGraph()const noexcept
		{
			return &m_rootNode;
		}
		
		std::string const & getName()const noexcept
		{
			return m_graph.getName();
		}

		AttachmentTransitions const & getTransitions()const noexcept
		{
			return m_transitions;
		}

		VkCommandPool getCommandPool()const noexcept
		{
			return m_commandPool.object;
		}

		VkQueryPool getTimerQueryPool()const noexcept
		{
			return m_timerQueries.object;
		}

		uint32_t & getTimerQueryOffset()noexcept
		{
			return m_timerQueryOffset;
		}

		ContextResourcesCache & getResources()noexcept
		{
			return m_resources;
		}

		Fence & getFence()noexcept
		{
			return m_fence;
		}

		Fence const & getFence()const noexcept
		{
			return m_fence;
		}

		FramePassTimer const & getTimer()const
		{
			return m_timer;
		}

		FramePassTimer & getTimer()
		{
			return m_timer;
		}

	private:
		FrameGraph & m_graph;
		GraphContext & m_context;
		ContextResourcesCache m_resources;
		FramePassDependencies m_inputTransitions;
		FramePassDependencies m_outputTransitions;
		AttachmentTransitions m_transitions;
		GraphNodePtrArray m_nodes;
		RootNode m_rootNode;
		ContextObjectT< VkQueryPool > m_timerQueries;
		uint32_t m_timerQueryOffset{};
		ContextObjectT< VkCommandPool > m_commandPool;
		std::vector< RunnablePassPtr > m_passes;
		RecordContext::GraphIndexMap m_states;
		VkCommandBuffer m_commandBuffer{};
		VkSemaphore m_semaphore{};
		Fence m_fence;
		FramePassTimer m_timer;
	};
}
