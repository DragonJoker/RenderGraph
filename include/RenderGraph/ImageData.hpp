/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraphPrerequisites.hpp"

namespace crg
{
	/**
	*\brief
	*	Basic image data, from which images will be created.
	*/
	struct ImageData
	{
		std::string name;
		VkImageCreateFlags flags;
		VkImageType imageType;
		VkFormat format;
		VkExtent2D extent;
		uint32_t mipLevels;
		uint32_t arrayLayers;
		VkSampleCountFlagBits samples;
		VkImageTiling tiling;
		VkImageUsageFlags usage;
	};

	inline bool operator==( ImageData const & lhs, ImageData const & rhs )
	{
		return lhs.name == rhs.name
			&& lhs.flags == rhs.flags
			&& lhs.imageType == rhs.imageType
			&& lhs.format == rhs.format
			&& lhs.mipLevels == rhs.mipLevels
			&& lhs.arrayLayers == rhs.arrayLayers
			&& lhs.samples == rhs.samples
			&& lhs.tiling == rhs.tiling
			&& lhs.usage == rhs.usage;
	}

	inline bool operator!=( ImageData const & lhs, ImageData const & rhs )
	{
		return !( lhs == rhs );
	}
}
