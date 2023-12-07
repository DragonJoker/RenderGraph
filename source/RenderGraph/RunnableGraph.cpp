/*
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnableGraph.hpp"
#include "RenderGraph/GraphVisitor.hpp"
#include "RenderGraph/Log.hpp"
#include "RenderGraph/ResourceHandler.hpp"

#include <array>
#include <cassert>
#include <string>
#include <type_traits>
#include <unordered_set>

#pragma warning( push )
#pragma warning( disable: 5262 )
#include <fstream>
#pragma warning( pop )

#pragma GCC diagnostic ignored "-Wnull-dereference"

namespace crg
{
	//************************************************************************************************

	namespace rungrf
	{
		static VkCommandPool createCommandPool( GraphContext & context
			, std::string const & name )
		{
			VkCommandPool result{};

			if ( context.device )
			{
				VkCommandPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO
					, nullptr
					, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
					, 0 };
				auto res = context.vkCreateCommandPool( context.device
					, &createInfo
					, context.allocator
					, &result );
				checkVkResult( res, name + " - CommandPool creation" );
				crgRegisterObject( context, name, result );
			}

			return result;
		}

		static LayerLayoutStates mergeRanges( LayerLayoutStatesMap nextLayouts
			, LayerLayoutStatesMap::value_type const & currentLayout )
		{
			LayerLayoutStates result;
			auto nextIt = nextLayouts.find( currentLayout.first );

			if ( nextIt != nextLayouts.end() )
			{
				auto & nxtLayout = nextIt->second;

				for ( auto curLayerIt : currentLayout.second )
				{
					auto nxtLayerIt = nxtLayout.find( curLayerIt.first );

					if ( nxtLayerIt != nxtLayout.end() )
					{
						auto resLayerIt = result.emplace( curLayerIt.first, MipLayoutStates{} ).first;

						for ( auto curLevelIt : curLayerIt.second )
						{
							auto nxtLevelIt = nxtLayerIt->second.find( curLevelIt.first );

							if ( nxtLevelIt != nxtLayerIt->second.end() )
							{
								resLayerIt->second.emplace( *nxtLevelIt );
							}
						}
					}
				}
			}

			return result;
		}

		static LayerLayoutStatesMap gatherNextImageLayouts( LayerLayoutStatesMap currentLayouts
			, std::vector< RunnablePassPtr >::iterator nextPassIt
			, std::vector< RunnablePassPtr >::iterator endIt )
		{
			LayerLayoutStatesMap result;

			while ( !currentLayouts.empty()
				&& endIt != nextPassIt )
			{
				auto & nextPass = **nextPassIt;

				if ( nextPass.isEnabled() )
				{
					auto & nextLayouts = nextPass.getImageLayouts();
					LayerLayoutStates layoutStates;
					auto currIt = std::find_if( currentLayouts.begin()
						, currentLayouts.end()
						, [&nextLayouts, &layoutStates]( LayerLayoutStatesMap::value_type const & lookup )
						{
							layoutStates = mergeRanges( nextLayouts, lookup );
							return !layoutStates.empty();
						} );

					if ( currIt != currentLayouts.end() )
					{
						result.emplace( currIt->first, layoutStates );
						currentLayouts.erase( currIt );
					}
				}
				
				++nextPassIt;
			}

			return result;
		}
	}

	//************************************************************************************************

	VkQueryPool createQueryPool( GraphContext & context
		, std::string const & name
		, uint32_t passesCount )
	{
		VkQueryPool result{};

		if ( context.device )
		{
			VkQueryPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO
				, nullptr
				, 0u
				, VK_QUERY_TYPE_TIMESTAMP
				, passesCount
				, 0u };
			auto res = context.vkCreateQueryPool( context.device
				, &createInfo
				, context.allocator
				, &result );
			checkVkResult( res, name + " - VkQueryPool creation" );
			crgRegisterObject( context, name, result );
		}

		return result;
	}

	//************************************************************************************************

	RunnableGraph::RunnableGraph( FrameGraph & graph
		, FramePassArray passes
		, FramePassDependencies inputTransitions
		, FramePassDependencies outputTransitions
		, AttachmentTransitions transitions
		, GraphNodePtrArray nodes
		, RootNode rootNode
		, GraphContext & context )
		: m_graph{ graph }
		, m_context{ context }
		, m_resources{ m_graph.getHandler(), m_context }
		, m_inputTransitions{ std::move( inputTransitions ) }
		, m_outputTransitions{ std::move( outputTransitions ) }
		, m_transitions{ std::move( transitions ) }
		, m_nodes{ std::move( nodes ) }
		, m_rootNode{ std::move( rootNode ) }
		, m_timerQueries{ m_context
			, createQueryPool( m_context, m_graph.getName() + "TimerQueries", uint32_t( ( m_nodes.size() + 1u ) * 2u * 2u ) )
			, []( GraphContext & ctx, VkQueryPool & object )
			{
				crgUnregisterObject( ctx, object );
				ctx.vkDestroyQueryPool( ctx.device, object, ctx.allocator );
				object = {};
			} }
		, m_commandPool{ m_context
			, rungrf::createCommandPool( m_context, m_graph.getName() )
			, []( GraphContext & ctx, VkCommandPool & object )
			{
				crgUnregisterObject( ctx, object );
				ctx.vkDestroyCommandPool( ctx.device, object, ctx.allocator );
				object = {};
			} }
		, m_fence{ m_context
			, graph.getName() + "/Graph"
			, { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT } }
		, m_timer{ context, graph.getName() + "/Graph", getTimerQueryPool(), getTimerQueryOffset() }
	{
		if ( m_context.device )
		{
			auto name = m_graph.getName() + "/Graph";
			VkSemaphoreCreateInfo createInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
				, nullptr
				, 0u };
			auto res = context.vkCreateSemaphore( m_context.device
				, &createInfo
				, m_context.allocator
				, &m_semaphore );
			checkVkResult( res, name + " - Semaphore creation" );
			crgRegisterObject( m_context, name, m_semaphore );

			VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
				, nullptr
				, getCommandPool()
				, VK_COMMAND_BUFFER_LEVEL_PRIMARY
				, 1u };
			res = m_context.vkAllocateCommandBuffers( m_context.device
				, &allocateInfo
				, &m_commandBuffer );
			checkVkResult( res, name + " - CommandBuffer allocation" );
			crgRegisterObject( m_context, name, m_commandBuffer );

			VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
				, nullptr
				, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
				, nullptr };
			m_context.vkBeginCommandBuffer( m_commandBuffer, &beginInfo );
			m_context.vkEndCommandBuffer( m_commandBuffer );
		}

		Logger::logDebug( m_graph.getName() + " - Initialising resources" );

		for ( auto & img : m_graph.m_images )
		{
			m_resources.createImage( img );
		}

		for ( auto & view : m_graph.m_imageViews )
		{
			m_resources.createImageView( view );
		}

		Logger::logDebug( m_graph.getName() + " - Creating runnable passes" );

		for ( auto & node : m_nodes )
		{
			if ( node->getKind() == GraphNode::Kind::FramePass )
			{
				auto & renderPassNode = nodeCast< FramePassNode >( *node );
				m_passes.push_back( renderPassNode.getFramePass().createRunnable( m_context
					, *this ) );
			}
		}

		Logger::logDebug( m_graph.getName() + " - Initialising passes" );

		for ( auto & pass : m_passes )
		{
			if ( pass->isEnabled() )
			{
				pass->initialise( pass->getIndex() );
			}
		}
	}

	RunnableGraph::~RunnableGraph()noexcept
	{
		if ( m_context.device )
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
					, getCommandPool()
					, 1u
					, &m_commandBuffer );
			}
		}
	}

	void RunnableGraph::record()
	{
		auto block( m_timer.start() );
		m_states.clear();
		RecordContext recordContext{ m_resources };

		for ( auto & dependency : m_graph.getDependencies() )
		{
			recordContext.addStates( dependency->getFinalStates() );
			m_states.emplace( dependency
				, dependency->getFinalStates().getIndexState() );
		}

		auto itGraph = m_states.emplace( &m_graph, RecordContext::PassIndexArray{} ).first;
		itGraph->second.resize( m_passes.size() );

		if ( !m_passes.empty() )
		{
			auto it = itGraph->second.begin();
			auto currPass = m_passes.begin();
			recordContext.setNextPipelineState( ( *currPass )->getPipelineState()
				, ( *currPass )->getImageLayouts() );
			auto nextPass = std::next( currPass );
			m_fence.wait( 0xFFFFFFFFFFFFFFFFull );
			m_context.vkResetCommandBuffer( m_commandBuffer, 0u );
			VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
				, nullptr
				, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
				, nullptr };
			m_context.vkBeginCommandBuffer( m_commandBuffer, &beginInfo );
			m_timer.beginPass( m_commandBuffer );

			while ( currPass != m_passes.end() )
			{
				auto & pass = *currPass;
				++currPass;

				if ( nextPass != m_passes.end() )
				{
					if ( ( *nextPass )->isEnabled() )
					{
						recordContext.setNextPipelineState( ( *nextPass )->getPipelineState()
							, rungrf::gatherNextImageLayouts( pass->getImageLayouts()
								, nextPass
								, m_passes.end() ) );
					}

					++nextPass;
				}
				else
				{
					recordContext.setNextPipelineState( pass->getPipelineState()
						, {} );
				}

				*it = pass->recordCurrentInto( recordContext, m_commandBuffer );
				++it;
			}

			m_timer.endPass( m_commandBuffer );
			m_context.vkEndCommandBuffer( m_commandBuffer );
		}

		m_graph.registerFinalState( recordContext );
	}

	SemaphoreWaitArray RunnableGraph::run( VkQueue queue )
	{
		return run( SemaphoreWaitArray{}
			, queue );
	}

	SemaphoreWaitArray RunnableGraph::run( SemaphoreWait toWait
		, VkQueue queue )
	{
		return run( ( toWait.semaphore
				? SemaphoreWaitArray{ 1u, toWait }
				: SemaphoreWaitArray{} )
			, queue );
	}

	SemaphoreWaitArray RunnableGraph::run( SemaphoreWaitArray const & toWait
		, VkQueue queue )
	{
		record();
		std::vector< VkSemaphore > semaphores;
		std::vector< VkPipelineStageFlags > dstStageMasks;
		convert( toWait, semaphores, dstStageMasks );
		m_timer.notifyPassRender();
		VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO
			, nullptr
			, uint32_t( semaphores.size() )
			, semaphores.data()
			, dstStageMasks.data()
			, 1u
			, &m_commandBuffer
			, 1u
			, &m_semaphore };
		m_fence.reset();
		m_context.delQueue.clear( m_context );
		m_context.vkQueueSubmit( queue
			, 1u
			, &submitInfo
			, m_fence );
		return { SemaphoreWait{ m_semaphore
			, m_graph.getFinalStates().getCurrPipelineState().pipelineStage } };
	}

	ImageViewId RunnableGraph::createView( ImageViewData const & view )
	{
		auto result = m_graph.createView( view );
		createImageView( result );
		return result;
	}

	VkImage RunnableGraph::createImage( ImageId const & image )
	{
		return m_resources.createImage( image );
	}

	VkImageView RunnableGraph::createImageView( ImageViewId const & view )
	{
		return m_resources.createImageView( view );
	}

	VkSampler RunnableGraph::createSampler( SamplerDesc const & samplerDesc )
	{
		return m_resources.createSampler( samplerDesc );
	}

	VertexBuffer const & RunnableGraph::createQuadTriVertexBuffer( bool texCoords
		, Texcoord const & config )
	{
		return m_resources.createQuadTriVertexBuffer( texCoords
			, config );
	}

	LayoutState RunnableGraph::getCurrentLayoutState( RecordContext & context
		, ImageId image
		, VkImageViewType viewType
		, VkImageSubresourceRange range )const
	{
		auto result = context.getLayoutState( image, viewType, range );

		if ( result.layout == VK_IMAGE_LAYOUT_UNDEFINED )
		{
			// Lookup in graph's external inputs.
			result = m_graph.getInputLayoutState( image, viewType, range );

			if ( result.layout != VK_IMAGE_LAYOUT_UNDEFINED )
			{
				context.setLayoutState( image, viewType, range, result );
			}
		}

		if ( result.layout == VK_IMAGE_LAYOUT_UNDEFINED )
		{
			// Lookup in graph's previous final state.
			result = m_graph.getFinalLayoutState( image, viewType, range );

			if ( result.layout != VK_IMAGE_LAYOUT_UNDEFINED )
			{
				context.setLayoutState( image, viewType, range, result );
			}
		}

		return result;
	}

	LayoutState RunnableGraph::getCurrentLayoutState( RecordContext & context
		, ImageViewId view )const
	{
		return getCurrentLayoutState( context
			, view.data->image
			, view.data->info.viewType
			, view.data->info.subresourceRange );
	}

	LayoutState RunnableGraph::getNextLayoutState( RecordContext & context
		, crg::RunnablePass const & runnable
		, ImageViewId view )const
	{
		auto result = context.getNextLayoutState( view );

		if ( result.layout == VK_IMAGE_LAYOUT_UNDEFINED )
		{
			// Next layout undefined means that there is no pass after this one in the graph.
			result = m_graph.getOutputLayoutState( view );
		}

		if ( result.layout == VK_IMAGE_LAYOUT_UNDEFINED )
		{
			// Prevent from outputing a VK_IMAGE_LAYOUT_UNDEFINED anyway.
			result = runnable.getLayoutState( view );
		}

		return result;
	}

	LayoutState RunnableGraph::getOutputLayoutState( ImageViewId view )const
	{
		return m_graph.getOutputLayoutState( view );
	}
}
