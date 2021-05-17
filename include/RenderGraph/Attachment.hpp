/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "ImageViewData.hpp"

#ifdef None
#undef None
#endif

namespace crg
{
	/**
	*\brief
	*	A sampled image, or an input/output colour/depth/stencil/depthstencil attachment.
	*/
	struct Attachment
	{
		/**
		*\brief
		*	The flags qualifying an Attachment.
		*/
		using FlagKind = uint16_t;
		enum class Flag : FlagKind
		{
			None = 0x00,
			Unique = 0x01 << 0,
			Sampled = 0x01 << 1,
			Depth = 0x01 << 2,
			Clearing = 0x01 << 3,
			Input = 0x01 << 4,
			Output = 0x01 << 5,
			StencilClearing = 0x01 << 6,
			StencilInput = 0x01 << 7,
			StencilOutput = 0x01 << 8,
		};
		/**
		*\name
		*	Mutators.
		*/
		/**@{*/
		inline void setUnique()
		{
			setFlag( Flag::Unique, true );
		}
		/**@}*/
		/**
		*\name
		*	Getters.
		*/
		/**@{*/
		inline bool hasFlag( Flag flag )const
		{
			return Flag( flags & FlagKind( flag ) ) == flag;
		}

		inline bool isUnique()const
		{
			return hasFlag( Flag::Unique );
		}

		inline bool isSampled()const
		{
			return hasFlag( Flag::Sampled );
		}

		inline bool isAttachment()const
		{
			return !isSampled();
		}

		inline bool isColourClearing()const
		{
			return hasFlag( Flag::Clearing )
				&& !hasFlag( Flag::Depth );
		}

		inline bool isColourInput()const
		{
			return hasFlag( Flag::Input )
				&& !hasFlag( Flag::Depth );
		}

		inline bool isColourOutput()const
		{
			return hasFlag( Flag::Output )
				&& !hasFlag( Flag::Depth );
		}

		inline bool isColourInOut()const
		{
			return isColourInput()
				&& isColourOutput();
		}

		inline bool isDepthClearing()const
		{
			return hasFlag( Flag::Clearing )
				&& hasFlag( Flag::Depth );
		}

		inline bool isDepthInput()const
		{
			return hasFlag( Flag::Input )
				&& hasFlag( Flag::Depth );
		}

		inline bool isDepthOutput()const
		{
			return hasFlag( Flag::Output )
				&& hasFlag( Flag::Depth );
		}

		inline bool isDepthInOut()const
		{
			return isDepthInput()
				&& isDepthOutput();
		}

		inline bool isStencilClearing()const
		{
			return hasFlag( Flag::StencilClearing );
		}

		inline bool isStencilInput()const
		{
			return hasFlag( Flag::StencilInput );
		}

		inline bool isStencilOutput()const
		{
			return hasFlag( Flag::StencilOutput );
		}

		inline bool isStencilInOut()const
		{
			return isStencilInput()
				&& isStencilOutput();
		}
		/**@}*/
		/**
		*\name
		*	Named constructor.
		*/
		/**@[*/
		/**
		*\brief
		*	Creates a sampled image attachment.
		*/
		static Attachment createSampled( ImageViewData viewData
			, VkImageLayout initialLayout );
		/**
		*\brief
		*	Creates a colour attachment.
		*/
		static Attachment createColour( ImageViewData viewData
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED );
		/**
		*\brief
		*	Creates a depth and/or stencil output attachment.
		*/
		static Attachment createDepthStencil( ImageViewData viewData
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkAttachmentLoadOp stencilLoadOp
			, VkAttachmentStoreOp stencilStoreOp
			, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_UNDEFINED );
		/**
		*\brief
		*	Creates an input colour attachment.
		*/
		static inline Attachment createInputColour( ImageViewData viewData
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			return createColour( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, initialLayout
				, finalLayout );
		}
		/**
		*\brief
		*	Creates an in/out colour attachment.
		*/
		static inline Attachment createInOutColour( ImageViewData viewData
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			return createColour( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, initialLayout
				, finalLayout );
		}
		/**
		*\brief
		*	Creates an output colour attachment.
		*/
		static inline Attachment createOutputColour( ImageViewData viewData
			, VkImageLayout finalLayout )
		{
			return createColour( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_CLEAR
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_IMAGE_LAYOUT_UNDEFINED
				, finalLayout );
		}
		/**
		*\brief
		*	Creates an input depth attachment.
		*/
		static inline Attachment createInputDepth( ImageViewData viewData
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			return createDepthStencil( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, initialLayout
				, finalLayout );
		}
		/**
		*\brief
		*	Creates an in/out depth attachment.
		*/
		static inline Attachment createInOutDepth( ImageViewData viewData
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			return createDepthStencil( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, initialLayout
				, finalLayout );
		}
		/**
		*\brief
		*	Creates an output depth attachment.
		*/
		static inline Attachment createOutputDepth( ImageViewData viewData
			, VkImageLayout finalLayout )
		{
			return createDepthStencil( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_CLEAR
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_IMAGE_LAYOUT_UNDEFINED
				, finalLayout );
		}
		/**
		*\brief
		*	Creates an input depth and stencil attachment.
		*/
		static inline Attachment createInputDepthStencil( ImageViewData viewData
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			return createDepthStencil( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, initialLayout
				, finalLayout );
		}
		/**
		*\brief
		*	Creates an in/out depth and stencil attachment.
		*/
		static inline Attachment createInOutDepthStencil( ImageViewData viewData
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			return createDepthStencil( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, initialLayout
				, finalLayout );
		}
		/**
		*\brief
		*	Creates an output depth and stencil attachment.
		*/
		static inline Attachment createOutputDepthStencil( ImageViewData viewData
			, VkImageLayout finalLayout )
		{
			return createDepthStencil( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_CLEAR
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_ATTACHMENT_LOAD_OP_CLEAR
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_IMAGE_LAYOUT_UNDEFINED
				, finalLayout );
		}
		/**
		*\brief
		*	Creates an input stencil attachment.
		*/
		static inline Attachment createInputStencil( ImageViewData viewData
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			return createDepthStencil( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, initialLayout
				, finalLayout );
		}
		/**
		*\brief
		*	Creates an in/out stencil attachment.
		*/
		static inline Attachment createInOutStencil( ImageViewData viewData
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout )
		{
			return createDepthStencil( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_LOAD
				, VK_ATTACHMENT_STORE_OP_STORE
				, initialLayout
				, finalLayout );
		}
		/**
		*\brief
		*	Creates an output stencil attachment.
		*/
		static inline Attachment createOutputStencil( ImageViewData viewData
			, VkImageLayout finalLayout )
		{
			return createDepthStencil( std::move( viewData )
				, VK_ATTACHMENT_LOAD_OP_DONT_CARE
				, VK_ATTACHMENT_STORE_OP_DONT_CARE
				, VK_ATTACHMENT_LOAD_OP_CLEAR
				, VK_ATTACHMENT_STORE_OP_STORE
				, VK_IMAGE_LAYOUT_UNDEFINED
				, finalLayout );
		}
		/**
		*\brief
		*	Creates a default empty attachment.
		*/
		static inline Attachment createDefault()
		{
			return Attachment{};
		}
		/**@}*/
		/**
		*\name
		*	Members.
		*/
		/**@[*/
		ImageViewData viewData{};
		VkAttachmentLoadOp loadOp{};
		VkAttachmentStoreOp storeOp{};
		VkAttachmentLoadOp stencilLoadOp{};
		VkAttachmentStoreOp stencilStoreOp{};
		VkImageLayout initialLayout{};
		VkImageLayout finalLayout{};
		/**@}*/

	private:
		Attachment();
		Attachment( FlagKind flags
			, ImageViewData viewData
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkAttachmentLoadOp stencilLoadOp
			, VkAttachmentStoreOp stencilStoreOp
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout );

		inline void setFlag( Flag flag, bool set )
		{
			if ( set )
			{
				flags |= FlagKind( flag );
			}
			else
			{
				flags &= ~FlagKind( flag );
			}
		}

		FlagKind flags{};

		friend bool operator==( Attachment const & lhs, Attachment const & rhs );
	};

	bool operator==( Attachment const & lhs, Attachment const & rhs );
	bool operator!=( Attachment const & lhs, Attachment const & rhs );
}
