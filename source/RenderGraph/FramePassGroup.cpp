/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/FramePassGroup.hpp"

#include "RenderGraph/Exception.hpp"
#include "RenderGraph/FrameGraph.hpp"

#include <array>

namespace crg
{
	FramePassGroup::FramePassGroup( FrameGraph & graph
		, std::string const & pname )
		: name{ pname }
		, m_graph{ graph }
	{
	}

	FramePassGroup::FramePassGroup( FramePassGroup & pparent
		, std::string const & pname )
		: name{ pname }
		, parent{ &pparent }
		, m_graph{ parent->m_graph }
	{
	}

	FramePass & FramePassGroup::createPass( std::string const & passName
		, RunnablePassCreator runnableCreator )
	{
		if ( hasPass( passName ) )
		{
			CRG_Exception( "Duplicate FramePass name detected." );
		}

		passes.emplace_back( new FramePass{ *this
			, m_graph
			, uint32_t( passes.size() + 1u )
			, passName
			, runnableCreator } );
		return *passes.back();
	}

	FramePassGroup & FramePassGroup::createPassGroup( std::string const & groupName )
	{
		auto it = std::find_if( groups.begin()
			, groups.end()
			, [&groupName]( FramePassGroupPtr const & lookup )
			{
				return lookup->name == groupName;
			} );

		if ( it == groups.end() )
		{
			groups.emplace_back( new FramePassGroup{ *this, groupName } );
			it = std::next( groups.begin(), ptrdiff_t( groups.size() - 1u ) );
		}

		return **it;
	}

	bool FramePassGroup::hasPass( std::string const & passName )const
	{
		return passes.end() != std::find_if( passes.begin()
			, passes.end()
			, [&passName]( FramePassPtr const & lookup )
			{
				return lookup->name == passName;
			} );
	}

	void FramePassGroup::listPasses( FramePassArray & result )const
	{
		for ( auto & pass : passes )
		{
			result.push_back( pass.get() );
		}

		for ( auto & group : groups )
		{
			group->listPasses( result );
		}
	}

	ResourceHandler & FramePassGroup::getHandler()const
	{
		return m_graph.getHandler();
	}

	std::string FramePassGroup::getName()const
	{
		return m_graph.getName() + "/" + name;
	}

	LayoutState FramePassGroup::getFinalLayoutState( ImageViewId view )const
	{
		return m_graph.getFinalLayoutState( view );
	}

	AccessState FramePassGroup::getFinalAccessState( Buffer const & buffer )const
	{
		return m_graph.getFinalAccessState( buffer );
	}

	ImageId FramePassGroup::createImage( ImageData const & img )const
	{
		return m_graph.createImage( img );
	}

	ImageViewId FramePassGroup::createView( ImageViewData const & view )const
	{
		return m_graph.createView( view );
	}
}
