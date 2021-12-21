/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "Attachment.hpp"
#include "ImageData.hpp"
#include "ImageViewData.hpp"
#include "GraphNode.hpp"
#include "FramePass.hpp"

#include <map>
#include <vector>

namespace crg
{
	class RecordContext
	{
	public:
		/**
		*\name	Images
		*/
		//@{
		CRG_API void setLayoutState( crg::ImageViewId view
			, LayoutState layoutState );
		CRG_API void setWantedState( ImageViewId view
			, LayoutState layoutState
			, bool needsClear = false );
		CRG_API LayoutState getLayoutState( ImageViewId view )const;

		CRG_API void setLayoutState( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & subresourceRange
			, LayoutState layoutState );
		CRG_API void setWantedState( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & subresourceRange
			, LayoutState layoutState
			, bool needsClear = false );
		CRG_API LayoutState getLayoutState( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & subresourceRange )const;
		//@}
		/**
		*\name	Buffers
		*/
		//@{
		CRG_API void setAccessState( VkBuffer buffer
			, BufferSubresourceRange const & subresourceRange
			, AccessState layoutState );
		CRG_API void setWantedState( VkBuffer buffer
			, BufferSubresourceRange const & subresourceRange
			, AccessState layoutState
			, bool needsClear = false );
		CRG_API AccessState getAccessState( VkBuffer buffer
			, BufferSubresourceRange const & subresourceRange )const;
		//@}

	private:
		std::map< uint32_t, LayerLayoutStates > m_images;
		std::map< uint32_t, LayerLayoutStates > m_imagesWanted;
		AccessStateMap m_buffers;
		AccessStateMap m_buffersWanted;
	};

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
		CRG_API FrameGraph( ResourceHandler & handler
			, std::string name = "FrameGraph" );
		/**@}*/
		/**
		*\name
		*	Resource creation.
		*/
		/**@{*/
		CRG_API ImageId createImage( ImageData const & img );
		CRG_API ImageViewId createView( ImageViewData const & view );
		CRG_API FramePass & createPass( std::string const & name
			, RunnablePassCreator runnableCreator );
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
		*	Getters.
		*/
		/**@{*/
		CRG_API LayoutState getFinalLayoutState( ImageViewId view )const;
		CRG_API AccessState getFinalAccessState( Buffer const & buffer )const;

		ResourceHandler & getHandler()
		{
			return m_handler;
		}

		std::string const & getName()const
		{
			return m_name;
		}
		/**@}*/

	private:
		void registerFinalState( RecordContext const & context );

	private:
		ResourceHandler & m_handler;
		std::string m_name;
		FramePassPtrArray m_passes;
		ImageIdAliasMap m_imageAliases;
		ImageViewIdAliasMap m_imageViewAliases;
		std::set< ImageId > m_images;
		std::set< ImageViewId > m_imageViews;
		std::map< std::string, ImageViewId > m_attachViews;
		RecordContext m_finalState;
	};
}
