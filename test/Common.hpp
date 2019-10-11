#pragma once

#include <RenderGraph/RenderGraphPrerequisites.hpp>

#include "BaseTest.hpp"

namespace test
{
	crg::ImageData createImage( VkFormat format
		, uint32_t mipLevels = 1u );
	crg::ImageViewData createView( crg::ImageData image
		, uint32_t baseMipLevel = 0u
		, uint32_t levelCount = 1u );
	crg::ImageViewData createView( crg::ImageId image
		, VkFormat format
		, uint32_t baseMipLevel = 0u
		, uint32_t levelCount = 1u );

	void display( TestCounts & testCounts
		, std::ostream & stream
		, crg::RenderGraph & value );
	void display( TestCounts & testCounts
		, crg::RenderGraph & value );

	template< typename TypeT >
	crg::Id< TypeT > makeId( TypeT const & data )
	{
		return { 0u, nullptr };
	}
}
