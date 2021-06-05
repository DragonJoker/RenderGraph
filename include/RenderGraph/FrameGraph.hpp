/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "Attachment.hpp"
#include "ImageData.hpp"
#include "ImageViewData.hpp"
#include "GraphNode.hpp"
#include "FramePass.hpp"

#include <map>
#include <vector>

namespace crg
{
	class FrameGraph
	{
		friend class RunnableGraph;

	public:
		CRG_API FrameGraph( std::string name = "FrameGraph" );
		CRG_API FramePass & createPass( std::string const & name
			, RunnablePassCreator runnableCreator );
		CRG_API RunnableGraphPtr compile( GraphContext context );
		CRG_API ImageId createImage( ImageData const & img );
		CRG_API ImageViewId createView( ImageViewData const & view );
		CRG_API void setFinalLayout( ImageViewId view
			, VkImageLayout layout );
		CRG_API VkImageLayout getFinalLayout( ImageViewId view )const;

	private:
		std::string m_name;
		FramePassPtrArray m_passes;
		ImageIdDataOwnerCont m_images;
		ImageViewIdDataOwnerCont m_imageViews;
		ImageIdAliasMap m_imageAliases;
		ImageViewIdAliasMap m_imageViewAliases;
		std::map< std::string, ImageViewId > m_attachViews;
		std::map< ImageViewId, VkImageLayout > m_finalLayouts;
	};
}
