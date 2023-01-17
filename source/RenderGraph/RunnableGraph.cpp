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

	RunnableGraph::LayoutsCache::LayoutsCache( FrameGraph & graph
		, GraphContext & context
		, FramePassArray & passes )
		: m_graph{ graph }
		, m_context{ context }
	{
		Logger::logDebug( m_graph.getName() + " - Initialising resources" );

		for ( auto pass : passes )
		{
			doRegisterViews( *pass );
			doRegisterBuffers( *pass );
		}

		doCreateImages();
		doCreateImageViews();
	}

	void RunnableGraph::LayoutsCache::registerPass( FramePass const & pass
		, uint32_t remainingPassCount )
	{
		m_passesLayouts.emplace( &pass
			, RemainingPasses{ remainingPassCount, {}, {} } );
	}

	void RunnableGraph::LayoutsCache::initialise( GraphNodePtrArray const & nodes
		, std::vector< RunnablePassPtr > const & passes
		, uint32_t maxPassCount )
	{
		Logger::logDebug( m_graph.getName() + " - Creating layouts" );
		m_viewsLayouts.resize( maxPassCount );
		m_buffersLayouts.resize( maxPassCount );

		Logger::logDebug( m_graph.getName() + " - Initialising nodes layouts" );
		auto remainingCount = maxPassCount;
		uint32_t index = 0u;

		for ( auto & node : nodes )
		{
			auto it = m_passesLayouts.find( getFramePass( *node ) );
			remainingCount /= it->second.count;
			it->second.count = remainingCount;

			for ( uint32_t i = 0u; i < passes[index]->getMaxPassCount(); ++i )
			{
				it->second.views.push_back( m_viewsLayouts.begin() + ( i * remainingCount ) );
				it->second.buffers.push_back( m_buffersLayouts.begin() + ( i * remainingCount ) );
			}

			++index;
		}
	}

	LayoutStateMap & RunnableGraph::LayoutsCache::getViewsLayout( FramePass const & pass
		, uint32_t passIndex )
	{
		auto it = m_passesLayouts.find( &pass );
		assert( it != m_passesLayouts.end() );
		assert( it->second.views.size() >= passIndex );
		auto & viewsLayouts = *it->second.views[passIndex];
		doInitialiseLayout( viewsLayouts );
		return *viewsLayouts;
	}

	AccessStateMap & RunnableGraph::LayoutsCache::getBuffersLayout( FramePass const & pass
		, uint32_t passIndex )
	{
		auto it = m_passesLayouts.find( &pass );
		assert( it != m_passesLayouts.end() );
		assert( it->second.buffers.size() >= passIndex );
		auto & buffersLayouts = *it->second.buffers[passIndex];
		doInitialiseLayout( buffersLayouts );
		return *buffersLayouts;
	}

	VkImage RunnableGraph::LayoutsCache::createImage( ImageId const & image )
	{
		auto result = m_graph.m_handler.createImage( m_context, image );
		m_images[image] = result;
		return result;
	}
	VkImageView RunnableGraph::LayoutsCache::createImageView( ImageViewId const & view )
	{
		auto result = m_graph.m_handler.createImageView( m_context, view );
		m_imageViews[view] = result;
		return result;
	}

	void RunnableGraph::LayoutsCache::doCreateImages()
	{
		if ( !m_context.device )
		{
			return;
		}

		for ( auto & img : m_graph.m_images )
		{
			createImage( img );
		}
	}


	void RunnableGraph::LayoutsCache::doCreateImageViews()
	{
		if ( !m_context.device )
		{
			return;
		}

		for ( auto & view : m_graph.m_imageViews )
		{
			createImageView( view );
		}
	}

	void RunnableGraph::LayoutsCache::doRegisterViews( FramePass const & pass )
	{
		static LayoutState const defaultState{ VK_IMAGE_LAYOUT_UNDEFINED
			, getAccessMask( VK_IMAGE_LAYOUT_UNDEFINED )
			, getStageMask( VK_IMAGE_LAYOUT_UNDEFINED ) };

		for ( auto & attach : pass.images )
		{
			if ( attach.count > 1u )
			{
				for ( uint32_t i = 0u; i < attach.count; ++i )
				{
					auto view = attach.view( i );
					auto image = view.data->image;
					createImage( image );
					createImageView( view );
					auto ires = m_viewsStates.emplace( image.id, LayerLayoutStates{} );

					if ( ires.second )
					{
						auto & layers = ires.first->second;
						auto sliceArrayCount = ( image.data->info.extent.depth > 1u
							? image.data->info.extent.depth
							: image.data->info.arrayLayers );

						for ( uint32_t slice = 0; slice < sliceArrayCount; ++slice )
						{
							auto & levels = layers.emplace( slice, MipLayoutStates{} ).first->second;

							for ( uint32_t level = 0; level < image.data->info.mipLevels; ++level )
							{
								levels.emplace( level, defaultState );
							}
						}
					}
				}
			}
			else
			{
				auto view = attach.view();
				auto image = view.data->image;
				createImage( image );
				createImageView( view );
				auto ires = m_viewsStates.emplace( image.id, LayerLayoutStates{} );

				if ( ires.second )
				{
					auto & layers = ires.first->second;
					auto sliceArrayCount = ( image.data->info.extent.depth > 1u
						? image.data->info.extent.depth
						: image.data->info.arrayLayers );

					for ( uint32_t slice = 0; slice < sliceArrayCount; ++slice )
					{
						auto & levels = layers.emplace( slice, MipLayoutStates{} ).first->second;

						for ( uint32_t level = 0; level < image.data->info.mipLevels; ++level )
						{
							levels.emplace( level, defaultState );
						}
					}
				}
			}
		}
	}

	void RunnableGraph::LayoutsCache::doRegisterBuffers( FramePass const & pass )
	{
		static AccessState const defaultState{ getAccessMask( VK_IMAGE_LAYOUT_UNDEFINED )
			, getStageMask( VK_IMAGE_LAYOUT_UNDEFINED ) };

		for ( auto & attach : pass.buffers )
		{
			auto buffer = attach.buffer.buffer.buffer;
			m_bufferStates.emplace( buffer, defaultState );
		}
	}

	void RunnableGraph::LayoutsCache::doInitialiseLayout( ViewsLayoutPtr & viewsLayouts )const
	{
		if ( !viewsLayouts )
		{
			viewsLayouts = std::make_unique< ViewsLayout >();

			for ( auto & srcLayers : m_viewsStates )
			{
				LayerLayoutStates & dstLayers = viewsLayouts->emplace( srcLayers.first, LayerLayoutStates{} ).first->second;

				for ( auto & srcMips : srcLayers.second )
				{
					MipLayoutStates & dstMips = dstLayers.emplace( srcMips.first, MipLayoutStates{} ).first->second;

					for ( auto & srcLevel : srcMips.second )
					{
						dstMips.emplace( srcLevel );
					}
				}
			}
		}
	}

	void RunnableGraph::LayoutsCache::doInitialiseLayout( BuffersLayoutPtr & buffersLayouts )const
	{
		if ( !buffersLayouts )
		{
			buffersLayouts = std::make_unique< BuffersLayout >();

			for ( auto & srcState : m_bufferStates )
			{
				buffersLayouts->emplace( srcState );
			}
		}
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
		, m_inputTransitions{ std::move( inputTransitions ) }
		, m_outputTransitions{ std::move( outputTransitions ) }
		, m_transitions{ std::move( transitions ) }
		, m_nodes{ std::move( nodes ) }
		, m_rootNode{ std::move( rootNode ) }
		, m_layouts{ std::make_unique< LayoutsCache >( graph, m_context, passes ) }
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
		RecordContext recordContext{ m_graph.getHandler(), m_context };

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
		return m_graph.getHandler().createQuadTriVertexBuffer( m_context
			, texCoords
			, config );
	}

	VkSampler RunnableGraph::createSampler( SamplerDesc const & samplerDesc )
	{
		return m_graph.getHandler().createSampler( m_context, samplerDesc );
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
