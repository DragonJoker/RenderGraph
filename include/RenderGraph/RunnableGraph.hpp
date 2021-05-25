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
		CRG_API RunnableGraph( FrameGraph graph
			, GraphContext context );
		CRG_API ~RunnableGraph();

		CRG_API void record();
		CRG_API void recordInto( VkCommandBuffer commandBuffer );

		CRG_API SemaphoreWait run( SemaphoreWait toWait
			, VkQueue queue );
		CRG_API SemaphoreWait run( SemaphoreWaitArray const & toWait
			, VkQueue queue );
		CRG_API VkImage getImage( ImageId const & image )const;
		CRG_API VkImage getImage( Attachment const & attach )const;
		CRG_API VkImageView getImageView( ImageViewId const & imageView )const;
		CRG_API VkImageView getImageView( Attachment const & attach )const;
		CRG_API VertexBuffer const & createQuadVertexBuffer( bool texCoords
			, bool invertU
			, bool invertV );
		CRG_API VkSampler createSampler( SamplerDesc const & samplerDesc );

		CRG_API VkImageLayout getInitialLayout( crg::FramePass const & pass
			, ImageViewId view
			, bool allowSrcClear = true );
		CRG_API VkImageLayout getFinalLayout( crg::FramePass const & pass
			, ImageViewId view );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
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
