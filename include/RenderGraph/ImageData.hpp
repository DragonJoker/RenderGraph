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
		VkImageCreateInfo info;

		ImageData( std::string name = {}
			, VkImageCreateFlags flags = {}
			, VkImageType imageType = {}
			, VkFormat format = {}
			, VkExtent3D extent = {}
			, VkImageUsageFlags usage = {}
			, uint32_t mipLevels = 1u
			, uint32_t arrayLayers = 1u
			, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT
			, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL )
			: name{ std::move( name ) }
			, info{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO
				, nullptr
				, flags
				, imageType
				, format
				, extent
				, mipLevels
				, arrayLayers
				, samples
				, tiling
				, usage
				, VK_SHARING_MODE_EXCLUSIVE
				, 0u
				, nullptr
				, VK_IMAGE_LAYOUT_UNDEFINED }
		{
		}
	};

	inline bool operator==( VkExtent3D const & lhs, VkExtent3D const & rhs )
	{
		return lhs.width == rhs.width
			&& lhs.height == rhs.height
			&& lhs.depth == rhs.depth;
	}

	inline bool operator==( VkImageCreateInfo const & lhs, VkImageCreateInfo const & rhs )
	{
		return lhs.flags == rhs.flags
			&& lhs.imageType == rhs.imageType
			&& lhs.format == rhs.format
			&& lhs.extent == rhs.extent
			&& lhs.mipLevels == rhs.mipLevels
			&& lhs.arrayLayers == rhs.arrayLayers
			&& lhs.samples == rhs.samples
			&& lhs.tiling == rhs.tiling
			&& lhs.usage == rhs.usage;
	}

	inline bool operator==( ImageData const & lhs, ImageData const & rhs )
	{
		return lhs.name == rhs.name
			&& lhs.info == rhs.info;
	}

	inline bool operator!=( ImageData const & lhs, ImageData const & rhs )
	{
		return !( lhs == rhs );
	}
}
