/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnablePass.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/Log.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <cassert>

#pragma warning( push )
#pragma warning( disable: 5262 )
#include <thread>
#pragma warning( pop )

namespace crg
{
	namespace details
	{
		static constexpr VkDeviceSize getAlignedSize( VkDeviceSize size, VkDeviceSize align )
		{
			auto rem = size % align;
			return ( rem
				? size + ( align - rem )
				: size );
		}
	}

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

	Fence::Fence( Fence && rhs )noexcept
		: m_context{ rhs.m_context }
		, m_fence{ rhs.m_fence }
		, m_fenceWaited{ rhs.m_fenceWaited }
	{
		rhs.m_context = {};
		rhs.m_fence = {};
	}

	Fence & Fence::operator=( Fence && rhs )noexcept
	{
		m_context = rhs.m_context;
		m_fence = rhs.m_fence;
		m_fenceWaited = rhs.m_fenceWaited;
		rhs.m_context = {};
		rhs.m_fence = {};
		return *this;
	}

	Fence::~Fence()noexcept
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
		if ( context.device )
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
	}

	RunnablePass::PassData::~PassData()noexcept
	{
		if ( context.device )
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
		, m_timer{ context, pass.getGroupName(), TimerScope::ePass, graph.getTimerQueryPool(), graph.getTimerQueryOffset() }
	{
		for ( uint32_t i = 0u; i < m_ruConfig.maxPassCount; ++i )
		{
			m_passes.emplace_back( m_graph, m_context, m_pass.getGroupName() );
		}

		for ( auto & attach : m_pass.images )
		{
			for ( uint32_t i = 0u; i < attach.getViewCount(); ++i )
			{
				auto view = attach.view( i );

				if ( view.data->source.empty() )
				{
					m_imageLayouts.setLayoutState( view
						, { attach.getImageLayout( m_context.separateDepthStencilLayouts )
							, attach.getAccessMask()
							, attach.getPipelineStageFlags( m_callbacks.isComputePass() ) } );
				}
				else
				{
					for ( auto & source : view.data->source )
					{
						m_imageLayouts.setLayoutState( source
							, { attach.getImageLayout( m_context.separateDepthStencilLayouts )
								, attach.getAccessMask()
								, attach.getPipelineStageFlags( m_callbacks.isComputePass() ) } );
					}
				}
			}
		}

		for ( auto & attach : m_pass.buffers )
		{
			if ( attach.isStorageBuffer() )
			{
				auto & buffer = attach.buffer.buffer;

				for ( uint32_t i = 0u; i < uint32_t( buffer.getCount() ); ++i )
				{
					m_bufferAccesses.insert_or_assign( buffer.buffer( i )
						, AccessState{ attach.getAccessMask()
							, attach.getPipelineStageFlags( m_callbacks.isComputePass() ) } );
				}
			}
		}
	}

	RunnablePass::~RunnablePass()noexcept
	{
		m_bufferAccesses.clear();
		m_passContexts.clear();
		m_passes.clear();
	}

	void RunnablePass::initialise( uint32_t passIndex )
	{
		assert( m_passes.size() > passIndex );
		auto & pass = m_passes[passIndex];
		m_callbacks.initialise( passIndex );
		pass.initialised = true;
	}

	uint32_t RunnablePass::recordCurrent( RecordContext & context )
	{
		auto index = m_callbacks.getPassIndex();
		assert( m_passes.size() > index );
		auto & pass = m_passes[index];
		recordOne( pass.commandBuffer
			, index
			, context );
		return isEnabled() ? index : InvalidIndex;
	}

	uint32_t RunnablePass::recordCurrentInto( RecordContext & context
		, VkCommandBuffer commandBuffer )
	{
		auto index = m_callbacks.getPassIndex();
		recordInto( commandBuffer
			, index
			, context );
		return isEnabled() ? index : InvalidIndex;
	}

	uint32_t RunnablePass::reRecordCurrent()
	{
		assert( m_ruConfig.resettable );
		auto index = m_callbacks.getPassIndex();
		auto & pass = m_passes[index];

		if ( pass.commandBuffer.commandBuffer )
		{
			auto it = m_passContexts.find( index );

			if ( it != m_passContexts.end() )
			{
				auto context = it->second;
				recordOne( pass.commandBuffer
					, index
					, context );
			}
		}

		return isEnabled() ? index : InvalidIndex;
	}

	void RunnablePass::recordAll( RecordContext & context )
	{
		uint32_t index{};

		for ( auto & pass : m_passes )
		{
			recordOne( pass.commandBuffer
				, index
				, context );
			++index;
		}
	}

	void checkUndefinedInput( std::string const & stepName
		, Attachment const & attach
		, ImageViewId const & view
		, VkImageLayout currentLayout )
	{
		if ( !attach.isTransitionView() && attach.isInput() && currentLayout == VK_IMAGE_LAYOUT_UNDEFINED )
		{
			Logger::logWarning( stepName + " - [" + attach.pass->getFullName() + "]: Input view [" + view.data->name + "] is currently in undefined layout" );
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
		recordInto( enabled.commandBuffer, index, context );
		m_context.vkEndCommandBuffer( enabled.commandBuffer );
	}

	void RunnablePass::recordInto( VkCommandBuffer commandBuffer
		, uint32_t index
		, RecordContext & context )
	{
		if ( m_ruConfig.resettable )
		{
			m_passContexts.insert_or_assign( index, context );
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
				, { std::format( "[{}] {}", m_pass.id, m_pass.getGroupName() )
				, m_context.getNextRainbowColour() } );
			m_timer.beginPass( commandBuffer );

			for ( auto & attach : m_pass.images )
			{
				auto view = attach.view( index );
				context.runImplicitTransition( commandBuffer
					, index
					, view );

				if ( !attach.isNoTransition()
					&& ( attach.isSampledView() || attach.isStorageView() || attach.isTransferView() || attach.isTransitionView() ) )
				{
					auto needed = makeLayoutState( attach.getImageLayout( m_context.separateDepthStencilLayouts ) );
					auto currentLayout = ( !attach.isInput()
						? crg::makeLayoutState( VK_IMAGE_LAYOUT_UNDEFINED )
						: m_graph.getCurrentLayoutState( context, view ) );
					checkUndefinedInput( "Record", attach, view, currentLayout.layout );

					if ( attach.isClearableImage() )
					{
						context.memoryBarrier( commandBuffer
							, view
							, currentLayout.layout
							, LayoutState{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, { VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT } } );

						if ( isColourFormat( getFormat( view ) ) )
						{
							VkClearColorValue colour{};
							m_context.vkCmdClearColorImage( commandBuffer
								, m_graph.createImage( view.data->image )
								, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
								, &colour
								, 1u
								, &view.data->info.subresourceRange );
						}
						else
						{
							VkClearDepthStencilValue depthStencil{};
							m_context.vkCmdClearDepthStencilImage( commandBuffer
								, m_graph.createImage( view.data->image )
								, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
								, &depthStencil
								, 1u
								, &view.data->info.subresourceRange );
						}

						currentLayout.layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
						currentLayout.state.access = VK_ACCESS_TRANSFER_WRITE_BIT;
						currentLayout.state.pipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
					}

					context.memoryBarrier( commandBuffer
						, view
						, currentLayout.layout
						, needed );
				}
			}

			for ( auto & attach : m_pass.buffers )
			{
				if ( !attach.isNoTransition()
					&& attach.isStorageBuffer() )
				{
					auto buffer = attach.buffer;
					auto currentState = context.getAccessState( buffer.buffer.buffer( index )
						, buffer.range );

					if ( attach.isClearableBuffer() )
					{
						context.memoryBarrier( commandBuffer
							, buffer.buffer.buffer( index )
							, buffer.range
							, currentState.access
							, currentState.pipelineStage
							, { VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT } );
						m_context.vkCmdFillBuffer( commandBuffer
							, buffer.buffer.buffer( index )
							, buffer.range.offset == 0u ? 0u : details::getAlignedSize( buffer.range.offset, 4u )
							, buffer.range.size == VK_WHOLE_SIZE ? VK_WHOLE_SIZE : details::getAlignedSize( buffer.range.size, 4u )
							, 0u );
						currentState.access = VK_ACCESS_TRANSFER_WRITE_BIT;
						currentState.pipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
					}

					context.memoryBarrier( commandBuffer
						, buffer.buffer.buffer( index )
						, buffer.range
						, currentState.access
						, currentState.pipelineStage
						, { attach.getAccessMask(), attach.getPipelineStageFlags( m_callbacks.isComputePass() ) } );
				}
			}

			for ( auto & action : m_ruConfig.prePassActions )
			{
				action( context, commandBuffer, index );
			}

			m_callbacks.record( context, commandBuffer, index );

			for ( auto & action : m_ruConfig.postPassActions )
			{
				action( context, commandBuffer, index );
			}

			m_timer.endPass( commandBuffer );
			m_context.vkCmdEndDebugBlock( commandBuffer );
		}

		for ( auto & [view, action] : m_ruConfig.implicitActions )
		{
			context.registerImplicitTransition( *this, view, action );
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

		if ( pass.toReset )
		{
			crg::RunnablePass::resetCommandBuffer( index );
			crg::RunnablePass::reRecordCurrent();
		}

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

		if ( pass.commandBuffer.commandBuffer )
		{
			pass.fence.wait( 0xFFFFFFFFFFFFFFFFull );
			pass.commandBuffer.recorded = false;
			m_context.vkResetCommandBuffer( pass.commandBuffer.commandBuffer
				, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT );
		}
	}

	void RunnablePass::setToReset( uint32_t passIndex )
	{
		if ( m_ruConfig.resettable )
		{
			assert( m_passes.size() > passIndex );
			auto & pass = m_passes[passIndex];
			pass.toReset = true;
		}
	}

	void RunnablePass::notifyPassRender()
	{
		if ( isEnabled() )
		{
			m_timer.notifyPassRender( getIndex() );
		}
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
}
