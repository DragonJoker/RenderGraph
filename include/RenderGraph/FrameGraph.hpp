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
		CRG_API FrameGraph( ResourceHandler & handler
			, std::string name = "FrameGraph" );
		CRG_API FramePass & createPass( std::string const & name
			, RunnablePassCreator runnableCreator );
		CRG_API RunnableGraphPtr compile( GraphContext context );
		CRG_API ImageId createImage( ImageData const & img );
		CRG_API ImageViewId createView( ImageViewData const & view );
		CRG_API void setFinalLayout( ImageViewId view
			, LayoutState layout );
		CRG_API LayoutState getFinalLayout( ImageViewId view )const;
		CRG_API void setFinalAccessState( Buffer const & buffer
			, AccessState layout );
		CRG_API AccessState getFinalAccessState( Buffer const & buffer )const;

	private:
		ResourceHandler & m_handler;
		std::string m_name;
		FramePassPtrArray m_passes;
		ImageIdAliasMap m_imageAliases;
		ImageViewIdAliasMap m_imageViewAliases;
		std::set< ImageId > m_images;
		std::set< ImageViewId > m_imageViews;
		std::map< std::string, ImageViewId > m_attachViews;
		std::map< ImageViewId, LayoutState > m_finalLayouts;
		std::map< VkBuffer, AccessState > m_finalAccesses;
	};
}
