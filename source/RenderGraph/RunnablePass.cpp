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
			, getDefaultV< GetPassIndexCallback >()
			, getDefaultV< IsEnabledCallback >()
			, getDefaultV< IsComputePassCallback >() }
	{
	}

	RunnablePass::Callbacks::Callbacks( InitialiseCallback initialise
		, GetSemaphoreWaitFlagsCallback getSemaphoreWaitFlags
		, RecordCallback record
		, GetPassIndexCallback getPassIndex )
		: Callbacks{ std::move( initialise )
			, std::move( getSemaphoreWaitFlags )
			, std::move( record )
			, std::move( getPassIndex )
			, getDefaultV< IsEnabledCallback >()
			, getDefaultV< IsComputePassCallback >() }
	{
	}

	RunnablePass::Callbacks::Callbacks( InitialiseCallback initialise
		, GetSemaphoreWaitFlagsCallback getSemaphoreWaitFlags
		, RecordCallback record
		, GetPassIndexCallback getPassIndex
		, IsEnabledCallback isEnabled )
		: Callbacks{ std::move( initialise )
			, std::move( getSemaphoreWaitFlags )
			, std::move( record )
			, std::move( getPassIndex )
			, std::move( isEnabled )
			, getDefaultV< IsComputePassCallback >() }
	{
	}

	RunnablePass::Callbacks::Callbacks( InitialiseCallback initialise
		, GetSemaphoreWaitFlagsCallback getSemaphoreWaitFlags
		, RecordCallback record
		, GetPassIndexCallback getPassIndex
		, IsEnabledCallback isEnabled
		, IsComputePassCallback isComputePass )
		: initialise{ std::move( initialise ) }
		, getSemaphoreWaitFlags{ std::move( getSemaphoreWaitFlags ) }
		, record{ std::move( record ) }
		, getPassIndex{ std::move( getPassIndex ) }
		, isEnabled{ std::move( isEnabled ) }
		, isComputePass{ std::move( isComputePass ) }
	{
	}

	//*********************************************************************************************

	Fence::Fence( GraphContext & context
		, std::string const & name
		, VkFenceCreateInfo createInfo )
		: m_context{ &context }
	{
		if ( m_context->device )
		{
			auto res = m_context->vkCreateFence( m_context->device
				, &createInfo
				, m_context->allocator
				, &m_fence );
			checkVkResult( res, name + " - Fence creation" );
			crgRegisterObject( *m_context, name, m_fence );
		}
	}

	Fence::Fence( Fence && rhs )
		: m_context{ rhs.m_context }
		, m_fence{ rhs.m_fence }
		, m_fenceWaited{ rhs.m_fenceWaited }
	{
		rhs.m_context = nullptr;
		rhs.m_fence = nullptr;
	}

	Fence & Fence::operator=( Fence && rhs )
	{
		m_context = rhs.m_context;
		m_fence = rhs.m_fence;
		m_fenceWaited = rhs.m_fenceWaited;
		rhs.m_context = nullptr;
		rhs.m_fence = nullptr;
		return *this;
	}

	Fence::~Fence()
	{
		if ( m_fence && m_context && m_context->device )
		{
			crgUnregisterObject( *m_context, m_fence );
			m_context->vkDestroyFence( m_context->device
				, m_fence
				, m_context->allocator );
		}
	}

	void Fence::reset()
	{
		if ( m_fence )
		{
			if ( !m_fenceWaited )
			{
				wait( 0xFFFFFFFFFFFFFFFFull );
			}

			m_context->vkResetFences( m_context->device
				, 1u
				, &m_fence );
		}

		m_fenceWaited = false;
	}

	VkResult Fence::wait( uint64_t timeout )
	{
		VkResult res = VK_SUCCESS;

		if ( m_fence )
		{
			res = m_context->vkWaitForFences( m_context->device
				, 1u
				, &m_fence
				, VK_TRUE
				, timeout );
		}

		m_fenceWaited = true;
		return res;
	}

	//*********************************************************************************************

	RunnablePass::RunnablePass( FramePass const & pass
		, GraphContext & context
		, RunnableGraph & graph
		, Callbacks callbacks
		, ru::Config ruConfig )
		: m_pass{ pass }
		, m_context{ context }
		, m_graph{ graph }
		, m_callbacks{ std::move( callbacks ) }
		, m_ruConfig{ std::move( ruConfig ) }
		, m_fence{ context, m_pass.getGroupName(), { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT } }
		, m_timer{ context, m_pass.getGroupName(), 1u }
	{
		m_commandBuffers.resize( m_ruConfig.maxPassCount );
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
		m_callbacks.initialise();
		return uint32_t( m_commandBuffers.size() );
	}

	void RunnablePass::recordCurrent( RecordContext & context )
	{
		auto index = m_callbacks.getPassIndex();
		recordOne( m_commandBuffers[index]
			, index
			, context );
	}

	void RunnablePass::reRecordCurrent()
	{
		assert( m_ruConfig.resettable );
		auto index = m_callbacks.getPassIndex();
		auto it = m_passContexts.find( index );

		if ( it != m_passContexts.end() )
		{
			auto context = it->second;
			recordOne( m_commandBuffers[index]
				, index
				, context );
		}
	}

	void RunnablePass::recordAll( RecordContext & context )
	{
		for ( uint32_t index = 0u; index < m_commandBuffers.size(); ++index )
		{
			recordOne( m_commandBuffers[index]
				, index
				, context );
		}
	}

	void RunnablePass::recordOne( CommandBuffer & enabled
		, uint32_t index
		, RecordContext & context )
	{
		if ( !m_commandPool )
		{
			doCreateCommandPool();
		}

		if ( !enabled.commandBuffer )
		{
			enabled.commandBuffer = doCreateCommandBuffer( std::string{} );
		}

		auto ctxSave = context;
		m_fence.wait( 0xFFFFFFFFFFFFFFFFull );
		VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
			, nullptr
			, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
			, nullptr };
		m_context.vkBeginCommandBuffer( enabled.commandBuffer, &beginInfo );
		recordInto( enabled.commandBuffer
			, index
			, context );
		m_context.vkEndCommandBuffer( enabled.commandBuffer );
	}

	void RunnablePass::recordInto( VkCommandBuffer commandBuffer
		, uint32_t index
		, RecordContext & context )
	{
		if ( m_ruConfig.resettable )
		{
			auto it = m_passContexts.emplace( index, context ).first;
			it->second = context;
		}

		if ( isEnabled() )
		{
			auto block = m_timer.start();
			m_context.vkCmdBeginDebugBlock( commandBuffer
				, { "[" + std::to_string( m_pass.id ) + "] " + m_pass.getGroupName(), m_context.getNextRainbowColour() } );
			m_timer.beginPass( commandBuffer );

			for ( auto & attach : m_pass.images )
			{
				if ( attach.count <= 1u )
				{
					context.runImplicitTransition( commandBuffer
						, index
						, attach.view( index ) );
				}
				else
				{
					for ( uint32_t i = 0u; i < attach.count; ++i )
					{
						context.runImplicitTransition( commandBuffer
							, index
							, attach.view( i ) );
					}
				}

				if ( !attach.isNoTransition()
					&& ( attach.isSampledView() || attach.isStorageView() || attach.isTransferView() || attach.isTransitionView() ) )
				{
					if ( attach.count <= 1u )
					{
						auto view = attach.view( index );
						auto layout = attach.getImageLayout( m_context.separateDepthStencilLayouts );
						LayoutState needed = { layout
							, getAccessMask( layout )
							, getStageMask( layout ) };
						m_graph.memoryBarrier( context
							, commandBuffer
							, view
							, attach.image.initialLayout
							, needed );
					}
					else
					{
						for ( uint32_t i = 0u; i < attach.count; ++i )
						{
							auto view = attach.view( i );
							auto transition = getTransition( index
								, view );
							m_graph.memoryBarrier( context
								, commandBuffer
								, view
								, attach.image.initialLayout
								, transition.needed );
						}
					}
				}
			}

			for ( auto & attach : m_pass.buffers )
			{
				if ( !attach.isNoTransition()
					&& attach.isStorageBuffer() )
				{
					auto buffer = attach.buffer;
					auto transition = getTransition( index
						, buffer.buffer );
					m_graph.memoryBarrier( context
						, commandBuffer
						, buffer.buffer.buffer
						, buffer.range
						, transition.from.access
						, transition.from.pipelineStage
						, transition.needed );
				}
			}

			m_callbacks.record( context, commandBuffer, index );

			m_timer.endPass( commandBuffer );
			m_context.vkCmdEndDebugBlock( commandBuffer );
		}

		for ( auto & action : m_ruConfig.actions )
		{
			context.registerImplicitTransition( *this
				, action.first
				, action.second );
		}
	}

	SemaphoreWaitArray RunnablePass::run( SemaphoreWait toWait
		, VkQueue queue )
	{
		return run( ( toWait.semaphore
				? SemaphoreWaitArray{ 1u, toWait }
				: SemaphoreWaitArray{} )
			, queue );
	}

	SemaphoreWaitArray RunnablePass::run( SemaphoreWaitArray const & toWait
		, VkQueue queue )
	{
		if ( !m_callbacks.isEnabled() )
		{
			return toWait;
		}

		if ( m_context.device )
		{
			std::vector< VkSemaphore > semaphores;
			std::vector< VkPipelineStageFlags > dstStageMasks;
			convert( toWait, semaphores, dstStageMasks );
			auto index = m_callbacks.getPassIndex();
			auto & cb = m_commandBuffers[index];
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
			m_fence.reset();
			m_context.vkQueueSubmit( queue
				, 1u
				, &submitInfo
				, m_fence );
		}

		return { 1u
			, { m_semaphore
				, m_callbacks.getSemaphoreWaitFlags() } };
	}

	void RunnablePass::resetCommandBuffer()
{
		if ( !m_context.device )
		{
			return;
		}

		m_fence.wait( 0xFFFFFFFFFFFFFFFFull );

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
			, 0 };
		auto res = m_context.vkCreateCommandPool( m_context.device
			, &createInfo
			, m_context.allocator
			, &m_commandPool );
		checkVkResult( res, m_pass.getGroupName() + " - CommandPool creation" );
		crgRegisterObject( m_context, m_pass.getGroupName(), m_commandPool );
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
		checkVkResult( res, m_pass.getGroupName() + suffix + " - CommandBuffer allocation" );
		crgRegisterObject( m_context, m_pass.getGroupName() + suffix, result );
		return result;
	}

	void RunnablePass::doCreateCommandBuffers()
	{
		for ( auto & cb : m_commandBuffers )
		{
			cb.commandBuffer = doCreateCommandBuffer( std::string{} );
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
		checkVkResult( res, m_pass.getGroupName() + " - Semaphore creation" );
		crgRegisterObject( m_context, m_pass.getGroupName(), m_semaphore );
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
