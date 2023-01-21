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
		, m_layouts{ std::make_unique< RunnableLayoutsCache >( graph, m_context, passes ) }
		, m_inputTransitions{ std::move( inputTransitions ) }
		, m_outputTransitions{ std::move( outputTransitions ) }
		, m_transitions{ std::move( transitions ) }
		, m_nodes{ std::move( nodes ) }
		, m_rootNode{ std::move( rootNode ) }
		, m_timerQueries{ context
			, createQueryPool( context, graph.getName() + "TimerQueries", uint32_t( m_nodes.size() * 2u ) )
			, []( GraphContext & ctx, VkQueryPool & object )
			{
				crgUnregisterObject( ctx, object );
				ctx.vkDestroyQueryPool( ctx.device, object, ctx.allocator );
				object = {};
			} }
		, m_commandPool{ context
			, rungrf::createCommandPool( context, graph.getName() )
			, []( GraphContext & ctx, VkCommandPool & object )
			{
				crgUnregisterObject( ctx, object );
				ctx.vkDestroyCommandPool( ctx.device, object, ctx.allocator );
				object = {};
			} }
	{
		Logger::logDebug( graph.getName() + " - Creating runnable passes" );

		for ( auto & node : m_nodes )
		{
			if ( node->getKind() == GraphNode::Kind::FramePass )
			{
				auto & renderPassNode = nodeCast< FramePassNode >( *node );
				m_passes.push_back( renderPassNode.getFramePass().createRunnable( m_context
					, *this ) );
				auto & pass = m_passes.back();
				auto passCount = pass->getMaxPassCount();
				m_layouts->registerPass( pass->getPass(), passCount );
				m_maxPassCount *= passCount;
			}
		}

		m_layouts->initialise( m_nodes, m_passes, m_maxPassCount );
		Logger::logDebug( graph.getName() + " - Initialising passes" );

		for ( auto & pass : m_passes )
		{
			pass->initialise();
		}
	}

	void RunnableGraph::record()
	{
		m_states.clear();
		RecordContext recordContext{ m_layouts->getResources() };

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
			recordContext.setNextPipelineState( ( *currPass )->getPipelineState() );
			auto nextPass = std::next( currPass );

			while ( currPass != m_passes.end() )
			{
				auto & pass = *currPass;
				++currPass;

				if ( nextPass != m_passes.end() )
				{
					recordContext.setNextPipelineState( ( *nextPass )->getPipelineState() );
					++nextPass;
				}
				else
				{
					recordContext.setNextPipelineState( pass->getPipelineState() );
				}

				pass->recordCurrent( recordContext );
				*it = pass->getIndex();
				++it;
			}
		}

		m_graph.registerFinalState( recordContext );
	}

	VkImage RunnableGraph::createImage( ImageId const & image )
	{
		return m_layouts->createImage( image );
	}

	VkImageView RunnableGraph::createImageView( ImageViewId const & view )
	{
		return m_layouts->createImageView( view );
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
		RecordContext::GraphIndexMap states;
		auto it = states.emplace( &m_graph, RecordContext::PassIndexArray{} ).first;
		it->second.reserve( m_passes.size() );

		for ( auto & pass : m_passes )
		{
			it->second.push_back( pass->getIndex() );
		}

		for ( auto & dependency : m_graph.getDependencies() )
		{
			states.emplace( dependency
				, dependency->getFinalStates().getIndexState() );
		}

		if ( m_states != states )
		{
			record();
		}

		auto result = toWait;

		for ( auto & pass : m_passes )
		{
			result = pass->run( result, queue );
		}

		return result;
	}

	ImageViewId RunnableGraph::createView( ImageViewData const & view )
	{
		auto result = m_graph.createView( view );
		createImageView( result );
		return result;
	}

	VertexBuffer const & RunnableGraph::createQuadTriVertexBuffer( bool texCoords
		, Texcoord const & config )
	{
		return m_layouts->getResources().createQuadTriVertexBuffer( texCoords
			, config );
	}

	VkSampler RunnableGraph::createSampler( SamplerDesc const & samplerDesc )
	{
		return m_layouts->getResources().createSampler( samplerDesc );
	}

	LayoutState RunnableGraph::getCurrentLayout( FramePass const & pass
		, uint32_t passIndex
		, ImageViewId view )const
	{
		auto & viewsLayouts = m_layouts->getViewsLayout( pass, passIndex );
		auto imageIt = viewsLayouts.find( view.data->image.id );

		if ( imageIt != viewsLayouts.end() )
		{
			return getSubresourceRangeLayout( imageIt->second
				, getVirtualRange( view.data->image
					, view.data->info.viewType
					, view.data->info.subresourceRange ) );
		}

		return { VK_IMAGE_LAYOUT_UNDEFINED, 0u, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
	}

	LayoutState RunnableGraph::updateCurrentLayout( FramePass const & pass
		, uint32_t passIndex
		, ImageViewId view
		, LayoutState newLayout )
	{
		auto & viewsLayouts = m_layouts->getViewsLayout( pass, passIndex );
		auto & ranges = viewsLayouts.emplace( view.data->image.id, LayerLayoutStates{} ).first->second;
		addSubresourceRangeLayout( ranges
			, getVirtualRange( view.data->image
				, view.data->info.viewType
				, view.data->info.subresourceRange )
			, newLayout );
		return newLayout;
	}

	LayoutState RunnableGraph::getOutputLayout( FramePass const & pass
		, ImageViewId view
		, bool isCompute )const
	{
		LayoutState result{ VK_IMAGE_LAYOUT_UNDEFINED, 0u, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
		auto passIt = std::find_if( m_outputTransitions.begin()
			, m_outputTransitions.end()
			, [&pass]( FramePassTransitions const & lookup )
			{
				return lookup.pass == &pass;
			} );
		assert( passIt != m_outputTransitions.end() );
		auto it = std::find_if( passIt->transitions.viewTransitions.begin()
			, passIt->transitions.viewTransitions.end()
			, [&view]( ViewTransition const & lookup )
			{
				return match( *view.data, *lookup.data.data )
					|| view.data->source.end() != std::find_if( view.data->source.begin()
						, view.data->source.end()
						, [&lookup]( ImageViewId const & lookupView )
						{
							return match( *lookup.data.data, *lookupView.data );
						} )
					|| lookup.data.data->source.end() != std::find_if( lookup.data.data->source.begin()
						, lookup.data.data->source.end()
						, [&view]( ImageViewId const & lookupView )
						{
							return match( *view.data, *lookupView.data );
						} );
			} );

		if ( it != passIt->transitions.viewTransitions.end() )
		{
			result.layout = it->inputAttach.getImageLayout( m_context.separateDepthStencilLayouts );
			result.state.access = it->inputAttach.getAccessMask();
			result.state.pipelineStage = it->inputAttach.getPipelineStageFlags( isCompute );
		}
		else
		{
			passIt = std::find_if( m_inputTransitions.begin()
				, m_inputTransitions.end()
				, [&pass]( FramePassTransitions const & lookup )
				{
					return lookup.pass == &pass;
				} );
			assert( passIt != m_inputTransitions.end() );
			it = std::find_if( passIt->transitions.viewTransitions.begin()
				, passIt->transitions.viewTransitions.end()
				, [&view]( ViewTransition const & lookup )
				{
					return match( *view.data, *lookup.data.data )
						|| view.data->source.end() != std::find_if( view.data->source.begin()
							, view.data->source.end()
							, [&lookup]( ImageViewId const & lookupView )
							{
								return match( *lookup.data.data, *lookupView.data );
							} )
						|| lookup.data.data->source.end() != std::find_if( lookup.data.data->source.begin()
							, lookup.data.data->source.end()
							, [&view]( ImageViewId const & lookupView )
							{
								return match( *view.data, *lookupView.data );
							} );
				} );

			if ( it == passIt->transitions.viewTransitions.end() )
			{
				result.layout = VK_IMAGE_LAYOUT_UNDEFINED;
				result.state.access = 0u;
				result.state.pipelineStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			}
			else if ( it->inputAttach.getFlags() != 0u )
			{
				result.layout = it->inputAttach.getImageLayout( m_context.separateDepthStencilLayouts );
				result.state.access = it->inputAttach.getAccessMask();
				result.state.pipelineStage = it->inputAttach.getPipelineStageFlags( isCompute );
			}
			else if ( it->outputAttach.isColourClearingAttach()
				|| it->outputAttach.isDepthClearingAttach() )
			{
				result.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				result.state.access = VK_ACCESS_SHADER_READ_BIT;
				result.state.pipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else
			{
				result.layout = it->outputAttach.getImageLayout( m_context.separateDepthStencilLayouts );
				result.state.access = it->outputAttach.getAccessMask();
				result.state.pipelineStage = it->outputAttach.getPipelineStageFlags( isCompute );
			}
		}

		return result;
	}

	AccessState RunnableGraph::getCurrentAccessState( FramePass const & pass
		, uint32_t passIndex
		, Buffer const & buffer )const
	{
		auto & buffersLayouts = m_layouts->getBuffersLayout( pass, passIndex );
		auto bufferIt = buffersLayouts.find( buffer.buffer );

		if ( bufferIt != buffersLayouts.end() )
		{
			return bufferIt->second;
		}

		return { 0u, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
	}

	AccessState RunnableGraph::updateCurrentAccessState( FramePass const & pass
		, uint32_t passIndex
		, Buffer const & buffer
		, AccessState newState )
	{
		auto & buffersLayouts = m_layouts->getBuffersLayout( pass, passIndex );
		buffersLayouts[buffer.buffer] = newState;
		return newState;
	}

	AccessState RunnableGraph::getOutputAccessState( FramePass const & pass
		, Buffer const & buffer
		, bool isCompute )const
	{
		AccessState result{ 0u, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
		auto passIt = std::find_if( m_outputTransitions.begin()
			, m_outputTransitions.end()
			, [&pass]( FramePassTransitions const & lookup )
			{
				return lookup.pass == &pass;
			} );
		assert( passIt != m_outputTransitions.end() );
		auto it = std::find_if( passIt->transitions.bufferTransitions.begin()
			, passIt->transitions.bufferTransitions.end()
			, [&buffer]( BufferTransition const & lookup )
			{
				return buffer == lookup.data;
			} );

		if ( it != passIt->transitions.bufferTransitions.end() )
		{
			result.access = it->inputAttach.getAccessMask();
			result.pipelineStage = it->inputAttach.getPipelineStageFlags( isCompute );
		}
		else
		{
			passIt = std::find_if( m_inputTransitions.begin()
				, m_inputTransitions.end()
				, [&pass]( FramePassTransitions const & lookup )
				{
					return lookup.pass == &pass;
				} );
			assert( passIt != m_inputTransitions.end() );
			it = std::find_if( passIt->transitions.bufferTransitions.begin()
				, passIt->transitions.bufferTransitions.end()
				, [&buffer]( BufferTransition const & lookup )
				{
					return buffer == lookup.data;
				} );

			if ( it == passIt->transitions.bufferTransitions.end() )
			{
				result.access = 0u;
				result.pipelineStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			}
			else if ( it->inputAttach.getFlags() != 0u )
			{
				result.access = it->inputAttach.getAccessMask();
				result.pipelineStage = it->inputAttach.getPipelineStageFlags( isCompute );
			}
			else
			{
				result.access = it->outputAttach.getAccessMask();
				result.pipelineStage = it->outputAttach.getPipelineStageFlags( isCompute );
			}
		}

		return result;
	}

	void RunnableGraph::memoryBarrier( RecordContext & context
			, VkCommandBuffer commandBuffer
			, ImageViewId const & view
			, VkImageLayout initialLayout
			, LayoutState const & wantedState )
	{
		context.memoryBarrier( commandBuffer
			, view.data->image
			, view.data->info.viewType
			, view.data->info.subresourceRange
			, initialLayout
			, wantedState );
	}

	void RunnableGraph::memoryBarrier( RecordContext & context
			, VkCommandBuffer commandBuffer
			, ImageId const & image
			, VkImageSubresourceRange const & subresourceRange
			, VkImageLayout initialLayout
			, LayoutState const & wantedState )
	{
		context.memoryBarrier( commandBuffer
			, image
			, VkImageViewType( image.data->info.imageType )
			, subresourceRange
			, initialLayout
			, wantedState );
	}

	void RunnableGraph::memoryBarrier( RecordContext & context
			, VkCommandBuffer commandBuffer
			, ImageId const & image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & subresourceRange
			, VkImageLayout initialLayout
			, LayoutState const & wantedState )
	{
		context.memoryBarrier( commandBuffer
			, image
			, viewType
			, subresourceRange
			, initialLayout
			, wantedState );
	}

	void RunnableGraph::memoryBarrier( RecordContext & context
		, VkCommandBuffer commandBuffer
		, VkBuffer buffer
		, BufferSubresourceRange const & subresourceRange
		, VkAccessFlags initialMask
		, VkPipelineStageFlags initialStage
		, AccessState const & wantedState )
	{
		context.memoryBarrier( commandBuffer
			, buffer
			, subresourceRange
			, initialMask
			, initialStage
			, wantedState );
	}
}
