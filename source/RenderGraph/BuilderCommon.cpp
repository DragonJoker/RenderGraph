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
		FramePassSet retrieveRoots( FramePassDependenciesMap const & dependencies )
		{
			FramePassSet result;
			// Retrieve passes for which no transition is set.
			for ( auto & depsIt : dependencies )
			{
				if ( depsIt.second.empty() )
				{
					result.insert( depsIt.first );
				}
			}

			return result;
		}

		FramePassSet retrieveLeafs( FramePassDependenciesMap const & dependencies )
		{
			FramePassSet result;
			// Retrieve passes that are not listed in other passes' transitions source.
			for ( auto & depsIt : dependencies )
			{
				auto it = std::find_if( dependencies.begin()
					, dependencies.end()
					, [&depsIt]( FramePassDependenciesMap::value_type const & lookup )
					{
						return lookup.first != depsIt.first
							&& lookup.second.end() != std::find_if( lookup.second.begin()
								, lookup.second.end()
								, [&depsIt]( AttachmentTransition const & lookup )
								{
									return lookup.srcAttach.pass == depsIt.first;
								} );
					} );

				if ( it == dependencies.end() )
				{
					result.insert( depsIt.first );
				}
			}

			return result;
		}
	}
}
