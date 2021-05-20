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
		FrameGraph( std::string name = "FrameGraph" );
		FramePass & createPass( std::string const & name
			, RunnablePassCreator runnableCreator );
		void compile();
		ImageId createImage( ImageData const & img );
		ImageViewId createView( ImageViewData const & view );

		inline GraphAdjacentNode getGraph()
		{
			return &m_root;
		}

		inline AttachmentTransitionArray const & getTransitions()
		{
			return m_transitions;
		}

	private:
		FramePassPtrArray m_passes;
		ImageIdDataOwnerCont m_images;
		ImageViewIdDataOwnerCont m_imageViews;
		GraphNodePtrArray m_nodes;
		AttachmentTransitionArray m_transitions;
		RootNode m_root;
		ImageIdAliasMap m_imageAliases;
		ImageViewIdAliasMap m_imageViewAliases;
		std::map< std::string, ImageViewId > m_attachViews;
	};
}
