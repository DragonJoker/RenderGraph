/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "FramePassDependenciesBuilder.hpp"

#include "RenderGraph/AttachmentTransition.hpp"
#include "RenderGraph/Exception.hpp"
#include "RenderGraph/FramePass.hpp"

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
			struct ViewAttaches
			{
				ImageViewId view;
				std::vector< Attachment > attaches;
			};

			using ViewAttachesArray = std::vector< ViewAttaches >;

			std::ostream & operator<<( std::ostream & stream, ViewAttaches const & attach )
			{
				stream << attach.view.data->name;
				std::string sep{ " -> " };

				for ( auto & attach : attach.attaches )
				{
					stream << sep << attach.image.name;
					sep = ", ";
				}

				return stream;
			}

			std::ostream & operator<<( std::ostream & stream, ViewAttachesArray const & attaches )
			{
				for ( auto & attach : attaches )
				{
					stream << attach << std::endl;
				}

				return stream;
			}

			std::ostream & operator<<( std::ostream & stream, AttachmentTransition const & transition )
			{
				stream << ( transition.outputAttach.pass ? transition.outputAttach.pass->name : std::string{ "External" } )
					<< " -> "
					<< ( transition.inputAttach.pass ? transition.inputAttach.pass->name : std::string{ "External" } );
				std::string sep = " on [";
				stream << sep << transition.view.data->name;
				stream << "]";
				return stream;
			}

			std::ostream & operator<<( std::ostream & stream, FramePassDependenciesMap const & dependencies )
			{
				for ( auto & depsIt : dependencies )
				{
					stream << depsIt.first->name << std::endl;

					for ( auto & transition : depsIt.second )
					{
						stream << "  " << transition << std::endl;
					}
				}

				return stream;
			}

			void printDebug( ViewAttachesArray const & inputs
				, ViewAttachesArray const & outputs
				, FramePassDependenciesMap const & inputTransitions
				, FramePassDependenciesMap const & outputTransitions )
			{
#if CRG_DebugPassAttaches
				std::clog << "Inputs" << std::endl;
				std::clog << inputs << std::endl;
				std::clog << "Outputs" << std::endl;
				std::clog << outputs << std::endl;
#endif
#if CRG_DebugPassDependencies
				std::clog << "Input Transitions" << std::endl;
				std::clog << inputTransitions << std::endl;
				std::clog << "Output Transitions" << std::endl;
				std::clog << outputTransitions << std::endl;
#endif
			}

			bool isInRange( uint32_t value
				, uint32_t left
				, uint32_t count )
			{
				return value >= left && value < left + count;
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
					&& areIntersecting( lhs.info.subresourceRange
						, rhs.info.subresourceRange );
			}

			bool areOverlapping( Attachment const & lhs
				, Attachment const & rhs )
			{
				return areOverlapping( *lhs.view().data, *rhs.view().data );
			}

			void insertAttach( Attachment const & attach
				, ViewAttachesArray & cont )
			{
				auto it = std::find_if( cont.begin()
					, cont.end()
					, [&attach]( ViewAttaches const & lookup )
					{
						return lookup.view == attach.view();
					} );

				if ( cont.end() == it )
				{
					cont.push_back( ViewAttaches{ attach.view() } );
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

			void processAttachSource( ViewAttaches & lookup
				, Attachment const & attach
				, ImageViewId const & attachView
				, ImageViewId const & sourceView
				, std::function< bool( ImageViewId const &, ImageViewId const & ) > processAttach )
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

			AttachmentArray splitAttach( Attachment const & attach )
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

			void processAttach( Attachment const & attach
				, ViewAttachesArray & cont
				, ViewAttachesArray & all
				, std::function< bool( ImageViewId const &, ImageViewId const & ) > processAttach )
			{
				std::vector< FramePass const * > passes;
				auto attaches = splitAttach( attach );

				for ( auto & splitAttach : attaches )
				{
					for ( auto & lookup : cont )
					{
						processAttachSource( lookup
							, splitAttach
							, lookup.view
							, splitAttach.view()
							, processAttach );
					}

					insertAttach( splitAttach, cont );
					insertAttach( splitAttach, all );
				}
			}

			void processAttachs( AttachmentArray const & attachs
				, ViewAttachesArray & cont
				, ViewAttachesArray & all )
			{
				for ( auto & attach : attachs )
				{
					processAttach( attach
						, cont
						, all
						, []( ImageViewId const & lookupView
							, ImageViewId const & attachView )
						{
							return areOverlapping( *lookupView.data, *attachView.data );
						} );
				}
			}

			void processInputAttachs( AttachmentArray const & attachs
				, ViewAttachesArray & cont
				, ViewAttachesArray & all )
			{
				for ( auto & attach : attachs )
				{
					if ( attach.isInput() )
					{
						processAttach( attach
							, cont
							, all
							, []( ImageViewId const & lookupView
								, ImageViewId const & attachView )
							{
								return areOverlapping( *lookupView.data, *attachView.data );
							} );
					}
				}
			}

			void processOutputAttachs( AttachmentArray const & attachs
				, ViewAttachesArray & cont
				, ViewAttachesArray & all )
			{
				for ( auto & attach : attachs )
				{
					if ( attach.isOutput() )
					{
						processAttach( attach
							, cont
							, all
							, []( ImageViewId const & lookupView
								, ImageViewId const & attachView )
							{
								return areOverlapping( *lookupView.data, *attachView.data );
							} );
					}
				}
			}

			void processDepthOrStencilInputAttach( Attachment const & attach
				, ViewAttachesArray & cont
				, ViewAttachesArray & all )
			{
				if ( attach.isDepthInput()
					|| attach.isStencilInput() )
				{
					processAttach( attach
						, cont
						, all
						, []( ImageViewId const & lookupView
							, ImageViewId const & attachView )
						{
							return areOverlapping( *lookupView.data, *attachView.data );
						} );
				}
			}

			void processDepthOrStencilOutputAttach( Attachment const & attach
				, ViewAttachesArray & cont
				, ViewAttachesArray & all )
			{
				if ( attach.isDepthOutput()
					|| attach.isStencilOutput() )
				{
					processAttach( attach
						, cont
						, all
						, []( ImageViewId const & lookupView
							, ImageViewId const & attachView )
						{
							return areOverlapping( *lookupView.data, *attachView.data );
						} );
				}
			}

			void processColourInputAttachs( AttachmentArray const & attachs
				, ViewAttachesArray & cont
				, ViewAttachesArray & all )
			{
				for ( auto & attach : attachs )
				{
					if ( attach.isColourInput() )
					{
						processAttach( attach
							, cont
							, all
							, []( ImageViewId const & lookupView
								, ImageViewId const & attachView )
							{
								return areOverlapping( *lookupView.data, *attachView.data );
							} );
					}
				}
			}

			void processColourOutputAttachs( AttachmentArray const & attachs
				, ViewAttachesArray & cont
				, ViewAttachesArray & all )
			{
				for ( auto & attach : attachs )
				{
					if ( attach.isColourOutput() )
					{
						return processAttach( attach
							, cont
							, all
							, []( ImageViewId const & lookupView
								, ImageViewId const & attachView )
							{
								return areOverlapping( *lookupView.data, *attachView.data );
							} );
					}
				}
			}

			void processTransferInputAttachs( AttachmentArray const & attachs
				, ViewAttachesArray & cont
				, ViewAttachesArray & all )
			{
				for ( auto & attach : attachs )
				{
					if ( attach.isTransferInput() )
					{
						processAttach( attach
							, cont
							, all
							, []( ImageViewId const & lookupView
								, ImageViewId const & attachView )
							{
								return areOverlapping( *lookupView.data, *attachView.data );
							} );
					}
				}
			}

			void processTransferOutputAttachs( AttachmentArray const & attachs
				, ViewAttachesArray & cont
				, ViewAttachesArray & all )
			{
				for ( auto & attach : attachs )
				{
					if ( attach.isTransferOutput() )
					{
						return processAttach( attach
							, cont
							, all
							, []( ImageViewId const & lookupView
								, ImageViewId const & attachView )
							{
								return areOverlapping( *lookupView.data, *attachView.data );
							} );
					}
				}
			}

			void addRemainingDependency( Attachment const & attach
				, FramePassDependenciesMap & inputTransitions
				, AttachmentTransitionArray & allTransitions )
			{
				auto & transitions = inputTransitions.emplace( attach.pass, AttachmentTransitionArray{} ).first->second;

				if ( attach.isColourInOut() )
				{
					transitions.push_back( { attach.view()
						, attach
						, attach } );
				}
				else if ( attach.isColourInput()
						|| attach.isSampled() )
				{
					transitions.push_back( { attach.view()
						, Attachment::createDefault( attach.view() )
						, attach } );
				}
				else
				{
					transitions.push_back( { attach.view()
						, attach
						, Attachment::createDefault( attach.view() ) } );
				}

				allTransitions.push_back( transitions.back() );
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

			void addDependency( Attachment const & outputAttach
				, Attachment const & inputAttach
				, FramePassDependenciesMap & inputTransitions
				, FramePassDependenciesMap & outputTransitions
				, AttachmentTransitionArray & allTransitions )
			{
				assert( outputAttach.view() == inputAttach.view()
					|| isSingleMipView( outputAttach.view(), inputAttach.view() )
					|| isSingleMipView( inputAttach.view(), outputAttach.view() ) );
				AttachmentTransition inputTransition{ getInputView( outputAttach.view(), inputAttach.view() )
					, outputAttach
					, inputAttach };
				AttachmentTransition outputTransition{ getOutputView( outputAttach.view(), inputAttach.view() )
					, outputAttach
					, inputAttach };

				if ( outputAttach.view() == inputAttach.view() )
				{
					allTransitions.push_back( inputTransition );
				}
				else
				{
					allTransitions.push_back( inputTransition );
					allTransitions.push_back( outputTransition );
				}
				{
					auto & transitions = inputTransitions.emplace( inputAttach.pass, AttachmentTransitionArray{} ).first->second;
					transitions.push_back( inputTransition );
				}
				{
					auto & transitions = outputTransitions.emplace( outputAttach.pass, AttachmentTransitionArray{} ).first->second;
					transitions.push_back( outputTransition );
				}
			}
		}

		void buildPassAttachDependencies( std::vector< FramePassPtr > const & passes
			, FramePassDependenciesMap & inputTransitions
			, FramePassDependenciesMap & outputTransitions
			, AttachmentTransitionArray & allTransitions )
		{
			ViewAttachesArray inputs;
			ViewAttachesArray outputs;
			ViewAttachesArray all;

			for ( auto & pass : passes )
			{
				processInputAttachs( pass->images, inputs, all );
				processOutputAttachs( pass->images, outputs, all );
				inputTransitions.emplace( pass.get(), AttachmentTransitionArray{} );
				outputTransitions.emplace( pass.get(), AttachmentTransitionArray{} );
			}

			for ( auto & output : outputs )
			{
				for ( auto & input : inputs )
				{
					if ( areOverlapping( *input.view.data, *output.view.data ) )
					{
						for ( auto & outputAttach : output.attaches )
						{
							for ( auto & inputAttach : input.attaches )
							{
								if ( inputAttach.pass->dependsOn( *outputAttach.pass
									, getOutputView( inputAttach.view()
										, outputAttach.view() ) ) )
								{
									addDependency( outputAttach
										, inputAttach
										, inputTransitions
										, outputTransitions
										, allTransitions );
								}
							}
						}

						auto it = std::find_if( all.begin()
							, all.end()
							, [&input]( ViewAttaches const & lookup )
							{
								return lookup.view == input.view;
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

			printDebug( inputs
				, outputs
				, inputTransitions
				, outputTransitions );
		}
	}
}
