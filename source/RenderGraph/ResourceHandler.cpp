/*
See LICENSE file in root folder.
*/
#include "RenderGraph/ResourceHandler.hpp"

#include "RenderGraph/Attachment.hpp"
#include "RenderGraph/GraphContext.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/ImageViewData.hpp"
#include "RenderGraph/Log.hpp"
#include "RenderGraph/RunnableGraph.hpp"

#include <cassert>

#pragma warning( push )
#pragma warning( disable: 5262 )
#include <sstream>
#pragma warning( pop )

namespace crg
{
	//*********************************************************************************************

	namespace reshdl
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

		static VkImageCreateInfo convert( ImageData const & data )
		{
			return data.info;
		}

		static VkImageViewCreateInfo convert( ImageViewData const & data
			, VkImage const & image )
		{
			auto result = data.info;
			result.image = image;
			return result;
		}

		static size_t makeHash( SamplerDesc const & samplerDesc )
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

		static size_t makeHash( bool texCoords
			, Texcoord const & config )
		{
			size_t result{ ( ( texCoords ? 0x01u : 0x00u ) << 0u )
				| ( ( config.invertU ? 0x01u : 0x00u ) << 1u )
				| ( ( config.invertV ? 0x01u : 0x00u ) << 2u ) };
			return result;
		}
	}

	//*********************************************************************************************

	ResourceHandler::~ResourceHandler()
	{
		for ( auto & imageView : m_imageViews )
		{
			std::stringstream stream;
			stream << "Leaked [VkImageView](" << imageView.first.data->name << ")";
			Logger::logError( stream.str() );
		}

		for ( auto & image : m_images )
		{
			std::stringstream stream;
			stream << "Leaked [VkImage](" << image.first.data->name << ")";
			Logger::logError( stream.str() );
		}

		for ( auto & vertexBuffer : m_vertexBuffers )
		{
			if ( vertexBuffer->memory )
			{
				std::stringstream stream;
				stream << "Leaked [VkDeviceMemory](" << vertexBuffer->buffer.name << ")";
				Logger::logError( stream.str() );
			}

			if ( vertexBuffer->buffer.buffer )
			{
				std::stringstream stream;
				stream << "Leaked [VkBuffer](" << vertexBuffer->buffer.name << ")";
				Logger::logError( stream.str() );
			}
		}

		for ( auto & sampler : m_samplers )
		{
			std::stringstream stream;
			stream << "Leaked [VkSampler](" << sampler.second.name << ")";
			Logger::logError( stream.str() );
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

	ResourceHandler::CreatedT< VkImage > ResourceHandler::createImage( GraphContext & context
		, ImageId imageId )
	{
		if ( !context.device )
		{
			return {};
		}

		bool created{};
		std::unique_lock< std::mutex > lock( m_imagesMutex );
		auto ires = m_images.emplace( imageId, std::pair< VkImage, VkDeviceMemory >{} );

		if ( ires.second )
		{
		// Create image
			auto createInfo = reshdl::convert( *imageId.data );
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
			created = true;
		}

		return { ires.first->second.first, created };
	}

	ResourceHandler::CreatedT< VkImageView > ResourceHandler::createImageView( GraphContext & context
		, ImageViewId view )
	{
		if ( !context.device )
		{
			return {};
		}

		bool created{};
		std::unique_lock< std::mutex > lock( m_viewsMutex );
		auto ires = m_imageViews.emplace( view, VkImageView{} );

		if ( ires.second )
		{
			auto image = createImage( context, view.data->image ).first;
			auto createInfo = reshdl::convert( *view.data, image );
			auto res = context.vkCreateImageView( context.device
				, &createInfo
				, context.allocator
				, &ires.first->second );
			checkVkResult( res, "ImageView creation" );
			crgRegisterObjectName( context, view.data->name, ires.first->second );
			created = true;
		}

		return { ires.first->second, created };
	}

	VkSampler ResourceHandler::createSampler( GraphContext & context
		, std::string const & suffix
		, SamplerDesc const & samplerDesc )
	{
		if ( !context.device )
		{
			return VkSampler{};
		}

		std::unique_lock< std::mutex > lock( m_samplersMutex );
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
		VkSampler result;
		auto res = context.vkCreateSampler( context.device
			, &createInfo
			, context.allocator
			, &result );
		auto & sampler = m_samplers.emplace( result, Sampler{ result, {} } ).first->second;
		checkVkResult( res, "Sampler creation" );
		sampler.name = "Sampler_" + suffix;
		crgRegisterObject( context, sampler.name, result );
		return result;
	}

	VertexBuffer const * ResourceHandler::createQuadTriVertexBuffer( GraphContext & context
		, std::string const & suffix
		, bool texCoords
		, Texcoord const & config )
	{
		std::unique_lock< std::mutex > lock( m_buffersMutex );

		if ( !context.device )
		{
			return nullptr;
		}

		auto result = std::make_unique< VertexBuffer >();
		auto vertexBuffer = result.get();
		VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO
			, nullptr
			, 0u
			, 3u * sizeof( reshdl::Quad::Vertex )
			, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
			, VK_SHARING_MODE_EXCLUSIVE
			, 0u
			, nullptr };
		auto res = context.vkCreateBuffer( context.device
			, &createInfo
			, context.allocator
			, &vertexBuffer->buffer.buffer );
		checkVkResult( res, "Vertex buffer creation" );
		crgRegisterObject( context, "QuadVertexBuffer_" + suffix, vertexBuffer->buffer.buffer );

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
		crgRegisterObject( context, "QuadVertexMemory_" + suffix, vertexBuffer->memory );

		res = context.vkBindBufferMemory( context.device
			, vertexBuffer->buffer.buffer
			, vertexBuffer->memory
			, 0u );
		checkVkResult( res, "Buffer memory binding" );

		reshdl::Quad::Vertex * buffer{};
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
			std::array<reshdl::Quad::Vertex, 3u > vertexData
				{ reshdl::Quad::Vertex{ reshdl::Quad::Data{ -1.0f, -3.0f }, reshdl::Quad::Data{ realMinU, realMinV } }
				, reshdl::Quad::Vertex{ reshdl::Quad::Data{ -1.0f, +1.0f }, reshdl::Quad::Data{ realMinU, realMaxV } }
				, reshdl::Quad::Vertex{ reshdl::Quad::Data{ +3.0f, +1.0f }, reshdl::Quad::Data{ realMaxU, realMaxV } } };
			std::copy( vertexData.begin(), vertexData.end(), buffer );

			VkMappedMemoryRange memoryRange{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE
				, nullptr
				, vertexBuffer->memory
				, 0u
				, VK_WHOLE_SIZE };
			context.vkFlushMappedMemoryRanges( context.device, 1u, &memoryRange );
			context.vkUnmapMemory( context.device, vertexBuffer->memory );
		}

		vertexBuffer->vertexAttribs.push_back( { 0u, 0u, VK_FORMAT_R32G32_SFLOAT, offsetof( reshdl::Quad::Vertex, position ) } );

		if ( texCoords )
		{
			vertexBuffer->vertexAttribs.push_back( { 1u, 0u, VK_FORMAT_R32G32_SFLOAT, offsetof( reshdl::Quad::Vertex, texture ) } );
		}

		vertexBuffer->vertexBindings.push_back( { 0u, sizeof( reshdl::Quad::Vertex ), VK_VERTEX_INPUT_RATE_VERTEX } );
		vertexBuffer->inputState = VkPipelineVertexInputStateCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
			, nullptr
			, 0u
			, uint32_t( vertexBuffer->vertexBindings.size() )
			, vertexBuffer->vertexBindings.data()
			, uint32_t( vertexBuffer->vertexAttribs.size() )
			, vertexBuffer->vertexAttribs.data() };

		m_vertexBuffers.emplace( std::move( result ) );
		return vertexBuffer;
	}

	void ResourceHandler::destroyImage( GraphContext & context
		, ImageId imageId )
	{
		std::unique_lock< std::mutex > lock( m_imagesMutex );
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

	void ResourceHandler::destroyImageView( GraphContext & context
		, ImageViewId viewId )
	{
		std::unique_lock< std::mutex > lock( m_viewsMutex );
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

	void ResourceHandler::destroySampler( GraphContext & context
		, VkSampler sampler )
	{
		std::unique_lock< std::mutex > lock( m_samplersMutex );
		auto it = m_samplers.find( sampler );

		if ( it != m_samplers.end() )
		{
			if ( context.device )
			{
				crgUnregisterObject( context, it->first );
				context.vkDestroySampler( context.device
					, it->first
					, context.allocator );
			}

			m_samplers.erase( it );
		}
	}

	void ResourceHandler::destroyVertexBuffer( GraphContext & context
		, VertexBuffer const * buffer )
	{
		std::unique_lock< std::mutex > lock( m_buffersMutex );
		auto it = std::find_if( m_vertexBuffers.begin()
			, m_vertexBuffers.end()
			, [buffer]( VertexBufferPtr const & lookup )
			{
				return lookup.get() == buffer;
			} );

		if ( it != m_vertexBuffers.end() )
		{
			if ( context.device )
			{
				auto & vertexBuffer = **it;

				if ( vertexBuffer.memory )
				{
					crgUnregisterObject( context, vertexBuffer.memory );
					context.vkFreeMemory( context.device
						, vertexBuffer.memory
						, context.allocator );
				}

				if ( vertexBuffer.buffer.buffer )
				{
					crgUnregisterObject( context, vertexBuffer.buffer.buffer );
					context.vkDestroyBuffer( context.device
						, vertexBuffer.buffer.buffer
						, context.allocator );
				}
			}

			m_vertexBuffers.erase( it );
		}
	}

	VkImage ResourceHandler::getImage( ImageId const & image )const
	{
		std::unique_lock< std::mutex > lock( m_imagesMutex );
		auto it = m_images.find( image );

		if ( it == m_images.end() )
		{
			return VkImage{};
		}

		return it->second.first;
	}

	VkImageView ResourceHandler::getImageView( ImageViewId const & view )const
	{
		std::unique_lock< std::mutex > lock( m_viewsMutex );
		auto it = m_imageViews.find( view );

		if ( it == m_imageViews.end() )
		{
			return VkImageView{};
		}

		return it->second;
	}

	//*********************************************************************************************

	ContextResourcesCache::ContextResourcesCache( ResourceHandler & handler
		, GraphContext & context )
		: m_handler{ handler }
		, m_context{ context }
	{
	}

	ContextResourcesCache::~ContextResourcesCache()noexcept
	{
		for ( auto & imageViewIt : m_imageViews )
		{
			m_handler.destroyImageView( m_context, imageViewIt.first );
		}

		for ( auto & imageIt : m_images )
		{
			m_handler.destroyImage( m_context, imageIt.first );
		}

		for ( auto & samplerIt : m_samplers )
		{
			m_handler.destroySampler( m_context, samplerIt.second );
		}

		for ( auto & bufferIt : m_vertexBuffers )
		{
			m_handler.destroyVertexBuffer( m_context, bufferIt.second );
		}
	}

	VkImage ContextResourcesCache::createImage( ImageId const & image )
	{
		auto [result, created] = m_handler.createImage( m_context, image );

		if ( created )
		{
			m_images[image] = result;
		}

		return result;
	}

	VkImageView ContextResourcesCache::createImageView( ImageViewId const & view )
	{
		auto [result, created] = m_handler.createImageView( m_context, view );

		if ( created )
		{
			m_imageViews[view] = result;
		}

		return result;
	}

	bool ContextResourcesCache::destroyImage( ImageId const & imageId )
	{
		auto it = m_images.find( imageId );
		auto result = it != m_images.end();

		if ( result )
		{
			m_handler.destroyImage( m_context, imageId );
		}

		return result;
	}

	bool ContextResourcesCache::destroyImageView( ImageViewId const & viewId )
	{
		auto it = m_imageViews.find( viewId );
		auto result = it != m_imageViews.end();

		if ( result )
		{
			m_handler.destroyImageView( m_context, viewId );
		}

		return result;
	}

	VkSampler ContextResourcesCache::createSampler( SamplerDesc const & samplerDesc )
	{
		auto hash = reshdl::makeHash( samplerDesc );
		auto ires = m_samplers.emplace( hash, VkSampler{} );

		if ( ires.second && m_context.device )
		{
			ires.first->second = m_handler.createSampler( m_context
				, std::to_string( hash )
				, samplerDesc );
		}

		return ires.first->second;
	}

	VertexBuffer const & ContextResourcesCache::createQuadTriVertexBuffer( bool texCoords
		, Texcoord const & config )
	{
		auto hash = reshdl::makeHash( texCoords, config );
		auto ires = m_vertexBuffers.emplace( hash, nullptr );

		if ( ires.second && m_context.device )
		{
			ires.first->second = m_handler.createQuadTriVertexBuffer( m_context
				, std::to_string( hash )
				, texCoords
				, config );
		}

		return *ires.first->second;
	}

	//*********************************************************************************************

	ResourcesCache::ResourcesCache( ResourceHandler & handler )
		: m_handler{ handler }
	{
	}

	VkImage ResourcesCache::createImage( GraphContext & context
		, ImageId const & imageId )
	{
		auto & cache = getContextCache( context );
		return cache.createImage( imageId );
	}

	VkImageView ResourcesCache::createImageView( GraphContext & context
		, ImageViewId const & viewId )
	{
		auto & cache = getContextCache( context );
		return cache.createImageView( viewId );
	}

	bool ResourcesCache::destroyImage( ImageId const & imageId )
	{
		auto it = std::find_if( m_caches.begin()
			, m_caches.end()
			, [&imageId]( ContextCacheMap::value_type & lookup )
			{
				return lookup.second.destroyImage( imageId );
			} );
		return it != m_caches.end();
	}

	bool ResourcesCache::destroyImageView( ImageViewId const & viewId )
	{
		auto it = std::find_if( m_caches.begin()
			, m_caches.end()
			, [&viewId]( ContextCacheMap::value_type & lookup )
			{
				return lookup.second.destroyImageView( viewId );
			} );
		return it != m_caches.end();
	}

	bool ResourcesCache::destroyImage( GraphContext & context
		, ImageId const & imageId )
	{
		auto & cache = getContextCache( context );
		return cache.destroyImage( imageId );
	}

	bool ResourcesCache::destroyImageView( GraphContext & context
		, ImageViewId const & viewId )
	{
		auto & cache = getContextCache( context );
		return cache.destroyImageView( viewId );
	}

	VkSampler ResourcesCache::createSampler( GraphContext & context
		, SamplerDesc const & samplerDesc )
	{
		auto & cache = getContextCache( context );
		return cache.createSampler( samplerDesc );
	}

	VertexBuffer const & ResourcesCache::createQuadTriVertexBuffer( GraphContext & context
		, bool texCoords
		, Texcoord const & config )
	{
		auto & cache = getContextCache( context );
		return cache.createQuadTriVertexBuffer( texCoords, config );
	}

	ContextResourcesCache & ResourcesCache::getContextCache( GraphContext & context )
	{
		auto it = m_caches.find( &context );

		if ( it == m_caches.end() )
		{
			it = m_caches.emplace( &context, ContextResourcesCache{ m_handler, context } ).first;
		}

		return it->second;
	}

	//*********************************************************************************************
}
