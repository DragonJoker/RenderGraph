#include "Common.hpp"

#include <RenderGraph/DotExport.hpp>
#include <RenderGraph/GraphVisitor.hpp>
#include <RenderGraph/ImageData.hpp>
#include <RenderGraph/ImageViewData.hpp>
#include <RenderGraph/FrameGraph.hpp>

#include <functional>
#include <map>
#include <sstream>

namespace test
{
	namespace
	{
		std::ostream & operator<<( std::ostream & stream
			, std::vector< crg::ImageViewId > const & values )
		{
			std::string sep;

			for ( auto & value : values )
			{
				stream << sep << value.id;
				sep = ", ";
			}

			return stream;
		}

		std::ostream & operator<<( std::ostream & stream
			, crg::FramePass const & value )
		{
			stream << value.name;
			return stream;
		}

		void displayPasses( TestCounts & testCounts
			, std::ostream & stream
			, crg::RunnableGraph & value )
		{
			crg::dot::displayPasses( stream, value );
			std::ofstream file{ testCounts.testName + ".dot" };
			crg::dot::displayPasses( file, value );
		}

		void displayTransitions( TestCounts & testCounts
			, std::ostream & stream
			, crg::RunnableGraph & value )
		{
			crg::dot::displayTransitions( stream, value );
			std::ofstream file{ testCounts.testName + "_transitions.dot" };
			crg::dot::displayTransitions( file, value );
		}

		bool isDepthFormat( VkFormat fmt )
		{
			return fmt == VK_FORMAT_D16_UNORM
				|| fmt == VK_FORMAT_X8_D24_UNORM_PACK32
				|| fmt == VK_FORMAT_D32_SFLOAT
				|| fmt == VK_FORMAT_D16_UNORM_S8_UINT
				|| fmt == VK_FORMAT_D24_UNORM_S8_UINT
				|| fmt == VK_FORMAT_D32_SFLOAT_S8_UINT;
		}

		bool isStencilFormat( VkFormat fmt )
		{
			return fmt == VK_FORMAT_S8_UINT
				|| fmt == VK_FORMAT_D16_UNORM_S8_UINT
				|| fmt == VK_FORMAT_D24_UNORM_S8_UINT
				|| fmt == VK_FORMAT_D32_SFLOAT_S8_UINT;
		}

		bool isColourFormat( VkFormat fmt )
		{
			return !isDepthFormat( fmt ) && !isStencilFormat( fmt );
		}

		bool isDepthStencilFormat( VkFormat fmt )
		{
			return isDepthFormat( fmt ) && isStencilFormat( fmt );
		}
	}

	crg::ImageData createImage( std::string name
		, VkFormat format
		, uint32_t mipLevels
		, uint32_t arrayLayers )
	{
		return crg::ImageData{ std::move( name )
			, 0u
			, VK_IMAGE_TYPE_2D
			, format
			, { 1024, 1024 }
			, ( VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
				| VK_IMAGE_USAGE_SAMPLED_BIT )
			, mipLevels
			, arrayLayers };
	}

	crg::ImageViewData createView( std::string name
		, crg::ImageId image
		, uint32_t baseMipLevel
		, uint32_t levelCount
		, uint32_t baseArrayLayer
		, uint32_t layerCount )
	{
		return createView( std::move( name )
			, image
			, image.data->info.format
			, baseMipLevel
			, levelCount
			, baseArrayLayer
			, layerCount );
	}

	crg::ImageViewData createView( std::string name
		, crg::ImageId image
		, VkFormat format
		, uint32_t baseMipLevel
		, uint32_t levelCount
		, uint32_t baseArrayLayer
		, uint32_t layerCount )
	{
		VkImageAspectFlags aspect = ( isDepthStencilFormat( format )
			? ( VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_DEPTH_BIT )
			: ( isDepthFormat( format )
				? VK_IMAGE_ASPECT_DEPTH_BIT
				: ( isStencilFormat( format )
					? VK_IMAGE_ASPECT_STENCIL_BIT
					: VK_IMAGE_ASPECT_COLOR_BIT ) ) );
		return crg::ImageViewData{ std::move( name )
			, image
			, 0u
			, VK_IMAGE_VIEW_TYPE_2D
			, format
			, { aspect, baseMipLevel, levelCount, baseArrayLayer, layerCount } };
	}

	void display( TestCounts & testCounts
		, std::ostream & stream
		, crg::RunnableGraph & value )
	{
		std::stringstream trans;
		displayTransitions( testCounts, trans, value );
		displayPasses( testCounts, stream, value );
	}

	void display( TestCounts & testCounts
		, crg::RunnableGraph & value )
	{
		display( testCounts, std::cout, value );
	}
}
