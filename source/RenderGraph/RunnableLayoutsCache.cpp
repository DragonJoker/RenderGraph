/*
See LICENSE file in root folder.
*/
#include "RenderGraph/RunnableLayoutsCache.hpp"
#include "RenderGraph/RunnablePass.hpp"
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
	RunnableLayoutsCache::RunnableLayoutsCache( FrameGraph & graph
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

	RunnableLayoutsCache::~RunnableLayoutsCache()
	{
		for ( auto & imageViewIt : m_imageViews )
		{
			m_graph.m_handler.destroyImageView( m_context, imageViewIt.first );
		}

		for ( auto & imageIt : m_images )
		{
			m_graph.m_handler.destroyImage( m_context, imageIt.first );
		}
	}

	void RunnableLayoutsCache::registerPass( FramePass const & pass
		, uint32_t remainingPassCount )
	{
		m_passesLayouts.emplace( &pass
			, RemainingPasses{ remainingPassCount, {}, {} } );
	}

	void RunnableLayoutsCache::initialise( GraphNodePtrArray const & nodes
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

	LayoutStateMap & RunnableLayoutsCache::getViewsLayout( FramePass const & pass
		, uint32_t passIndex )
	{
		auto it = m_passesLayouts.find( &pass );
		assert( it != m_passesLayouts.end() );
		assert( it->second.views.size() >= passIndex );
		auto & viewsLayouts = *it->second.views[passIndex];
		doInitialiseLayout( viewsLayouts );
		return *viewsLayouts;
	}

	AccessStateMap & RunnableLayoutsCache::getBuffersLayout( FramePass const & pass
		, uint32_t passIndex )
	{
		auto it = m_passesLayouts.find( &pass );
		assert( it != m_passesLayouts.end() );
		assert( it->second.buffers.size() >= passIndex );
		auto & buffersLayouts = *it->second.buffers[passIndex];
		doInitialiseLayout( buffersLayouts );
		return *buffersLayouts;
	}

	VkImage RunnableLayoutsCache::createImage( ImageId const & image )
	{
		auto result = m_graph.m_handler.createImage( m_context, image );
		m_images[image] = result;
		return result;
	}

	VkImageView RunnableLayoutsCache::createImageView( ImageViewId const & view )
	{
		auto result = m_graph.m_handler.createImageView( m_context, view );
		m_imageViews[view] = result;
		return result;
	}

	void RunnableLayoutsCache::doCreateImages()
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

	void RunnableLayoutsCache::doCreateImageViews()
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

	void RunnableLayoutsCache::doRegisterViews( FramePass const & pass )
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

	void RunnableLayoutsCache::doRegisterBuffers( FramePass const & pass )
	{
		static AccessState const defaultState{ getAccessMask( VK_IMAGE_LAYOUT_UNDEFINED )
			, getStageMask( VK_IMAGE_LAYOUT_UNDEFINED ) };

		for ( auto & attach : pass.buffers )
		{
			auto buffer = attach.buffer.buffer.buffer;
			m_bufferStates.emplace( buffer, defaultState );
		}
	}

	void RunnableLayoutsCache::doInitialiseLayout( ViewsLayoutPtr & viewsLayouts )const
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

	void RunnableLayoutsCache::doInitialiseLayout( BuffersLayoutPtr & buffersLayouts )const
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
}
