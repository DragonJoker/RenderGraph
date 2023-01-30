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
		*	Graph interface.
		*/
		/**@[*/
		CRG_API ResourceHandler & getHandler()const;
		CRG_API LayoutState getFinalLayoutState( ImageViewId view )const;
		CRG_API AccessState getFinalAccessState( Buffer const & buffer )const;
		CRG_API ImageId createImage( ImageData const & img )const;
		CRG_API ImageViewId createView( ImageViewData const & view )const;
		CRG_API void addOutput( ImageViewId view );

		std::unordered_set< uint32_t > const & getOutputs()const
		{
			return m_outputs;
		}
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
		std::unordered_set< uint32_t > m_outputs;
	};
}
