/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePass.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <cassert>
#include <thread>

namespace crg
{
	RunnablePass::RunnablePass( FramePass const & pass
		, GraphContext const & context
		, RunnableGraph & graph )
		: m_pass{ pass }
		, m_context{ context }
		, m_graph{ graph }
		, m_timer{ context, m_pass.name, 1u }
	{
	}

	RunnablePass::~RunnablePass()
	{
		if ( m_fence )
		{
			crgUnregisterObject( m_context, m_fence );
			m_context.vkDestroyFence( m_context.device
				, m_fence
				, m_context.allocator );
		}
		
		if ( m_event )
		{
			crgUnregisterObject( m_context, m_event );
			m_context.vkDestroyEvent( m_context.device
				, m_event
				, m_context.allocator );
		}

		if ( m_semaphore )
		{
			crgUnregisterObject( m_context, m_semaphore );
			m_context.vkDestroySemaphore( m_context.device
				, m_semaphore
				, m_context.allocator );
		}
		
		if ( m_commandBuffer )
		{
			crgUnregisterObject( m_context, m_commandBuffer );
			m_context.vkFreeCommandBuffers( m_context.device
				, m_commandPool
				, 1u
				, &m_commandBuffer );
		}

		if ( m_commandPool )
		{
			crgUnregisterObject( m_context, m_commandPool );
			m_context.vkDestroyCommandPool( m_context.device
				, m_commandPool
				, m_context.allocator );
		}
	}

	void RunnablePass::initialise()
	{
		doCreateCommandPool();
		doCreateCommandBuffer();
		doCreateSemaphore();
		doCreateEvent();
		doCreateFence();
		doInitialise();
	}

	void RunnablePass::record()
	{
		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
			, nullptr
			, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
			, nullptr };
		m_context.vkBeginCommandBuffer( m_commandBuffer, &beginInfo );
		recordInto( m_commandBuffer );
		m_context.vkCmdSetEvent( m_commandBuffer, m_event, doGetSemaphoreWaitFlags() );
		m_context.vkEndCommandBuffer( m_commandBuffer );
	}

	void RunnablePass::recordInto( VkCommandBuffer commandBuffer )
	{
		auto block = m_timer.start();
		m_context.vkCmdBeginDebugBlock( m_commandBuffer
			, { m_pass.name, m_context.getNextRainbowColour() } );
		m_timer.beginPass( commandBuffer );
		doRecordInto( commandBuffer );
		m_timer.endPass( commandBuffer );
		m_context.vkCmdEndDebugBlock( m_commandBuffer );
	}

	SemaphoreWait RunnablePass::run( SemaphoreWait toWait
		, VkQueue queue )
	{
		return run( ( toWait.semaphore
				? SemaphoreWaitArray{ 1u, toWait }
				: SemaphoreWaitArray{} )
			, queue );
	}

	SemaphoreWait RunnablePass::run( SemaphoreWaitArray const & toWait
		, VkQueue queue )
	{
		m_timer.notifyPassRender();
		std::vector< VkSemaphore > semaphores;
		std::vector< VkPipelineStageFlags > dstStageMasks;

		for ( auto & wait : toWait )
		{
			semaphores.push_back( wait.semaphore );
			dstStageMasks.push_back( wait.dstStageMask );
		}

		VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO
			, nullptr
			, uint32_t( toWait.size() )
			, semaphores.data()
			, dstStageMasks.data()
			, 1u
			, &m_commandBuffer
			, 1u
			, &m_semaphore };
		m_context.vkResetEvent( m_context.device
			, m_event );
		m_context.vkResetFences( m_context.device
			, 1u
			, &m_fence );
		m_context.vkQueueSubmit( queue
			, 1u
			, &submitInfo
			, m_fence );
		return { m_semaphore
			, doGetSemaphoreWaitFlags() };
	}

	void RunnablePass::resetCommandBuffer()
	{
		m_context.vkWaitForFences( m_context.device
			, 1u
			, &m_fence
			, VK_TRUE
			, 0xFFFFFFFFFFFFFFFFull );
		m_context.vkResetCommandBuffer( m_commandBuffer
			, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT );
	}

	void RunnablePass::doCreateCommandPool()
	{
		VkCommandPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO
			, nullptr
			, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
			, };
		auto res = m_context.vkCreateCommandPool( m_context.device
			, &createInfo
			, m_context.allocator
			, &m_commandPool );
		checkVkResult( res, m_pass.name + " - CommandPool creation" );
		crgRegisterObject( m_context, m_pass.name, m_commandPool );
	}

	void RunnablePass::doCreateCommandBuffer()
	{
		VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
			, nullptr
			, m_commandPool
			, VK_COMMAND_BUFFER_LEVEL_PRIMARY
			, 1u };
		auto res = m_context.vkAllocateCommandBuffers( m_context.device
			, &allocateInfo
			, &m_commandBuffer );
		checkVkResult( res, m_pass.name + " - CommandBuffer allocation" );
		crgRegisterObject( m_context, m_pass.name, m_commandBuffer );
	}

	void RunnablePass::doCreateSemaphore()
	{
		VkSemaphoreCreateInfo createInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
			, nullptr
			, 0u };
		auto res = m_context.vkCreateSemaphore( m_context.device
			, &createInfo
			, m_context.allocator
			, &m_semaphore );
		checkVkResult( res, m_pass.name + " - Semaphore creation" );
		crgRegisterObject( m_context, m_pass.name, m_semaphore );
	}

	void RunnablePass::doCreateEvent()
	{
		VkEventCreateInfo createInfo{ VK_STRUCTURE_TYPE_EVENT_CREATE_INFO
			, nullptr
			, 0u };
		auto res = m_context.vkCreateEvent( m_context.device
			, &createInfo
			, m_context.allocator
			, &m_event );
		checkVkResult( res, m_pass.name + " - Event creation" );
		crgRegisterObject( m_context, m_pass.name, m_event );
	}

	void RunnablePass::doCreateFence()
	{
		VkFenceCreateInfo createInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
			, nullptr
			, VK_FENCE_CREATE_SIGNALED_BIT };
		auto res = m_context.vkCreateFence( m_context.device
			, &createInfo
			, m_context.allocator
			, &m_fence );
		checkVkResult( res, m_pass.name + " - Fence creation" );
		crgRegisterObject( m_context, m_pass.name, m_fence );
	}

	void RunnablePass::doRecordInto( VkCommandBuffer commandBuffer )const
	{
	}
}
