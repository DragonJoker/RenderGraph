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
	CRG_API VkImageAspectFlags getAspectMask( VkFormat format )noexcept;
	CRG_API VkAccessFlags getAccessMask( VkImageLayout layout )noexcept;
	CRG_API VkPipelineStageFlags getStageMask( VkImageLayout layout )noexcept;

	class RunnableGraph
	{
	public:
		CRG_API RunnableGraph( FrameGraph & graph
			, FramePassDependenciesMap inputTransitions
			, FramePassDependenciesMap outputTransitions
			, AttachmentTransitionArray transitions
			, GraphNodePtrArray nodes
			, RootNode rootNode
			, GraphContext context );
		CRG_API ~RunnableGraph();

		CRG_API void record();
		CRG_API void recordInto( VkCommandBuffer commandBuffer );

		CRG_API SemaphoreWait run( VkQueue queue );
		CRG_API SemaphoreWait run( SemaphoreWait toWait
			, VkQueue queue );
		CRG_API SemaphoreWait run( SemaphoreWaitArray const & toWait
			, VkQueue queue );
		CRG_API ImageViewId createView( ImageViewData const & view );
		CRG_API VkImage getImage( ImageId const & image )const;
		CRG_API VkImage getImage( ImageViewId const & imageView )const;
		CRG_API VkImage getImage( Attachment const & attach
			, uint32_t index = 0u )const;
		CRG_API VkImageView getImageView( ImageViewId const & imageView )const;
		CRG_API VkImageView getImageView( Attachment const & attach
			, uint32_t index = 0u )const;
		CRG_API VertexBuffer const & createQuadVertexBuffer( bool texCoords
			, bool invertU
			, bool invertV );
		CRG_API VkSampler createSampler( SamplerDesc const & samplerDesc );

		CRG_API VkImageLayout getCurrentLayout( ImageViewId view )const;
		CRG_API VkImageLayout updateCurrentLayout( ImageViewId view
			, VkImageLayout newLayout );
		CRG_API VkImageLayout getOutputLayout( crg::FramePass const & pass
			, ImageViewId view )const;
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, ImageId const & image
			, VkImageSubresourceRange const & subresourceRange
			, VkImageLayout currentLayout
			, VkImageLayout wantedLayout );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, ImageViewId const & view
			, VkImageLayout currentLayout
			, VkImageLayout wantedLayout );

		ConstGraphAdjacentNode getGraph()const
		{
			return &m_rootNode;
		}

		AttachmentTransitionArray const & getTransitions()const
		{
			return m_transitions;
		}

	private:
		void doCreateImages();
		void doCreateImageView( ImageViewId view );
		void doCreateImageViews();

	private:
		FrameGraph & m_graph;
		FramePassDependenciesMap m_inputTransitions;
		FramePassDependenciesMap m_outputTransitions;
		AttachmentTransitionArray m_transitions;
		GraphNodePtrArray m_nodes;
		RootNode m_rootNode;
		GraphContext m_context;
		std::vector< RunnablePassPtr > m_passes;
		ImageMemoryMap m_images;
		ImageViewMap m_imageViews;
		std::unordered_map< size_t, VertexBufferPtr > m_vertexBuffers;
		std::unordered_map< size_t, VkSampler > m_samplers;
		std::unordered_map< uint32_t, VkImageLayout > m_viewsLayouts;
		uint32_t m_maxPassCount{ 1u };
	};
}
