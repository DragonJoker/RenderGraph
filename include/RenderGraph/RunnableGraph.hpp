/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "GraphContext.hpp"
#include "FrameGraph.hpp"
#include "RunnablePass.hpp"

#include <unordered_map>

namespace crg
{
	class RunnableGraph
	{
	public:
		RunnableGraph( FrameGraph graph
			, GraphContext context );
		~RunnableGraph();

		void record();
		void recordInto( VkCommandBuffer commandBuffer );

		SemaphoreWait run( SemaphoreWait toWait
			, VkQueue queue );
		SemaphoreWait run( SemaphoreWaitArray const & toWait
			, VkQueue queue );
		VkImage getImage( ImageId const & image )const;
		VkImage getImage( Attachment const & attach )const;
		VkImageView getImageView( ImageViewId const & imageView )const;
		VkImageView getImageView( Attachment const & attach )const;
		VertexBuffer const & createQuadVertexBuffer( bool texCoords
			, bool invertU
			, bool invertV );
		VkSampler createSampler( SamplerDesc const & samplerDesc );

		VkImageLayout getInitialLayout( crg::FramePass const & pass
			, ImageViewId view
			, bool allowSrcClear = true );
		VkImageLayout getFinalLayout( crg::FramePass const & pass
			, ImageViewId view );
		void memoryBarrier( VkCommandBuffer commandBuffer
			, ImageViewId const & view
			, VkImageLayout currentLayout
			, VkImageLayout wantedLayout );

	private:
		void doCreateImages();
		void doCreateImageViews();

	private:
		FrameGraph m_graph;
		GraphContext m_context;
		std::vector< RunnablePassPtr > m_passes;
		ImageMemoryMap m_images;
		ImageViewMap m_imageViews;
		std::unordered_map< size_t, VertexBufferPtr > m_vertexBuffers;
		std::unordered_map< size_t, VkSampler > m_samplers;
	};
}
