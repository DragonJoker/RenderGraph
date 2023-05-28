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
		, VkImageViewType viewType
		, VkImageSubresourceRange const & subresourceRange
		, LayoutState layoutState )
	{
		auto range = getVirtualRange( image
			, viewType
			, subresourceRange );
		auto ires = images.emplace( image.id, LayerLayoutStates{} );
		addSubresourceRangeLayout( ires.first->second
			, range
			, layoutState );
	}

	void LayerLayoutStatesHandler::setLayoutState( crg::ImageViewId view
		, LayoutState layoutState )
	{
		assert( view.data->source.empty()
			&& "Merged image views must be resolved before setting their layout state" );
		setLayoutState( view.data->image
			, view.data->info.viewType
			, view.data->info.subresourceRange
			, layoutState );
	}

	LayoutState LayerLayoutStatesHandler::getLayoutState( ImageId image
		, VkImageViewType viewType
		, VkImageSubresourceRange const & subresourceRange )const
	{
		static LayoutState const undefLayout{ VK_IMAGE_LAYOUT_UNDEFINED, { 0u, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT } };
		auto imageIt = images.find( image.id );

		if ( imageIt != images.end() )
		{
			auto range = getVirtualRange( image
				, viewType
				, subresourceRange );
			return getSubresourceRangeLayout( imageIt->second
				, range );
		}

		return undefLayout;
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
