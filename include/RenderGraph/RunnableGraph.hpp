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
		/**
		*\param inputTransitions
		*	Transitions for which the pass is the destination.
		*\param outputTransitions
		*	Transitions for which the pass is the source.
		*\param transitions
		*	All transitions.
		*/
		CRG_API RunnableGraph( FrameGraph & graph
			, FramePassDependencies inputTransitions
			, FramePassDependencies outputTransitions
			, AttachmentTransitions transitions
			, GraphNodePtrArray nodes
			, RootNode rootNode
			, GraphContext context );
		CRG_API ~RunnableGraph();

		CRG_API void record();
		CRG_API void recordInto( VkCommandBuffer commandBuffer );

		CRG_API VkImage createImage( ImageId const & image );
		CRG_API VkImageView createImageView( ImageViewId const & view );

		CRG_API SemaphoreWait run( VkQueue queue );
		CRG_API SemaphoreWait run( SemaphoreWait toWait
			, VkQueue queue );
		CRG_API SemaphoreWait run( SemaphoreWaitArray const & toWait
			, VkQueue queue );
		CRG_API ImageViewId createView( ImageViewData const & view );
		CRG_API VertexBuffer const & createQuadVertexBuffer( bool texCoords
			, bool invertU
			, bool invertV );
		CRG_API VkSampler createSampler( SamplerDesc const & samplerDesc );

		CRG_API LayoutState getCurrentLayout( uint32_t passIndex
			, ImageViewId view )const;
		CRG_API LayoutState updateCurrentLayout( uint32_t passIndex
			, ImageViewId view
			, LayoutState newLayout );
		CRG_API LayoutState getOutputLayout( crg::FramePass const & pass
			, ImageViewId view
			, bool isCompute )const;
		CRG_API AccessState getCurrentAccessState( uint32_t passIndex
			, Buffer const & buffer )const;
		CRG_API AccessState updateCurrentAccessState( uint32_t passIndex
			, Buffer const & buffer
			, AccessState newState );
		CRG_API AccessState getOutputAccessState( crg::FramePass const & pass
			, Buffer const & buffer
			, bool isCompute )const;
		CRG_API RunnablePass::LayoutTransition getTransition( crg::FramePass const & pass
			, crg::ImageViewId const & view )const;
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, ImageId const & image
			, VkImageSubresourceRange const & subresourceRange
			, VkImageLayout currentLayout
			, VkImageLayout wantedLayout
			, VkAccessFlags currentMask
			, VkAccessFlags wantedMask
			, VkPipelineStageFlags previousStage
			, VkPipelineStageFlags nextStage );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, ImageId const & image
			, VkImageSubresourceRange const & subresourceRange
			, LayoutState const & currentState
			, LayoutState const & wantedState );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, ImageViewId const & view
			, VkImageLayout currentLayout
			, VkImageLayout wantedLayout
			, VkAccessFlags currentMask
			, VkAccessFlags wantedMask
			, VkPipelineStageFlags previousStage
			, VkPipelineStageFlags nextStage );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, ImageViewId const & view
			, LayoutState const & currentState
			, LayoutState const & wantedState );
		CRG_API void imageMemoryBarrier( VkCommandBuffer commandBuffer
			, Attachment const & from
			, uint32_t fromIndex
			, Attachment const & to
			, uint32_t toIndex
			, bool isCompute );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, Buffer const & buffer
			, VkDeviceSize offset
			, VkDeviceSize range
			, VkAccessFlags currentMask
			, VkAccessFlags wantedMask
			, VkPipelineStageFlags previousStage
			, VkPipelineStageFlags nextStage );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, Buffer const & buffer
			, VkDeviceSize offset
			, VkDeviceSize range
			, AccessState const & currentState
			, AccessState const & wantedState );
		CRG_API void bufferMemoryBarrier( VkCommandBuffer commandBuffer
			, Attachment const & from
			, Attachment const & to
			, bool isCompute );

		ConstGraphAdjacentNode getGraph()const
		{
			return &m_rootNode;
		}

		AttachmentTransitions const & getTransitions()const
		{
			return m_transitions;
		}

	private:
		using MipLayoutStates = std::map< uint32_t, LayoutState >;
		using LayerLayoutStates = std::map< uint32_t, MipLayoutStates >;
		using LayoutStateMap = std::unordered_map< uint32_t, LayerLayoutStates >;

		void doRegisterImages( crg::FramePass const & pass
			, LayoutStateMap & images );
		void doCreateImages();
		void doCreateImageViews();
		LayoutState doGetSubresourceRangeLayout( LayerLayoutStates const & layers
			, VkImageSubresourceRange const & range )const;
		LayoutState doAddSubresourceRangeLayout( LayerLayoutStates & layers
			, VkImageSubresourceRange const & range
			, LayoutState const & newLayout );

	private:
		FrameGraph & m_graph;
		FramePassDependencies m_inputTransitions;
		FramePassDependencies m_outputTransitions;
		AttachmentTransitions m_transitions;
		GraphNodePtrArray m_nodes;
		RootNode m_rootNode;
		GraphContext m_context;
		std::vector< RunnablePassPtr > m_passes;
		std::map< ImageId, VkImage > m_images;
		std::map< ImageViewId, VkImageView > m_imageViews;
		using AccessStateMap = std::unordered_map< VkBuffer, AccessState >;
		std::unordered_map< size_t, VertexBufferPtr > m_vertexBuffers;
		std::unordered_map< size_t, VkSampler > m_samplers;
		std::vector< LayoutStateMap > m_viewsLayouts;
		std::vector< AccessStateMap > m_buffersLayouts;
		uint32_t m_maxPassCount{ 1u };
	};
}
