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

		CRG_API VkBuffer createBuffer( BufferId const & buffer );
		CRG_API VkBufferView createBufferView( BufferViewId const & view );
		CRG_API VkImage createImage( ImageId const & image );
		CRG_API VkImageView createImageView( ImageViewId const & view );
		CRG_API VkSampler createSampler( SamplerDesc const & samplerDesc );
		CRG_API VertexBuffer const & createQuadTriVertexBuffer( bool texCoords
			, Texcoord const & config );

		CRG_API LayoutState getCurrentLayoutState( RecordContext & context
			, ImageId image
			, ImageViewType viewType
			, ImageSubresourceRange range )const;
		CRG_API LayoutState getCurrentLayoutState( RecordContext & context
			, ImageViewId view )const;
		CRG_API LayoutState getNextLayoutState( RecordContext const & context
			, crg::RunnablePass const & runnable
			, ImageViewId view )const;
		CRG_API LayoutState getOutputLayoutState( ImageViewId view )const;

		CRG_API VkDescriptorType getDescriptorType( Attachment const & attach )const;
		CRG_API WriteDescriptorSet getDescriptorWrite( Attachment const & attach, uint32_t binding, uint32_t index = 0u );
		CRG_API WriteDescriptorSet getDescriptorWrite( Attachment const & attach, SamplerDesc const & samplerDesc, uint32_t binding, uint32_t index = 0u );

		template< typename EnumT >
		WriteDescriptorSet getDescriptorWriteT( Attachment const & attach, EnumT binding, uint32_t index = 0u )
		{
			return getDescriptorWrite( attach, uint32_t( binding ), index );
		}

		template< typename EnumT >
		WriteDescriptorSet getDescriptorWriteT( Attachment const & attach, SamplerDesc const & samplerDesc, EnumT binding, uint32_t index = 0u )
		{
			return getDescriptorWrite( attach, samplerDesc, uint32_t( binding ), index );
		}

		ConstGraphAdjacentNode getNodeGraph()const noexcept
		{
			return &m_rootNode;
		}
		
		std::string const & getName()const noexcept
		{
			return m_graph.getName();
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

		FramePassTimer const & getTimer()const noexcept
		{
			return m_timer;
		}

		FramePassTimer & getTimer()noexcept
		{
			return m_timer;
		}

		GraphContext & getContext()const noexcept
		{
			return m_context;
		}

	private:
		FrameGraph & m_graph;
		GraphContext & m_context;
		ContextResourcesCache m_resources;
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
