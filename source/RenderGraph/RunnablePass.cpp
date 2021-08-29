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
	//*********************************************************************************************

	void convert( SemaphoreWaitArray const & toWait
		, std::vector< VkSemaphore > & semaphores
		, std::vector< VkPipelineStageFlags > & dstStageMasks )
	{
		for ( auto & wait : toWait )
		{
			if ( wait.semaphore
				&& semaphores.end() == std::find( semaphores.begin()
					, semaphores.end()
					, wait.semaphore ) )
			{
				semaphores.push_back( wait.semaphore );
				dstStageMasks.push_back( wait.dstStageMask );
			}
		}
	}

	void convert( SemaphoreWaitArray const & toWait
		, std::vector< VkSemaphore > & semaphores )
	{
		for ( auto & wait : toWait )
		{
			if ( wait.semaphore
				&& semaphores.end() == std::find( semaphores.begin()
					, semaphores.end()
					, wait.semaphore ) )
			{
				semaphores.push_back( wait.semaphore );
			}
		}
	}

	//*********************************************************************************************

	RunnablePass::Callbacks::Callbacks( InitialiseCallback initialise
		, GetSemaphoreWaitFlagsCallback getSemaphoreWaitFlags )
		: Callbacks{ std::move( initialise )
			, std::move( getSemaphoreWaitFlags )
			, getDefaultV< RecordCallback >()
			, getDefaultV< RecordCallback >()
			, getDefaultV< GetPassIndexCallback >()
			, getDefaultV< IsEnabledCallback >()
			, getDefaultV< IsComputePassCallback >() }
	{
	}

	RunnablePass::Callbacks::Callbacks( InitialiseCallback initialise
		, GetSemaphoreWaitFlagsCallback getSemaphoreWaitFlags
		, RecordCallback record )
		: Callbacks{ std::move( initialise )
			, std::move( getSemaphoreWaitFlags )
			, std::move( record )
			, getDefaultV< RecordCallback >()
			, getDefaultV< GetPassIndexCallback >()
			, getDefaultV< IsEnabledCallback >()
			, getDefaultV< IsComputePassCallback >() }
	{
	}

	RunnablePass::Callbacks::Callbacks( InitialiseCallback initialise
		, GetSemaphoreWaitFlagsCallback getSemaphoreWaitFlags
		, RecordCallback record
		, RecordCallback recordDisabled )
		: Callbacks{ std::move( initialise )
			, std::move( getSemaphoreWaitFlags )
			, std::move( record )
			, std::move( recordDisabled )
			, getDefaultV< GetPassIndexCallback >()
			, getDefaultV< IsEnabledCallback >()
			, getDefaultV< IsComputePassCallback >() }
	{
	}

	RunnablePass::Callbacks::Callbacks( InitialiseCallback initialise
		, GetSemaphoreWaitFlagsCallback getSemaphoreWaitFlags
		, RecordCallback record
		, RecordCallback recordDisabled
		, GetPassIndexCallback getPassIndex )
		: Callbacks{ std::move( initialise )
			, std::move( getSemaphoreWaitFlags )
			, std::move( record )
			, std::move( recordDisabled )
			, std::move( getPassIndex )
			, getDefaultV< IsEnabledCallback >()
			, getDefaultV< IsComputePassCallback >() }
	{
	}

	RunnablePass::Callbacks::Callbacks( InitialiseCallback initialise
		, GetSemaphoreWaitFlagsCallback getSemaphoreWaitFlags
		, RecordCallback record
		, RecordCallback recordDisabled
		, GetPassIndexCallback getPassIndex
		, IsEnabledCallback isEnabled )
		: Callbacks{ std::move( initialise )
			, std::move( getSemaphoreWaitFlags )
			, std::move( record )
			, std::move( recordDisabled )
			, std::move( getPassIndex )
			, std::move( isEnabled )
			, getDefaultV< IsComputePassCallback >() }
	{
	}

	RunnablePass::Callbacks::Callbacks( InitialiseCallback initialise
		, GetSemaphoreWaitFlagsCallback getSemaphoreWaitFlags
		, RecordCallback record
		, RecordCallback recordDisabled
		, GetPassIndexCallback getPassIndex
		, IsEnabledCallback isEnabled
		, IsComputePassCallback isComputePass )
		: initialise{ std::move( initialise ) }
		, getSemaphoreWaitFlags{ std::move( getSemaphoreWaitFlags ) }
		, record{ std::move( record ) }
		, recordDisabled{ std::move( recordDisabled ) }
		, getPassIndex{ std::move( getPassIndex ) }
		, isEnabled{ std::move( isEnabled ) }
		, isComputePass{ std::move( isComputePass ) }
	{
	}

	//*********************************************************************************************

	RunnablePass::RunnablePass( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, Callbacks callbacks
		, uint32_t maxPassCount
		, bool optional )
		: m_pass{ pass }
		, m_context{ context }
		, m_graph{ graph }
		, m_callbacks{ std::move( callbacks ) }
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

	uint32_t RunnablePass::initialise()
	{
		for ( uint32_t index = 0u; index < m_commandBuffers.size(); ++index )
		{
			for ( auto & attach : m_pass.images )
			{
				if ( attach.count <= 1u )
				{
					auto view = attach.view( index );
					auto inputLayout = m_graph.getCurrentLayout( m_pass, index, view );

					if ( ( attach.isInput() )
						&& attach.image.initialLayout != VK_IMAGE_LAYOUT_UNDEFINED )
					{
						inputLayout = { attach.image.initialLayout
							, getAccessMask( attach.image.initialLayout )
							, getStageMask( attach.image.initialLayout ) };
					}

					auto outputLayout = m_graph.getOutputLayout( m_pass, view, m_callbacks.isComputePass() );

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
						, attach.getPipelineStageFlags( m_callbacks.isComputePass() ) }
					, outputLayout } );
				}
				else
				{
					for ( uint32_t i = 0u; i < attach.count; ++i )
					{
						auto view = attach.view( i );
						auto inputLayout = m_graph.getCurrentLayout( m_pass, index, view );

						if ( ( attach.isInput() )
							&& attach.image.initialLayout != VK_IMAGE_LAYOUT_UNDEFINED )
						{
							inputLayout = { attach.image.initialLayout
								, getAccessMask( attach.image.initialLayout )
								, getStageMask( attach.image.initialLayout ) };
						}

						auto outputLayout = m_graph.getOutputLayout( m_pass, view, m_callbacks.isComputePass() );

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
							, attach.getPipelineStageFlags( m_callbacks.isComputePass() ) }
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
						, { m_graph.getCurrentAccessState( m_pass, index, buffer )
						, { attach.getAccessMask()
						, attach.getPipelineStageFlags( m_callbacks.isComputePass() ) }
					, m_graph.getOutputAccessState( m_pass, buffer, m_callbacks.isComputePass() ) } );
				}
			}
		}

		doCreateSemaphore();
		doCreateFence();
		m_callbacks.initialise();
		return uint32_t( m_commandBuffers.size() );
	}

	void RunnablePass::recordCurrent()
	{
		auto index = m_callbacks.getPassIndex();
		recordOne( m_commandBuffers[index]
			, m_disabledCommandBuffers[index]
			, index );
	}

	void RunnablePass::recordAll()
	{
		for ( uint32_t index = 0u; index < m_commandBuffers.size(); ++index )
		{
			recordOne( m_commandBuffers[index]
				, m_disabledCommandBuffers[index]
				, index );
		}
	}

	void RunnablePass::recordOne( CommandBuffer & enabled
		, CommandBuffer & disabled
		, uint32_t index )
	{
		if ( enabled.recorded )
		{
			return;
		}

		if ( !m_commandPool )
		{
			doCreateCommandPool();
		}

		if ( !enabled.commandBuffer )
		{
			enabled.commandBuffer = doCreateCommandBuffer( std::string{} );
		}

		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
			, nullptr
			, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
			, nullptr };
		m_context.vkBeginCommandBuffer( enabled.commandBuffer, &beginInfo );
		recordInto( enabled.commandBuffer, index );
		m_context.vkEndCommandBuffer( enabled.commandBuffer );
		enabled.recorded = true;

		if ( isOptional() )
		{
			if ( !disabled.commandBuffer )
			{
				disabled.commandBuffer = doCreateCommandBuffer( "Disabled" );
			}

			VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
				, nullptr
				, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
				, nullptr };
			m_context.vkBeginCommandBuffer( disabled.commandBuffer, &beginInfo );
			recordDisabledInto( disabled.commandBuffer, index );
			m_context.vkEndCommandBuffer( disabled.commandBuffer );
			disabled.recorded = true;
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

		m_callbacks.record( commandBuffer, index );

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
		m_callbacks.recordDisabled( commandBuffer, index );

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
		if ( m_context.device )
		{
			std::vector< VkSemaphore > semaphores;
			std::vector< VkPipelineStageFlags > dstStageMasks;
			convert( toWait, semaphores, dstStageMasks );
			auto index = m_callbacks.getPassIndex();
			auto & cb = m_callbacks.isEnabled()
				? m_commandBuffers[index]
				: m_disabledCommandBuffers[index];
			assert( cb.recorded );
			m_timer.notifyPassRender();
			VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO
				, nullptr
				, uint32_t( semaphores.size() )
				, semaphores.data()
				, dstStageMasks.data()
				, 1u
				, &cb.commandBuffer
				, 1u
				, &m_semaphore };

			if ( !m_fenceWaited )
			{
				m_context.vkWaitForFences( m_context.device
					, 1u
					, &m_fence
					, VK_TRUE
					, 0xFFFFFFFFFFFFFFFFull );
			}

			m_fenceWaited = false;
			m_context.vkResetFences( m_context.device
				, 1u
				, &m_fence );
			m_context.vkQueueSubmit( queue
				, 1u
				, &submitInfo
				, m_fence );
		}

		return { m_semaphore
			, m_callbacks.getSemaphoreWaitFlags() };
	}

	void RunnablePass::resetCommandBuffer()
{
		if ( !m_context.device )
		{
			return;
		}

		if ( m_fence )
		{
			m_context.vkWaitForFences( m_context.device
				, 1u
				, &m_fence
				, VK_TRUE
				, 0xFFFFFFFFFFFFFFFFull );
			m_fenceWaited = true;
		}

		for ( auto & cb : m_commandBuffers )
		{
			if ( cb.commandBuffer )
			{
				cb.recorded = false;
				m_context.vkResetCommandBuffer( cb.commandBuffer
					, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT );
			}
		}
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
		if ( !m_context.device )
		{
			return;
		}

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

	VkCommandBuffer RunnablePass::doCreateCommandBuffer( std::string const & suffix )
	{
		VkCommandBuffer result{};

		if ( !m_context.device )
		{
			return result;
		}

		VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
			, nullptr
			, m_commandPool
			, VK_COMMAND_BUFFER_LEVEL_PRIMARY
			, 1u };
		auto res = m_context.vkAllocateCommandBuffers( m_context.device
			, &allocateInfo
			, &result );
		checkVkResult( res, m_pass.name + suffix + " - CommandBuffer allocation" );
		crgRegisterObject( m_context, m_pass.name, result );
		return result;
	}

	void RunnablePass::doCreateCommandBuffers()
	{
		for ( auto & cb : m_commandBuffers )
		{
			cb.commandBuffer = doCreateCommandBuffer( std::string{} );
		}
	}

	void RunnablePass::doCreateDisabledCommandBuffers()
	{
		if ( !m_context.device )
		{
			return;
		}

		for ( auto & cb : m_disabledCommandBuffers )
		{
			cb.commandBuffer = doCreateCommandBuffer( "Disabled" );
		}
	}

	void RunnablePass::doCreateSemaphore()
	{
		if ( !m_context.device )
		{
			return;
		}

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
		if ( !m_context.device )
		{
			return;
		}

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

		m_graph.updateCurrentLayout( m_pass
			, passIndex
			, view
			, transition.to );

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

		m_graph.updateCurrentAccessState( m_pass
			, passIndex
			, buffer
			, transition.to );
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
		m_graph.updateCurrentLayout( m_pass
			, passIndex, view, it->second.to );

		for ( auto & source : view.data->source )
		{
			doUpdateFinalLayout( passIndex
				, source
				, layout
				, accessMask
				, pipelineStage );
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
		m_graph.updateCurrentAccessState( m_pass
			, passIndex
			, buffer
			, it->second.to );
	}
}
