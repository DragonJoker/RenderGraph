/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/AttachmentTransition.hpp"

#include "RenderGraph/FramePass.hpp"

#include <algorithm>

namespace crg
{	
	bool operator==( AttachmentTransition const & lhs, AttachmentTransition const & rhs )
	{
		return lhs.view == rhs.view
			&& lhs.srcAttach == rhs.srcAttach
			&& lhs.dstAttach == rhs.dstAttach;
	}

	AttachmentTransitionArray mergeIdenticalTransitions( AttachmentTransitionArray transitions )
	{
		AttachmentTransitionArray result;

		for ( auto & transition : transitions )
		{
			auto it = std::find( result.begin(), result.end(), transition );

			if ( it == result.end() )
			{
				result.push_back( transition );
			}
		}

		return result;
	}

	AttachmentTransitionArray reduceDirectPaths( AttachmentTransitionArray transitions )
	{
		// Currently, just removes from the transitions the sampled attachments to a pass
		// that doesn't directly need them.
		auto it = std::remove_if( transitions.begin()
			, transitions.end()
			, []( AttachmentTransition const & transition )
			{
				bool result = false;

				if ( transition.dstAttach.isSampled() )
				{
					auto inputPass = *transition.dstAttach.pass;
					auto it = std::find( inputPass.sampled.begin()
						, inputPass.sampled.end()
						, transition.dstAttach );
					result = it == inputPass.sampled.end();
				}

				return result;
			} );
		transitions.erase( it, transitions.end() );
		return transitions;
	}
}
