/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FrameGraphPrerequisites.hpp"

#pragma warning( push )
#pragma warning( disable: 4365 )
#pragma warning( disable: 5262 )
#include <mutex>
#pragma warning( pop )
#include <unordered_map>
#include <unordered_set>

namespace crg
{
	struct Sampler
	{
		VkSampler sampler;
		std::string name;
	};

	class ResourceHandler
	{
		template< typename ValueT >
		using CreatedT = std::pair< ValueT, bool >;

	public:
		CRG_API ResourceHandler() = default;
		CRG_API ResourceHandler( ResourceHandler const & ) = delete;
		CRG_API ResourceHandler( ResourceHandler && )noexcept = delete;
		CRG_API ResourceHandler & operator=( ResourceHandler const & ) = delete;
		CRG_API ResourceHandler & operator=( ResourceHandler && )noexcept = delete;
		CRG_API ~ResourceHandler()noexcept;

		CRG_API ImageId createImageId( ImageData const & img );
		CRG_API ImageViewId createViewId( ImageViewData const & view );
		CRG_API ImageId findImageId( uint32_t id )const;

		CRG_API CreatedT< VkImage > createImage( GraphContext & context
			, ImageId imageId );
		CRG_API CreatedT< VkImageView > createImageView( GraphContext & context
			, ImageViewId viewId );
		CRG_API VkSampler createSampler( GraphContext & context
			, std::string const & suffix
			, SamplerDesc const & samplerDesc );
		CRG_API VertexBuffer const * createQuadTriVertexBuffer( GraphContext & context
			, std::string const & suffix
			, bool texCoords
			, Texcoord const & config );
		CRG_API void destroyImage( GraphContext & context
			, ImageId imageId );
		CRG_API void destroyImageView( GraphContext & context
			, ImageViewId viewId );
		CRG_API void destroySampler( GraphContext & context
			, VkSampler sampler );
		CRG_API void destroyVertexBuffer( GraphContext & context
			, VertexBuffer const * buffer );

		CRG_API VkImage getImage( ImageId const & image )const;
		CRG_API VkImageView getImageView( ImageViewId const & imageView )const;

	private:
		mutable std::mutex m_imagesMutex;
		ImageIdDataOwnerCont m_imageIds;
		mutable std::mutex m_viewsMutex;
		ImageViewIdDataOwnerCont m_imageViewIds;
		ImageMemoryMap m_images;
		ImageViewMap m_imageViews;
		std::mutex m_samplersMutex;
		std::unordered_map< VkSampler, Sampler > m_samplers;
		std::mutex m_buffersMutex;
		std::unordered_set< VertexBufferPtr > m_vertexBuffers;
	};

	class ContextResourcesCache
	{
	public:
		CRG_API ContextResourcesCache( ContextResourcesCache const & ) = delete;
		CRG_API ContextResourcesCache & operator=( ContextResourcesCache const & ) = delete;
		CRG_API ContextResourcesCache( ContextResourcesCache && )noexcept = default;
		CRG_API ContextResourcesCache & operator=( ContextResourcesCache && )noexcept = delete;

		CRG_API ContextResourcesCache( ResourceHandler & handler
			, GraphContext & context );
		CRG_API ~ContextResourcesCache()noexcept;

		CRG_API VkImage createImage( ImageId const & imageId );
		CRG_API VkImageView createImageView( ImageViewId const & viewId );
		CRG_API bool destroyImage( ImageId const & imageId );
		CRG_API bool destroyImageView( ImageViewId const & viewId );

		CRG_API VkSampler createSampler( SamplerDesc const & samplerDesc );
		CRG_API VertexBuffer const & createQuadTriVertexBuffer( bool texCoords
			, Texcoord const & config );

		GraphContext * operator->()const noexcept
		{
			return &m_context;
		}

		operator GraphContext & ()const noexcept
		{
			return m_context;
		}

		ResourceHandler & getHandler()const
		{
			return m_handler;
		}

	private:
		using VkImageIdMap = std::map< ImageId, VkImage >;
		using VkImageViewIdMap = std::map< ImageViewId, VkImageView >;

		ResourceHandler & m_handler;
		GraphContext & m_context;
		VkImageIdMap m_images;
		VkImageViewIdMap m_imageViews;
		std::unordered_map< size_t, VkSampler > m_samplers;
		std::unordered_map< size_t, VertexBuffer const * > m_vertexBuffers;
	};

	class ResourcesCache
	{
	public:
		CRG_API explicit ResourcesCache( ResourceHandler & handler );

		CRG_API VkImage createImage( GraphContext & context
			, ImageId const & imageId );
		CRG_API VkImageView createImageView( GraphContext & context
			, ImageViewId const & viewId );
		CRG_API bool destroyImage( ImageId const & imageId );
		CRG_API bool destroyImageView( ImageViewId const & viewId );
		CRG_API bool destroyImage( GraphContext & context
			, ImageId const & imageId );
		CRG_API bool destroyImageView( GraphContext & context
			, ImageViewId const & viewId );

		CRG_API VkSampler createSampler( GraphContext & context
			, SamplerDesc const & samplerDesc );
		CRG_API VertexBuffer const & createQuadTriVertexBuffer( GraphContext & context
			, bool texCoords
			, Texcoord const & config );

		CRG_API ContextResourcesCache & getContextCache( GraphContext & context );

		ResourceHandler & getHandler()const
		{
			return m_handler;
		}

	private:
		using ContextCacheMap = std::unordered_map< GraphContext *, ContextResourcesCache >;

		ResourceHandler & m_handler;
		ContextCacheMap m_caches;
	};
}
