/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/RenderGraph.hpp"

#include "RenderGraph/RenderPass.hpp"

namespace crg
{
	namespace
	{
		struct PassAttach
		{
			Attachment const attach;
			std::vector< RenderPass const * > passes;
		};
		using PassAttachCont = std::vector< PassAttach >;

		void doProcessAttach( Attachment const & attach
			, RenderPass const & pass
			, PassAttachCont & cont )
		{
			auto it = std::find_if( cont.begin()
				, cont.end()
				, [&attach]( PassAttach const & lookup )
				{
					return &attach.view == &lookup.attach.view;
				} );

			if ( cont.end() == it )
			{
				cont.push_back( PassAttach{ attach } );
				it = cont.begin() + cont.size() - 1u;
			}

			it->passes.push_back( &pass );
		}

		void doProcessAttachs( AttachmentArray const & attachs
			, RenderPass const & pass
			, PassAttachCont & cont )
		{
			for ( auto & attach : attachs )
			{
				doProcessAttach( attach, pass, cont );
			}
		}

		std::set< RenderPass * > doRetrieveRoots( std::vector< RenderPassPtr > const & passes
			, std::vector< RenderGraph::RenderPassDependencies > const & dependencies )
		{
			std::set< RenderPass * > result;

			for ( auto & pass : passes )
			{
				if ( dependencies.end() == std::find_if( dependencies.begin()
					, dependencies.end()
					, [&pass]( RenderGraph::RenderPassDependencies const & lookup )
					{
						return lookup.dstPass == pass.get();
					} ) )
				{
					result.insert( pass.get() );
				}
			}

			return result;
		}

		std::set< RenderPass * > doRetrieveLeafs( std::vector< RenderPassPtr > const & passes
			, std::vector< RenderGraph::RenderPassDependencies > const & dependencies )
		{
			std::set< RenderPass * > result;

			for ( auto & pass : passes )
			{
				if ( dependencies.end() == std::find_if( dependencies.begin()
					, dependencies.end()
					, [&pass]( RenderGraph::RenderPassDependencies const & lookup )
					{
						return lookup.srcPass == pass.get();
					} ) )
				{
					result.insert( pass.get() );
				}
			}

			return result;
		}

		std::vector< RenderGraph::RenderPassDependencies > doBuildPath( RenderPass * root
			, RenderPass * leaf
			, std::vector< RenderGraph::RenderPassDependencies > const & dependencies )
		{

		}

		void doMergePaths( std::vector< std::vector< RenderGraph::RenderPassDependencies > > const & dependencies )
		{
		}
	}

	RenderGraph::RenderGraph( ashes::Device const & device )
		: m_device{ device }
	{
	}

	void RenderGraph::add( RenderPass const & pass )
	{
		if ( m_passes.end() != std::find_if( m_passes.begin()
			, m_passes.end()
			, [&pass]( RenderPassPtr const & lookup )
			{
				return lookup->name == pass.name;
			} ) )
		{
			throw std::runtime_error{ "Duplicate RenderPass name detected." };
		}

		m_passes.push_back( std::make_unique< RenderPass >( pass ) );
	}

	void RenderGraph::remove( RenderPass const & pass )
	{
		auto it = std::find_if( m_passes.begin()
			, m_passes.end()
			, [&pass]( RenderPassPtr const & lookup )
			{
				return lookup->name == pass.name;
			} );

		if ( m_passes.end() == it )
		{
			throw std::runtime_error{ "RenderPass was not found." };
		}

		m_passes.erase( it );
	}

	bool RenderGraph::compile()
	{
		if ( m_passes.empty() )
		{
			throw std::runtime_error{ "No RenderPass registered." };
		}

		doUpdateDependencies();
		doReduceDependencies();
		doBuildGraph();
		return true;
	}

	void RenderGraph::doUpdateDependencies()
	{
		PassAttachCont inputs;
		PassAttachCont outputs;

		for ( auto & pass : m_passes )
		{
			doProcessAttachs( pass->inputs, *pass, inputs );
			doProcessAttachs( pass->colourOutputs, *pass, outputs );

			if ( pass->depthStencilOutput )
			{
				doProcessAttach( *pass->depthStencilOutput, *pass, outputs );
			}
		}

		for ( auto & input : inputs )
		{
			for ( auto & output : outputs )
			{
				if ( &output.attach.view == &input.attach.view )
				{
					doAddDependency( output.attach
						, output.passes
						, input.passes );
				}
			}
		}
	}

	void RenderGraph::doReduceDependencies()
	{
	}

	void RenderGraph::doBuildGraph()
	{
		// Retrieve roots and leave passes.
		auto roots = doRetrieveRoots( m_passes, m_dependencies );
		auto leafs = doRetrieveLeafs( m_passes, m_dependencies );

		// Build paths from each root pass to leaf pass
		std::vector< std::vector< RenderPassDependencies > > paths;
		for ( auto & root : roots )
		{
			for ( auto & leaf : roots )
			{
				auto path = doBuildPath( root, leaf, m_dependencies );

				if ( !path.empty() )
				{
					paths.push_back( path );
				}
			}
		}

		// Merge the paths to the final graph
		doMergePaths( paths );
	}

	void RenderGraph::doAddDependency( Attachment const & attach
		, std::vector< RenderPass const * > const & srcs
		, std::vector< RenderPass const * > const & dsts )
	{
		for ( auto & src : srcs )
		{
			for ( auto & dst : dsts )
			{
				auto it = std::find_if( m_dependencies.begin()
					, m_dependencies.end()
					, [&dst, &src]( RenderPassDependencies const & lookup )
					{
						return lookup.dstPass == dst
							&& lookup.srcPass == src;
					} );

				if ( it == m_dependencies.end() )
				{
					m_dependencies.push_back( RenderPassDependencies{ src, dst } );
					it = m_dependencies.begin() + ( m_dependencies.size() - 1u );
				}

				it->dependencies.push_back( &attach.view );
			}
		}
	}
}
