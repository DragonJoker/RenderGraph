/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/AttachmentTransition.hpp"

#include "RenderGraph/FramePass.hpp"

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

		static ViewTransitionArray reduceDirectPathsT( ViewTransitionArray transitions )
		{
			// Currently, just removes from the transitions the sampled attachments to a pass
			// that doesn't directly need them.
			auto it = std::remove_if( transitions.begin()
				, transitions.end()
				, []( ViewTransition const & transition )
				{
					bool result = false;

					if ( transition.inputAttach.isSampledView() )
					{
						auto inputPass = *transition.inputAttach.pass;
						auto pit = std::find_if( inputPass.images.begin()
							, inputPass.images.end()
							, [&transition]( Attachment const & lookup )
							{
								return lookup.isSampledView()
									&& lookup == transition.inputAttach;
							} );
						result = pit == inputPass.images.end();
					}

					return result;
				} );
			transitions.erase( it, transitions.end() );
			return transitions;
		}

		static BufferTransitionArray reduceDirectPathsT( BufferTransitionArray transitions )
		{
			// Currently, just removes from the transitions the sampled attachments to a pass
			// that doesn't directly need them.
			auto it = std::remove_if( transitions.begin()
				, transitions.end()
				, []( BufferTransition const & transition )
				{
					bool result = false;

					if ( transition.inputAttach.isStorageBuffer() )
					{
						auto inputPass = *transition.inputAttach.pass;
						auto pit = std::find_if( inputPass.buffers.begin()
							, inputPass.buffers.end()
							, [&transition]( Attachment const & lookup )
							{
								return lookup.isStorageBuffer()
									&& lookup == transition.inputAttach;
							} );
						result = pit == inputPass.buffers.end();
					}

					return result;
				} );
			transitions.erase( it, transitions.end() );
			return transitions;
		}
	}

	bool operator==( ViewTransition const & lhs, ViewTransition const & rhs )
	{
		return match( *lhs.data.data, *rhs.data.data )
			&& lhs.outputAttach == rhs.outputAttach
			&& lhs.inputAttach == rhs.inputAttach;
	}

	bool operator==( BufferTransition const & lhs, BufferTransition const & rhs )
	{
		return lhs.data == rhs.data
			&& lhs.outputAttach == rhs.outputAttach
			&& lhs.inputAttach == rhs.inputAttach;
	}

	bool operator==( Buffer const & lhs, Buffer const & rhs )
	{
		return lhs.buffer == rhs.buffer;
	}

	AttachmentTransitions mergeIdenticalTransitions( AttachmentTransitions transitions )
	{
		return { attTran::mergeIdenticalTransitionsT( transitions.viewTransitions )
			, attTran::mergeIdenticalTransitionsT( transitions.bufferTransitions ) };
	}

	AttachmentTransitions reduceDirectPaths( AttachmentTransitions transitions )
	{
		return { attTran::reduceDirectPathsT( transitions.viewTransitions )
			, attTran::reduceDirectPathsT( transitions.bufferTransitions ) };
	}
}
