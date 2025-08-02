/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FramePass.hpp"

#include <unordered_set>

namespace crg
{
	struct FramePassGroup
	{
		friend class FrameGraph;
		class Token
		{
			friend class FrameGraph;
			friend struct FramePassGroup;

		private:
			Token() noexcept = default;
		};
		/**
		*\name
		*	Construction.
		*/
		/**@{*/
		CRG_API FramePassGroup( FrameGraph & graph
			, uint32_t id
			, std::string const & name
			, Token token );
		CRG_API FramePassGroup( FramePassGroup & parent
			, uint32_t id
			, std::string const & name
			, Token token );
		/**@}*/

	public:
		/**
		*\name
		*	Passes.
		*/
		/**@{*/
		CRG_API FramePass & createPass( std::string const & name
			, RunnablePassCreator runnableCreator );
		CRG_API FramePassGroup & createPassGroup( std::string const & name );
		CRG_API bool hasPass( std::string const & name )const;
		CRG_API void listPasses( FramePassArray & result )const;
		/**@}*/
		/**
		*\name
		*	Group I/O.
		*/
		/**@{*/
		CRG_API void addGroupInput( ImageViewId view );
		CRG_API void addGroupOutput( ImageViewId view );
		/**@}*/
		/**
		*\name
		*	Graph interface.
		*/
		/**@{*/
		/**
		*\copydoc crg::FrameGraph::getFinalLayoutState
		*/
		CRG_API LayoutState getFinalLayoutState( ImageViewId view
			, uint32_t passIndex = 0u )const;
		/**
		*\copydoc crg::FrameGraph::createBuffer
		*/
		CRG_API BufferId createBuffer( BufferData const & img )const;
		/**
		*\copydoc crg::FrameGraph::createView
		*/
		CRG_API BufferViewId createView( BufferViewData const & view )const;
		/**
		*\copydoc crg::FrameGraph::createImage
		*/
		CRG_API ImageId createImage( ImageData const & img )const;
		/**
		*\copydoc crg::FrameGraph::createView
		*/
		CRG_API ImageViewId createView( ImageViewData const & view )const;
		/**
		*\copydoc crg::FrameGraph::addInput
		*/
		CRG_API void addInput( ImageId image
			, ImageViewType viewType
			, ImageSubresourceRange const & range
			, LayoutState const & outputLayout );
		/**
		*\copydoc crg::FrameGraph::addInput
		*/
		CRG_API void addInput( ImageViewId view
			, LayoutState const & outputLayout );
		/**
		*\copydoc crg::FrameGraph::addOutput
		*/
		CRG_API void addOutput( ImageId image
			, ImageViewType viewType
			, ImageSubresourceRange const & range
			, LayoutState const & outputLayout );
		/**
		*\copydoc crg::FrameGraph::addOutput
		*/
		CRG_API void addOutput( ImageViewId view
			, LayoutState const & outputLayout );
		/**
		*\copydoc crg::FrameGraph::mergeViews
		*/
		CRG_API ImageViewId mergeViews( ImageViewIdArray const & views
			, bool mergeMipLevels = true
			, bool mergeArrayLayers = true );
		/**
		*\copydoc crg::FrameGraph::mergeViews
		*/
		CRG_API BufferViewId mergeViews( BufferViewIdArray const & views );
		/**
		*\copydoc crg::FrameGraph::mergeAttachments
		*/
		CRG_API Attachment const * mergeAttachments( AttachmentArray const & attachments
			, bool mergeMipLevels = true
			, bool mergeArrayLayers = true );
		/**@}*/

		CRG_API std::string getFullName()const;

		std::string const & getName()const
		{
			return m_name;
		}

	public:
		uint32_t id;
		FramePassPtrArray passes;
		FramePassGroupPtrArray groups;
		FramePassGroup * parent{};

	private:
		std::string m_name;
		FrameGraph & m_graph;
		std::unordered_set< uint32_t > m_inputs;
		std::unordered_set< uint32_t > m_outputs;
	};
}
