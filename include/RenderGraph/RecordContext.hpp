/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "Attachment.hpp"

#include <functional>
#include <map>
#include <vector>

namespace crg
{
	CRG_API LayoutState makeLayoutState( VkImageLayout layout );
	CRG_API VkImageAspectFlags getAspectMask( VkFormat format )noexcept;
	CRG_API VkAccessFlags getAccessMask( VkImageLayout layout )noexcept;
	CRG_API VkPipelineStageFlags getStageMask( VkImageLayout layout )noexcept;
	CRG_API LayoutState addSubresourceRangeLayout( LayerLayoutStates & ranges
		, VkImageSubresourceRange const & range
		, LayoutState const & newLayout );
	CRG_API LayoutState getSubresourceRangeLayout( LayerLayoutStates const & ranges
		, VkImageSubresourceRange const & range );

	class RecordContext
	{
	public:
		using PassIndexArray = std::vector< uint32_t >;
		using GraphIndexMap = std::map< FrameGraph const *, PassIndexArray >;
		using ImplicitAction = std::function< void( RecordContext &, VkCommandBuffer, uint32_t ) >;

		struct ImplicitTransition
		{
			RunnablePass const * pass;
			ImageViewId view;
			ImplicitAction action;
		};

	public:
		CRG_API RecordContext( ResourceHandler & handler
			, GraphContext & context );
		CRG_API RecordContext( ResourceHandler & handler );
		/**
		*\name	States
		*/
		//@{
		CRG_API void addStates( RecordContext const & data );
		/**
		*\name	Images
		*/
		//@{
		CRG_API void setLayoutState( crg::ImageViewId view
			, LayoutState layoutState );
		CRG_API LayoutState getLayoutState( ImageViewId view )const;

		CRG_API void setLayoutState( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & subresourceRange
			, LayoutState layoutState );
		CRG_API LayoutState getLayoutState( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & subresourceRange )const;

		CRG_API void registerImplicitTransition( RunnablePass const & pass
			, crg::ImageViewId view
			, ImplicitAction action = []( RecordContext &, VkCommandBuffer, uint32_t ){} );
		CRG_API void registerImplicitTransition( ImplicitTransition transition );
		CRG_API void runImplicitTransition( VkCommandBuffer commandBuffer
			, uint32_t index
			, crg::ImageViewId view );
		//@}
		/**
		*\name	Buffers
		*/
		//@{
		CRG_API void setAccessState( VkBuffer buffer
			, BufferSubresourceRange const & subresourceRange
			, AccessState layoutState );
		CRG_API AccessState getAccessState( VkBuffer buffer
			, BufferSubresourceRange const & subresourceRange )const;
		//@}
		//@}
		/**
		*\name	Memory barriers
		*/
		//@{
		/**
		*\name	Images
		*/
		//@{
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, ImageViewId const & view
			, VkImageLayout initialLayout
			, LayoutState const & wantedState );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, ImageId const & image
			, VkImageSubresourceRange const & subresourceRange
			, VkImageLayout initialLayout
			, LayoutState const & wantedState );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, ImageId const & image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & subresourceRange
			, VkImageLayout initialLayout
			, LayoutState const & wantedState );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, ImageViewId const & view
			, LayoutState const & wantedState );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, ImageId const & image
			, VkImageSubresourceRange const & subresourceRange
			, LayoutState const & wantedState );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, ImageId const & image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & subresourceRange
			, LayoutState const & wantedState );
		//@}
		/**
		*\name	Buffers
		*/
		//@{
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, VkBuffer buffer
			, BufferSubresourceRange const & subresourceRange
			, VkAccessFlags initialMask
			, VkPipelineStageFlags initialStage
			, AccessState const & wantedState );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, VkBuffer buffer
			, BufferSubresourceRange const & subresourceRange
			, AccessState const & wantedState );
		//@}
		//@}
		CRG_API static ImplicitAction copyImage( ImageViewId srcView
			, ImageViewId dstView
			, VkExtent2D extent );
		CRG_API static ImplicitAction clearAttachment( Attachment attach );
		CRG_API static ImplicitAction clearAttachment( ImageViewId view
			, VkClearValue const & clearValue );

		ResourceHandler & getHandler()const
		{
			return *m_handler;
		}

		GraphContext & getContext()const
		{
			return *m_context;
		}

		PassIndexArray const & getIndexState()const
		{
			return m_state;
		}

	private:
		ResourceHandler * m_handler;
		GraphContext * m_context;
		std::map< uint32_t, LayerLayoutStates > m_images;
		AccessStateMap m_buffers;
		std::vector< ImplicitTransition > m_implicitTransitions;
		PassIndexArray m_state;
	};
}
