/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FramePass.hpp"

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
			, std::string const & name );
		CRG_API FramePassGroup( FramePassGroup & parent
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
		*	Graph interface.
		*/
		/**@[*/
		CRG_API ResourceHandler & getHandler()const;
		CRG_API std::string getName()const;
		CRG_API LayoutState getFinalLayoutState( ImageViewId view )const;
		CRG_API AccessState getFinalAccessState( Buffer const & buffer )const;
		CRG_API ImageId createImage( ImageData const & img )const;
		CRG_API ImageViewId createView( ImageViewData const & view )const;
		/**@}*/

	public:
		std::string name;
		FramePassPtrArray passes;
		FramePassGroupPtrArray groups;
		FramePassGroup * parent{};

	private:
		FrameGraph & m_graph;
	};
}
