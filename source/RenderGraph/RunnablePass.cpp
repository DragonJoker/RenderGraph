/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePass.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <cassert>

#pragma warning( push )
#pragma warning( disable: 5262 )
#include <thread>
#pragma warning( pop )

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
		, GetPipelineStateCallback getPipelineState )
		: Callbacks{ std::move( initialise )
			, std::move( getPipelineState )
			, getDefaultV< RecordCallback >()
			, getDefaultV< GetPassIndexCallback >()
			, getDefaultV< IsEnabledCallback >()
			, getDefaultV< IsComputePassCallback >() }
	{
	}

	RunnablePass::Callbacks::Callbacks( InitialiseCallback initialise
		, GetPipelineStateCallback getPipelineState
		, RecordCallback record )
		: Callbacks{ std::move( initialise )
			, std::move( getPipelineState )
			, std::move( record )
			, getDefaultV< GetPassIndexCallback >()
			, getDefaultV< IsEnabledCallback >()
			, getDefaultV< IsComputePassCallback >() }
	{
	}

	RunnablePass::Callbacks::Callbacks( InitialiseCallback initialise
		, GetPipelineStateCallback getPipelineState
		, RecordCallback record
		, GetPassIndexCallback getPassIndex )
		: Callbacks{ std::move( initialise )
			, std::move( getPipelineState )
			, std::move( record )
			, std::move( getPassIndex )
			, getDefaultV< IsEnabledCallback >()
			, getDefaultV< IsComputePassCallback >() }
	{
	}

	RunnablePass::Callbacks::Callbacks( InitialiseCallback initialise
		, GetPipelineStateCallback getPipelineState
		, RecordCallback record
		, GetPassIndexCallback getPassIndex
		, IsEnabledCallback isEnabled )
		: Callbacks{ std::move( initialise )
			, std::move( getPipelineState )
			, std::move( record )
			, std::move( getPassIndex )
			, std::move( isEnabled )
			, getDefaultV< IsComputePassCallback >() }
	{
	}

	RunnablePass::Callbacks::Callbacks( InitialiseCallback initialise
		, GetPipelineStateCallback getPipelineState
		, RecordCallback record
		, GetPassIndexCallback getPassIndex
		, IsEnabledCallback isEnabled
		, IsComputePassCallback isComputePass )
		: initialise{ std::move( initialise ) }
		, getPipelineState{ std::move( getPipelineState ) }
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
		rhs.m_context = {};
		rhs.m_fence = {};
	}

	Fence & Fence::operator=( Fence && rhs )
	{
		m_context = rhs.m_context;
		m_fence = rhs.m_fence;
		m_fenceWaited = rhs.m_fenceWaited;
		rhs.m_context = {};
		rhs.m_fence = {};
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

	RunnablePass::PassData::PassData( RunnableGraph & grh
		, GraphContext & ctx
		, std::string const & baseName )
		: graph{ grh }
		, context{ ctx }
		, fence{ context, baseName, { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT } }
	{
		VkSemaphoreCreateInfo createInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
			, nullptr
			, 0u };
		auto res = context.vkCreateSemaphore( context.device
			, &createInfo
			, context.allocator
			, &semaphore );
		checkVkResult( res, baseName + " - Semaphore creation" );
		crgRegisterObject( context, baseName, semaphore );
	}

	RunnablePass::PassData::~PassData()
	{
		if ( semaphore )
		{
			crgUnregisterObject( context, semaphore );
			context.vkDestroySemaphore( context.device
				, semaphore
				, context.allocator );
		}

		if ( commandBuffer.commandBuffer )
		{
			crgUnregisterObject( context, commandBuffer.commandBuffer );
			context.vkFreeCommandBuffers( context.device
				, graph.getCommandPool()
				, 1u
				, &commandBuffer.commandBuffer );
		}
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
		, m_pipelineState{ m_callbacks.getPipelineState() }
		, m_timer{ context, pass.getGroupName(), graph.getTimerQueryPool(), graph.getTimerQueryOffset() }
	{
		for ( uint32_t i = 0u; i < m_ruConfig.maxPassCount; ++i )
		{
			m_passes.emplace_back( m_graph, m_context, m_pass.getGroupName() );
		}
	}

	RunnablePass::~RunnablePass()
	{
		m_passes.clear();
	}

	void RunnablePass::initialise( uint32_t passIndex )
	{
		assert( m_passes.size() > passIndex );
		auto & pass = m_passes[passIndex];

		for ( auto & attach : m_pass.images )
		{
			if ( attach.count <= 1u )
			{
				auto view = attach.view( passIndex );
				auto inputLayout = m_graph.getCurrentLayout( m_pass, passIndex, view );

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

				doRegisterTransition( passIndex
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
					auto inputLayout = m_graph.getCurrentLayout( m_pass, passIndex, view );

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

					doRegisterTransition( passIndex
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
				doRegisterTransition( passIndex
					, buffer
					, { m_graph.getCurrentAccessState( m_pass, passIndex, buffer )
					, { attach.getAccessMask()
					, attach.getPipelineStageFlags( m_callbacks.isComputePass() ) }
				, m_graph.getOutputAccessState( m_pass, buffer, m_callbacks.isComputePass() ) } );
			}
		}

		m_callbacks.initialise( passIndex );
		pass.initialised = true;
	}

	void RunnablePass::recordCurrent( RecordContext & context )
	{
		auto index = m_callbacks.getPassIndex();
		assert( m_passes.size() > index );
		auto & pass = m_passes[index];
		recordOne( pass.commandBuffer
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
			auto & pass = m_passes[index];
			recordOne( pass.commandBuffer
				, index
				, context );
		}
	}

	void RunnablePass::recordAll( RecordContext & context )
	{
		uint32_t index{};

		for ( auto & pass : m_passes )
		{
			recordOne( pass.commandBuffer
				, index++
				, context );
		}
	}

	void RunnablePass::recordOne( CommandBuffer & enabled
		, uint32_t index
		, RecordContext & context )
	{
		if ( !enabled.commandBuffer )
		{
			enabled.commandBuffer = doCreateCommandBuffer( std::string{} );
		}

		assert( m_passes.size() > index );
		auto & pass = m_passes[index];
		pass.fence.wait( 0xFFFFFFFFFFFFFFFFull );
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
			assert( m_passes.size() > index );
			auto & pass = m_passes[index];

			if ( !pass.initialised )
			{
				initialise( index );
			}

			auto block( m_timer.start() );
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
						context.memoryBarrier( commandBuffer
							, view
							, attach.image.initialLayout
							, needed );
					}
					else
					{
						for ( uint32_t i = 0u; i < attach.count; ++i )
						{
							auto view = attach.view( i );
							auto & transition = getTransition( index
								, view );
							context.memoryBarrier( commandBuffer
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
					auto & transition = getTransition( index
						, buffer.buffer );
					context.memoryBarrier( commandBuffer
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

		auto index = m_callbacks.getPassIndex();
		assert( m_passes.size() > index );
		auto & pass = m_passes[index];

		if ( m_context.device )
		{
			std::vector< VkSemaphore > semaphores;
			std::vector< VkPipelineStageFlags > dstStageMasks;
			convert( toWait, semaphores, dstStageMasks );
			auto & cb = pass.commandBuffer;
			m_timer.notifyPassRender();
			VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO
				, nullptr
				, uint32_t( semaphores.size() )
				, semaphores.data()
				, dstStageMasks.data()
				, 1u
				, &cb.commandBuffer
				, 1u
				, &pass.semaphore };
			pass.fence.reset();
			m_context.vkQueueSubmit( queue
				, 1u
				, &submitInfo
				, pass.fence );
		}

		return { 1u
			, { pass.semaphore
				, m_callbacks.getPipelineState().pipelineStage } };
	}

	void RunnablePass::resetCommandBuffer( uint32_t passIndex )
	{
		if ( !m_context.device )
		{
			return;
		}

		assert( m_passes.size() > passIndex );
		auto & pass = m_passes[passIndex];
		pass.fence.wait( 0xFFFFFFFFFFFFFFFFull );

		if ( pass.commandBuffer.commandBuffer )
		{
			pass.commandBuffer.recorded = false;
			m_context.vkResetCommandBuffer( pass.commandBuffer.commandBuffer
				, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT );
		}
	}

	void RunnablePass::resetCommandBuffers()
	{
		for ( uint32_t i = 0u; i < uint32_t( m_passes.size() ); ++i )
		{
			resetCommandBuffer( i );
		}
	}

	RunnablePass::LayoutTransition const & RunnablePass::getTransition( uint32_t passIndex
		, ImageViewId const & view )const
	{
		assert( m_passes.size() > passIndex );
		auto & pass = m_passes[passIndex];
		auto it = pass.layoutTransitions.find( view );
		assert( it != pass.layoutTransitions.end() );
		return it->second;
	}

	RunnablePass::AccessTransition const & RunnablePass::getTransition( uint32_t passIndex
		, Buffer const & buffer )const
	{
		assert( m_passes.size() > passIndex );
		auto & pass = m_passes[passIndex];
		auto it = pass.accessTransitions.find( buffer.buffer );
		assert( it != pass.accessTransitions.end() );
		return it->second;
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
			, m_graph.getCommandPool()
			, VK_COMMAND_BUFFER_LEVEL_PRIMARY
			, 1u };
		auto res = m_context.vkAllocateCommandBuffers( m_context.device
			, &allocateInfo
			, &result );
		checkVkResult( res, m_pass.getGroupName() + suffix + " - CommandBuffer allocation" );
		crgRegisterObject( m_context, m_pass.getGroupName() + suffix, result );
		return result;
	}

	void RunnablePass::doRegisterTransition( uint32_t passIndex
		, ImageViewId view
		, LayoutTransition transition )
	{
		assert( m_passes.size() > passIndex );
		auto & pass = m_passes[passIndex];

		if ( transition.to.layout == VK_IMAGE_LAYOUT_UNDEFINED )
		{
			transition.to = transition.needed;
		}

		auto ires = pass.layoutTransitions.emplace( view, transition );

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
		assert( m_passes.size() > passIndex );
		auto & pass = m_passes[passIndex];

		if ( transition.to.access == 0u )
		{
			transition.to = transition.needed;
		}

		auto ires = pass.accessTransitions.emplace( buffer.buffer, transition );

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
		assert( m_passes.size() > passIndex );
		auto & pass = m_passes[passIndex];
		auto it = pass.layoutTransitions.find( view );
		assert( it != pass.layoutTransitions.end() );
		it->second.to.layout = layout;
		it->second.to.state.access = accessMask;
		it->second.to.state.pipelineStage = pipelineStage;
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
		assert( m_passes.size() > passIndex );
		auto & pass = m_passes[passIndex];
		auto it = pass.accessTransitions.find( buffer.buffer );
		assert( it != pass.accessTransitions.end() );
		it->second.to.access = accessMask;
		it->second.to.pipelineStage = pipelineStage;
		m_graph.updateCurrentAccessState( m_pass
			, passIndex
			, buffer
			, it->second.to );
	}
}
