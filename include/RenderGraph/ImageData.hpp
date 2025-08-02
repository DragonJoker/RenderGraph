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
	*	Basic image data, from which images will be created.
	*/
	struct ImageData
	{
		std::string name;
		ImageCreateInfo info;

		explicit ImageData( std::string name = {}
			, ImageCreateFlags flags = {}
			, ImageType imageType = {}
			, PixelFormat format = {}
			, Extent3D extent = {}
			, ImageUsageFlags usage = {}
			, uint32_t mipLevels = 1u
			, uint32_t arrayLayers = 1u
			, SampleCount samples = SampleCount::e1
			, ImageTiling tiling = ImageTiling::eOptimal
			, MemoryPropertyFlags memory = MemoryPropertyFlags::eDeviceLocal )
			: name{ std::move( name ) }
			, info{ flags, imageType, format
				, extent, mipLevels, arrayLayers, samples
				, tiling, usage, memory }
		{
		}

	private:
		friend bool operator==( ImageData const & lhs, ImageData const & rhs ) = default;
	};
}
