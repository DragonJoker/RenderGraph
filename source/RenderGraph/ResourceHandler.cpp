/*
See LICENSE file in root folder.
*/
#include "RenderGraph/ResourceHandler.hpp"

#include "RenderGraph/Attachment.hpp"
#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/ImageViewData.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <cassert>
#include <iostream>
#include <sstream>

namespace crg
{
	namespace
	{
		struct Quad
		{
			using Data = std::array< float, 2u >;
			struct Vertex
			{
				Data position;
				Data texture;
			};

			Vertex vertex[6];
		};

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

		size_t makeHash( SamplerDesc const & samplerDesc )
		{
			auto result = std::hash< uint32_t >{}( samplerDesc.magFilter );
			result = hashCombine( result, samplerDesc.minFilter );
			result = hashCombine( result, samplerDesc.mipmapMode );
			result = hashCombine( result, samplerDesc.addressModeU );
			result = hashCombine( result, samplerDesc.addressModeV );
			result = hashCombine( result, samplerDesc.addressModeW );
			result = hashCombine( result, samplerDesc.mipLodBias );
			result = hashCombine( result, samplerDesc.minLod );
			result = hashCombine( result, samplerDesc.maxLod );
			return result;
		}

		size_t makeHash( bool texCoords
			, Texcoord const & config )
		{
			size_t result{ ( ( texCoords ? 0x01u : 0x00u ) << 0u )
				| ( ( config.invertU ? 0x01u : 0x00u ) << 1u )
				| ( ( config.invertV ? 0x01u : 0x00u ) << 2u ) };
			return result;
		}
	}

	ResourceHandler::~ResourceHandler()
	{
		for ( auto & imageView : m_imageViews )
		{
			std::stringstream stream;
			stream << "Leaked [VkImageView](" << imageView.first.data->name << ")";
			std::cerr << stream.str() << "\n";
		}

		for ( auto & image : m_images )
		{
			std::stringstream stream;
			stream << "Leaked [VkImage](" << image.first.data->name << ")";
			std::cerr << stream.str() << "\n";
		}
	}

	void ResourceHandler::clear( GraphContext & context )
	{
		if ( context.device )
		{
			{
				std::unique_lock< std::mutex > lock( m_buffersMutex );

				for ( auto & vertexBuffer : m_vertexBuffers )
				{
					if ( vertexBuffer.second->memory )
					{
						context.vkFreeMemory( context.device
							, vertexBuffer.second->memory
							, context.allocator );
					}

					if ( vertexBuffer.second->buffer.buffer )
					{
						context.vkDestroyBuffer( context.device
							, vertexBuffer.second->buffer.buffer
							, context.allocator );
					}
				}

				m_vertexBuffers.clear();
			}
			{
				std::unique_lock< std::mutex > lock( m_samplersMutex );

				for ( auto & sampler : m_samplers )
				{
					context.vkDestroySampler( context.device
						, sampler.second
						, context.allocator );
				}

				m_samplers.clear();
			}
		}

		{
			std::unique_lock< std::mutex > lock( m_viewsMutex );

			while ( !m_imageViewIds.empty() )
			{
				auto it = m_imageViewIds.begin();
				doDestroyImageView( context, it->first );
				m_imageViewIds.erase( it );
			}

			m_imageViews.clear();
		}

		{
			std::unique_lock< std::mutex > lock( m_imagesMutex );

			while ( !m_imageIds.empty() )
			{
				auto it = m_imageIds.begin();
				doDestroyImage( context, it->first );
				m_imageIds.erase( it );
			}

			m_images.clear();
		}
	}

	ImageId ResourceHandler::createImageId( ImageData const & img )
	{
		std::unique_lock< std::mutex > lock( m_imagesMutex );
		auto data = std::make_unique< ImageData >( img );
		ImageId result{ uint32_t( m_imageIds.size() + 1u ), data.get() };
		m_imageIds.insert( { result, std::move( data ) } );
		return result;
	}

	ImageViewId ResourceHandler::createViewId( ImageViewData const & view )
	{
		std::unique_lock< std::mutex > lock( m_viewsMutex );
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

	ImageId ResourceHandler::findImageId( uint32_t id )const
	{
		std::unique_lock< std::mutex > lock( m_imagesMutex );
		auto it = std::find_if( m_imageIds.begin()
			, m_imageIds.end()
			, [&id]( ImageIdDataOwnerCont::value_type const & lookup )
			{
				return lookup.first.id == id;
			} );
		return it != m_imageIds.end()
			? it->first
			: ImageId{};
	}

	VkImage ResourceHandler::createImage( GraphContext & context
		, ImageId imageId )
	{
		if ( !context.device )
		{
			return nullptr;
		}

		std::unique_lock< std::mutex > lock( m_imagesMutex );
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
			crgRegisterObjectName( context, imageId.data->name, image );

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
			crgRegisterObjectName( context, imageId.data->name, memory );

			// Bind image and memory
			res = context.vkBindImageMemory( context.device
				, image
				, memory
				, 0u );
			checkVkResult( res, "Image memory binding" );
		}

		return ires.first->second.first;
	}

	VkImageView ResourceHandler::createImageView( GraphContext & context
		, ImageViewId view )
	{
		if ( !context.device )
		{
			return nullptr;
		}

		std::unique_lock< std::mutex > lock( m_viewsMutex );
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
			crgRegisterObjectName( context, view.data->name, ires.first->second );
		}

		return ires.first->second;
	}

	VkSampler ResourceHandler::createSampler( GraphContext & context
		, SamplerDesc const & samplerDesc )
	{
		if ( !context.device )
		{
			return nullptr;
		}

		std::unique_lock< std::mutex > lock( m_samplersMutex );
		auto ires = m_samplers.emplace( makeHash( samplerDesc ), VkSampler{} );

		if ( ires.second )
		{
			if ( !context.device )
			{
				return ires.first->second;
			}

			VkSamplerCreateInfo createInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO
				, nullptr
				, 0u
				, samplerDesc.magFilter
				, samplerDesc.minFilter
				, samplerDesc.mipmapMode
				, samplerDesc.addressModeU
				, samplerDesc.addressModeV
				, samplerDesc.addressModeW
				, samplerDesc.mipLodBias // mipLodBias
				, VK_FALSE // anisotropyEnable
				, 0.0f // maxAnisotropy
				, VK_FALSE // compareEnable
				, VK_COMPARE_OP_ALWAYS // compareOp
				, samplerDesc.minLod
				, samplerDesc.maxLod
				, VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK
				, VK_FALSE };
			auto res = context.vkCreateSampler( context.device
				, &createInfo
				, context.allocator
				, &ires.first->second );
			checkVkResult( res, "Sampler creation" );
			crgRegisterObject( context, "Sampler" + std::to_string( makeHash( samplerDesc ) ), ires.first->second );
		}

		return ires.first->second;
	}

	VertexBuffer const & ResourceHandler::createQuadTriVertexBuffer( GraphContext & context
		, bool texCoords
		, Texcoord const & config )
	{
		auto hash = makeHash( texCoords, config );

		std::unique_lock< std::mutex > lock( m_buffersMutex );
		auto ires = m_vertexBuffers.emplace( hash, std::make_unique< VertexBuffer >() );

		if ( ires.second && context.device )
		{
			auto & vertexBuffer = ires.first->second;
			VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO
				, nullptr
				, 0u
				, 3u * sizeof( Quad::Vertex )
				, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
				, VK_SHARING_MODE_EXCLUSIVE
				, 0u
				, nullptr };
			auto res = context.vkCreateBuffer( context.device
				, &createInfo
				, context.allocator
				, &vertexBuffer->buffer.buffer );
			checkVkResult( res, "Vertex buffer creation" );
			crgRegisterObject( context, "QuadVertexBuffer", vertexBuffer->buffer.buffer );

			VkMemoryRequirements requirements{};
			context.vkGetBufferMemoryRequirements( context.device
				, vertexBuffer->buffer.buffer
				, &requirements );
			uint32_t deduced = context.deduceMemoryType( requirements.memoryTypeBits
				, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
			VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
				, nullptr
				, requirements.size
				, deduced };
			res = context.vkAllocateMemory( context.device
				, &allocateInfo
				, context.allocator
				, &vertexBuffer->memory );
			checkVkResult( res, "Buffer memory allocation" );
			crgRegisterObject( context, "QuadVertexMemory", vertexBuffer->memory );

			res = context.vkBindBufferMemory( context.device
				, vertexBuffer->buffer.buffer
				, vertexBuffer->memory
				, 0u );
			checkVkResult( res, "Buffer memory binding" );

			Quad::Vertex * buffer{};
			res = context.vkMapMemory( context.device
				, vertexBuffer->memory
				, 0u
				, VK_WHOLE_SIZE
				, 0u
				, reinterpret_cast< void ** >( &buffer ) );
			checkVkResult( res, "Buffer memory mapping" );

			if ( buffer )
			{
				auto rangeU = 1.0;
				auto minU = 0.0;
				auto maxU = minU + 2.0 * rangeU;
				auto rangeV = 1.0;
				auto minV = -rangeV;
				auto maxV = minV + 2.0 * rangeV;
				auto realMinU = float( config.invertU ? maxU : minU );
				auto realMaxU = float( config.invertU ? minU : maxU );
				auto realMinV = float( config.invertV ? maxV : minV );
				auto realMaxV = float( config.invertV ? minV : maxV );
				std::array< Quad::Vertex, 3u > vertexData
					{ Quad::Vertex{ Quad::Data{ -1.0f, -3.0f }, Quad::Data{ realMinU, realMinV } }
					, Quad::Vertex{ Quad::Data{ -1.0f, +1.0f }, Quad::Data{ realMinU, realMaxV } }
					, Quad::Vertex{ Quad::Data{ +3.0f, +1.0f }, Quad::Data{ realMaxU, realMaxV } } };
				std::copy( vertexData.begin(), vertexData.end(), buffer );

				VkMappedMemoryRange memoryRange{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE
					, nullptr
					, vertexBuffer->memory
					, 0u
					, VK_WHOLE_SIZE };
				context.vkFlushMappedMemoryRanges( context.device, 1u, &memoryRange );
				context.vkUnmapMemory( context.device, vertexBuffer->memory );
			}

			vertexBuffer->vertexAttribs.push_back( { 0u, 0u, VK_FORMAT_R32G32_SFLOAT, offsetof( Quad::Vertex, position ) } );

			if ( texCoords )
			{
				vertexBuffer->vertexAttribs.push_back( { 1u, 0u, VK_FORMAT_R32G32_SFLOAT, offsetof( Quad::Vertex, texture ) } );
			}

			vertexBuffer->vertexBindings.push_back( { 0u, sizeof( Quad::Vertex ), VK_VERTEX_INPUT_RATE_VERTEX } );
			vertexBuffer->inputState = VkPipelineVertexInputStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
				, nullptr
				, 0u
				, uint32_t( vertexBuffer->vertexBindings.size() )
				, vertexBuffer->vertexBindings.data()
				, uint32_t( vertexBuffer->vertexAttribs.size() )
				, vertexBuffer->vertexAttribs.data() };
		}

		return *ires.first->second;
	}

	void ResourceHandler::destroyImage( GraphContext & context
		, ImageId imageId )
	{
		std::unique_lock< std::mutex > lock( m_imagesMutex );
		doDestroyImage( context, imageId );
	}

	void ResourceHandler::destroyImageView( GraphContext & context
		, ImageViewId viewId )
	{
		std::unique_lock< std::mutex > lock( m_viewsMutex );
		doDestroyImageView( context, viewId );
	}

	VkImage ResourceHandler::getImage( ImageId const & image )const
	{
		std::unique_lock< std::mutex > lock( m_imagesMutex );
		auto it = m_images.find( image );

		if ( it == m_images.end() )
		{
			return nullptr;
		}

		return it->second.first;
	}

	VkImageView ResourceHandler::getImageView( ImageViewId const & view )const
	{
		std::unique_lock< std::mutex > lock( m_viewsMutex );
		auto it = m_imageViews.find( view );

		if ( it == m_imageViews.end() )
		{
			return nullptr;
		}

		return it->second;
	}

	void ResourceHandler::doDestroyImage( GraphContext & context
		, ImageId imageId )
	{
		auto it = m_images.find( imageId );

		if ( it != m_images.end() )
		{
			if ( context.device )
			{
				context.vkFreeMemory( context.device, it->second.second, context.allocator );
				context.vkDestroyImage( context.device, it->second.first, context.allocator );
			}

			m_images.erase( it );
		}
	}

	void ResourceHandler::doDestroyImageView( GraphContext & context
		, ImageViewId viewId )
	{
		auto it = m_imageViews.find( viewId );

		if ( it != m_imageViews.end() )
		{
			if ( context.device )
			{
				context.vkDestroyImageView( context.device, it->second, context.allocator );
			}

			m_imageViews.erase( it );
		}
	}
}
