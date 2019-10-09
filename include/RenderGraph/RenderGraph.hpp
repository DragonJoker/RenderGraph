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
	public:
		RenderGraph( std::string name = "RenderGraph" );
		void add( RenderPass const & pass );
		void remove( RenderPass const & pass );
		bool compile();
		ImageId createImage( ImageData const & img );
		ImageViewId createView( ImageViewData const & img );

		inline GraphAdjacentNode getGraph()
		{
			return &m_root;
		}

	private:
		std::vector< RenderPassPtr > m_passes;
		AttachmentArray m_attachments;
		std::map< ImageId, std::unique_ptr< ImageData > > m_images;
		std::map< ImageViewId, std::unique_ptr< ImageViewData > > m_imageViews;
		GraphNodePtrArray m_nodes;
		RootNode m_root;
	};
}
