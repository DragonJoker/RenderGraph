/*
This file belongs to RenderGraph.
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
		VkImageViewCreateFlags flags;
		VkImageViewType viewType;
		VkFormat format;
		VkImageSubresourceRange subresourceRange;
	};

	inline bool operator==( VkImageSubresourceRange const & lhs, VkImageSubresourceRange const & rhs )
	{
		return lhs.aspectMask == rhs.aspectMask
			&& lhs.baseArrayLayer == rhs.baseArrayLayer
			&& lhs.layerCount == rhs.layerCount
			&& lhs.baseMipLevel == rhs.baseMipLevel
			&& lhs.levelCount == rhs.levelCount;
	}

	inline bool operator==( ImageViewData const & lhs, ImageViewData const & rhs )
	{
		return lhs.name == rhs.name
			&& lhs.flags == rhs.flags
			&& lhs.viewType == rhs.viewType
			&& lhs.format == rhs.format
			&& lhs.subresourceRange == rhs.subresourceRange;
	}

	inline bool operator!=( ImageViewData const & lhs, ImageViewData const & rhs )
	{
		return !( lhs == rhs );
	}
}
