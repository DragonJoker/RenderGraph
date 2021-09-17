/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "BuilderCommon.hpp"

#include "RenderGraph/AttachmentTransition.hpp"
#include "RenderGraph/FramePassDependencies.hpp"
#include "RenderGraph/FramePass.hpp"

#include <algorithm>

namespace crg
{
	namespace builder
	{
		FramePassSet retrieveRoots( FramePassDependencies const & dependencies )
		{
			FramePassSet result;
			uint32_t dstCount = 0u;

			// Retrieve passes for which no transition is set.
			for ( auto & depsIt : dependencies )
			{
				if ( depsIt.transitions.viewTransitions.empty()
					&& depsIt.transitions.bufferTransitions.empty() )
				{
					result.insert( depsIt.pass );
				}
				else
				{
					dstCount += uint32_t( std::count_if( depsIt.transitions.viewTransitions.begin()
						, depsIt.transitions.viewTransitions.end()
						, []( ViewTransition const & lookup )
						{
							return lookup.inputAttach.getFlags() != 0;
						} ) );
					dstCount += uint32_t( std::count_if( depsIt.transitions.bufferTransitions.begin()
						, depsIt.transitions.bufferTransitions.end()
						, []( BufferTransition const & lookup )
						{
							return lookup.inputAttach.getFlags() != 0;
						} ) );
				}
			}

			if ( !dstCount )
			{
				for ( auto & depsIt : dependencies )
				{
					result.insert( depsIt.pass );
				}
			}

			return result;
		}

		FramePassSet retrieveLeafs( FramePassDependencies const & dependencies )
		{
			FramePassSet result;
			// Retrieve passes that are not listed in other passes' transitions source.
			for ( auto & depsIt : dependencies )
			{
				auto it = std::find_if( dependencies.begin()
					, dependencies.end()
					, [&depsIt]( FramePassTransitions const & lookup )
					{
						return lookup.pass != depsIt.pass
							&& ( lookup.transitions.viewTransitions.end() != std::find_if( lookup.transitions.viewTransitions.begin()
								, lookup.transitions.viewTransitions.end()
								, [&depsIt]( ViewTransition const & ilookup )
								{
									return ilookup.outputAttach.pass == depsIt.pass;
								} )
							|| lookup.transitions.bufferTransitions.end() != std::find_if( lookup.transitions.bufferTransitions.begin()
								, lookup.transitions.bufferTransitions.end()
								, [&depsIt]( BufferTransition const & ilookup )
								{
									return ilookup.outputAttach.pass == depsIt.pass;
								} ) );
					} );

				if ( it == dependencies.end() )
				{
					result.insert( depsIt.pass );
				}
			}

			return result;
		}
	}
}
