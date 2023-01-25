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
		, ContextResourcesCache & resources
		, FramePassArray passes )
		: m_graph{ graph }
		, m_resources{ resources }
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

	void RunnableLayoutsCache::registerPass( FramePass const & pass
		, uint32_t passCount )
	{
		m_passesLayouts.emplace( &pass
			, RemainingPasses{ passCount, {}, {} } );
	}

	LayoutStateMap & RunnableLayoutsCache::getViewsLayout( FramePass const & pass
		, uint32_t passIndex )
	{
		auto it = m_passesLayouts.find( &pass );
		assert( it != m_passesLayouts.end() );
		auto & viewsLayouts = it->second.views[passIndex];
		doInitialiseLayout( viewsLayouts );
		return *viewsLayouts;
	}

	AccessStateMap & RunnableLayoutsCache::getBuffersLayout( FramePass const & pass
		, uint32_t passIndex )
	{
		auto it = m_passesLayouts.find( &pass );
		assert( it != m_passesLayouts.end() );
		auto & buffersLayouts = it->second.buffers[passIndex];
		doInitialiseLayout( buffersLayouts );
		return *buffersLayouts;
	}

	void RunnableLayoutsCache::doCreateImages()
	{
		for ( auto & img : m_graph.m_images )
		{
			m_resources.createImage( img );
		}
	}

	void RunnableLayoutsCache::doCreateImageViews()
	{
		for ( auto & view : m_graph.m_imageViews )
		{
			m_resources.createImageView( view );
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
					m_resources.createImage( image );
					m_resources.createImageView( view );
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
				m_resources.createImage( image );
				m_resources.createImageView( view );
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
