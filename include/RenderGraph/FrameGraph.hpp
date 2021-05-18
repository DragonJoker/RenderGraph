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
		void add( FramePass const & pass );
		void remove( FramePass const & pass );
		void compile();
		ImageId createImage( ImageData const & img );

		inline GraphAdjacentNode getGraph()
		{
			return &m_root;
		}

		inline AttachmentTransitionArray const & getTransitions()
		{
			return m_transitions;
		}

	private:
		ImageViewId createView( ImageViewData const & img );

	private:
		FramePassPtrArray m_passes;
		AttachmentArray m_attachments;
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
