#pragma once

#include <RenderGraph/FrameGraphPrerequisites.hpp>

#include "BaseTest.hpp"

namespace test
{
	crg::ImageData createImage( std::string name
		, VkFormat format
		, uint32_t mipLevels = 1u );
	crg::ImageViewData createView( std::string name
		, crg::ImageId image
		, uint32_t baseMipLevel = 0u
		, uint32_t levelCount = 1u );
	crg::ImageViewData createView( std::string name
		, crg::ImageId image
		, VkFormat format
		, uint32_t baseMipLevel = 0u
		, uint32_t levelCount = 1u );

	void display( TestCounts & testCounts
		, std::ostream & stream
		, crg::FrameGraph & value );
	void display( TestCounts & testCounts
		, crg::FrameGraph & value );

	template< typename TypeT >
	crg::Id< TypeT > makeId( TypeT const & data )
	{
		return { 0u, nullptr };
	}
}
