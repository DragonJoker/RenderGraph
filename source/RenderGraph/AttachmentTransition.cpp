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
				auto it = std::find( result.begin(), result.end(), transition );

				if ( it == result.end() )
				{
					result.push_back( transition );
				}
			}

			return result;
		}
	}

	bool operator==( Buffer const & lhs, Buffer const & rhs )
	{
		return lhs.m_buffers == rhs.m_buffers;
	}

	AttachmentTransitions mergeIdenticalTransitions( AttachmentTransitions transitions )
	{
		AttachmentTransitions result{};
		result.viewTransitions = attTran::mergeIdenticalTransitionsT( std::move( transitions.viewTransitions ) );
		result.bufferTransitions = attTran::mergeIdenticalTransitionsT( std::move( transitions.bufferTransitions ) );
		return result;
	}
}
