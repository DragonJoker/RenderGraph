/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePass.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <cassert>

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
		doInitialise();
	}

	void RunnablePass::record()
	{
		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
			, nullptr
			, 0u
			, nullptr };
		m_context.vkBeginCommandBuffer( m_commandBuffer, &beginInfo );
		recordInto( m_commandBuffer );
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
		m_timer.notifyPassRender();
		VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO
			, nullptr
			, 1u
			, &toWait.semaphore
			, & toWait.dstStageMask
			, 1u
			, &m_commandBuffer
			, 1u
			, &m_semaphore };
		m_context.vkQueueSubmit( queue
			, 1u
			, &submitInfo
			, VK_NULL_HANDLE );
		return { m_semaphore
			, doGetSemaphoreWaitFlags() };
	}

	void RunnablePass::doCreateCommandPool()
	{
		VkCommandPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO
			, nullptr
			, 0u
			, };
		auto res = m_context.vkCreateCommandPool( m_context.device
			, &createInfo
			, m_context.allocator
			, &m_commandPool );
		checkVkResult( res, "CommandPool creation" );
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
		checkVkResult( res, "CommandBuffer allocation" );
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
		checkVkResult( res, "Semaphore creation" );
		crgRegisterObject( m_context, m_pass.name, m_semaphore );
	}

	void RunnablePass::doRecordInto( VkCommandBuffer commandBuffer )const
	{
	}
}
