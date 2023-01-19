/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "GraphContext.hpp"
#include "FrameGraph.hpp"

namespace crg
{
	class RunnableLayoutsCache
	{
	public:
		CRG_API RunnableLayoutsCache( FrameGraph & graph
			, GraphContext & context
			, FramePassArray & passes );
		CRG_API ~RunnableLayoutsCache();

		CRG_API void registerPass( FramePass const & pass
			, uint32_t remainingPassCount );
		CRG_API void initialise( GraphNodePtrArray const & nodes
			, std::vector< RunnablePassPtr > const & passes
			, uint32_t maxPassCount );

		CRG_API LayoutStateMap & getViewsLayout( FramePass const & pass
			, uint32_t passIndex );
		CRG_API AccessStateMap & getBuffersLayout( FramePass const & pass
			, uint32_t passIndex );

		CRG_API VkImage createImage( ImageId const & image );
		CRG_API VkImageView createImageView( ImageViewId const & view );

	private:
		void doCreateImages();
		void doCreateImageViews();
		void doRegisterViews( crg::FramePass const & pass );
		void doRegisterBuffers( crg::FramePass const & pass );
		void doInitialiseLayout( ViewsLayoutPtr & viewsLayouts )const;
		void doInitialiseLayout( BuffersLayoutPtr & buffersLayouts )const;

	private:
		struct RemainingPasses
		{
			uint32_t count;
			ViewLayoutIterators views;
			BufferLayoutIterators buffers;
		};

	private:
		FrameGraph & m_graph;
		GraphContext & m_context;
		std::map< ImageId, VkImage > m_images;
		std::map< ImageViewId, VkImageView > m_imageViews;
		LayoutStateMap m_viewsStates;
		ViewsLayouts m_viewsLayouts;
		AccessStateMap m_bufferStates;
		BuffersLayouts m_buffersLayouts;
		std::map< FramePass const *, RemainingPasses > m_passesLayouts;
	};
}
