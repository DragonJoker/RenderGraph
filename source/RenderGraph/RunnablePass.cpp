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
		, GraphContext & context
		, RunnableGraph & graph
		, uint32_t maxPassCount
		, bool optional )
		: m_pass{ pass }
		, m_context{ context }
		, m_graph{ graph }
		, m_optional{ optional }
		, m_timer{ context, m_pass.name, 1u }
	{
		m_commandBuffers.resize( maxPassCount );
		m_disabledCommandBuffers.resize( maxPassCount );
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
		if ( index >= m_commandBuffers.size() )
		{
			return 0u;
		}

		for ( auto & attach : m_pass.images )
		{
			if ( attach.count <= 1u )
			{
				auto view = attach.view( index );
				auto inputLayout = m_graph.getCurrentLayout( index, view );

				if ( ( attach.isInput() )
					&& attach.image.initialLayout != VK_IMAGE_LAYOUT_UNDEFINED )
				{
					inputLayout = { attach.image.initialLayout
						, getAccessMask( attach.image.initialLayout )
						, getStageMask( attach.image.initialLayout ) };
				}

				auto outputLayout = m_graph.getOutputLayout( m_pass, view, doIsComputePass() );

				if ( ( attach.isOutput() )
					&& attach.image.finalLayout != VK_IMAGE_LAYOUT_UNDEFINED )
				{
					outputLayout = { attach.image.finalLayout
						, getAccessMask( attach.image.finalLayout )
						, getStageMask( attach.image.finalLayout ) };
				}

				doRegisterTransition( index
					, view
					, { inputLayout
						, { attach.getImageLayout( m_context.separateDepthStencilLayouts )
							, attach.getAccessMask()
							, attach.getPipelineStageFlags( doIsComputePass() ) }
						, outputLayout } );
			}
			else
			{
				for ( uint32_t i = 0u; i < attach.count; ++i )
				{
					auto view = attach.view( i );
					auto inputLayout = m_graph.getCurrentLayout( index, view );

					if ( ( attach.isInput() )
						&& attach.image.initialLayout != VK_IMAGE_LAYOUT_UNDEFINED )
					{
						inputLayout = { attach.image.initialLayout
							, getAccessMask( attach.image.initialLayout )
							, getStageMask( attach.image.initialLayout ) };
					}

					auto outputLayout = m_graph.getOutputLayout( m_pass, view, doIsComputePass() );

					if ( ( attach.isOutput() )
						&& attach.image.finalLayout != VK_IMAGE_LAYOUT_UNDEFINED )
					{
						outputLayout = { attach.image.finalLayout
							, getAccessMask( attach.image.finalLayout )
							, getStageMask( attach.image.finalLayout ) };
					}

					doRegisterTransition( index
						, view
						, { inputLayout
							, { attach.getImageLayout( m_context.separateDepthStencilLayouts )
								, attach.getAccessMask()
								, attach.getPipelineStageFlags( doIsComputePass() ) }
							, outputLayout } );
				}
			}
		}
		
		for ( auto & attach : m_pass.buffers )
		{
			if ( attach.isStorageBuffer() )
			{
				auto buffer = attach.buffer.buffer;
				doRegisterTransition( index
					, buffer
					, { m_graph.getCurrentAccessState( index, buffer )
						, { attach.getAccessMask()
							, attach.getPipelineStageFlags( doIsComputePass() ) }
						, m_graph.getOutputAccessState( m_pass, buffer, doIsComputePass() ) } );
			}
		}

		if ( !m_initialised )
		{
			doCreateCommandPool();
			doCreateCommandBuffer();
			doCreateDisabledCommandBuffer();
			doCreateSemaphore();
			doCreateFence();
			m_initialised = true;
		}

		doInitialise( index );
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

		if ( isOptional() )
		{
			auto & cb = m_disabledCommandBuffers[index];
			VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
				, nullptr
				, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
				, nullptr };
			m_context.vkBeginCommandBuffer( cb.commandBuffer, &beginInfo );
			recordDisabledInto( cb.commandBuffer, index );
			m_context.vkEndCommandBuffer( cb.commandBuffer );
		}
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
			if ( attach.isSampledView() || attach.isStorageView() || attach.isTransferView() )
			{
				if ( attach.count <= 1u )
				{
					auto view = attach.view( index );
					auto transition = getTransition( index
						, view );
					m_graph.memoryBarrier( commandBuffer
						, view
						, transition.from
						, transition.needed );
				}
				else
				{
					for ( uint32_t i = 0u; i < attach.count; ++i )
					{
						auto view = attach.view( i );
						auto transition = getTransition( index
							, view );
						m_graph.memoryBarrier( commandBuffer
							, view
							, transition.from
							, transition.needed );
					}
				}
			}
		}

		for ( auto & attach : m_pass.buffers )
		{
			if ( attach.isStorageBuffer() )
			{
				auto buffer = attach.buffer;
				auto transition = getTransition( index
					, buffer.buffer );
				m_graph.memoryBarrier( commandBuffer
					, buffer.buffer
					, buffer.offset
					, buffer.range
					, transition.from
					, transition.needed );
			}
		}

		doRecordInto( commandBuffer, index );

		for ( auto & attach : m_pass.images )
		{
			if ( attach.isSampledView() || attach.isStorageView() || attach.isTransferView() )
			{
				if ( attach.count <= 1u )
				{
					auto view = attach.view( index );
					auto transition = getTransition( index
						, view );
					m_graph.memoryBarrier( commandBuffer
						, view
						, transition.needed
						, transition.to );
				}
				else
				{
					for ( uint32_t i = 0u; i < attach.count; ++i )
					{
						auto view = attach.view( i );
						auto transition = getTransition( index
							, view );
						m_graph.memoryBarrier( commandBuffer
							, view
							, transition.needed
							, transition.to );
					}
				}
			}
		}

		for ( auto & attach : m_pass.buffers )
		{
			if ( attach.isStorageBuffer() )
			{
				auto buffer = attach.buffer;
				auto transition = getTransition( index
					, buffer.buffer );
				m_graph.memoryBarrier( commandBuffer
					, buffer.buffer
					, buffer.offset
					, buffer.range
					, transition.needed
					, transition.to );
			}
		}

		m_timer.endPass( commandBuffer );
		m_context.vkCmdEndDebugBlock( commandBuffer );
	}

	void RunnablePass::recordDisabledInto( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
		m_context.vkCmdBeginDebugBlock( commandBuffer
			, { "(Disabled)" + m_pass.name, { 0.5f, 0.5f, 0.5f, 1.0f } } );
		m_timer.beginPass( commandBuffer );
		doRecordDisabledInto( commandBuffer, index );

		for ( auto & attach : m_pass.images )
		{
			if ( attach.isSampledView() || attach.isStorageView() || attach.isTransferView() )
			{
				if ( attach.count <= 1u )
				{
					auto view = attach.view( index );
					auto transition = getTransition( index
						, view );
					m_graph.memoryBarrier( commandBuffer
						, view
						, transition.from
						, transition.to );
				}
				else
				{
					for ( uint32_t i = 0u; i < attach.count; ++i )
					{
						auto view = attach.view( i );
						auto transition = getTransition( index
							, view );
						m_graph.memoryBarrier( commandBuffer
							, view
							, transition.from
							, transition.to );
					}
				}
			}
		}

		for ( auto & attach : m_pass.buffers )
		{
			if ( attach.isStorageBuffer() )
			{
				auto buffer = attach.buffer;
				auto transition = getTransition( index
					, buffer.buffer );
				m_graph.memoryBarrier( commandBuffer
					, buffer.buffer
					, buffer.offset
					, buffer.range
					, transition.from
					, transition.to );
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
		std::vector< VkSemaphore > semaphores;
		std::vector< VkPipelineStageFlags > dstStageMasks;

		for ( auto & wait : toWait )
		{
			semaphores.push_back( wait.semaphore );
			dstStageMasks.push_back( wait.dstStageMask );
		}

		auto index = doGetPassIndex();
		auto & cb = doIsEnabled()
			? m_commandBuffers[index]
			: m_disabledCommandBuffers[index];

		if ( !cb.recorded )
		{
			record( index );
		}

		m_timer.notifyPassRender();
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

	RunnablePass::LayoutTransition const & RunnablePass::getTransition( uint32_t passIndex
		, ImageViewId const & view )const
	{
		assert( m_layoutTransitions.size() > passIndex );
		auto & layoutTransitions = m_layoutTransitions[passIndex];
		auto it = layoutTransitions.find( view );
		assert( it != layoutTransitions.end() );
		return it->second;
	}

	RunnablePass::AccessTransition const & RunnablePass::getTransition( uint32_t passIndex
		, Buffer const & buffer )const
	{
		assert( m_accessTransitions.size() > passIndex );
		auto & accessTransitions = m_accessTransitions[passIndex];
		auto it = accessTransitions.find( buffer.buffer );
		assert( it != accessTransitions.end() );
		return it->second;
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

	void RunnablePass::doCreateDisabledCommandBuffer()
	{
		for ( auto & cb : m_disabledCommandBuffers )
		{
			VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
				, nullptr
				, m_commandPool
				, VK_COMMAND_BUFFER_LEVEL_PRIMARY
				, 1u };
			auto res = m_context.vkAllocateCommandBuffers( m_context.device
				, &allocateInfo
				, &cb.commandBuffer );
			checkVkResult( res, m_pass.name + "Disabled - CommandBuffer allocation" );
			crgRegisterObject( m_context, m_pass.name + "Disabled", cb.commandBuffer );
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

	void RunnablePass::doRegisterTransition( uint32_t passIndex
		, ImageViewId view
		, LayoutTransition transition )
	{
		if ( m_layoutTransitions.size() <= passIndex )
		{
			m_layoutTransitions.push_back( {} );
		}

		assert( m_layoutTransitions.size() > passIndex );
		auto & layoutTransitions = m_layoutTransitions[passIndex];

		if ( transition.to.layout == VK_IMAGE_LAYOUT_UNDEFINED )
		{
			transition.to = transition.needed;
		}

		auto ires = layoutTransitions.emplace( view, transition );

		if ( !ires.second )
		{
			ires.first->second = transition;
		}

		m_graph.updateCurrentLayout( passIndex, view, transition.to );

		for ( auto & source : view.data->source )
		{
			doRegisterTransition( passIndex, source, transition );
		}
	}

	void RunnablePass::doRegisterTransition( uint32_t passIndex
		, Buffer buffer
		, AccessTransition transition )
	{
		if ( m_accessTransitions.size() <= passIndex )
		{
			m_accessTransitions.push_back( {} );
		}

		assert( m_accessTransitions.size() > passIndex );
		auto & accessTransitions = m_accessTransitions[passIndex];

		if ( transition.to.access == 0u )
		{
			transition.to = transition.needed;
		}

		auto ires = accessTransitions.emplace( buffer.buffer, transition );

		if ( !ires.second )
		{
			ires.first->second = transition;
		}

		m_graph.updateCurrentAccessState( passIndex, buffer, transition.to );
	}

	void RunnablePass::doUpdateFinalLayout( uint32_t passIndex
		, ImageViewId const & view
		, VkImageLayout layout
		, VkAccessFlags accessMask
		, VkPipelineStageFlags pipelineStage )
	{
		assert( m_layoutTransitions.size() > passIndex );
		auto & layoutTransitions = m_layoutTransitions[passIndex];
		auto it = layoutTransitions.find( view );
		assert( it != layoutTransitions.end() );
		it->second.to.layout = layout;
		it->second.to.access = accessMask;
		it->second.to.pipelineStage = pipelineStage;
		m_graph.updateCurrentLayout( passIndex, view, it->second.to );

		for ( auto & source : view.data->source )
		{
			doUpdateFinalLayout( passIndex, source, layout, accessMask, pipelineStage );
		}
	}

	void RunnablePass::doUpdateFinalAccess( uint32_t passIndex
		, Buffer const & buffer
		, VkAccessFlags accessMask
		, VkPipelineStageFlags pipelineStage )
	{
		assert( m_accessTransitions.size() > passIndex );
		auto & accessTransitions = m_accessTransitions[passIndex];
		auto it = accessTransitions.find( buffer.buffer );
		assert( it != accessTransitions.end() );
		it->second.to.access = accessMask;
		it->second.to.pipelineStage = pipelineStage;
		m_graph.updateCurrentAccessState( passIndex, buffer, it->second.to );
	}

	void RunnablePass::doRecordInto( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
	}

	void RunnablePass::doRecordDisabledInto( VkCommandBuffer commandBuffer
		, uint32_t index )
	{
	}

	uint32_t RunnablePass::doGetPassIndex()const
	{
		return 0u;
	}

	bool RunnablePass::doIsEnabled()const
	{
		return true;
	}

	bool RunnablePass::doIsComputePass()const
	{
		return false;
	}
}
