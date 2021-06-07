/*
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/FrameGraphPrerequisites.hpp"

namespace crg
{
	class ResourceHandler
	{
	public:
		CRG_API ResourceHandler() = default;
		CRG_API ResourceHandler( ResourceHandler const & ) = delete;
		CRG_API ResourceHandler( ResourceHandler && ) = default;
		CRG_API ResourceHandler & operator=( ResourceHandler const & ) = delete;
		CRG_API ResourceHandler & operator=( ResourceHandler && ) = default;

		CRG_API void clear( GraphContext const & context );

		CRG_API ImageId createImageId( ImageData const & img );
		CRG_API ImageViewId createViewId( ImageViewData const & view );

		CRG_API VkImage createImage( GraphContext const & context
			, ImageId imageId );
		CRG_API VkImageView createImageView( GraphContext const & context
			, ImageViewId viewId );
		CRG_API void destroyImage( GraphContext const & context
			, ImageId imageId );
		CRG_API void destroyImageView( GraphContext const & context
			, ImageViewId viewId );

		CRG_API VkImage getImage( ImageId const & image )const;
		CRG_API VkImageView getImageView( ImageViewId const & imageView )const;

	private:
		ImageIdDataOwnerCont m_imageIds;
		ImageViewIdDataOwnerCont m_imageViewIds;
		ImageMemoryMap m_images;
		ImageViewMap m_imageViews;
	};
}
