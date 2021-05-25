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
				std::set< Attachment const * > attaches;
			};

			using ViewAttachesArray = std::vector< ViewAttaches >;

			std::ostream & operator<<( std::ostream & stream, ViewAttaches const & attach )
			{
				stream << attach.view.data->name;
				std::string sep{ " -> " };

				for ( auto & attach : attach.attaches )
				{
					stream << sep << attach->name;
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
				stream << transition.srcAttach.pass->name << " -> " << transition.dstAttach.pass->name;
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

			void printDebug( ViewAttachesArray const & sampled
				, ViewAttachesArray const & inputs
				, ViewAttachesArray const & outputs
				, FramePassDependenciesMap const & inputTransitions
				, FramePassDependenciesMap const & outputTransitions )
			{
#if CRG_DebugPassAttaches
				std::clog << "Sampled" << std::endl;
				std::clog << sampled << std::endl;
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
				return areOverlapping( *lhs.view.data, *rhs.view.data );
			}

			void insertAttach( Attachment const & attach
				, FramePass const & pass
				, ViewAttachesArray & cont )
			{
				auto it = std::find_if( cont.begin()
					, cont.end()
					, [&attach]( ViewAttaches const & lookup )
					{
						return lookup.view == attach.view;
					} );

				if ( cont.end() == it )
				{
					cont.push_back( ViewAttaches{ attach.view } );
					it = std::prev( cont.end() );
				}

				it->attaches.insert( &attach );
			}

			void processAttach( Attachment const & attach
				, FramePass const & pass
				, ViewAttachesArray & cont
				, ViewAttachesArray & all
				, std::function< bool( ImageViewId const & ) > processAttach )
			{
				bool found{ false };
				std::vector< FramePass const * > passes;

				for ( auto & lookup : cont )
				{
					if ( processAttach( lookup.view ) )
					{
						if ( lookup.attaches.end() == std::find( lookup.attaches.begin()
							, lookup.attaches.end()
							, &attach ) )
						{
							lookup.attaches.insert( &attach );
						}

						found = true;
					}
				}

				insertAttach( attach, pass, cont );
				insertAttach( attach, pass, all );
			}

			void processSampledAttach( Attachment const & attach
				, FramePass const & pass
				, ViewAttachesArray & cont
				, ViewAttachesArray & all )
			{
				if ( attach.isSampled() )
				{
					processAttach( attach
						, pass
						, cont
						, all
						, [&attach]( ImageViewId const & lookup )
						{
							return areOverlapping( *lookup.data, *attach.view.data );
						} );
				}
			}

			void processColourInputAttach( Attachment const & attach
				, FramePass const & pass
				, ViewAttachesArray & cont
				, ViewAttachesArray & all )
			{
				if ( attach.isColourInput() )
				{
					processAttach( attach
						, pass
						, cont
						, all
						, [&attach]( ImageViewId const & lookup )
						{
							return areOverlapping( *lookup.data, *attach.view.data );
						} );
				}
			}

			void processColourOutputAttach( Attachment const & attach
				, FramePass const & pass
				, ViewAttachesArray & cont
				, ViewAttachesArray & all )
			{
				if ( attach.isColourOutput() )
				{
					return processAttach( attach
						, pass
						, cont
						, all
						, [&attach]( ImageViewId const & lookup )
						{
							return areOverlapping( *lookup.data, *attach.view.data );
						} );
				}
			}

			void processDepthOrStencilInputAttach( Attachment const & attach
				, FramePass const & pass
				, ViewAttachesArray & cont
				, ViewAttachesArray & all )
			{
				if ( attach.isDepthInput()
					|| attach.isStencilInput() )
				{
					processAttach( attach
						, pass
						, cont
						, all
						, [&attach]( ImageViewId const & lookup )
						{
							return areOverlapping( *lookup.data, *attach.view.data );
						} );
				}
			}

			void processDepthOrStencilOutputAttach( Attachment const & attach
				, FramePass const & pass
				, ViewAttachesArray & cont
				, ViewAttachesArray & all )
			{
				if ( attach.isDepthOutput()
					|| attach.isStencilOutput() )
				{
					processAttach( attach
						, pass
						, cont
						, all
						, [&attach]( ImageViewId const & lookup )
						{
							return areOverlapping( *lookup.data, *attach.view.data );
						} );
				}
			}

			void processSampledAttachs( AttachmentArray const & attachs
				, FramePass const & pass
				, ViewAttachesArray & cont
				, ViewAttachesArray & all )
			{
				for ( auto & attach : attachs )
				{
					processSampledAttach( attach, pass, cont, all );
				}
			}

			void processColourInputAttachs( AttachmentArray const & attachs
				, FramePass const & pass
				, ViewAttachesArray & cont
				, ViewAttachesArray & all )
			{
				for ( auto & attach : attachs )
				{
					processColourInputAttach( attach, pass, cont, all );
				}
			}

			void processColourOutputAttachs( AttachmentArray const & attachs
				, FramePass const & pass
				, ViewAttachesArray & cont
				, ViewAttachesArray & all )
			{
				for ( auto & attach : attachs )
				{
					processColourOutputAttach( attach, pass, cont, all );
				}
			}

			void addRemainingDependency( Attachment const & attach
				, FramePassDependenciesMap & inputTransitions
				, AttachmentTransitionArray & allTransitions )
			{
				auto & transitions = inputTransitions.emplace( attach.pass, AttachmentTransitionArray{} ).first->second;

				if ( attach.isColourInOut() )
				{
					transitions.push_back( { attach.view, attach, attach } );
				}
				else if ( attach.isColourInput()
						|| attach.isSampled() )
				{
					transitions.push_back( { attach.view, Attachment::createDefault( attach.view ), attach } );
				}
				else
				{
					transitions.push_back( { attach.view, attach, Attachment::createDefault( attach.view ) } );
				}

				allTransitions.push_back( transitions.back() );
			}

			void addDependency( Attachment const & outputAttach
				, Attachment const & inputAttach
				, FramePassDependenciesMap & inputTransitions
				, FramePassDependenciesMap & outputTransitions
				, AttachmentTransitionArray & allTransitions )
			{
				assert( outputAttach.view == inputAttach.view );
				AttachmentTransition transition{ outputAttach.view, outputAttach, inputAttach };
				allTransitions.push_back( transition );
				{
					auto & transitions = inputTransitions.emplace( inputAttach.pass, AttachmentTransitionArray{} ).first->second;
					transitions.push_back( transition );
				}
				{
					auto & transitions = outputTransitions.emplace( outputAttach.pass, AttachmentTransitionArray{} ).first->second;
					transitions.push_back( transition );
				}
			}
		}

		void buildPassAttachDependencies( std::vector< FramePassPtr > const & passes
			, FramePassDependenciesMap & inputTransitions
			, FramePassDependenciesMap & outputTransitions
			, AttachmentTransitionArray & allTransitions )
		{
			ViewAttachesArray sampled;
			ViewAttachesArray inputs;
			ViewAttachesArray outputs;
			ViewAttachesArray all;

			for ( auto & pass : passes )
			{
				processSampledAttachs( pass->sampled, *pass, inputs, all );
				processSampledAttachs( pass->storage, *pass, inputs, all );
				processColourInputAttachs( pass->colourInOuts, *pass, inputs, all );
				processColourInputAttachs( pass->transferInOuts, *pass, inputs, all );
				processColourOutputAttachs( pass->colourInOuts, *pass, outputs, all );
				processColourOutputAttachs( pass->transferInOuts, *pass, outputs, all );

				if ( pass->depthStencilInOut )
				{
					processDepthOrStencilInputAttach( *pass->depthStencilInOut, *pass, inputs, all );
					processDepthOrStencilOutputAttach( *pass->depthStencilInOut, *pass, outputs, all );
				}
			}

			for ( auto & pass : passes )
			{
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
								if ( inputAttach->pass->directDependsOn( *outputAttach->pass ) )
								{
									addDependency( *outputAttach
										, *inputAttach
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
					addRemainingDependency( *attach
						, inputTransitions
						, allTransitions );
				}
			}

			printDebug( sampled
				, inputs
				, outputs
				, inputTransitions
				, outputTransitions );
		}
	}
}
