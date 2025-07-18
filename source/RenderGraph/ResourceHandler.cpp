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
	using lock_type = std::unique_lock< std::mutex >;

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
		};

		static VkImageCreateInfo convert( ImageData const & data )
		{
			return convert( data.info );
		}

		static VkImageViewCreateInfo convert( ImageViewData const & data
			, VkImage const & image )
		{
			auto result = convert( data.info );
			result.image = image;
			return result;
		}

		template< typename T >
		static size_t hashCombine( size_t hash
			, T const & rhs )
		{
			const uint64_t kMul = 0x9ddfea08eb382d69ULL;
			auto seed = hash;

			std::hash< T > hasher;
			uint64_t a = ( hasher( rhs ) ^ seed ) * kMul;
			a ^= ( a >> 47 );

			uint64_t b = ( seed ^ a ) * kMul;
			b ^= ( b >> 47 );

#pragma warning( push )
#pragma warning( disable: 4068 )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
			hash = static_cast< std::size_t >( b * kMul );
#pragma GCC diagnostic pop
#pragma clang diagnostic pop
#pragma warning( pop )
			return hash;
		}

		static size_t makeHash( SamplerDesc const & samplerDesc )
		{
			auto result = std::hash< FilterMode >{}( samplerDesc.magFilter );
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

	ResourceHandler::~ResourceHandler()noexcept
	{
		std::array< char, 1024u > buffer;

		for ( auto const & [data, _] : m_imageViews )
		{
			snprintf( buffer.data(), buffer.size(), "Leaked [VkImageView](%.900s)", data.data->name.c_str() );
			Logger::logError( buffer.data() );
		}

		for ( auto const & [data, _] : m_images )
		{
			snprintf( buffer.data(), buffer.size(), "Leaked [VkImage](%.900s)", data.data->name.c_str() );
			Logger::logError( buffer.data() );
		}

		for ( auto const & vertexBuffer : m_vertexBuffers )
		{
			if ( vertexBuffer->memory )
			{
				snprintf( buffer.data(), buffer.size(), "Leaked [VkDeviceMemory](%.900s)", vertexBuffer->buffer.name.c_str() );
				Logger::logError( buffer.data() );
			}

			if ( vertexBuffer->buffer.buffer() )
			{
				snprintf( buffer.data(), buffer.size(), "Leaked [VkBuffer](%.900s)", vertexBuffer->buffer.name.c_str() );
				Logger::logError( buffer.data() );
			}
		}

		for ( auto const & [_, data] : m_samplers )
		{
			snprintf( buffer.data(), buffer.size(), "Leaked [VkSampler](%.900s)", data.name.c_str() );
			Logger::logError( buffer.data() );
		}
	}

	ImageId ResourceHandler::createImageId( ImageData const & img )
	{
		lock_type lock( m_imagesMutex );
		auto data = std::make_unique< ImageData >( img );
		ImageId result{ uint32_t( m_imageIds.size() + 1u ), data.get() };
		m_imageIds.try_emplace( result, std::move( data ) );
		return result;
	}

	ImageViewId ResourceHandler::createViewId( ImageViewData const & view )
	{
		lock_type lock( m_viewsMutex );
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
			result = ImageViewId{ uint32_t( m_imageViewIds.size() + 1u ), data.get() };
			m_imageViewIds.try_emplace( result, std::move( data ) );
		}
		else
		{
			result = it->first;
		}

		return result;
	}

	ResourceHandler::CreatedT< VkImage > ResourceHandler::createImage( GraphContext & context
		, ImageId imageId )
	{
		ResourceHandler::CreatedT< VkImage > result{};

		if ( context.vkCreateImage )
		{
			bool created{};
			lock_type lock( m_imagesMutex );
			auto [it, ins] = m_images.try_emplace( imageId, std::pair< VkImage, VkDeviceMemory >{} );

			if ( ins )
			{
				// Create image
				auto createInfo = reshdl::convert( *imageId.data );
				auto res = context.vkCreateImage( context.device
					, &createInfo
					, context.allocator
					, &it->second.first );
				auto image = it->second.first;
				checkVkResult( res, "Image creation" );
				crgRegisterObjectName( context, imageId.data->name, image );

				if ( context.device )
				{
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
						, &it->second.second );
					auto memory = it->second.second;
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
			}

			result = { it->second.first, created };
		}

		return result;
	}

	ResourceHandler::CreatedT< VkImageView > ResourceHandler::createImageView( GraphContext & context
		, ImageViewId view )
	{
		ResourceHandler::CreatedT< VkImageView > result{};

		if ( context.vkCreateImageView )
		{
			bool created{};
			lock_type lock( m_viewsMutex );
			auto [it, ins] = m_imageViews.try_emplace( view, VkImageView{} );

			if ( ins )
			{
				auto image = createImage( context, view.data->image ).first;
				auto createInfo = reshdl::convert( *view.data, image );
				auto res = context.vkCreateImageView( context.device
					, &createInfo
					, context.allocator
					, &it->second );
				checkVkResult( res, "ImageView creation" );
				crgRegisterObjectName( context, view.data->name, it->second );
				created = true;
			}

			result = { it->second, created };
		}

		return result;
	}

	VkSampler ResourceHandler::createSampler( GraphContext & context
		, std::string const & suffix
		, SamplerDesc const & samplerDesc )
	{
		VkSampler result{};

		if ( context.vkCreateSampler )
		{
			lock_type lock( m_samplersMutex );
			VkSamplerCreateInfo createInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO
				, nullptr
				, 0u
				, convert( samplerDesc.magFilter )
				, convert( samplerDesc.minFilter )
				, convert( samplerDesc.mipmapMode )
				, convert( samplerDesc.addressModeU )
				, convert( samplerDesc.addressModeV )
				, convert( samplerDesc.addressModeW )
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
				, &result );
			auto & sampler = m_samplers.try_emplace( result, Sampler{ result, {} } ).first->second;
			checkVkResult( res, "Sampler creation" );
			sampler.name = "Sampler_" + suffix;
			crgRegisterObject( context, sampler.name, result );
		}

		return result;
	}

	VertexBuffer const * ResourceHandler::createQuadTriVertexBuffer( GraphContext & context
		, std::string const & suffix
		, bool texCoords
		, Texcoord const & config )
	{
		VertexBuffer * vertexBuffer{};
		lock_type lock( m_buffersMutex );

		if ( context.vkCreateBuffer )
		{
			auto result = std::make_unique< VertexBuffer >();
			vertexBuffer = result.get();
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
				, &vertexBuffer->buffer.buffer() );
			checkVkResult( res, "Vertex buffer creation" );
			crgRegisterObject( context, "QuadVertexBuffer_" + suffix, vertexBuffer->buffer.buffer() );

			if ( context.device )
			{
				VkMemoryRequirements requirements{};
				context.vkGetBufferMemoryRequirements( context.device
					, vertexBuffer->buffer.buffer()
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
					, vertexBuffer->buffer.buffer()
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
			}

			m_vertexBuffers.emplace( std::move( result ) );
		}

		return vertexBuffer;
	}

	void ResourceHandler::destroyImage( GraphContext & context
		, ImageId imageId )
	{
		lock_type lock( m_imagesMutex );
		auto it = m_images.find( imageId );

		if ( it != m_images.end() )
		{
			if ( context.vkFreeMemory && it->second.second )
			{
				context.vkFreeMemory( context.device, it->second.second, context.allocator );
			}

			if ( context.vkDestroyImage && it->second.first )
			{
				context.vkDestroyImage( context.device, it->second.first, context.allocator );
			}

			m_images.erase( it );
		}
	}

	void ResourceHandler::destroyImageView( GraphContext & context
		, ImageViewId viewId )
	{
		lock_type lock( m_viewsMutex );
		auto it = m_imageViews.find( viewId );

		if ( it != m_imageViews.end() )
		{
			if ( context.vkDestroyImageView && it->second )
			{
				context.vkDestroyImageView( context.device, it->second, context.allocator );
			}

			m_imageViews.erase( it );
		}
	}

	void ResourceHandler::destroySampler( GraphContext & context
		, VkSampler sampler )
	{
		lock_type lock( m_samplersMutex );
		auto it = m_samplers.find( sampler );

		if ( it != m_samplers.end() )
		{
			if ( context.vkDestroySampler && it->first )
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
		lock_type lock( m_buffersMutex );
		auto it = std::find_if( m_vertexBuffers.begin()
			, m_vertexBuffers.end()
			, [buffer]( VertexBufferPtr const & lookup )
			{
				return lookup.get() == buffer;
			} );

		if ( it != m_vertexBuffers.end() )
		{
			auto & vertexBuffer = **it;

			if ( context.vkFreeMemory && vertexBuffer.memory )
			{
				crgUnregisterObject( context, vertexBuffer.memory );
				context.vkFreeMemory( context.device
					, vertexBuffer.memory
					, context.allocator );
			}

			if ( context.vkDestroyBuffer && vertexBuffer.buffer.buffer() )
			{
				crgUnregisterObject( context, vertexBuffer.buffer.buffer() );
				context.vkDestroyBuffer( context.device
					, vertexBuffer.buffer.buffer()
					, context.allocator );
			}

			m_vertexBuffers.erase( it );
		}
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
		for ( auto const & [imageView, _] : m_imageViews )
		{
			m_handler.destroyImageView( m_context, imageView );
		}

		for ( auto const & [image, _] : m_images )
		{
			m_handler.destroyImage( m_context, image );
		}

		for ( auto const & [_, sampler] : m_samplers )
		{
			m_handler.destroySampler( m_context, sampler );
		}

		for ( auto const & [_, buffer] : m_vertexBuffers )
		{
			m_handler.destroyVertexBuffer( m_context, buffer );
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
		auto [it, res] = m_samplers.try_emplace( hash, VkSampler{} );

		if ( res )
		{
			it->second = m_handler.createSampler( m_context
				, std::to_string( hash )
				, samplerDesc );
		}

		return it->second;
	}

	VertexBuffer const & ContextResourcesCache::createQuadTriVertexBuffer( bool texCoords
		, Texcoord const & config )
	{
		auto hash = reshdl::makeHash( texCoords, config );
		auto [it, res] = m_vertexBuffers.emplace( hash, nullptr );

		if ( res )
		{
			it->second = m_handler.createQuadTriVertexBuffer( m_context
				, std::to_string( hash )
				, texCoords
				, config );
		}

		return *it->second;
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
			it = m_caches.try_emplace( &context, m_handler, context ).first;
		}

		return it->second;
	}

	//*********************************************************************************************
}
