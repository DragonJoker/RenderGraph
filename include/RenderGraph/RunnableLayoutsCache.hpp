/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "GraphContext.hpp"
#include "FrameGraph.hpp"
#include "ResourceHandler.hpp"

namespace crg
{
	class RunnableLayoutsCache
	{
	public:
		CRG_API RunnableLayoutsCache( FrameGraph & graph
			, ContextResourcesCache & resources
			, FramePassArray passes );

		CRG_API void registerPass( FramePass const & pass
			, uint32_t remainingPassCount );

		CRG_API LayoutStateMap & getViewsLayout( FramePass const & pass
			, uint32_t passIndex );
		CRG_API AccessStateMap & getBuffersLayout( FramePass const & pass
			, uint32_t passIndex );

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
			std::map< uint32_t, ViewsLayoutPtr > views;
			std::map< uint32_t, BuffersLayoutPtr > buffers;
		};

	private:
		FrameGraph & m_graph;
		ContextResourcesCache & m_resources;
		LayoutStateMap m_viewsStates;
		AccessStateMap m_bufferStates;
		std::map< FramePass const *, RemainingPasses > m_passesLayouts;
	};
}
