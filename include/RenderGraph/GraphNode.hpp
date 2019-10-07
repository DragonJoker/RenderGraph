/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/RenderPassDependencies.hpp"

#include <cassert>

namespace crg
{
	struct GraphNode
	{
		enum class Kind
		{
			Root,
			RenderPass,
			Phi,
		};

		virtual ~GraphNode();
		virtual void accept( GraphVisitor * vis ) = 0;

		inline Kind getKind()const
		{
			return m_kind;
		}

		std::vector< GraphNode * > next;

	protected:
		GraphNode( Kind kind );

	private:
		Kind m_kind;
	};

	struct RenderPassNode
		: public GraphNode
	{
		RenderPassNode( RenderPass const * pass = nullptr
			, RenderPassDependenciesArray attachesToPrev = {} );
		void accept( GraphVisitor * vis ) override;

		RenderPass const * pass;
		RenderPassDependenciesArray attachesToPrev;
		static constexpr Kind kind = Kind::RenderPass;
	};

	struct RootNode
		: public GraphNode
	{
		RootNode();
		void accept( GraphVisitor * vis ) override;

		static constexpr Kind kind = Kind::Root;
	};

	struct PhiNode
		: public GraphNode
	{
		PhiNode( std::vector< GraphNode * > nodes = {} );
		void accept( GraphVisitor * vis ) override;

		std::vector< GraphNode * > nodes;
		static constexpr Kind kind = Kind::Phi;
	};

	template< typename NodeT >
	NodeT const & nodeCast( GraphNode const & node )
	{
		assert( NodeT::kind == node.getKind() );
		return static_cast< NodeT const & >( node );
	}

	template< typename NodeT >
	NodeT & nodeCast( GraphNode & node )
	{
		assert( NodeT::kind == node.getKind() );
		return static_cast< NodeT & >( node );
	}
}
