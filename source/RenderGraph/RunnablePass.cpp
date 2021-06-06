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
		, RunnableGraph & graph
		, uint32_t maxPassCount )
		: m_pass{ pass }
		, m_context{ context }
		, m_graph{ graph }
		, m_timer{ context, m_pass.name, 1u }
	{
		m_commandBuffers.resize( maxPassCount );
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

		if ( m_semaphore )
		{
			crgUnregisterObject( m_context, m_semaphore );
			m_context.vkDestroySemaphore( m_context.device
				, m_semaphore
				, m_context.allocator );
		}

		for ( auto & cb : m_commandBuffers )
		{
			if ( cb.commandBuffer )
			{
				crgUnregisterObject( m_context, cb.commandBuffer );
				m_context.vkFreeCommandBuffers( m_context.device
					, m_commandPool
					, 1u
					, &cb.commandBuffer );
			}
		}

		if ( m_commandPool )
		{
			crgUnregisterObject( m_context, m_commandPool );
			m_context.vkDestroyCommandPool( m_context.device
				, m_commandPool
				, m_context.allocator );
		}
	}

	uint32_t RunnablePass::initialise( uint32_t index )
	{
		for ( auto & attach : m_pass.images )
		{
			auto view = attach.view( index );

			if ( ( attach.isSampled() || attach.isStorage() )
				&& attach.initialLayout != VK_IMAGE_LAYOUT_UNDEFINED )
			{
				m_graph.updateCurrentLayout( view, attach.initialLayout );
			}

			doRegisterTransition( view
				, { m_graph.getCurrentLayout( view )
				, attach.getImageLayout( m_context.separateDepthStencilLayouts )
				, m_graph.getOutputLayout( m_pass, view ) } );
		}

		if ( !m_initialised )
		{
			doCreateCommandPool();
			doCreateCommandBuffer();
			doCreateSemaphore();
			doCreateFence();
			doInitialise();
			m_initialised = true;
		}

		return uint32_t( m_commandBuffers.size() );
	}

	void RunnablePass::record( uint32_t index )
	{
		auto & cb = m_commandBuffers[index];
		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
			, nullptr
			, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
			, nullptr };
		m_context.vkBeginCommandBuffer( cb.commandBuffer, &beginInfo );
		recordInto( cb.commandBuffer, index );
		m_context.vkEndCommandBuffer( cb.commandBuffer );
		cb.recorded = true;
	}

	void RunnablePass::recordInto( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		auto block = m_timer.start();
		m_context.vkCmdBeginDebugBlock( commandBuffer
			, { m_pass.name, m_context.getNextRainbowColour() } );
		m_timer.beginPass( commandBuffer );

		for ( auto & attach : m_pass.images )
		{
			if ( attach.isSampled() || attach.isStorage() || attach.isTransfer() )
			{
				auto view = attach.view( index );
				auto transition = doGetTransition( view );
				m_graph.memoryBarrier( commandBuffer
					, view
					, transition.fromLayout
					, transition.neededLayout );
			}
		}

		doRecordInto( commandBuffer, index );

		for ( auto & attach : m_pass.images )
		{
			if ( attach.isSampled() || attach.isStorage() || attach.isTransfer() )
			{
				auto view = attach.view( index );
				auto transition = doGetTransition( view );
				m_graph.memoryBarrier( commandBuffer
					, view
					, transition.neededLayout
					, transition.toLayout );
			}
		}

		m_timer.endPass( commandBuffer );
		m_context.vkCmdEndDebugBlock( commandBuffer );
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
		auto index = doGetPassIndex();
		auto & cb = m_commandBuffers[index];

		if ( !cb.recorded )
		{
			record( index );
		}

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
			, &cb.commandBuffer
			, 1u
			, &m_semaphore };
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

	void RunnablePass::resetCommandBuffer( uint32_t index )
	{
		auto & cb = m_commandBuffers[index];
		cb.recorded = false;
		m_context.vkWaitForFences( m_context.device
			, 1u
			, &m_fence
			, VK_TRUE
			, 0xFFFFFFFFFFFFFFFFull );
		m_context.vkResetCommandBuffer( cb.commandBuffer
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
		for ( auto & cb : m_commandBuffers )
		{
			VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
				, nullptr
				, m_commandPool
				, VK_COMMAND_BUFFER_LEVEL_PRIMARY
				, 1u };
			auto res = m_context.vkAllocateCommandBuffers( m_context.device
				, &allocateInfo
				, &cb.commandBuffer );
			checkVkResult( res, m_pass.name + " - CommandBuffer allocation" );
			crgRegisterObject( m_context, m_pass.name, cb.commandBuffer );
		}
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

	void RunnablePass::doRegisterTransition( ImageViewId view
		, LayoutTransition transition )
	{
		if ( transition.toLayout == VK_IMAGE_LAYOUT_UNDEFINED )
		{
			transition.toLayout = transition.neededLayout;
		}

		auto ires = m_transitions.emplace( view, transition );

		if ( !ires.second )
		{
			ires.first->second = transition;
		}

		m_graph.updateCurrentLayout( view, transition.toLayout );

		for ( auto & source : view.data->source )
		{
			doRegisterTransition( source, transition );
		}
	}

	void RunnablePass::doUpdateFinalLayout( ImageViewId view
		, VkImageLayout layout )
	{
		auto it = m_transitions.find( view );
		assert( it != m_transitions.end() );
		it->second.toLayout = layout;
		m_graph.updateCurrentLayout( view, it->second.toLayout );

		for ( auto & source : view.data->source )
		{
			doUpdateFinalLayout( source, layout );
		}
	}

	RunnablePass::LayoutTransition const & RunnablePass::doGetTransition( ImageViewId view )const
	{
		auto it = m_transitions.find( view );
		assert( it != m_transitions.end() );
		return it->second;
	}

	void RunnablePass::doRecordInto( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
	}

	uint32_t RunnablePass::doGetPassIndex()const
	{
		return 0u;
	}
}
