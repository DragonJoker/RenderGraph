/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "FramePassDependenciesBuilder.hpp"

#include "RenderGraph/AttachmentTransition.hpp"
#include "RenderGraph/Exception.hpp"
#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/GraphNode.hpp"

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <stdexcept>

#define CRG_DebugPassAttaches 0
#define CRG_DebugPassDependencies 0

namespace crg
{
	namespace builder
	{
		namespace
		{
			bool isInRange( uint32_t value
				, uint32_t left
				, uint32_t count )
			{
				return value >= left && value < left + count;
			}

			bool isSingleMipView( ImageViewId const & sub
				, ImageViewId const & main )
			{
				return main.data->image == sub.data->image
					&& isInRange( sub.data->info.subresourceRange.baseMipLevel
						, main.data->info.subresourceRange.baseMipLevel
						, main.data->info.subresourceRange.levelCount )
					&& sub.data->info.subresourceRange.levelCount == 1u;
			}

			ImageViewId const & getInputView( ImageViewId const & lhs
				, ImageViewId const & rhs )
			{
				if ( lhs == rhs )
				{
					return lhs;
				}

				if ( isSingleMipView( lhs, rhs ) )
				{
					return lhs;
				}

				return rhs;
			}

			ImageViewId const & getOutputView( ImageViewId const & lhs
				, ImageViewId const & rhs )
			{
				if ( lhs == rhs )
				{
					return lhs;
				}

				if ( isSingleMipView( lhs, rhs ) )
				{
					return rhs;
				}

				return lhs;
			}

			template< typename DataT >
			struct AttachDataTraitsT;

			template<>
			struct AttachDataTraitsT< ImageViewId >
			{
				static ImageViewId get( Attachment const & attach )
				{
					return attach.view();
				}

				static ImageViewId getOutput( Attachment const & inputAttach
					, Attachment const & outputAttach )
				{
					return getOutputView( inputAttach.view()
						, outputAttach.view() );
				}

				static ImageViewId getInput( Attachment const & inputAttach
					, Attachment const & outputAttach )
				{
					return getInputView( inputAttach.view()
						, outputAttach.view() );
				}

				static DataTransitionArrayT< ImageViewId > & getTransitions( AttachmentTransitions & transitions )
				{
					return transitions.viewTransitions;
				}

				static AttachmentArray split( Attachment const & attach )
				{
					AttachmentArray result;

					if ( attach.view().data->source.empty() )
					{
						result.push_back( attach );
					}
					else
					{
						for ( auto & view : attach.view().data->source )
						{
							result.push_back( Attachment{ view, attach } );
						}
					}

					return result;
				}

				static bool isInput( Attachment const & attach )
				{
					return attach.isInput();
				}

				static bool isOutput( Attachment const & attach )
				{
					return attach.isOutput();
				}
			};

			template<>
			struct AttachDataTraitsT< Buffer >
			{
				static Buffer get( Attachment const & attach )
				{
					return attach.buffer.buffer;
				}

				static Buffer getOutput( Attachment const & inputAttach
					, Attachment const & outputAttach )
				{
					return inputAttach.buffer.buffer;
				}

				static Buffer getInput( Attachment const & inputAttach
					, Attachment const & outputAttach )
				{
					return inputAttach.buffer.buffer;
				}

				static DataTransitionArrayT< Buffer > & getTransitions( AttachmentTransitions & transitions )
				{
					return transitions.bufferTransitions;
				}

				static AttachmentArray split( Attachment const & attach )
				{
					return { attach };
				}

				static bool isInput( Attachment const & attach )
				{
					return attach.isInput() && attach.isStorageBuffer();
				}

				static bool isOutput( Attachment const & attach )
				{
					return attach.isOutput() && attach.isStorageBuffer();
				}
			};

			template< typename DataT >
			DataT const & getAttachOutputData( Attachment const & inputAttach
				, Attachment const & outputAttach )
			{
				return AttachDataTraitsT< DataT >::getOutput( inputAttach, outputAttach );
			}

			template< typename DataT >
			DataT const & getAttachInputData( Attachment const & inputAttach
				, Attachment const & outputAttach )
			{
				return AttachDataTraitsT< DataT >::getInput( inputAttach, outputAttach );
			}

			template< typename DataT >
			struct AttachesT
			{
				DataT data;
				std::vector< Attachment > attaches{};
			};

			template< typename DataT >
			using AttachesArrayT = std::vector< AttachesT< DataT > >;

			using ViewAttaches = AttachesT< ImageViewId >;
			using BufferAttaches = AttachesT< Buffer >;

			using ViewAttachesArray = AttachesArrayT< ImageViewId >;
			using BufferAttachesArray = AttachesArrayT< Buffer >;

#if CRG_DebugPassAttaches
			std::string const & getAttachDataName( ImageViewId const & data )
			{
				return data.data->name;
			}

			std::string const & getAttachDataName( Buffer const & data )
			{
				return data.name;
			}

			template< typename DataT >
			std::ostream & operator<<( std::ostream & stream, AttachesT< DataT > const & attach )
			{
				stream << getAttachDataName( attach.data );
				std::string sep{ " -> " };

				for ( auto & attach : attach.attaches )
				{
					stream << sep << attach.name;
					sep = ", ";
				}

				return stream;
			}

			template< typename DataT >
			std::ostream & operator<<( std::ostream & stream, AttachesArrayT< DataT > const & attaches )
			{
				for ( auto & attach : attaches )
				{
					stream << attach << std::endl;
				}

				return stream;
			}

			template< typename DataT >
			std::ostream & operator<<( std::ostream & stream, DataTransitionT< DataT > const & transition )
			{
				stream << ( transition.outputAttach.pass ? transition.outputAttach.pass->name : std::string{ "External" } )
					<< " -> "
					<< ( transition.inputAttach.pass ? transition.inputAttach.pass->name : std::string{ "External" } );
				std::string sep = " on [";
				stream << sep << getAttachDataName( transition.data );
				stream << "]";
				return stream;
			}
#endif
#if CRG_DebugPassDependencies
			std::ostream & operator<<( std::ostream & stream, FramePassDependencies const & dependencies )
			{
				for ( auto & depsIt : dependencies )
				{
					stream << depsIt.pass->name << std::endl;

					for ( auto & transition : depsIt.transitions.viewTransitions )
					{
						stream << "  " << transition << std::endl;
					}

					for ( auto & transition : depsIt.transitions.bufferTransitions )
					{
						stream << "  " << transition << std::endl;
					}
				}

				return stream;
			}
#endif

			void printDebug( ViewAttachesArray const & inputs
				, ViewAttachesArray const & outputs )
			{
#if CRG_DebugPassAttaches
				std::clog << "Inputs" << std::endl;
				std::clog << inputs << std::endl;
				std::clog << "Outputs" << std::endl;
				std::clog << outputs << std::endl;
#endif
			}

			void printDebug( BufferAttachesArray const & inputs
				, BufferAttachesArray const & outputs )
			{
#if CRG_DebugPassAttaches
				std::clog << "Inputs" << std::endl;
				std::clog << inputs << std::endl;
				std::clog << "Outputs" << std::endl;
				std::clog << outputs << std::endl;
#endif
			}

			void printDebug( FramePassDependencies const & inputTransitions
				, FramePassDependencies const & outputTransitions )
			{
#if CRG_DebugPassDependencies
				std::clog << "Input Transitions" << std::endl;
				std::clog << inputTransitions << std::endl;
				std::clog << "Output Transitions" << std::endl;
				std::clog << outputTransitions << std::endl;
#endif
			}

			bool areIntersecting( uint32_t lhsLBound
				, uint32_t lhsCount
				, uint32_t rhsLBound
				, uint32_t rhsCount )
			{
				return isInRange( lhsLBound, rhsLBound, rhsCount )
					|| isInRange( rhsLBound, lhsLBound, lhsCount );
			}

			bool areIntersecting( VkImageSubresourceRange const & lhs
				, VkImageSubresourceRange const & rhs )
			{
				return areIntersecting( lhs.baseMipLevel
					, lhs.levelCount
					, rhs.baseMipLevel
					, rhs.levelCount )
					&& areIntersecting( lhs.baseArrayLayer
						, lhs.layerCount
						, rhs.baseArrayLayer
						, lhs.layerCount );
			}

			bool areOverlapping( ImageViewData const & lhs
				, ImageViewData const & rhs )
			{
				return lhs.image == rhs.image
					&& areIntersecting( getVirtualRange( lhs.image, lhs.info.viewType, lhs.info.subresourceRange )
						, getVirtualRange( rhs.image, rhs.info.viewType, rhs.info.subresourceRange ) );
			}

			bool areOverlapping( ImageViewId const & lhs
				, ImageViewId const & rhs )
			{
				return areOverlapping( *lhs.data, *rhs.data );
			}

			bool areOverlapping( Buffer const & lhs
				, Buffer const & rhs )
			{
				return lhs.buffer == rhs.buffer;
			}

			template< typename DataT >
			bool areOverlapping( Attachment const & lhs
				, Attachment const & rhs )
			{
				return areOverlapping( AttachDataTraitsT< DataT >::get( lhs )
					, AttachDataTraitsT< DataT >::get( rhs ) );
			}

			template< typename DataT >
			void insertAttach( Attachment const & attach
				, AttachesArrayT< DataT > & cont )
			{
				auto it = std::find_if( cont.begin()
					, cont.end()
					, [&attach]( AttachesT< DataT > const & lookup )
					{
						return lookup.data == AttachDataTraitsT< DataT >::get( attach );
					} );

				if ( cont.end() == it )
				{
					cont.push_back( AttachesT< DataT >{ AttachDataTraitsT< DataT >::get( attach ) } );
					it = std::prev( cont.end() );
				}

				auto attachesIt = std::find( it->attaches.begin()
					, it->attaches.end()
					, attach );

				if ( attachesIt == it->attaches.end() )
				{
					it->attaches.push_back( attach );
				}
			}

			template< typename DataT >
			void processAttachSource( AttachesT< DataT > & lookup
				, Attachment const & attach
				, DataT const & attachView
				, DataT const & sourceView
				, std::function< bool( DataT const &, DataT const & ) > processAttach )
			{
				if ( processAttach( sourceView, attachView ) )
				{
					if ( lookup.attaches.end() == std::find( lookup.attaches.begin()
						, lookup.attaches.end()
						, attach ) )
					{
						lookup.attaches.push_back( attach );
					}
				}
			}

			template< typename DataT >
			void processAttach( Attachment const & attach
				, AttachesArrayT< DataT > & cont
				, AttachesArrayT< DataT > & all
				, std::function< bool( DataT const &, DataT const & ) > processAttach )
			{
				std::vector< FramePass const * > passes;
				auto attaches = AttachDataTraitsT< DataT >::split( attach );

				for ( auto & splitAttach : attaches )
				{
					for ( auto & lookup : cont )
					{
						processAttachSource( lookup
							, splitAttach
							, lookup.data
							, AttachDataTraitsT< DataT >::get( splitAttach )
							, processAttach );
					}

					insertAttach( splitAttach, cont );
					insertAttach( splitAttach, all );
				}
			}

			template< typename DataT >
			void processInputAttachs( AttachmentArray const & attachs
				, AttachesArrayT< DataT > & cont
				, AttachesArrayT< DataT > & all )
			{
				for ( auto & attach : attachs )
				{
					if ( AttachDataTraitsT< DataT >::isInput( attach ) )
					{
						processAttach< DataT >( attach
							, cont
							, all
							, []( DataT const & lookupView
								, DataT const & attachView )
							{
								return areOverlapping( lookupView, attachView );
							} );
					}
				}
			}

			template< typename DataT >
			void processOutputAttachs( AttachmentArray const & attachs
				, AttachesArrayT< DataT > & cont
				, AttachesArrayT< DataT > & all )
			{
				for ( auto & attach : attachs )
				{
					if ( AttachDataTraitsT< DataT >::isOutput( attach ) )
					{
						processAttach< DataT >( attach
							, cont
							, all
							, []( DataT const & lookupView
								, DataT const & attachView )
							{
								return areOverlapping( lookupView, attachView );
							} );
					}
				}
			}

			template< typename DataT >
			bool hasTransition( DataTransitionArrayT< DataT > const & transitions
				, DataTransitionT< DataT > const & transition )
			{
				return transitions.end() != std::find( transitions.begin()
					, transitions.end()
					, transition );
			}

			FramePassTransitions & insertPass( FramePass const * pass
				, FramePassDependencies & transitions )
			{
				auto passIt = std::find_if( transitions.begin()
					, transitions.end()
					, [pass]( FramePassTransitions const & lookup )
					{
						return lookup.pass == pass;
					} );

				if ( passIt == transitions.end() )
				{
					transitions.push_back( FramePassTransitions{ pass, {} } );
					return transitions.back();
				}

				return *passIt;
			}

			template< typename DataT >
			bool dependsOn( DataTransitionT< DataT > const & transition
				, DataTransitionT< DataT > const & lookup )
			{
				return transition.inputAttach.pass
					&& transition.outputAttach.pass
					&& transition.inputAttach.pass->dependsOn( *transition.outputAttach.pass
					, transition.data );
			}

			template< typename DataT >
			void insertTransition( DataTransitionT< DataT > const & transition
				, DataTransitionArrayT< DataT > & transitions )
			{
				if ( !hasTransition( transitions, transition ) )
				{
					auto it = std::find_if( transitions.begin()
						, transitions.end()
						, [&transition]( DataTransitionT< DataT > const & lookup )
						{
							return dependsOn( lookup, transition );
						} );

					if ( it != transitions.end() )
					{
						transitions.insert( it, transition );
					}
					else
					{
						auto rit = std::find_if( transitions.rbegin()
							, transitions.rend()
							, [&transition]( DataTransitionT< DataT > const & lookup )
							{
								return dependsOn( transition, lookup );
							} );

						if ( rit != transitions.rend() )
						{
							transitions.insert( rit.base(), transition );
						}
						else
						{
							transitions.push_back( transition );
						}
					}
				}
			}

			void addRemainingDependency( Attachment const & attach
				, FramePassDependencies & inputTransitions
				, ViewTransitionArray & allTransitions )
			{
				auto & transitions = insertPass( attach.pass, inputTransitions ).transitions;

				if ( attach.isColourInOutAttach() )
				{
					ViewTransition transition{ attach.view(), attach, attach };
					insertTransition( transition, transitions.viewTransitions );
				}
				else if ( attach.isColourInputAttach()
					|| attach.isSampledView() )
				{
					ViewTransition transition{ attach.view(), Attachment::createDefault( attach.view() ), attach };
					insertTransition( transition, transitions.viewTransitions );
				}
				else
				{
					ViewTransition transition{ attach.view(), attach, Attachment::createDefault( attach.view() ) };
					insertTransition( transition, transitions.viewTransitions );
				}

				insertTransition( transitions.viewTransitions.back(), allTransitions );
			}

			void addRemainingDependency( Attachment const & attach
				, FramePassDependencies & inputTransitions
				, BufferTransitionArray & allTransitions )
			{
				auto & transitions = insertPass( attach.pass, inputTransitions ).transitions;

				if ( attach.isColourInOutAttach() )
				{
					BufferTransition transition{ attach.buffer.buffer, attach, attach };
					insertTransition( transition, transitions.bufferTransitions );
				}
				else if ( attach.isColourInputAttach()
					|| attach.isSampledView() )
				{
					BufferTransition transition{ attach.buffer.buffer, Attachment::createDefault( attach.buffer.buffer ), attach };
					insertTransition( transition, transitions.bufferTransitions );
				}
				else
				{
					BufferTransition transition{ attach.buffer.buffer, attach, Attachment::createDefault( attach.buffer.buffer ) };
					insertTransition( transition, transitions.bufferTransitions );
				}

				insertTransition( transitions.bufferTransitions.back(), allTransitions );
			}

			void addRemainingDependency( Attachment const & attach
				, FramePassDependencies & inputTransitions
				, AttachmentTransitions & allTransitions )
			{
				if ( attach.isImage() )
				{
					addRemainingDependency( attach
						, inputTransitions
						, allTransitions.viewTransitions );
				}
				else
				{
					addRemainingDependency( attach
						, inputTransitions
						, allTransitions.bufferTransitions );
				}
			}

			bool match( Buffer const & lhs, Buffer const & rhs )
			{
				return lhs.buffer != rhs.buffer;
			}

			bool match( ImageViewId const & lhs, ImageViewId const & rhs )
			{
				return match( *lhs.data, *rhs.data );
			}

			template< typename DataT >
			void addDependency( Attachment const & outputAttach
				, Attachment const & inputAttach
				, FramePassDependencies & inputTransitions
				, FramePassDependencies & outputTransitions
				, AttachmentTransitions & allTransitions )
			{
				DataTransitionT< DataT > inputTransition{ AttachDataTraitsT< DataT >::getInput( outputAttach, inputAttach )
					, outputAttach
					, inputAttach };
				DataTransitionT< DataT > outputTransition{ AttachDataTraitsT< DataT >::getOutput( outputAttach, inputAttach )
					, outputAttach
					, inputAttach };
				insertTransition( inputTransition, AttachDataTraitsT< DataT >::getTransitions( allTransitions ) );

				if ( !match( AttachDataTraitsT< DataT >::get( outputAttach )
					, AttachDataTraitsT< DataT >::get( inputAttach ) ) )
				{
					insertTransition( outputTransition, AttachDataTraitsT< DataT >::getTransitions( allTransitions ) );
				}
				{
					auto & transitions = insertPass( inputAttach.pass, inputTransitions ).transitions;
					insertTransition( inputTransition, AttachDataTraitsT< DataT >::getTransitions( transitions ) );
				}
				{
					auto & transitions = insertPass( outputAttach.pass, outputTransitions ).transitions;
					insertTransition( outputTransition, AttachDataTraitsT< DataT >::getTransitions( transitions ) );
				}
			}

			template< typename DataT >
			void buildPassDependencies( AttachesArrayT< DataT > const & inputs
				, AttachesArrayT< DataT > const & outputs
				, AttachesArrayT< DataT > & all
				, FramePassDependencies & inputTransitions
				, FramePassDependencies & outputTransitions
				, AttachmentTransitions & allTransitions )
			{
				for ( AttachesT< DataT > const & output : outputs )
				{
					for ( AttachesT< DataT > const & input : inputs )
					{
						if ( areOverlapping( input.data, output.data ) )
						{
							for ( Attachment const & outputAttach : output.attaches )
							{
								for ( Attachment const & inputAttach : input.attaches )
								{
									if ( inputAttach.pass->dependsOn( *outputAttach.pass
										, AttachDataTraitsT< DataT >::getOutput( inputAttach, outputAttach ) ) )
									{
										addDependency< DataT >( outputAttach
											, inputAttach
											, inputTransitions
											, outputTransitions
											, allTransitions );
									}
								}
							}

							auto it = std::find_if( all.begin()
								, all.end()
								, [&input]( AttachesT< DataT > const & lookup )
								{
									return lookup.data == input.data;
								} );

							if ( all.end() != it )
							{
								all.erase( it );
							}
						}
					}
				}

				// `all` should now only contain sampled/input/output from/to nothing attaches.
				for ( auto & remaining : all )
				{
					for ( auto & attach : remaining.attaches )
					{
						addRemainingDependency( attach
							, inputTransitions
							, allTransitions );
					}
				}

				printDebug( inputs, outputs );
			}
		}

		void buildPassAttachDependencies( GraphNodePtrArray const & nodes
			, FramePassDependencies & inputTransitions
			, FramePassDependencies & outputTransitions
			, AttachmentTransitions & allTransitions )
		{
			ViewAttachesArray imgInputs;
			ViewAttachesArray imgOutputs;
			ViewAttachesArray imgAll;
			BufferAttachesArray bufInputs;
			BufferAttachesArray bufOutputs;
			BufferAttachesArray bufAll;

			for ( auto & node : nodes )
			{
				auto pass = getFramePass( *node );
				assert( pass );
				processInputAttachs( pass->images, imgInputs, imgAll );
				processOutputAttachs( pass->images, imgOutputs, imgAll );
				processInputAttachs( pass->buffers, bufInputs, bufAll );
				processOutputAttachs( pass->buffers, bufOutputs, bufAll );
				insertPass( pass, inputTransitions );
				insertPass( pass, outputTransitions );
			}

			buildPassDependencies( imgInputs
				, imgOutputs
				, imgAll
				, inputTransitions
				, outputTransitions
				, allTransitions );
			buildPassDependencies( bufInputs
				, bufOutputs
				, bufAll
				, inputTransitions
				, outputTransitions
				, allTransitions );
			printDebug( inputTransitions
				, outputTransitions );
		}
	}
}
