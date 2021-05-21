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
		RunnablePass( FramePass const & pass
			, GraphContext const & context
			, RunnableGraph & graph );
		virtual ~RunnablePass();
		/**
		*\brief
		*	Initialises the descriptor set.
		*/
		void initialise();
		/**
		*\brief
		*	Records the render pass into the given command buffer.
		*\param[in,out] commandBuffer
		*	The command buffer.
		*/
		void record();
		/**
		*\brief
		*	Records the render pass into the given command buffer.
		*\param[in,out] commandBuffer
		*	The command buffer.
		*/
		void recordInto( VkCommandBuffer commandBuffer );
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
		SemaphoreWait run( SemaphoreWait toWait
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
		SemaphoreWait run( SemaphoreWaitArray const & toWait
			, VkQueue queue );
		/**
		*\brief
		*	Resets the command buffer to initial state.
		*/
		void resetCommandBuffer();

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

	protected:
		virtual void doInitialise() = 0;
		virtual void doRecordInto( VkCommandBuffer commandBuffer )const = 0;
		virtual VkPipelineStageFlags doGetSemaphoreWaitFlags()const = 0;

	protected:
		FramePass const & m_pass;
		GraphContext const & m_context;
		RunnableGraph & m_graph;
		VkCommandPool m_commandPool{ VK_NULL_HANDLE };
		VkCommandBuffer m_commandBuffer{ VK_NULL_HANDLE };
		VkSemaphore m_semaphore{ VK_NULL_HANDLE };
		FramePassTimer m_timer;
	};
}
