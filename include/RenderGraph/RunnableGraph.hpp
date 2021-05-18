/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "GraphContext.hpp"
#include "RenderGraph.hpp"
#include "RunnablePass.hpp"

#include <unordered_map>

namespace crg
{
	class RunnableGraph
	{
	public:
		RunnableGraph( RenderGraph graph
			, GraphContext context );
		~RunnableGraph();

		void record()const;
		void recordInto( VkCommandBuffer commandBuffer )const;
		SemaphoreWait run( SemaphoreWait toWait
			, VkQueue queue );
		VkImage getImage( ImageId const & image )const;
		VkImage getImage( Attachment const & attach )const;
		VkImageView getImageView( ImageViewId const & imageView )const;
		VkImageView getImageView( Attachment const & attach )const;
		VertexBuffer const & createQuadVertexBuffer( bool texCoords
			, bool invertU
			, bool invertV );

	private:
		void doCreateImages();
		void doCreateImageViews();

	private:
		RenderGraph m_graph;
		GraphContext m_context;
		std::vector< RunnablePassPtr > m_passes;
		ImageMemoryMap m_images;
		ImageViewMap m_imageViews;
		std::unordered_map< size_t, VertexBufferPtr > m_vertexBuffers;
	};
}
