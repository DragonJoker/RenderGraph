/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

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

		GraphNode( GraphNode const & ) = delete;
		GraphNode & operator=( GraphNode const & ) = delete;
		CRG_API GraphNode & operator=( GraphNode && rhs )noexcept = default;
		CRG_API virtual ~GraphNode()noexcept = default;
		CRG_API GraphNode( GraphNode && rhs )noexcept;

		CRG_API void attachNode( GraphNode & child );

		CRG_API virtual void accept( GraphVisitor * vis )const = 0;

		void setTransitions( AttachmentTransitions transitions )noexcept
		{
			m_transitions = std::move( transitions );
		}

		template< typename NodeT >
		NodeT const & cast()const noexcept
		{
			assert( kind == NodeT::MyKind );
			return static_cast< NodeT const & >( *this );
		}

		template< typename NodeT >
		NodeT & cast()noexcept
		{
			assert( kind == NodeT::MyKind );
			return static_cast< NodeT & >( *this );
		}

		Kind getKind()const noexcept
		{
			return kind;
		}

		std::string const & getName()const noexcept
		{
			return name;
		}

		FramePassGroup const & getGroup()const noexcept
		{
			return *group;
		}

		uint32_t getId()const noexcept
		{
			return id;
		}

		GraphAdjacentNodeArray const & getPredecessors()const noexcept
		{
			return prev;
		}

		GraphAdjacentNodeArray & getPredecessors()noexcept
		{
			return prev;
		}

		ImageTransitionArray const & getImageTransitions()const noexcept
		{
			return m_transitions.imageTransitions;
		}

		BufferTransitionArray const & getBufferTransitions()const noexcept
		{
			return m_transitions.bufferTransitions;
		}

	protected:
		CRG_API GraphNode( Kind kind
			, uint32_t id
			, std::string name
			, FramePassGroup const & group );

	private:
		Kind kind{};
		uint32_t id{};
		std::string name{};
		FramePassGroup const * group;
		GraphAdjacentNodeArray prev{};
		AttachmentTransitions m_transitions{};
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

		CRG_API explicit FramePassNode( FramePass const & pass );
		CRG_API void accept( GraphVisitor * vis )const override;

		FramePass const & getFramePass()const
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

		CRG_API explicit RootNode( FrameGraph const & graph );
		CRG_API void accept( GraphVisitor * vis )const override;

		FrameGraph const & getFrameGraph()const
		{
			return *graph;
		}

	private:
		FrameGraph const * graph;
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
