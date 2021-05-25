/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FramePassDependencies.hpp"
#include "RenderGraph/AttachmentTransition.hpp"

#include <cassert>
#include <map>

namespace crg
{
	/**
	*\brief
	*	Base class for all graph nodes
	*\remarks
	*	It holds a name, the next nodes, and its dependencies from the previous nodes.
	*/
	struct GraphNode
	{
		enum class Kind
		{
			Undefined,
			Root,
			FramePass,
		};

		CRG_API virtual ~GraphNode() = default;
		CRG_API void addAttaches( GraphAdjacentNode prev
			, AttachmentTransitionArray inputAttaches );
		CRG_API void attachNode( GraphAdjacentNode next
			, AttachmentTransitionArray inputAttaches );
		CRG_API GraphAdjacentNode findInNext( FramePass const & pass )const;
		CRG_API AttachmentTransitionArray const & getInputAttaches( ConstGraphAdjacentNode pred = nullptr )const;

		CRG_API virtual void accept( GraphVisitor * vis )const = 0;

		template< typename NodeT >
		NodeT const & cast()const
		{
			assert( kind == NodeT::MyKind );
			return static_cast< NodeT const & >( *this );
		}

		template< typename NodeT >
		NodeT & cast()
		{
			assert( kind == NodeT::MyKind );
			return static_cast< NodeT & >( *this );
		}

		inline Kind getKind()const
		{
			return kind;
		}

		inline std::string const & getName()const
		{
			return name;
		}

		inline auto & getNext()const
		{
			return next;
		}

	protected:
		CRG_API GraphNode( Kind kind
			, std::string name );

	protected:
		Kind kind;
		std::string name;
		GraphAdjacentNodeArray next;
		AttachmentsNodeMap inputAttaches;
	};
	/**
	*\brief
	*	Graph node coming from a render pass
	*\remarks
	*	The node name wil be the FramePass name.
	*/
	struct FramePassNode
		: public GraphNode
	{
		static constexpr Kind MyKind = Kind::FramePass;

		CRG_API FramePassNode( FramePass const & pass );
		CRG_API void accept( GraphVisitor * vis )const override;

		inline FramePass const & getFramePass()const
		{
			return *pass;
		}

	private:
		FramePass const * pass;
	};
	/**
	*\brief
	*	Root node for the graph.
	*\remarks
	*	Logically, it has no dependency with a previous node.
	*/
	struct RootNode
		: public GraphNode
	{
		static constexpr Kind MyKind = Kind::Root;

		CRG_API RootNode( std::string name );
		CRG_API void accept( GraphVisitor * vis )const override;
	};

	CRG_API FramePass const * getFramePass( GraphNode const & node );

	inline bool isFramePassNode( GraphNode const & node )
	{
		return node.getKind() == FramePassNode::MyKind;
	}

	inline bool isRootNode( GraphNode const & node )
	{
		return node.getKind() == RootNode::MyKind;
	}

	inline bool isFramePassNode( ConstGraphAdjacentNode node )
	{
		return node && isFramePassNode( *node );
	}

	inline bool isRootNode( ConstGraphAdjacentNode node )
	{
		return node && isRootNode( *node );
	}

	inline bool isFramePassNode( GraphNodePtr const & node )
	{
		return node && isFramePassNode( *node );
	}

	inline bool isRootNode( GraphNodePtr const & node )
	{
		return node && isRootNode( *node );
	}

	template< typename NodeT >
	NodeT const & nodeCast( GraphNode const & node )
	{
		return node.cast< NodeT >();
	}

	template< typename NodeT >
	NodeT & nodeCast( GraphNode & node )
	{
		return node.cast< NodeT >();
	}
}
