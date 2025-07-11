/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "FrameGraphFunctions.hpp"

namespace crg
{
	static constexpr PipelineColorBlendAttachmentState DefaultBlendState;

	template< typename TypeT >
	static inline const TypeT defaultV = DefaultValueGetterT< TypeT >::get();

	template< typename TypeT >
	static inline TypeT getDefaultV()
	{
		return DefaultValueGetterT< TypeT >::get();
	}

	template<>
	struct DefaultValueGetterT< VkPipelineVertexInputStateCreateInfo >
	{
		static VkPipelineVertexInputStateCreateInfo get()
		{
			VkPipelineVertexInputStateCreateInfo const result{ []()
				{
					return VkPipelineVertexInputStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
						, nullptr
						, {}
						, {}
						, {}
						, {}
						, {} };
				}() };
			return result;
		}
	};

	template<>
	struct DefaultValueGetterT< IndirectBuffer >
	{
		static IndirectBuffer get()
		{
			IndirectBuffer const result{ Buffer{ VkBuffer{}, std::string{} }, 0u };
			return result;
		}
	};

	template< typename TypeT >
	using RawTypeT = typename RawTyperT< TypeT >::Type;
}
