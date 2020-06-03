/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/AttachmentTransition.hpp"

#include "RenderGraph/RenderPass.hpp"

namespace crg
{
	bool operator==( AttachmentPasses const & lhs, AttachmentPasses const & rhs )
	{
		return lhs.attachment == rhs.attachment
			&& lhs.passes == rhs.passes;
	}
	
	bool operator==( AttachmentTransition const & lhs, AttachmentTransition const & rhs )
	{
		return lhs.srcOutputs == rhs.srcOutputs
			&& lhs.dstInput == rhs.dstInput;
	}

	AttachmentTransitionArray mergeIdenticalTransitions( AttachmentTransitionArray transitions )
	{
		// Merges the transitions for which the attachements (first in, and out) are the same.
		AttachmentTransitionArray result;
		auto findSrcAttach = [&result]( auto it )
		{
			return std::find_if( result.begin()
				, result.end()
				, [&it]( AttachmentTransition const & lookup )
				{
					return lookup.srcOutputs.front().attachment == it->srcOutputs.front().attachment
						&& lookup.dstInput.attachment == it->dstInput.attachment;
				} );
		};
		auto it = transitions.begin();

		while ( it != transitions.end() )
		{
			auto itr = findSrcAttach( it );

			if ( itr == result.end() )
			{
				result.push_back( std::move( *it ) );
				++it;
			}
			else
			{
				for ( auto pass : it->srcOutputs.front().passes )
				{
					itr->srcOutputs.front().passes.insert( pass );
				}

				for ( auto pass : it->dstInput.passes )
				{
					itr->dstInput.passes.insert( pass );
				}

				it = transitions.erase( it );
			}
		}

		return result;
	}

	AttachmentTransitionArray mergeTransitionsPerInput( AttachmentTransitionArray transitions )
	{
		AttachmentTransitionArray result;
		auto findSrcAttach = [&result]( auto it )
		{
			return std::find_if( result.begin()
				, result.end()
				, [&it]( AttachmentTransition const & lookup )
				{
					return lookup.dstInput.attachment == it->dstInput.attachment;
				} );
		};
		auto it = transitions.begin();

		while ( it != transitions.end() )
		{
			auto itr = findSrcAttach( it );

			if ( itr == result.end() )
			{
				result.push_back( std::move( *it ) );
				++it;
			}
			else
			{
				itr->srcOutputs.insert( itr->srcOutputs.end()
					, it->srcOutputs.begin()
					, it->srcOutputs.end() );
				it = transitions.erase( it );
			}
		}

		return result;
	}

	AttachmentTransitionArray reduceDirectPaths( AttachmentTransitionArray transitions )
	{
		// Currently, just removes from the transitions the sampled attachments to a pass
		// that doesn't directly need them.
		for ( auto & transition : transitions )
		{
			if ( transition.dstInput.attachment.isSampled() )
			{
				auto passIt = transition.dstInput.passes.begin();

				while ( passIt != transition.dstInput.passes.end() )
				{
					auto inputPass = *passIt;
					auto it = std::find( inputPass->sampled.begin()
						, inputPass->sampled.end()
						, transition.dstInput.attachment );

					if ( it == inputPass->sampled.end() )
					{
						passIt = transition.dstInput.passes.erase( passIt );
					}
					else
					{
						++passIt;
					}
				}
			}
		}

		return transitions;
	}
}
