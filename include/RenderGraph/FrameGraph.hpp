/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "Attachment.hpp"
#include "BufferData.hpp"
#include "BufferViewData.hpp"
#include "FramePassGroup.hpp"
#include "GraphNode.hpp"
#include "ImageData.hpp"
#include "ImageViewData.hpp"
#include "RecordContext.hpp"

#include <map>
#include <vector>

namespace crg
{
	class FrameGraph
	{
		friend class RunnableGraph;

	public:
		/**
		*\name
		*	Construction/Destruction.
		*/
		/**@{*/
		FrameGraph( FrameGraph const & ) = delete;
		FrameGraph & operator=( FrameGraph const & ) = delete;
		FrameGraph & operator=( FrameGraph && )noexcept = delete;
		CRG_API FrameGraph( FrameGraph && )noexcept = default;
		CRG_API ~FrameGraph()noexcept = default;
		CRG_API explicit FrameGraph( ResourceHandler & handler
			, std::string name = "FrameGraph" );
		/**@}*/
		/**
		*\name
		*	Resource creation.
		*/
		/**@{*/
		CRG_API BufferId createBuffer( BufferData const & img );
		CRG_API BufferViewId createView( BufferViewData const & view );
		CRG_API ImageId createImage( ImageData const & img );
		CRG_API ImageViewId createView( ImageViewData const & view );
		/**@}*/
		/**
		*\name
		*	Views merging.
		*/
		/**@{*/
		/**
		*\brief
		*	Creates a view which represents the given views merging.
		*/
		CRG_API ImageViewId mergeViews( ImageViewIdArray const & views
			, bool mergeMipLevels = true
			, bool mergeArrayLayers = true );
		/**
		*\brief
		*	Creates a view which represents the given views merging.
		*/
		CRG_API BufferViewId mergeViews( BufferViewIdArray const & views );
		/**@}*/
		/**
		*\name
		*	Attachments merging.
		*/
		/**@{*/
		/**
		*\brief
		*	Creates a view which represents the given views merging.
		*/
		CRG_API Attachment const * mergeAttachments( AttachmentArray const & attachments
			, bool mergeMipLevels = true
			, bool mergeArrayLayers = true );
		/**@}*/
		/**
		*\name
		*	Passes and groups.
		*/
		/**@{*/
		CRG_API FramePass & createPass( std::string const & name
			, RunnablePassCreator runnableCreator );
		CRG_API FramePassGroup & createPassGroup( std::string const & name );
		/**@}*/
		/**
		*\name
		*	Compilation.
		*/
		/**@{*/
		CRG_API RunnableGraphPtr compile( GraphContext & context );
		/**@}*/
		/**
		*\name
		*	Dependencies.
		*/
		/**@[*/
		void addDependency( FrameGraph const & pgraph )
		{
			m_depends.push_back( &pgraph );
		}
		/**@}*/
		/**
		*\name
		*	Getters.
		*/
		/**@{*/
		CRG_API LayoutState getFinalLayoutState( ImageId image
			, ImageViewType viewType
			, ImageSubresourceRange const & range )const;
		CRG_API LayoutState getFinalLayoutState( ImageViewId view
			, uint32_t passIndex = 0u )const;
		CRG_API AccessState const & getFinalAccessState( BufferId buffer
			, BufferSubresourceRange const & range )const;
		CRG_API AccessState const & getFinalAccessState( BufferViewId view
			, uint32_t passIndex = 0u )const;
		CRG_API void addInput( ImageId image
			, ImageViewType viewType
			, ImageSubresourceRange const & range
			, LayoutState const & outputLayout );
		CRG_API void addInput( ImageViewId view
			, LayoutState const & outputLayout );
		CRG_API LayoutState getInputLayoutState( ImageId image
			, ImageViewType viewType
			, ImageSubresourceRange const & range )const;
		CRG_API LayoutState getInputLayoutState( ImageViewId view )const;
		CRG_API void addOutput( ImageId image
			, ImageViewType viewType
			, ImageSubresourceRange const & range
			, LayoutState const & outputLayout );
		CRG_API void addOutput( ImageViewId view
			, LayoutState const & outputLayout );
		CRG_API LayoutState getOutputLayoutState( ImageId image
			, ImageViewType viewType
			, ImageSubresourceRange const & range )const;
		CRG_API LayoutState getOutputLayoutState( ImageViewId view )const;
		CRG_API LayerLayoutStatesMap const & getOutputLayoutStates()const;

		ResourceHandler & getHandler()noexcept
		{
			return m_handler;
		}

		std::string const & getName()const noexcept
		{
			return m_name;
		}

		FrameGraphArray const & getDependencies()const noexcept
		{
			return m_depends;
		}

		RecordContext const & getFinalStates()const noexcept
		{
			return m_finalState;
		}

		FramePassGroup & getDefaultGroup()const noexcept
		{
			return *m_defaultGroup;
		}
		/**@}*/

	private:
		void registerFinalState( RecordContext const & context );

	private:
		ResourceHandler & m_handler;
		std::string m_name;
		FramePassGroupPtr m_defaultGroup;
		std::set< BufferId > m_buffers;
		std::set< BufferViewId > m_bufferViews;
		std::set< ImageId > m_images;
		std::set< ImageViewId > m_imageViews;
		std::map< std::string, ImageViewId, std::less<> > m_attachViews;
		RecordContext m_finalState;
		FrameGraphArray m_depends;
		LayerLayoutStatesHandler m_inputs;
		LayerLayoutStatesHandler m_outputs;
		std::unordered_map< size_t, AttachmentPtr > m_mergedAttachments;
	};
}
