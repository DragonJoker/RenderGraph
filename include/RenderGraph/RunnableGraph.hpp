/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "GraphContext.hpp"
#include "RenderGraph.hpp"
#include "RunnablePass.hpp"

namespace crg
{
	class RunnableGraph
	{
	public:
		RunnableGraph( RenderGraph graph
			, GraphContext context );
		~RunnableGraph();

		VkSemaphore run( VkSemaphore toWait );
		VkImageView getImageView( Attachment const & attach )const;

	private:
		void doCreateImages();
		void doCreateImageViews();

	private:
		RenderGraph m_graph;
		GraphContext m_context;
		std::vector< RunnablePassPtr > m_passes;
		ImageMemoryMap m_images;
		ImageViewMap m_imageViews;
	};
}
