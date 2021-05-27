/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "Id.hpp"

namespace crg
{
	/**
	*\brief
	*	Basic image view data, from which views will be created.
	*/
	struct ImageViewData
	{
		std::string name;
		ImageId image;
		VkImageViewCreateInfo info;
		ImageViewIdArray source;

		ImageViewData( std::string name = {}
			, ImageId image = {}
			, VkImageViewCreateFlags flags = {}
			, VkImageViewType viewType = {}
			, VkFormat format = {}
			, VkImageSubresourceRange subresourceRange = {} )
			: name{ std::move( name ) }
			, image{ std::move( image ) }
			, info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
				, nullptr
				, flags
				, VK_NULL_HANDLE
				, viewType
				, format
				, { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }
				, subresourceRange }
		{
		}
	};

	inline bool operator==( VkImageSubresourceRange const & lhs, VkImageSubresourceRange const & rhs )
	{
		return lhs.aspectMask == rhs.aspectMask
			&& lhs.baseArrayLayer == rhs.baseArrayLayer
			&& lhs.layerCount == rhs.layerCount
			&& lhs.baseMipLevel == rhs.baseMipLevel
			&& lhs.levelCount == rhs.levelCount;
	}

	inline bool operator==( VkImageViewCreateInfo const & lhs, VkImageViewCreateInfo const & rhs )
	{
		return lhs.flags == rhs.flags
			&& lhs.viewType == rhs.viewType
			&& lhs.format == rhs.format
			&& lhs.subresourceRange == rhs.subresourceRange;
	}

	inline bool operator==( ImageViewData const & lhs, ImageViewData const & rhs )
	{
		return lhs.name == rhs.name
			&& lhs.info == rhs.info;
	}

	inline bool operator!=( ImageViewData const & lhs, ImageViewData const & rhs )
	{
		return !( lhs == rhs );
	}
}
