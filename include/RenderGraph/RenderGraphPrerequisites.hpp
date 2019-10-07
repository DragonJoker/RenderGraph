/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <algorithm>
#include <memory>
#include <string>
#include <set>
#include <vector>

namespace crg
{
	template< typename DataT >
	struct Id;

	struct Attachment;
	struct ImageData;
	struct ImageViewData;
	struct RenderGraphNode;
	struct RenderPass;
	struct RenderPassDependencies;

	class RenderGraph;

	using ImageId = Id < ImageData >;
	using ImageViewId = Id < ImageViewData >;

	using RenderPassPtr = std::unique_ptr< RenderPass >;
	using RenderGraphNodePtr = std::unique_ptr< RenderGraphNode >;

	using AttachmentArray = std::vector< Attachment >;
	using RenderPassPtrArray = std::vector< RenderPassPtr >;
	using RenderGraphNodePtrArray = std::vector< RenderGraphNodePtr >;
	using RenderPassDependenciesArray = std::vector< RenderPassDependencies >;
}
