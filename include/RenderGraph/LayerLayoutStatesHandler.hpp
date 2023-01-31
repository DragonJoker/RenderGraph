/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "FrameGraphPrerequisites.hpp"

namespace crg
{
	struct LayerLayoutStatesHandler
	{
		CRG_API LayerLayoutStatesHandler() = default;
		CRG_API explicit LayerLayoutStatesHandler( LayerLayoutStatesMap const & rhs );
		CRG_API void addStates( LayerLayoutStatesHandler const & data );

		CRG_API void setLayoutState( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & subresourceRange
			, LayoutState layoutState );
		CRG_API void setLayoutState( crg::ImageViewId view
			, LayoutState layoutState );
		CRG_API LayoutState getLayoutState( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & subresourceRange )const;
		CRG_API LayoutState getLayoutState( ImageViewId view )const;

		LayerLayoutStatesMap images;
	};
}
