/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "Attachment.hpp"
#include "ImageData.hpp"
#include "ImageViewData.hpp"
#include "GraphNode.hpp"
#include "RenderPass.hpp"

#include <map>
#include <vector>

namespace crg
{
	class RenderGraph
	{
		friend class RunnableGraph;

	public:
		RenderGraph( std::string name = "RenderGraph" );
		void add( RenderPass const & pass );
		void remove( RenderPass const & pass );
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
		RenderPassPtrArray m_passes;
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
