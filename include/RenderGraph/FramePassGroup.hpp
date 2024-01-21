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
	protected:
		friend class FrameGraph;
		/**
		*\name
		*	Construction.
		*/
		/**@[*/
		CRG_API FramePassGroup( FrameGraph & graph
			, uint32_t id
			, std::string const & name );
		CRG_API FramePassGroup( FramePassGroup & parent
			, uint32_t id
			, std::string const & name );
		/**@}*/

	public:
		/**
		*\name
		*	Passes.
		*/
		/**@[*/
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
		/**@[*/
		CRG_API void addGroupInput( ImageViewId view );
		CRG_API void addGroupOutput( ImageViewId view );
		/**@}*/
		/**
		*\name
		*	Graph interface.
		*/
		/**@[*/
		CRG_API ResourceHandler & getHandler()const;
		CRG_API LayoutState getFinalLayoutState( ImageViewId view
			, uint32_t passIndex = 0u )const;
		CRG_API AccessState const & getFinalAccessState( Buffer const & buffer
			, uint32_t passIndex = 0u )const;
		CRG_API ImageId createImage( ImageData const & img )const;
		CRG_API ImageViewId createView( ImageViewData const & view )const;
		CRG_API void addInput( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & range
			, LayoutState const & outputLayout );
		CRG_API void addInput( ImageViewId view
			, LayoutState const & outputLayout );
		CRG_API LayoutState getInputLayoutState( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & range )const;
		CRG_API LayoutState getInputLayoutState( ImageViewId view )const;
		CRG_API void addOutput( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & range
			, LayoutState const & outputLayout );
		CRG_API void addOutput( ImageViewId view
			, LayoutState const & outputLayout );
		CRG_API LayoutState getOutputLayoutState( ImageId image
			, VkImageViewType viewType
			, VkImageSubresourceRange const & range )const;
		CRG_API LayoutState getOutputLayoutState( ImageViewId view )const;
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
