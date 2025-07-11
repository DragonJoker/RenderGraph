/*
See LICENSE file in root folder.
*/
#include "RenderGraph/LayerLayoutStatesHandler.hpp"

#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/ImageViewData.hpp"

#include <cassert>

namespace crg
{
	LayerLayoutStatesHandler::LayerLayoutStatesHandler( LayerLayoutStatesMap const & rhs )
		: images{ rhs }
	{
	}

	void LayerLayoutStatesHandler::addStates( LayerLayoutStatesHandler const & data )
	{
		for ( auto & state : data.images )
		{
			images.insert( state );
		}
	}

	void LayerLayoutStatesHandler::setLayoutState( ImageId image
		, ImageViewType viewType
		, ImageSubresourceRange const & subresourceRange
		, LayoutState const & layoutState )
	{
		auto range = getVirtualRange( image
			, viewType
			, subresourceRange );
		auto [it, _] = images.try_emplace( image.id );
		addSubresourceRangeLayout( it->second
			, range
			, layoutState );
	}

	void LayerLayoutStatesHandler::setLayoutState( crg::ImageViewId view
		, LayoutState const & layoutState )
	{
		assert( view.data->source.empty()
			&& "Merged image views must be resolved before setting their layout state" );
		setLayoutState( view.data->image
			, view.data->info.viewType
			, view.data->info.subresourceRange
			, layoutState );
	}

	LayoutState LayerLayoutStatesHandler::getLayoutState( ImageId image
		, ImageViewType viewType
		, ImageSubresourceRange const & subresourceRange )const
	{
		if ( auto imageIt = images.find( image.id ); imageIt != images.end() )
		{
			auto range = getVirtualRange( image
				, viewType
				, subresourceRange );
			return getSubresourceRangeLayout( imageIt->second
				, range );
		}

		return { ImageLayout::eUndefined, { AccessFlags::eNone, PipelineStageFlags::eBottomOfPipe } };
	}

	LayoutState LayerLayoutStatesHandler::getLayoutState( ImageViewId view )const
	{
		assert( view.data->source.empty()
			&& "Merged image views must be resolved before finding their layout state" );
		return getLayoutState( view.data->image
			, view.data->info.viewType
			, view.data->info.subresourceRange );
	}
}
