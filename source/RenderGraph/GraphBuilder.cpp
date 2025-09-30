/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "GraphBuilder.hpp"

#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/GraphNode.hpp"
#include "RenderGraph/Log.hpp"

#include <algorithm>

namespace crg::builder
{
	//*********************************************************************************************

	namespace endpoints
	{
		static void addAttach( Attachment const & attach
			, AttachmentArray & result )
		{
			if ( attach.source.empty() )
				result.push_back( &attach );
			for ( auto & source : attach.source )
				result.push_back( source.attach.get() );
		}

		static void listAllAttachs( FramePassArray const & passes
			, AttachmentArray & result )
		{
			for ( auto pass : passes )
			{
				for ( auto & [binding, attach] : pass->getInouts() )
					addAttach( *attach, result );
				for ( auto & [binding, attach] : pass->getOutputs() )
					addAttach( *attach, result );
				for ( auto & attach : pass->getTargets() )
					if ( attach->isOutput() )
						addAttach( *attach, result );
			}
		}

		static void listBuffersRec( Attachment const & attach
			, FramePass const * pass
			, BufferViewIdArray & result )
		{
			if ( attach.source.empty() && ( !pass || attach.pass == pass ) )
				for ( auto & view : attach.bufferAttach.buffers )
					result.push_back( view );

			for ( auto const & source : attach.source )
				listBuffersRec( *source.attach, pass, result );
		}

		static BufferViewIdArray listBuffers( Attachment const & attach
			, FramePass const * pass )
		{
			BufferViewIdArray result;
			listBuffersRec( attach, pass, result );
			return result;
		}

		static void listImagesRec( Attachment const & attach
			, FramePass const * pass
			, ImageViewIdArray & result )
		{
			if ( attach.source.empty() && ( !pass || attach.pass == pass ) )
				for ( auto & view : attach.imageAttach.views )
					result.push_back( view );

			for ( auto const & source : attach.source )
				listImagesRec( *source.attach, pass, result );
		}

		static ImageViewIdArray listImages( Attachment const & attach
			, FramePass const * pass )
		{
			ImageViewIdArray result;
			listImagesRec( attach, pass, result );
			return result;
		}

		static bool isBufferAttachParent( Attachment const & parent, Attachment const & attach )
		{
			auto parentBuffers = listBuffers( parent, attach.pass );
			auto attachBuffers = listBuffers( attach, nullptr );
			return std::any_of( attachBuffers.begin(), attachBuffers.end()
				, [&parentBuffers]( BufferViewId const & lookup )
				{
					return parentBuffers.end() != std::find( parentBuffers.begin(), parentBuffers.end(), lookup );
				} );
		}

		static bool isImageAttachParent( Attachment const & parent, Attachment const & attach )
		{
			auto parentBuffers = listImages( parent, attach.pass );
			auto attachBuffers = listImages( attach, nullptr );
			return std::any_of( attachBuffers.begin(), attachBuffers.end()
				, [&parentBuffers]( ImageViewId const & lookup )
				{
					return parentBuffers.end() != std::find( parentBuffers.begin(), parentBuffers.end(), lookup );
				} );
		}

		static bool isAttachParent( Attachment const * parent, Attachment const * attach )
		{
			if ( bool result = ( parent == attach );
				result || !parent )
				return result;

			if ( attach->isBuffer() != parent->isBuffer() )
				return false;

			if ( attach->isBuffer() )
				return isBufferAttachParent( *parent, *attach );

			return isImageAttachParent( *parent, *attach );
		}

		static bool isParentAttachForPass( Attachment const * attach, FramePass const * pass )
		{
			return pass->end() != std::find_if( pass->begin(), pass->end()
				, [&attach]( auto const & lookup )
				{
					return isAttachParent( lookup.second.parent, attach );
				} );
		}

		static void listNonParentAttachs( FramePassArray const & passes
			, AttachmentArray const & allAttachs
			, AttachmentArray & result )
		{
			for ( auto attach : allAttachs )
			{
				if ( std::none_of( passes.begin(), passes.end()
					, [&attach]( FramePass const * pass )
					{
						return attach->pass != pass
							&& isParentAttachForPass( attach, pass );
					} ) )
				{
					addAttach( *attach, result );
				}
			}
		}

		static bool hasOutput( FramePass const & pass )
		{
			auto result = ( !pass.getOutputs().empty() || !pass.getInouts().empty() );

			if ( !result )
			{
				for ( auto & attach : pass.getTargets() )
					result = result || attach->isOutput();
			}

			return result;
		}

		static void addPassInputs( FramePass const & pass
			, AttachmentArray & result )
		{
			for ( auto & [_, attach] : pass.getInputs() )
				addAttach( *attach, result );
			for ( auto & [_, attach] : pass.getUniforms() )
				addAttach( *attach, result );
			for ( auto & [_, attach] : pass.getSampled() )
				result.push_back( attach.attach );
			for ( auto & attach : pass.getTargets() )
				if ( attach->isInput() )
					addAttach( *attach, result );
		}

		static void addSinkPassInputs( FramePassArray const & passes
			, AttachmentArray & result )
		{
			for ( auto & pass : passes )
				if ( !hasOutput( *pass ) )
				{
					addPassInputs( *pass, result );
				}
		}

		static AttachmentArray listPassOutputs( FramePass const & pass )
		{
			AttachmentArray result;
			for ( auto & [_, attach] : pass.getOutputs() )
				addAttach( *attach, result );
			for ( auto & [_, attach] : pass.getInouts() )
				addAttach( *attach, result );
			for ( auto & attach : pass.getTargets() )
				if ( attach->isOutput() )
					addAttach( *attach, result );
			return result;
		}

		static bool areAllPassAttachsListed( FramePass const & pass
			, AttachmentArray const & result )
		{
			auto passOutputs = listPassOutputs( pass );
			return std::all_of( passOutputs.begin(), passOutputs.end()
				, [&result]( Attachment const * lookup )
				{

					return result.end() != std::find( result.begin(), result.end(), lookup );
				} );
		}

		static void removeMismatchs( AttachmentArray & result )
		{
			auto it = result.begin();

			while ( it != result.end() )
			{
				if ( !areAllPassAttachsListed( *( *it )->pass, result ) )
					it = result.erase( it );
				else
					++it;
			}
		}
	}

	//*********************************************************************************************

	namespace graph
	{
		struct AttachmentStates
		{
			bool separateDepthStencilLayouts;
			std::vector< ImageLayout > imageStates{};
			std::vector< AccessFlags > bufferStates{};
		};

		static FramePassArray listAttachmentPasses( Attachment const & attach )
		{
			if ( attach.pass )
				return { attach.pass };

			FramePassArray result;
			for ( auto const & source : attach.source )
				result.push_back( source.pass );
			return result;
		}

		static AttachmentArray splitImage( Attachment const & attach )
		{
			assert( attach.view().data->source.empty() );
			return { &attach };
		}

		static AttachmentArray splitBuffer( Attachment const & attach )
		{
			assert( attach.buffer().data->source.empty() );
			return { &attach };
		}

		static AttachmentArray splitAttach( Attachment const & attach )
		{
			if ( !attach.source.empty() )
			{
				AttachmentArray result;
				for ( auto & source : attach.source )
				{
					AttachmentArray splitSource = splitAttach( *source.attach );
					result.insert( result.end(), splitSource.begin(), splitSource.end() );
				}
				return result;
			}

			if ( attach.isImage() )
				return splitImage( attach );
			return splitBuffer( attach );
		}

		template< typename UIntT >
		static bool isInRange( UIntT value
			, UIntT offset, UIntT count )
		{
			return value >= offset && value < offset + count;
		}

		template< typename UIntT >
		static bool areIntersecting( UIntT lhsOffset, UIntT lhsCount
			, UIntT rhsOffset, UIntT rhsCount )
		{
			return isInRange( lhsOffset, rhsOffset, rhsCount )
				|| isInRange( rhsOffset, lhsOffset, lhsCount );
		}

		static bool areIntersecting( ImageSubresourceRange const & lhs
			, ImageSubresourceRange const & rhs )
		{
			return areIntersecting( lhs.baseMipLevel, lhs.levelCount, rhs.baseMipLevel, rhs.levelCount )
				&& areIntersecting( lhs.baseArrayLayer, lhs.layerCount, rhs.baseArrayLayer, lhs.layerCount );
		}

		static bool areIntersecting( BufferSubresourceRange const & lhs
			, BufferSubresourceRange const & rhs )
		{
			return areIntersecting( lhs.offset, lhs.size
				, rhs.offset, rhs.size );
		}

		static bool areOverlapping( ImageViewData const & lhs
			, ImageViewData const & rhs )
		{
			return lhs.image == rhs.image
				&& areIntersecting( getVirtualRange( lhs.image, lhs.info.viewType, lhs.info.subresourceRange )
					, getVirtualRange( rhs.image, rhs.info.viewType, rhs.info.subresourceRange ) );
		}

		static bool areOverlapping( BufferViewData const & lhs
			, BufferViewData const & rhs )
		{
			return lhs.buffer == rhs.buffer
				&& areIntersecting( lhs.info.subresourceRange
					, rhs.info.subresourceRange );
		}

		static bool areOverlapping( Attachment const & lhs
			, Attachment const & rhs )
		{
			if ( lhs.isImage() )
				return areOverlapping( *lhs.view().data, *rhs.view().data );
			return areOverlapping( *lhs.buffer().data, *rhs.buffer().data );
		}

		void traverseAttachmentPasses( GraphNode & parent
			, Attachment const * currentAttach
			, Attachment const * parentAttach
			, AttachmentTransitions & transitions
			, GraphNodePtrArray & nodes );

		static void traversePassAttach( FramePassNode & node, std::map< uint32_t, Attachment const * > const & attachments
			, AttachmentTransitions & transitions
			, GraphNodePtrArray & nodes )
		{
			for ( auto [binding, attach] : attachments )
				traverseAttachmentPasses( node
					, node.getFramePass().getParentAttachment( *attach ), attach
					, transitions, nodes );
		}

		static void traversePassAttach( FramePassNode & node, std::map< uint32_t, FramePass::SampledAttachment > const & attachments
			, AttachmentTransitions & transitions
			, GraphNodePtrArray & nodes )
		{
			for ( auto const & [binding, attach] : attachments )
				traverseAttachmentPasses( node
					, node.getFramePass().getParentAttachment( *attach.attach ), attach.attach
					, transitions, nodes );
		}

		static void traversePassAttach( FramePassNode & node, AttachmentArray const & targets, Attachment::Flag flag
			, AttachmentTransitions & transitions
			, GraphNodePtrArray & nodes )
		{
			for ( auto attach : targets )
			{
				if ( attach->hasFlag( flag ) )
					traverseAttachmentPasses( node
						, node.getFramePass().getParentAttachment( *attach ), attach
						, transitions, nodes );
			}
		}

		static void insertTransition( bool isImage
			, Attachment const * output
			, Attachment const * input
			, AttachmentTransitions & transitions )
		{
			if ( isImage )
			{
				if ( output && input )
					transitions.imageTransitions.emplace_back( output->view(), *output, *input );
				else if ( output )
					transitions.imageTransitions.emplace_back( output->view(), *output, Attachment::createDefault( output->view() ) );
				else if ( input )
					transitions.imageTransitions.emplace_back( input->view(), Attachment::createDefault( input->view() ), *input );
			}
			else
			{
				if ( output && input )
					transitions.bufferTransitions.emplace_back( output->buffer(), *output, *input );
				else if ( output )
					transitions.bufferTransitions.emplace_back( output->buffer(), *output, Attachment::createDefault( output->buffer() ) );
				else if ( input )
					transitions.bufferTransitions.emplace_back( input->buffer(), Attachment::createDefault( input->buffer() ), *input );
			}
		}

		void traverseAttachmentPasses( GraphNode & parent
			, Attachment const * outputAttach
			, Attachment const * inputAttach
			, AttachmentTransitions & parentTransitions
			, GraphNodePtrArray & nodes )
		{
			if ( outputAttach )
			{
				for ( auto pass : listAttachmentPasses( *outputAttach ) )
				{
					auto it = std::find_if( nodes.begin(), nodes.end()
						, [&pass]( GraphNodePtr const & lookup )
						{
							return lookup->getName() == pass->getGroupName();
						} );
					if ( nodes.end() == it )
					{
						auto & node = static_cast< FramePassNode & >( *nodes.emplace_back( std::make_unique< FramePassNode >( *pass ) ) );
						AttachmentTransitions transitions;
						traversePassAttach( node, pass->getTargets(), Attachment::Flag::InOut, transitions, nodes );
						traversePassAttach( node, pass->getInouts(), transitions, nodes );
						traversePassAttach( node, pass->getUniforms(), transitions, nodes );
						traversePassAttach( node, pass->getSampled(), transitions, nodes );
						traversePassAttach( node, pass->getInputs(), transitions, nodes );
						traversePassAttach( node, pass->getTargets(), Attachment::Flag::Input, transitions, nodes );
						node.setTransitions( mergeIdenticalTransitions( std::move( transitions ) ) );
						parent.attachNode( node );
					}
					else
					{
						parent.attachNode( **it );
					}
				}
			}

			if ( outputAttach && inputAttach )
			{
				auto outputs = splitAttach( *outputAttach );
				auto inputs = splitAttach( *inputAttach );
				for ( auto output : outputs )
				{
					for ( auto input : inputs )
					{
						if ( areOverlapping( *output, *input ) )
							insertTransition( output->isImage(), output, input, parentTransitions );
					}
				}
			}
			else if ( outputAttach )
			{
				// Attach to external
				auto outputs = splitAttach( *outputAttach );
				for ( auto output : outputs )
					insertTransition( output->isImage(), output, nullptr, parentTransitions );
			}
			else if ( inputAttach )
			{
				// External resource
				auto inputs = splitAttach( *inputAttach );
				for ( auto input : inputs )
					insertTransition( input->isImage(), nullptr, input, parentTransitions );
			}
		}

		static bool hasInPredecessors( GraphNode & parent, GraphNode & child )
		{
			return parent.getPredecessors().end() != std::find( parent.getPredecessors().begin()
				, parent.getPredecessors().end()
				, &child );
		}

		static void removeShortcuts( GraphNode & node )
		{
			GraphAdjacentNodeArray & predecessors = node.getPredecessors();
			auto it = predecessors.begin();

			while ( it != predecessors.end() )
			{
				auto curr = *it;
				bool found = false;
				auto oit = predecessors.begin();
				while ( oit != predecessors.end() && !found )
				{
					if ( oit != it )
						found = hasInPredecessors( **oit, *curr );
					++oit;
				}

				removeShortcuts( *curr );
				if ( found )
					it = predecessors.erase( it );
				else
					++it;
			}
		}

		static void sortNodes( GraphNode & node
			, GraphNodePtrArray & sourceGraph
			, GraphNodePtrArray & targetGraph )
		{
			for ( auto & pred : node.getPredecessors() )
				sortNodes( *pred, sourceGraph, targetGraph );

			auto it = std::find_if( sourceGraph.begin(), sourceGraph.end()
				, [&node]( GraphNodePtr const & lookup )
				{
					return &node == lookup.get();
				} );
			if ( sourceGraph.end() != it )
			{
				targetGraph.emplace_back( std::move( *it ) );
				sourceGraph.erase( it );
			}
		}

		static void sortNodes( RootNode & root
			, GraphNodePtrArray & graph )
		{
			auto sourceGraph = std::move( graph );
			graph.clear();
			for ( auto & pred : root.getPredecessors() )
				sortNodes( *pred, sourceGraph, graph );
		}

		static void updateState( Attachment const & inputAttach
			, std::vector< ImageLayout > & states
			, bool separateDepthStencilLayouts )
		{
			auto id = inputAttach.view().id;
			while ( states.size() <= id )
				states.resize( std::max< size_t >( 1u, states.size() * 2u ) );
			states[id] = inputAttach.getImageLayout( separateDepthStencilLayouts );
		}

		static void updateState( Attachment const & inputAttach
			, std::vector< AccessFlags > & states )
		{
			auto id = inputAttach.buffer().id;
			while ( states.size() <= id )
				states.resize( std::max< size_t >( 1u, states.size() * 2u ) );
			states[id] = inputAttach.getAccessMask();
		}

		static void insertNeededTransition( Attachment const & output
			, Attachment const & input
			, AttachmentTransitions & transitions
			, graph::AttachmentStates & states )
		{
			if ( output.isImage() )
			{
				transitions.imageTransitions.emplace_back( output.view(), output, input );
				updateState( input, states.imageStates, states.separateDepthStencilLayouts );
			}
			else
			{
				transitions.bufferTransitions.emplace_back( output.buffer(), output, input );
				updateState( input, states.bufferStates );
			}
		}

		static bool isInNeededState( Attachment const & inputAttach
			, std::vector< ImageLayout > const & states
			, bool separateDepthStencilLayouts )
		{
			auto id = inputAttach.view().id;
			if ( states.size() <= id )
				return false;

			auto & state = states[id];
			return state == inputAttach.getImageLayout( separateDepthStencilLayouts );
		}

		static bool isInNeededState( Attachment const & inputAttach
			, std::vector< AccessFlags > const & states )
		{
			auto id = inputAttach.buffer().id;
			if ( states.size() <= id )
				return false;

			auto & state = states[id];
			return state == inputAttach.getAccessMask();
		}

		static bool isInNeededState( Attachment const & inputAttach
			, AttachmentStates const & states )
		{
			if ( inputAttach.isImage() )
				return isInNeededState( inputAttach, states.imageStates, states.separateDepthStencilLayouts );
			return isInNeededState( inputAttach, states.bufferStates );
		}

		static void buildTransitions( GraphNodePtrArray const & graph
			, graph::AttachmentStates & states )
		{
			for ( auto & node : graph )
			{
				if ( node->getKind() == GraphNode::Kind::FramePass )
				{
					AttachmentTransitions transitions;
					for ( auto & transition : node->getImageTransitions() )
					{
						if ( !isInNeededState( transition.inputAttach, states ) )
							insertNeededTransition( transition.outputAttach, transition.inputAttach, transitions, states );
					}
					for ( auto & transition : node->getBufferTransitions() )
					{
						if ( !isInNeededState( transition.inputAttach, states ) )
							insertNeededTransition( transition.outputAttach, transition.inputAttach, transitions, states );
					}
					node->setTransitions( std::move( transitions ) );
				}
			}
			AttachmentTransitions transitions;
		}
	}

	//*********************************************************************************************

	AttachmentArray findEndPoints( FramePassArray const & passes )
	{
		// List all attaches
		AttachmentArray allAttachs;
		endpoints::listAllAttachs( passes, allAttachs );

		// Filter out the attaches that are used as parent to pass attaches
		AttachmentArray result;
		endpoints::listNonParentAttachs( passes, allAttachs, result );

		// Also look for passes without outputs, and add their inputs
		endpoints::addSinkPassInputs( passes, result );

		// Remove attachs for which the pass has other output attachs which are not in the list
		endpoints::removeMismatchs( result );

		return result;
	}

	//*********************************************************************************************

	void buildGraph( AttachmentArray const & endPoints
		, RootNode & root
		, GraphNodePtrArray & graph
		, bool separateDepthStencilLayouts )
	{
		// First generate the graph with all transitions and links.
		AttachmentTransitions transitions;
		for ( auto endPoint : endPoints )
			graph::traverseAttachmentPasses( root, endPoint, nullptr, transitions, graph );

		// Then remove the shortcuts (if pass C depends on A and B, if B depends on A, then remove link between A and C)
		graph::removeShortcuts( root );

		// Now sort the graph nodes regarding their position in the final graph.
		graph::sortNodes( root, graph );

		// Eventually parse the sorted nodes to generate a curated transitions list per node.
		graph::AttachmentStates states{ separateDepthStencilLayouts };
		graph::buildTransitions( graph, states );
	}

	//*********************************************************************************************
}
