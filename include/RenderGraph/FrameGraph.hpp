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
		CRG_API LayoutState const & getFinalLayoutState( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & range )const;
		CRG_API LayoutState getFinalLayoutState( ImageViewId view
			, uint32_t passIndex = 0u )const;
		CRG_API AccessState const & getFinalAccessState( Buffer const & buffer
			, uint32_t passIndex = 0u )const;
		CRG_API void addInput( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & range
			, LayoutState const & outputLayout );
		CRG_API void addInput( ImageViewId view
			, LayoutState const & outputLayout );
		CRG_API LayoutState const & getInputLayoutState( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & range )const;
		CRG_API LayoutState const & getInputLayoutState( ImageViewId view )const;
		CRG_API void addOutput( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & range
			, LayoutState const & outputLayout );
		CRG_API void addOutput( ImageViewId view
			, LayoutState const & outputLayout );
		CRG_API LayoutState const & getOutputLayoutState( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & range )const;
		CRG_API LayoutState const & getOutputLayoutState( ImageViewId view )const;

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
		ImageIdAliasMap m_imageAliases;
		ImageViewIdAliasMap m_imageViewAliases;
		std::set< ImageId > m_images;
		std::set< ImageViewId > m_imageViews;
		std::map< std::string, ImageViewId, std::less<> > m_attachViews;
		RecordContext m_finalState;
		FrameGraphArray m_depends;
		LayerLayoutStatesHandler m_inputs;
		LayerLayoutStatesHandler m_outputs;
	};
}
