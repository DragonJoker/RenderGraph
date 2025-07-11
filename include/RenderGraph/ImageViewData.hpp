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
		ImageViewCreateInfo info;
		ImageViewIdArray source{};

		explicit ImageViewData( std::string name = {}
			, ImageId image = ImageId{}
			, ImageViewCreateFlags flags = {}
			, ImageViewType viewType = {}
			, PixelFormat format = {}
			, ImageSubresourceRange subresourceRange = {} )
			: name{ std::move( name ) }
			, image{ std::move( image ) }
			, info{ flags, viewType, format, subresourceRange }
		{
		}

	private:
		friend bool operator==( ImageViewData const & lhs, ImageViewData const & rhs )
		{
			return lhs.image == rhs.image
				&& lhs.info == rhs.info;
		}
	};
}
