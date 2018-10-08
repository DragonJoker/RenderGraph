/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include <AshesPrerequisites.hpp>

#include <memory>
#include <vector>

namespace crg
{
	struct Attachment;
	struct RenderPass;

	class RenderGraph;

	using AttachmentArray = std::vector< Attachment >;

	using RenderPassPtr = std::unique_ptr< RenderPass >;
}
