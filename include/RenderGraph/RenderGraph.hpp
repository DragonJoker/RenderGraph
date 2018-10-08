/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "Attachment.hpp"
#include "RenderPass.hpp"

#include <vector>

namespace crg
{
	class RenderGraph
	{
	public:
		struct RenderPassDependencies
		{
			RenderPass const * srcPass;
			RenderPass const * dstPass;
			std::vector< ashes::TextureView const * > dependencies;
		};

	public:
		RenderGraph( ashes::Device const & device );
		void add( RenderPass const & pass );
		void remove( RenderPass const & pass );
		bool compile();

	private:
		/**
		*\brief
		*	Fills the dependencies list (including loops).
		*/
		void doUpdateDependencies();
		/**
		*\brief
		*	Removes loops from dependencies list.
		*/
		void doReduceDependencies();
		/**
		*\brief
		*	Builds the render graph, and creates the render passes.
		*/
		void doBuildGraph();
		/**
		*\brief
		*	Adds dependencies over the attachment \p attach between \p srcs and \p dsts passes.
		*\remarks
		*	This will probably introduce loops (the fact that you can have multiple source passes).
		*/
		void doAddDependency( Attachment const & attach
			, std::vector< RenderPass const * > const & srcs
			, std::vector< RenderPass const * > const & dsts );

	private:
		ashes::Device const & m_device;
		std::vector< RenderPassPtr > m_passes;
		std::vector< Attachment > m_attachments;
		std::vector< RenderPassDependencies > m_dependencies;
	};
}
