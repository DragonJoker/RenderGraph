/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/FramePassTimer.hpp"
#include "RenderGraph/WriteDescriptorSet.hpp"

#include <optional>

namespace crg
{
	class RunnablePass
	{
	public:
		struct LayoutTransition
		{
			// The layout the view is in, when coming to this pass
			LayoutState from;
			// The layout this pass needs the view to be in
			LayoutState needed;
			// The layout the view needs to be, out from this pass.
			LayoutState to;
		};

		struct AccessTransition
		{
			// The layout the buffer is in, when coming to this pass
			AccessState from;
			// The layout this pass needs the buffer to be in
			AccessState needed;
			// The layout the buffer needs to be, out from this pass.
			AccessState to;
		};

	public:
		CRG_API RunnablePass( FramePass const & pass
			, GraphContext const & context
			, RunnableGraph & graph
			, uint32_t maxPassCount = 1u
			, bool optional = false );
		CRG_API virtual ~RunnablePass();
		/**
		*\brief
		*	Initialises the descriptor set.
		*/
		CRG_API uint32_t initialise( uint32_t index = 0u );
		/**
		*\brief
		*	Records the pass commands into its command buffer.
		*\param[in] index
		*	The pass index.
		*/
		CRG_API void record( uint32_t index = 0u );
		/**
		*\brief
		*	Records the pass commands into the given command buffer.
		*\param[in,out] commandBuffer
		*	The command buffer.
		*\param[in] index
		*	The pass index.
		*/
		CRG_API void recordInto( VkCommandBuffer commandBuffer
			, uint32_t index = 0u );
		/**
		*\brief
		*	Records the disabled pass commands into the given command buffer.
		*\param[in,out] commandBuffer
		*	The command buffer.
		*/
		CRG_API void recordDisabledInto( VkCommandBuffer commandBuffer
			, uint32_t index );
		/**
		*\brief
		*	Submits this pass' command buffer to the given queue.
		*\param[in] toWait
		*	The semaphore to wait for.
		*\param[out] queue
		*	The queue to submit to.
		*\return
		*	This pass' semaphore.
		*/
		CRG_API SemaphoreWait run( SemaphoreWait toWait
			, VkQueue queue );
		/**
		*\brief
		*	Submits this pass' command buffer to the given queue.
		*\param[in] toWait
		*	The semaphores to wait for.
		*\param[out] queue
		*	The queue to submit to.
		*\return
		*	This pass' semaphore.
		*/
		CRG_API SemaphoreWait run( SemaphoreWaitArray const & toWait
			, VkQueue queue );
		/**
		*\brief
		*	Resets the command buffer to initial state.
		*/
		CRG_API void resetCommandBuffer( uint32_t index = 0u );
		CRG_API LayoutTransition const & getTransition( uint32_t passIndex
			, ImageViewId const & view )const;
		CRG_API AccessTransition const & getTransition( uint32_t passIndex
			, Buffer const & buffer )const;

		bool isOptional()const
		{
			return m_optional;
		}

		FramePassTimer const & getTimer()const
		{
			return m_timer;
		}

		FramePassTimer & getTimer()
		{
			return m_timer;
		}

		uint32_t getMaxPassCount()const
		{
			return uint32_t( m_commandBuffers.size() );
		}

	private:
		CRG_API virtual void doCreateCommandPool();
		CRG_API virtual void doCreateCommandBuffer();
		CRG_API virtual void doCreateDisabledCommandBuffer();
		CRG_API virtual void doCreateSemaphore();
		CRG_API virtual void doCreateFence();
		void doRegisterTransition( uint32_t passIndex
			, ImageViewId view
			, LayoutTransition transition );
		void doRegisterTransition( uint32_t passIndex
			, Buffer buffer
			, AccessTransition transition );

	protected:
		CRG_API void doUpdateFinalLayout( uint32_t passIndex
			, ImageViewId const & view
			, VkImageLayout layout
			, VkAccessFlags accessMask
			, VkPipelineStageFlags pipelineStage );
		CRG_API void doUpdateFinalAccess( uint32_t passIndex
			, Buffer const & buffer
			, VkAccessFlags accessMask
			, VkPipelineStageFlags pipelineStage );
		CRG_API virtual void doInitialise( uint32_t index ) = 0;
		CRG_API virtual void doRecordInto( VkCommandBuffer commandBuffer
			, uint32_t index );
		CRG_API virtual void doRecordDisabledInto( VkCommandBuffer commandBuffer
			, uint32_t index );
		CRG_API virtual VkPipelineStageFlags doGetSemaphoreWaitFlags()const = 0;
		CRG_API virtual uint32_t doGetPassIndex()const;
		CRG_API virtual bool doIsEnabled()const;
		CRG_API virtual bool doIsComputePass()const;

	protected:
		struct CommandBuffer
		{
			VkCommandBuffer commandBuffer{};
			bool recorded{};
		};
		FramePass const & m_pass;
		GraphContext const & m_context;
		RunnableGraph & m_graph;
		bool m_optional;
		VkCommandPool m_commandPool{ VK_NULL_HANDLE };
		std::vector< CommandBuffer > m_commandBuffers;
		std::vector< CommandBuffer > m_disabledCommandBuffers;
		VkSemaphore m_semaphore{ VK_NULL_HANDLE };
		VkFence m_fence{ VK_NULL_HANDLE };
		FramePassTimer m_timer;
		using LayoutTransitionMap = std::map< ImageViewId, LayoutTransition >;
		using AccessTransitionMap = std::map< VkBuffer, AccessTransition >;
		std::vector< LayoutTransitionMap > m_layoutTransitions;
		std::vector< AccessTransitionMap > m_accessTransitions;
		bool m_initialised{ false };
	};
}
