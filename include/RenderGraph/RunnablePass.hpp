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
		CRG_API RunnablePass( FramePass const & pass
			, GraphContext const & context
			, RunnableGraph & graph );
		CRG_API virtual ~RunnablePass();
		/**
		*\brief
		*	Initialises the descriptor set.
		*/
		CRG_API void initialise();
		/**
		*\brief
		*	Records the render pass into the given command buffer.
		*\param[in,out] commandBuffer
		*	The command buffer.
		*/
		CRG_API void record();
		/**
		*\brief
		*	Records the render pass into the given command buffer.
		*\param[in,out] commandBuffer
		*	The command buffer.
		*/
		CRG_API void recordInto( VkCommandBuffer commandBuffer );
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
		CRG_API void resetCommandBuffer();

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
		void doCreateEvent();
		void doCreateFence();

	protected:
		CRG_API virtual void doInitialise() = 0;
		CRG_API virtual void doRecordInto( VkCommandBuffer commandBuffer )const = 0;
		CRG_API virtual VkPipelineStageFlags doGetSemaphoreWaitFlags()const = 0;

	protected:
		FramePass const & m_pass;
		GraphContext const & m_context;
		RunnableGraph & m_graph;
		VkCommandPool m_commandPool{ VK_NULL_HANDLE };
		VkCommandBuffer m_commandBuffer{ VK_NULL_HANDLE };
		VkSemaphore m_semaphore{ VK_NULL_HANDLE };
		VkEvent m_event{ VK_NULL_HANDLE };
		VkFence m_fence{ VK_NULL_HANDLE };
		FramePassTimer m_timer;
	};
}
