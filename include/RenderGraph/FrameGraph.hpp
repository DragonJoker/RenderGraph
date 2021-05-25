/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "Attachment.hpp"
#include "ImageData.hpp"
#include "ImageViewData.hpp"
#include "GraphNode.hpp"
#include "FramePass.hpp"

#include <map>
#include <vector>

namespace crg
{
	class FrameGraph
	{
		friend class RunnableGraph;

	public:
		CRG_API FrameGraph( std::string name = "FrameGraph" );
		CRG_API FramePass & createPass( std::string const & name
			, RunnablePassCreator runnableCreator );
		CRG_API void compile();
		CRG_API ImageId createImage( ImageData const & img );
		CRG_API ImageViewId createView( ImageViewData const & view );

		GraphAdjacentNode getGraph()
		{
			return &m_root;
		}

		ConstGraphAdjacentNode getGraph()const
		{
			return &m_root;
		}

		AttachmentTransitionArray const & getTransitions()const
		{
			return m_transitions;
		}

		FramePassDependenciesMap const & getInputTransitions()const
		{
			return m_inputTransitions;
		}

		FramePassDependenciesMap const & getOutputTransitions()const
		{
			return m_outputTransitions;
		}

	private:
		FramePassPtrArray m_passes;
		ImageIdDataOwnerCont m_images;
		ImageViewIdDataOwnerCont m_imageViews;
		GraphNodePtrArray m_nodes;
		AttachmentTransitionArray m_transitions;
		// Transitions for which the pass is the destination.
		FramePassDependenciesMap m_inputTransitions;
		// Transitions for which the pass is the source.
		FramePassDependenciesMap m_outputTransitions;
		RootNode m_root;
		ImageIdAliasMap m_imageAliases;
		ImageViewIdAliasMap m_imageViewAliases;
		std::map< std::string, ImageViewId > m_attachViews;
	};
}
