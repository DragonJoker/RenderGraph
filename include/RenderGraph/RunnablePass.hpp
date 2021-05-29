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
			VkImageLayout fromLayout;
			// The layout this pass needs the view to be in
			VkImageLayout neededLayout;
			// The layout the view needs to be, out from this pass.
			VkImageLayout toLayout;
		};

	public:
		CRG_API RunnablePass( FramePass const & pass
			, GraphContext const & context
			, RunnableGraph & graph
			, uint32_t maxPassCount = 1u );
		CRG_API virtual ~RunnablePass();
		/**
		*\brief
		*	Initialises the descriptor set.
		*/
		CRG_API uint32_t initialise( uint32_t index = 0u );
		/**
		*\brief
		*	Records the render pass into the given command buffer.
		*\param[in,out] commandBuffer
		*	The command buffer.
		*/
		CRG_API void record( uint32_t index = 0u );
		/**
		*\brief
		*	Records the render pass into the given command buffer.
		*\param[in,out] commandBuffer
		*	The command buffer.
		*/
		CRG_API void recordInto( VkCommandBuffer commandBuffer
			, uint32_t index = 0u );
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

		FramePassTimer const & getTimer()const
		{
			return m_timer;
		}

		FramePassTimer & getTimer()
		{
			return m_timer;
		}

	private:
		void doCreateCommandPool();
		void doCreateCommandBuffer();
		void doCreateSemaphore();
		void doCreateFence();
		void doRegisterTransition( ImageViewId view
			, LayoutTransition transition );

	protected:
		CRG_API LayoutTransition const & doGetTransition( ImageViewId view )const;
		CRG_API virtual void doInitialise() = 0;
		CRG_API virtual void doRecordInto( VkCommandBuffer commandBuffer
			, uint32_t index );
		CRG_API virtual VkPipelineStageFlags doGetSemaphoreWaitFlags()const = 0;
		CRG_API virtual uint32_t doGetPassIndex()const;

	protected:
		struct CommandBuffer
		{
			VkCommandBuffer commandBuffer{};
			bool recorded{};
		};
		FramePass const & m_pass;
		GraphContext const & m_context;
		RunnableGraph & m_graph;
		VkCommandPool m_commandPool{ VK_NULL_HANDLE };
		std::vector< CommandBuffer > m_commandBuffers;
		VkSemaphore m_semaphore{ VK_NULL_HANDLE };
		VkFence m_fence{ VK_NULL_HANDLE };
		FramePassTimer m_timer;
		std::map< ImageViewId, LayoutTransition > m_transitions;
		bool m_initialised{ false };
	};
}
