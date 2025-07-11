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
		static constexpr DeviceSize getAlignedSize( DeviceSize size, DeviceSize align )
		{
			auto rem = size % align;
			return ( rem
				? size + ( align - rem )
				: size );
		}

		static void registerImage( Attachment const & attach
			, bool isComputePass, bool separateDepthStencilLayouts
			, LayerLayoutStatesHandler & imageLayouts )
		{
			for ( uint32_t i = 0u; i < attach.getViewCount(); ++i )
			{
				auto view = attach.view( i );

				if ( view.data->source.empty() )
				{
					imageLayouts.setLayoutState( view
						, { attach.getImageLayout( separateDepthStencilLayouts )
							, attach.getAccessMask()
							, attach.getPipelineStageFlags( isComputePass ) } );
				}
				else
				{
					for ( auto & source : view.data->source )
					{
						imageLayouts.setLayoutState( source
							, { attach.getImageLayout( separateDepthStencilLayouts )
								, attach.getAccessMask()
								, attach.getPipelineStageFlags( isComputePass ) } );
					}
				}
			}
		}

		static void registerBuffer( Attachment const & attach
			, bool isComputePass
			, AccessStateMap & bufferAccesses )
		{
			for ( uint32_t i = 0u; i < attach.getBufferCount(); ++i )
			{
				bufferAccesses.insert_or_assign( attach.buffer( i )
					, AccessState{ attach.getAccessMask()
						, attach.getPipelineStageFlags( isComputePass ) } );
			}
		}

		static void prepareImage( VkCommandBuffer commandBuffer
			, uint32_t index
			, Attachment const & attach
			, bool separateDepthStencilLayouts
			, RunnableGraph & graph
			, RecordContext & recordContext )
		{
			auto view = attach.view( index );
			recordContext.runImplicitTransition( commandBuffer
				, index
				, view );

			if ( !attach.isNoTransition()
				&& ( attach.isSampledView() || attach.isStorageView() || attach.isTransferView() || attach.isTransitionView() ) )
			{
				auto needed = makeLayoutState( attach.getImageLayout( separateDepthStencilLayouts ) );
				auto currentLayout = ( !attach.isInput()
					? crg::makeLayoutState( ImageLayout::eUndefined )
					: graph.getCurrentLayoutState( recordContext, view ) );
				checkUndefinedInput( "Record", attach, view, currentLayout.layout );

				if ( attach.isClearableImage() )
				{
					recordContext.memoryBarrier( commandBuffer
						, view
						, currentLayout.layout
						, makeLayoutState( ImageLayout::eTransferDst ) );
					auto subresourceRange = convert( getSubresourceRange( view ) );

					if ( isColourFormat( getFormat( view ) ) )
					{
						VkClearColorValue colour{};
						recordContext->vkCmdClearColorImage( commandBuffer
							, graph.createImage( view.data->image )
							, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
							, &colour
							, 1u, &subresourceRange );
					}
					else
					{
						VkClearDepthStencilValue depthStencil{};
						recordContext->vkCmdClearDepthStencilImage( commandBuffer
							, graph.createImage( view.data->image )
							, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
							, &depthStencil
							, 1u, &subresourceRange );
					}

					currentLayout.layout = ImageLayout::eTransferDst;
					currentLayout.state.access = AccessFlags::eTransferWrite;
					currentLayout.state.pipelineStage = PipelineStageFlags::eTransfer;
				}

				recordContext.memoryBarrier( commandBuffer
					, view
					, currentLayout.layout
					, needed );
			}
		}

		static void prepareBuffer( VkCommandBuffer commandBuffer
			, uint32_t index
			, Attachment const & attach
			, bool isComputePass
			, RecordContext & recordContext )
		{
			if ( !attach.isNoTransition()
				&& ( attach.isStorageBuffer() || attach.isTransferBuffer() || attach.isTransitionBuffer() ) )
			{
				auto & range = attach.getBufferRange();
				auto buffer = attach.buffer( index );
				auto currentState = recordContext.getAccessState( buffer, range );

				if ( attach.isClearableBuffer() )
				{
					recordContext.memoryBarrier( commandBuffer
						, buffer
						, range
						, currentState
						, { AccessFlags::eTransferWrite, PipelineStageFlags::eTransfer } );
					recordContext->vkCmdFillBuffer( commandBuffer
						, buffer
						, range.offset == 0u ? 0u : details::getAlignedSize( range.offset, 4u )
						, range.size == VK_WHOLE_SIZE ? VK_WHOLE_SIZE : details::getAlignedSize( range.size, 4u )
						, 0u );
					currentState.access = AccessFlags::eTransferWrite;
					currentState.pipelineStage = PipelineStageFlags::eTransfer;
				}

				recordContext.memoryBarrier( commandBuffer
					, buffer
					, range
					, currentState
					, { attach.getAccessMask(), attach.getPipelineStageFlags( isComputePass ) } );
			}
		}
	}

	//*********************************************************************************************

	void checkUndefinedInput( std::string const & stepName
		, Attachment const & attach
		, ImageViewId const & view
		, ImageLayout currentLayout )
	{
		if ( !attach.isTransitionView() && attach.isInput() && currentLayout == ImageLayout::eUndefined )
			Logger::logWarning( stepName + " - [" + attach.pass->getFullName() + "]: Input view [" + view.data->name + "] is currently in undefined layout" );
	}

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
				dstStageMasks.push_back( getPipelineStageFlags( wait.dstStageMask ) );
			}
		}
	}

	std::vector< VkClearValue > convert( std::vector< ClearValue > const & v )
	{
		std::vector< VkClearValue > result;
		result.reserve( v.size() );
		for ( auto & value : v )
			result.push_back( convert( value ) );
		return result;
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
		if ( m_context->vkCreateFence )
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

	Fence::~Fence()noexcept
	{
		if ( m_fence && m_context && m_context->vkDestroyFence )
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
				wait( 0xFFFFFFFFFFFFFFFFULL );
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
		if ( context.vkCreateSemaphore )
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
		if ( context.vkDestroySemaphore && semaphore )
		{
			crgUnregisterObject( context, semaphore );
			context.vkDestroySemaphore( context.device
				, semaphore
				, context.allocator );
		}

		if ( context.vkFreeCommandBuffers && commandBuffer.commandBuffer )
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
		, m_timer{ context, pass.getGroupName(), TimerScope::ePass, graph.getTimerQueryPool(), graph.getTimerQueryOffset() }
	{
		for ( uint32_t i = 0u; i < m_ruConfig.maxPassCount; ++i )
		{
			m_passes.emplace_back( m_graph, m_context, m_pass.getGroupName() );
			m_passContexts.emplace_back( graph.getResources() );
		}

		bool isComputePass = m_callbacks.isComputePass();
		for ( auto & attach : m_pass.images )
			details::registerImage( attach, isComputePass, m_context.separateDepthStencilLayouts, m_imageLayouts );
		for ( auto & attach : m_pass.buffers )
			details::registerBuffer( attach, isComputePass, m_bufferAccesses );
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

		if ( index < m_passContexts.size() )
		{
			auto context = m_passContexts[index];
			recordOne( m_passes[index].commandBuffer
				, index
				, context );
		}

		return isEnabled() ? index : InvalidIndex;
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
		pass.fence.wait( 0xFFFFFFFFFFFFFFFFULL );
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
			m_passContexts[index] = context;
		}

		if ( isEnabled() )
		{
			assert( m_passes.size() > index );

			if ( auto const & pass = m_passes[index]; !pass.initialised )
			{
				initialise( index );
			}

			auto block( m_timer.start() );
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
			m_context.vkCmdBeginDebugBlock( commandBuffer
				, { "[" + std::to_string( m_pass.id ) + "] " + m_pass.getGroupName()
				, m_context.getNextRainbowColour() } );
#pragma GCC diagnostic pop
			m_timer.beginPass( commandBuffer );

			for ( auto & attach : m_pass.images )
				details::prepareImage( commandBuffer, index, attach, m_context.separateDepthStencilLayouts, m_graph, context );

			for ( auto & attach : m_pass.buffers )
				details::prepareBuffer( commandBuffer, index, attach, m_callbacks.isComputePass(), context );

			for ( auto const & action : m_ruConfig.prePassActions )
			{
				action( context, commandBuffer, index );
			}

			m_callbacks.record( context, commandBuffer, index );

			for ( auto const & action : m_ruConfig.postPassActions )
			{
				action( context, commandBuffer, index );
			}

			m_timer.endPass( commandBuffer );
			m_context.vkCmdEndDebugBlock( commandBuffer );
		}

		for ( auto const & [view, action] : m_ruConfig.implicitActions )
		{
			context.registerImplicitTransition( *this, view, action );
		}
	}

	void RunnablePass::resetCommandBuffer( uint32_t passIndex )
	{
		if ( m_context.device )
		{
			assert( m_passes.size() > passIndex );
			auto & pass = m_passes[passIndex];

			if ( pass.commandBuffer.commandBuffer )
			{
				pass.fence.wait( 0xFFFFFFFFFFFFFFFFULL );
				pass.commandBuffer.recorded = false;
				m_context.vkResetCommandBuffer( pass.commandBuffer.commandBuffer
					, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT );
			}
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

		if ( m_context.vkAllocateCommandBuffers )
		{
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
		}

		return result;
	}
}
