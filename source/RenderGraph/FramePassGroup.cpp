/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/FramePassGroup.hpp"

#include "RenderGraph/Exception.hpp"
#include "RenderGraph/Log.hpp"
#include "RenderGraph/FrameGraph.hpp"

#include <numeric>
#include <array>

namespace crg
{
	namespace group
	{
		static FramePassGroup const * getOutermost( FramePassGroup const * group )
		{
			while ( group && group->parent )
				group = group->parent;
			return group;
		}

		static uint32_t countPasses( FramePassGroup const * group )
		{
			return std::accumulate( group->groups.begin()
				, group->groups.end()
				, uint32_t( group->passes.size() )
				, []( uint32_t val, FramePassGroupPtr const & lookup )
				{
					return val + countPasses( lookup.get() );
				} );
		}

		static uint32_t countGroups( FramePassGroup const * group )
		{
			return std::accumulate( group->groups.begin()
				, group->groups.end()
				, uint32_t( group->groups.size() )
				, []( uint32_t val, FramePassGroupPtr const & lookup )
				{
					return val + countGroups( lookup.get() );
				} );
		}
	}

	FramePassGroup::FramePassGroup( FrameGraph & graph
		, uint32_t pid
		, std::string const & name )
		: id{ pid }
		, m_name{ name }
		, m_graph{ graph }
	{
	}

	FramePassGroup::FramePassGroup( FramePassGroup & pparent
		, uint32_t pid
		, std::string const & name )
		: id{ pid }
		, parent{ &pparent }
		, m_name{ name }
		, m_graph{ parent->m_graph }
	{
	}

	FramePass & FramePassGroup::createPass( std::string const & passName
		, RunnablePassCreator runnableCreator )
	{
		if ( hasPass( passName ) )
		{
			Logger::logWarning( "Duplicate FramePass name detected." );
			CRG_Exception( "Duplicate FramePass name detected." );
		}

		auto count = group::countPasses( group::getOutermost( this ) );
		passes.emplace_back( new FramePass{ *this
			, m_graph
			, count + 1u
			, passName
			, std::move( runnableCreator ) } );
		return *passes.back();
	}

	FramePassGroup & FramePassGroup::createPassGroup( std::string const & groupName )
	{
		auto it = std::find_if( groups.begin()
			, groups.end()
			, [&groupName]( FramePassGroupPtr const & lookup )
			{
				return lookup->getName() == groupName;
			} );

		if ( it == groups.end() )
		{
			auto count = group::countGroups( group::getOutermost( this ) );
			groups.emplace_back( new FramePassGroup{ *this, count + 1u, groupName } );
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
				return lookup->getName() == passName;
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

	void FramePassGroup::addGroupInput( ImageViewId view )
	{
		m_inputs.emplace( view.id );
	}

	void FramePassGroup::addGroupOutput( ImageViewId view )
	{
		m_outputs.emplace( view.id );
	}

	LayoutState FramePassGroup::getFinalLayoutState( ImageViewId view
		, uint32_t passIndex )const
	{
		return m_graph.getFinalLayoutState( view, passIndex );
	}

	ImageId FramePassGroup::createImage( ImageData const & img )const
	{
		return m_graph.createImage( img );
	}

	ImageViewId FramePassGroup::createView( ImageViewData const & view )const
	{
		return m_graph.createView( view );
	}

	void FramePassGroup::addInput( ImageId image
		, ImageViewType viewType
		, ImageSubresourceRange const & range
		, LayoutState const & outputLayout )
	{
		m_graph.addInput( image
			, viewType
			, range
			, outputLayout );
	}

	void FramePassGroup::addInput( ImageViewId view
		, LayoutState const & outputLayout )
	{
		addInput( view.data->image
			, view.data->info.viewType
			, view.data->info.subresourceRange
			, outputLayout );
	}

	void FramePassGroup::addOutput( ImageId image
		, ImageViewType viewType
		, ImageSubresourceRange const & range
		, LayoutState const & outputLayout )
	{
		m_graph.addOutput( image
			, viewType
			, range
			, outputLayout );
	}

	void FramePassGroup::addOutput( ImageViewId view
		, LayoutState const & outputLayout )
	{
		addOutput( view.data->image
			, view.data->info.viewType
			, view.data->info.subresourceRange
			, outputLayout );
	}

	std::string FramePassGroup::getFullName()const
	{
		return ( &m_graph.getDefaultGroup() == this )
			? m_graph.getName()
			: parent->getFullName() + "/" + getName();
	}
}
