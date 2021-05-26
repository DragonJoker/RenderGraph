﻿/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "RenderPassDependenciesBuilder.hpp"

#include "RenderGraph/Exception.hpp"
#include "RenderGraph/RenderPass.hpp"

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
			struct PassAttach
			{
				Attachment const attach;
				std::set< RenderPass const * > passes;
			};

			using PassAttachCont = std::vector< PassAttach >;

			std::ostream & operator<<( std::ostream & stream, PassAttach const & attach )
			{
				stream << attach.attach.name;
				std::string sep{ " -> " };

				for ( auto & pass : attach.passes )
				{
					stream << sep << pass->name;
					sep = ", ";
				}

				return stream;
			}

			std::ostream & operator<<( std::ostream & stream, PassAttachCont const & attaches )
			{
				for ( auto & attach : attaches )
				{
					stream << attach << std::endl;
				}

				return stream;
			}

			std::ostream & operator<<( std::ostream & stream, RenderPassDependencies const & dependency )
			{
				stream << dependency.srcPass->name << " -> " << dependency.dstPass->name;
				std::string sep = " [";

				for ( auto & attach : dependency.srcOutputs )
				{
					stream << sep << attach.name;
					sep = ", ";
				}

				for ( auto & attach : dependency.dstInputs )
				{
					stream << sep << attach.name;
					sep = ", ";
				}

				stream << "]";
				return stream;
			}

			std::ostream & operator<<( std::ostream & stream, RenderPassDependenciesArray const & dependencies )
			{
				for ( auto & dependency : dependencies )
				{
					stream << dependency << std::endl;
				}

				return stream;
			}

			void printDebug( PassAttachCont const & sampled
				, PassAttachCont const & inputs
				, PassAttachCont const & outputs
				, RenderPassDependenciesArray const & dependencies )
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
				std::clog << "Dependencies" << std::endl;
				std::clog << dependencies << std::endl;
#endif
			}

			inline bool isInRange( uint32_t value
				, uint32_t left
				, uint32_t count )
			{
				return value >= left && value < left + count;
			}

			inline bool areIntersecting( uint32_t lhsLBound
				, uint32_t lhsCount
				, uint32_t rhsLBound
				, uint32_t rhsCount )
			{
				return isInRange( lhsLBound, rhsLBound, rhsCount )
					|| isInRange( rhsLBound, lhsLBound, lhsCount );
			}

			inline bool areIntersecting( VkImageSubresourceRange const & lhs
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

			inline bool areOverlapping( ImageViewId const & lhs
				, ImageViewId const & rhs )
			{
				return lhs.data->image == rhs.data->image
					&& areIntersecting( lhs.data->subresourceRange
						, rhs.data->subresourceRange );
			}

			void processAttach( Attachment const & attach
				, RenderPass const & pass
				, PassAttachCont & cont
				, std::function< bool( Attachment const & ) > processAttach )
			{
				bool found{ false };
				std::vector< RenderPass const * > passes;

				for ( auto & lookup : cont )
				{
					if ( processAttach( lookup.attach ) )
					{
						if ( lookup.passes.end() == std::find( lookup.passes.begin()
							, lookup.passes.end()
							, &pass ) )
						{
							lookup.passes.insert( &pass );
						}

						found = true;
					}
				}

				auto it = std::find_if( cont.begin()
					, cont.end()
					, [&attach]( PassAttach const & lookup )
					{
						return lookup.attach.name == attach.name
							&& lookup.attach.view == attach.view;
					} );

				if ( cont.end() == it )
				{
					cont.push_back( PassAttach{ attach } );
					it = std::prev( cont.end() );
				}

				it->passes.insert( &pass );
			}

			void processSampledAttach( Attachment const & attach
				, RenderPass const & pass
				, PassAttachCont & cont )
			{
				if ( attach.isSampled() )
				{
					processAttach( attach
						, pass
						, cont
						, [&attach]( Attachment const & lookup )
						{
							return areOverlapping( lookup.view, attach.view );
						} );
				}
			}

			void processColourInputAttach( Attachment const & attach
				, RenderPass const & pass
				, PassAttachCont & cont )
			{
				if ( attach.isColourInput() )
				{
					processAttach( attach
						, pass
						, cont
						, [&attach]( Attachment const & lookup )
						{
							return areOverlapping( lookup.view, attach.view );
						} );
				}
			}

			void processColourOutputAttach( Attachment const & attach
				, RenderPass const & pass
				, PassAttachCont & cont )
			{
				if ( attach.isColourOutput() )
				{
					return processAttach( attach
						, pass
						, cont
						, [&attach]( Attachment const & lookup )
						{
							return areOverlapping( lookup.view, attach.view );
						} );
				}
			}

			void processDepthOrStencilInputAttach( Attachment const & attach
				, RenderPass const & pass
				, PassAttachCont & cont )
			{
				if ( attach.isDepthInput()
					|| attach.isStencilInput() )
				{
					processAttach( attach
						, pass
						, cont
						, [&attach]( Attachment const & lookup )
						{
							return areOverlapping( lookup.view, attach.view );
						} );
				}
			}

			void processDepthOrStencilOutputAttach( Attachment const & attach
				, RenderPass const & pass
				, PassAttachCont & cont )
			{
				if ( attach.isDepthOutput()
					|| attach.isStencilOutput() )
				{
					processAttach( attach
						, pass
						, cont
						, [&attach]( Attachment const & lookup )
						{
							return areOverlapping( lookup.view, attach.view );
						} );
				}
			}

			void processSampledAttachs( AttachmentArray const & attachs
				, RenderPass const & pass
				, PassAttachCont & cont )
			{
				for ( auto & attach : attachs )
				{
					processSampledAttach( attach, pass, cont );
				}
			}

			void processColourInputAttachs( AttachmentArray const & attachs
				, RenderPass const & pass
				, PassAttachCont & cont )
			{
				for ( auto & attach : attachs )
				{
					processColourInputAttach( attach, pass, cont );
				}
			}

			void processColourOutputAttachs( AttachmentArray const & attachs
				, RenderPass const & pass
				, PassAttachCont & cont )
			{
				for ( auto & attach : attachs )
				{
					processColourOutputAttach( attach, pass, cont );
				}
			}

			void addDependency( Attachment const & outAttach
				, Attachment const & inAttach
				, std::set< RenderPass const * > const & srcs
				, std::set< RenderPass const * > const & dsts
				, RenderPassDependenciesArray & dependencies )
			{
				for ( auto & src : srcs )
				{
					for ( auto & dst : dsts )
					{
						if ( src != dst )
						{
							auto it = std::find_if( dependencies.begin()
								, dependencies.end()
								, [&dst, &src]( RenderPassDependencies & lookup )
								{
									return lookup.srcPass == src
										&& lookup.dstPass == dst;
								} );

							if ( it == dependencies.end() )
							{
								dependencies.push_back( { src, dst } );
								it = std::prev( dependencies.end() );
							}

							auto & dep = *it;

							if ( dep.srcOutputs.end() == std::find( dep.srcOutputs.begin()
								, dep.srcOutputs.end()
								, outAttach )
								|| dep.dstInputs.end() == std::find( dep.dstInputs.begin()
									, dep.dstInputs.end()
									, inAttach ) )
							{
								dep.srcOutputs.push_back( outAttach );
								dep.dstInputs.push_back( inAttach );
							}
						}
					}
				}
			}
		}

		RenderPassDependenciesArray buildPassDependencies( std::vector< RenderPassPtr > const & passes )
		{
			PassAttachCont sampled;
			PassAttachCont inputs;
			PassAttachCont outputs;

			for ( auto & pass : passes )
			{
				processSampledAttachs( pass->sampled, *pass, sampled );
				processColourInputAttachs( pass->colourInOuts, *pass, inputs );
				processColourOutputAttachs( pass->colourInOuts, *pass, outputs );

				if ( pass->depthStencilInOut )
				{
					processDepthOrStencilInputAttach( *pass->depthStencilInOut, *pass, inputs );
					processDepthOrStencilOutputAttach( *pass->depthStencilInOut, *pass, outputs );
				}
			}

			RenderPassDependenciesArray result;

			for ( auto & output : outputs )
			{
				for ( auto & input : inputs )
				{
					if ( areOverlapping( output.attach.view, input.attach.view ) )
					{
						addDependency( output.attach
							, input.attach
							, output.passes
							, input.passes
							, result );
					}
				}

				for ( auto & sample : sampled )
				{
					if ( areOverlapping( output.attach.view, sample.attach.view ) )
					{
						addDependency( output.attach
							, sample.attach
							, output.passes
							, sample.passes
							, result );
					}
				}
			}

			printDebug( sampled, inputs, outputs, result );
			return result;
		}
	}
}
