/*
See LICENSE file in root folder.
*/
#include "RenderGraph/ResourceHandler.hpp"

#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/ImageViewData.hpp"

#include <cassert>

namespace crg
{
	namespace
	{
		VkImageCreateInfo convert( ImageData const & data )
		{
			return data.info;
		}

		VkImageViewCreateInfo convert( ImageViewData const & data
			, VkImage const & image )
		{
			auto result = data.info;
			result.image = image;
			return result;
		}
	}

	void ResourceHandler::clear( GraphContext const & context )
	{
		for ( auto & imageView : m_imageViewIds )
		{
			destroyImageView( context, imageView.first );
		}

		m_imageViews.clear();
		m_imageViewIds.clear();

		for ( auto & image : m_imageIds )
		{
			destroyImage( context, image.first );
		}

		m_images.clear();
		m_imageIds.clear();
	}

	ImageId ResourceHandler::createImageId( ImageData const & img )
	{
		auto data = std::make_unique< ImageData >( img );
		ImageId result{ uint32_t( m_imageIds.size() + 1u ), data.get() };
		m_imageIds.insert( { result, std::move( data ) } );
		return result;
	}

	ImageViewId ResourceHandler::createViewId( ImageViewData const & view )
	{
		auto it = std::find_if( m_imageViewIds.begin()
			, m_imageViewIds.end()
			, [&view]( ImageViewIdDataOwnerCont::value_type const & lookup )
			{
				return *lookup.second == view;
			} );
		ImageViewId result{};

		if ( it == m_imageViewIds.end() )
		{
			auto data = std::make_unique< ImageViewData >( view );
			result = { uint32_t( m_imageViewIds.size() + 1u ), data.get() };
			m_imageViewIds.insert( { result, std::move( data ) } );
		}
		else
		{
			result = it->first;
		}

		return result;
	}

	VkImage ResourceHandler::createImage( GraphContext const & context
		, ImageId imageId )
	{
		if ( !context.device )
		{
			return VK_NULL_HANDLE;
		}

		auto ires = m_images.emplace( imageId, std::pair< VkImage, VkDeviceMemory >{} );

		if ( ires.second )
		{
		// Create image
			auto createInfo = convert( *imageId.data );
			auto res = context.vkCreateImage( context.device
				, &createInfo
				, context.allocator
				, &ires.first->second.first );
			auto image = ires.first->second.first;
			checkVkResult( res, "Image creation" );
			crgRegisterObject( context, imageId.data->name, image );

			// Create Image memory
			VkMemoryRequirements requirements{};
			context.vkGetImageMemoryRequirements( context.device
				, image
				, &requirements );
			uint32_t deduced = context.deduceMemoryType( requirements.memoryTypeBits
				, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
			VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
				, nullptr
				, requirements.size
				, deduced };
			res = context.vkAllocateMemory( context.device
				, &allocateInfo
				, context.allocator
				, &ires.first->second.second );
			auto memory = ires.first->second.second;
			checkVkResult( res, "Image memory allocation" );
			crgRegisterObject( context, imageId.data->name, memory );

			// Bind image and memory
			res = context.vkBindImageMemory( context.device
				, image
				, memory
				, 0u );
			checkVkResult( res, "Image memory binding" );
		}

		return ires.first->second.first;
	}

	VkImageView ResourceHandler::createImageView( GraphContext const & context
		, ImageViewId view )
	{
		if ( !context.device )
		{
			return VK_NULL_HANDLE;
		}

		auto ires = m_imageViews.emplace( view, VkImageView{} );

		if ( ires.second )
		{
			auto image = createImage( context, view.data->image );
			auto createInfo = convert( *view.data, image );
			auto res = context.vkCreateImageView( context.device
				, &createInfo
				, context.allocator
				, &ires.first->second );
			checkVkResult( res, "ImageView creation" );
			crgRegisterObject( context, view.data->name, ires.first->second );
		}

		return ires.first->second;
	}

	void ResourceHandler::destroyImage( GraphContext const & context
		, ImageId imageId )
	{
		auto it = m_images.find( imageId );

		if ( it != m_images.end() )
		{
			if ( context.device )
			{
				crgUnregisterObject( context, it->second.second );
				context.vkFreeMemory( context.device, it->second.second, context.allocator );
				crgUnregisterObject( context, it->second.first );
				context.vkDestroyImage( context.device, it->second.first, context.allocator );
			}

			m_images.erase( it );
		}
	}

	void ResourceHandler::destroyImageView( GraphContext const & context
		, ImageViewId viewId )
	{
		auto it = m_imageViews.find( viewId );

		if ( it != m_imageViews.end() )
		{
			if ( context.device )
			{
				crgUnregisterObject( context, it->second );
				context.vkDestroyImageView( context.device, it->second, context.allocator );
			}

			m_imageViews.erase( it );
		}
	}

	VkImage ResourceHandler::getImage( ImageId const & image )const
	{
		auto it = m_images.find( image );

		if ( it == m_images.end() )
		{
			return VK_NULL_HANDLE;
		}

		return it->second.first;
	}

	VkImageView ResourceHandler::getImageView( ImageViewId const & view )const
	{
		auto it = m_imageViews.find( view );

		if ( it == m_imageViews.end() )
		{
			return VK_NULL_HANDLE;
		}

		return it->second;
	}
}