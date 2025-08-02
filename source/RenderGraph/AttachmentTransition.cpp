/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/AttachmentTransition.hpp"

#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/LayerLayoutStatesHandler.hpp"

#include <algorithm>

namespace crg
{
	namespace attTran
	{
		template< typename TransitionT >
		std::vector< TransitionT > mergeIdenticalTransitionsT( std::vector< TransitionT > transitions )
		{
			std::vector< TransitionT > result;

			for ( auto & transition : transitions )
			{
				if ( auto it = std::find( result.begin(), result.end(), transition );
					it == result.end() )
				{
					result.push_back( transition );
				}
			}

			return result;
		}
	}

	AttachmentTransitions mergeIdenticalTransitions( AttachmentTransitions transitions )
	{
		AttachmentTransitions result{};
		result.imageTransitions = attTran::mergeIdenticalTransitionsT( std::move( transitions.imageTransitions ) );
		result.bufferTransitions = attTran::mergeIdenticalTransitionsT( std::move( transitions.bufferTransitions ) );
		return result;
	}
}
