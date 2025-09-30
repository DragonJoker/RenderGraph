/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "Attachment.hpp"
#include "LayerLayoutStatesHandler.hpp"

#include <functional>
#include <map>
#include <vector>

namespace crg
{
	class RecordContext
	{
	public:
		using PassIndexArray = std::vector< uint32_t >;
		using GraphIndexMap = std::map< FrameGraph const *, PassIndexArray >;
		using ImplicitAction = std::function< void( RecordContext &, VkCommandBuffer, uint32_t ) >;

		struct ImplicitImageTransition
		{
			RunnablePass const * pass;
			ImageViewId view;
			ImplicitAction action;
		};

		struct ImplicitBufferTransition
		{
			RunnablePass const * pass;
			BufferViewId view;
			ImplicitAction action;
		};

	public:
		CRG_API explicit RecordContext( ContextResourcesCache & resources );
		CRG_API explicit RecordContext( ResourceHandler & handler );
		/**
		*\name	States
		*/
		//@{
		CRG_API void addStates( RecordContext const & data );
		/**
		*\name	Pipeline
		*/
		//@{
		CRG_API void setNextPipelineState( PipelineState const & state
			, LayerLayoutStatesMap const & imageLayouts );
		//@}
		/**
		*\name	Images
		*/
		//@{
		CRG_API void setLayoutState( crg::ImageViewId view
			, LayoutState const & layoutState );
		CRG_API LayoutState getLayoutState( ImageViewId view )const;

		CRG_API void setLayoutState( ImageId image
			, ImageViewType viewType
			, ImageSubresourceRange const & subresourceRange
			, LayoutState const & layoutState );
		CRG_API LayoutState getLayoutState( ImageId image
			, ImageViewType viewType
			, ImageSubresourceRange const & subresourceRange )const;

		CRG_API LayoutState getNextLayoutState( ImageViewId view )const;
		CRG_API LayoutState getNextLayoutState( ImageId image
			, ImageViewType viewType
			, ImageSubresourceRange const & subresourceRange )const;

		CRG_API void registerImplicitTransition( RunnablePass const & pass
			, crg::ImageViewId view
			, ImplicitAction action = []( RecordContext const &, VkCommandBuffer, uint32_t ){} );
		CRG_API void registerImplicitTransition( RunnablePass const & pass
			, crg::BufferViewId view
			, ImplicitAction action = []( RecordContext const &, VkCommandBuffer, uint32_t ){} );
		CRG_API void registerImplicitTransition( ImplicitImageTransition transition );
		CRG_API void registerImplicitTransition( ImplicitBufferTransition transition );
		CRG_API void runImplicitTransition( VkCommandBuffer commandBuffer
			, uint32_t index
			, crg::ImageViewId view );
		CRG_API void runImplicitTransition( VkCommandBuffer commandBuffer
			, uint32_t index
			, crg::BufferViewId view );
		//@}
		/**
		*\name	Buffers
		*/
		//@{
		CRG_API void setAccessState( BufferViewId buffer
			, AccessState const & accessState );
		CRG_API AccessState getAccessState( BufferViewId view )const;

		CRG_API void setAccessState( BufferId buffer
			, BufferSubresourceRange const & subresourceRange
			, AccessState const & accessState );
		CRG_API AccessState const & getAccessState( BufferId buffer
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
			, ImageLayout initialLayout
			, LayoutState const & wantedState
			, bool force = false );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, ImageId const & image
			, ImageSubresourceRange const & subresourceRange
			, ImageLayout initialLayout
			, LayoutState const & wantedState
			, bool force = false );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, ImageId const & image
			, ImageViewType viewType
			, ImageSubresourceRange const & subresourceRange
			, ImageLayout initialLayout
			, LayoutState const & wantedState
			, bool force = false );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, ImageViewId const & view
			, LayoutState const & wantedState
			, bool force = false );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, ImageId const & image
			, ImageSubresourceRange const & subresourceRange
			, LayoutState const & wantedState
			, bool force = false );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, ImageId const & image
			, ImageViewType viewType
			, ImageSubresourceRange const & subresourceRange
			, LayoutState const & wantedState
			, bool force = false );
		//@}
		/**
		*\name	Buffers
		*/
		//@{
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, BufferId buffer
			, BufferSubresourceRange const & subresourceRange
			, AccessState const & initialState
			, AccessState const & wantedState
			, bool force = false );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, BufferViewId view
			, AccessState const & initialState
			, AccessState const & wantedState
			, bool force = false );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, BufferId buffer
			, BufferSubresourceRange const & subresourceRange
			, AccessState const & wantedState
			, bool force = false );
		CRG_API void memoryBarrier( VkCommandBuffer commandBuffer
			, BufferViewId view
			, AccessState const & wantedState
			, bool force = false );
		//@}
		//@}
		CRG_API GraphContext & getContext()const;
		CRG_API ContextResourcesCache & getResources()const;

		GraphContext * operator->()const
		{
			return &getContext();
		}

		CRG_API void copyImage( VkCommandBuffer commandBuffer
			, uint32_t index
			, ImageViewId srcView
			, ImageViewId dstView
			, Extent2D const & extent
			, ImageLayout finalLayout = ImageLayout::eUndefined );
		CRG_API void blitImage( VkCommandBuffer commandBuffer
			, uint32_t index
			, ImageViewId srcView
			, ImageViewId dstView
			, Rect2D const & srcRect
			, Rect2D const & dstRect
			, FilterMode filter
			, ImageLayout finalLayout = ImageLayout::eUndefined );
		CRG_API void clearAttachment( VkCommandBuffer commandBuffer
			, ImageViewId dstView
			, ClearColorValue const & clearValue
			, ImageLayout finalLayout = ImageLayout::eUndefined );
		CRG_API void clearAttachment( VkCommandBuffer commandBuffer
			, ImageViewId dstView
			, ClearDepthStencilValue const & clearValue
			, ImageLayout finalLayout = ImageLayout::eUndefined );
		CRG_API void copyBuffer( VkCommandBuffer commandBuffer
			, uint32_t index
			, BufferViewId srcView
			, BufferViewId dstView
			, DeviceSize srcOffset, DeviceSize dstOffset
			, DeviceSize size
			, AccessState const & finalState = {} );
		CRG_API void clearBuffer( VkCommandBuffer commandBuffer
			, BufferViewId dstView
			, uint32_t clearValue
			, AccessState const & finalState = {} );

		CRG_API static ImplicitAction copyImage( ImageViewId srcView
			, ImageViewId dstView
			, Extent2D const & extent
			, ImageLayout finalLayout = ImageLayout::eUndefined );
		CRG_API static ImplicitAction blitImage( ImageViewId srcView
			, ImageViewId dstView
			, Rect2D const & srcRect
			, Rect2D const & dstRect
			, FilterMode filter
			, ImageLayout finalLayout = ImageLayout::eUndefined );
		CRG_API static ImplicitAction clearAttachment( Attachment const & attach
			, ImageLayout finalLayout = ImageLayout::eUndefined );
		CRG_API static ImplicitAction clearAttachment( ImageViewId view
			, ClearColorValue const & clearValue
			, ImageLayout finalLayout = ImageLayout::eUndefined );
		CRG_API static ImplicitAction clearAttachment( ImageViewId view
			, ClearDepthStencilValue const & clearValue
			, ImageLayout finalLayout = ImageLayout::eUndefined );
		CRG_API static ImplicitAction clearBuffer( BufferViewId dstView
			, AccessState const & finalState = {} );
		CRG_API static ImplicitAction clearBuffer( BufferViewId dstView
			, uint32_t clearValue
			, AccessState const & finalState = {} );
		CRG_API static ImplicitAction copyBuffer( BufferViewId srcView
			, BufferViewId dstView
			, DeviceSize srcOffset, DeviceSize dstOffset
			, DeviceSize size
			, AccessState const & finalState = {} );

		ResourceHandler & getHandler()const
		{
			return *m_handler;
		}

		PassIndexArray const & getIndexState()const
		{
			return m_state;
		}

		PipelineState const & getPrevPipelineState()const
		{
			return m_prevPipelineState;
		}

		PipelineState const & getCurrPipelineState()const
		{
			return m_currPipelineState;
		}

		PipelineState const & getNextPipelineState()const
		{
			return m_nextPipelineState;
		}

	private:
		ResourceHandler * m_handler;
		ContextResourcesCache * m_resources;
		LayerLayoutStatesHandler m_images;
		AccessStateMap m_buffers;
		std::vector< ImplicitImageTransition > m_implicitImageTransitions;
		std::vector< ImplicitBufferTransition > m_implicitBufferTransitions;
		PassIndexArray m_state;
		PipelineState m_prevPipelineState{};
		PipelineState m_currPipelineState{};
		PipelineState m_nextPipelineState{};
		LayerLayoutStatesHandler m_nextImages;
	};
}
