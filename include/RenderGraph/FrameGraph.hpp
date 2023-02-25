/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "Attachment.hpp"
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
		CRG_API FrameGraph( FrameGraph const & ) = delete;
		CRG_API FrameGraph & operator=( FrameGraph const & ) = delete;
		CRG_API FrameGraph( FrameGraph && ) = default;
		CRG_API FrameGraph & operator=( FrameGraph && ) = delete;
		CRG_API explicit FrameGraph( ResourceHandler & handler
			, std::string name = "FrameGraph" );
		/**@}*/
		/**
		*\name
		*	Resource creation.
		*/
		/**@{*/
		CRG_API ImageId createImage( ImageData const & img );
		CRG_API ImageViewId createView( ImageViewData const & view );
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
			, VkImageViewType viewType
			, VkImageSubresourceRange range )const;
		CRG_API LayoutState getFinalLayoutState( ImageViewId view )const;
		CRG_API AccessState getFinalAccessState( Buffer const & buffer )const;
		CRG_API void addInput( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange range
			, LayoutState outputLayout );
		CRG_API void addInput( ImageViewId view
			, LayoutState outputLayout );
		CRG_API LayoutState getInputLayoutState( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange range )const;
		CRG_API LayoutState getInputLayoutState( ImageViewId view )const;
		CRG_API void addOutput( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange range
			, LayoutState outputLayout );
		CRG_API void addOutput( ImageViewId view
			, LayoutState outputLayout );
		CRG_API LayoutState getOutputLayoutState( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange range )const;
		CRG_API LayoutState getOutputLayoutState( ImageViewId view )const;

		ResourceHandler & getHandler()
		{
			return m_handler;
		}

		std::string const & getName()const
		{
			return m_name;
		}

		FrameGraphArray const & getDependencies()const
		{
			return m_depends;
		}

		RecordContext const & getFinalStates()const
		{
			return m_finalState;
		}

		FramePassGroup & getDefaultGroup()const
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
		ImageIdAliasMap m_imageAliases;
		ImageViewIdAliasMap m_imageViewAliases;
		std::set< ImageId > m_images;
		std::set< ImageViewId > m_imageViews;
		std::map< std::string, ImageViewId > m_attachViews;
		RecordContext m_finalState;
		FrameGraphArray m_depends;
		LayerLayoutStatesHandler m_inputs;
		LayerLayoutStatesHandler m_outputs;
	};
}
