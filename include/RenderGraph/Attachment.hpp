/*
This file belongs to FrameGraph.
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
		friend class FramePass;
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
			Storage = 0x01 << 2,
			Depth = 0x01 << 3,
			Clearing = 0x01 << 4,
			Input = 0x01 << 5,
			Output = 0x01 << 6,
			StencilClearing = 0x01 << 7,
			StencilInput = 0x01 << 8,
			StencilOutput = 0x01 << 9,
			Transfer = 0x01 << 10,
		};
		/**
		*\name
		*	Mutators.
		*/
		/**@{*/
		void setUnique()
		{
			setFlag( Flag::Unique, true );
		}
		/**@}*/
		/**
		*\name
		*	Getters.
		*/
		/**@{*/
		VkImageLayout getImageLayout( bool separateDepthStencilLayouts )const;
		FlagKind getFlags()const
		{
			return flags;
		}
		
		bool hasFlag( Flag flag )const
		{
			return Flag( flags & FlagKind( flag ) ) == flag;
		}

		bool isUnique()const
		{
			return hasFlag( Flag::Unique );
		}

		bool isSampled()const
		{
			return hasFlag( Flag::Sampled );
		}

		bool isStorage()const
		{
			return hasFlag( Flag::Storage );
		}

		bool isAttachment()const
		{
			return !isSampled()
				&& !isStorage();
		}

		bool isColourClearing()const
		{
			return hasFlag( Flag::Clearing )
				&& !hasFlag( Flag::Depth );
		}

		bool isColourInput()const
		{
			return hasFlag( Flag::Input )
				&& !hasFlag( Flag::Depth );
		}

		bool isColourOutput()const
		{
			return hasFlag( Flag::Output )
				&& !hasFlag( Flag::Depth );
		}

		bool isColourInOut()const
		{
			return isColourInput()
				&& isColourOutput();
		}

		bool isDepthClearing()const
		{
			return hasFlag( Flag::Clearing )
				&& hasFlag( Flag::Depth );
		}

		bool isDepthInput()const
		{
			return hasFlag( Flag::Input )
				&& hasFlag( Flag::Depth );
		}

		bool isDepthOutput()const
		{
			return hasFlag( Flag::Output )
				&& hasFlag( Flag::Depth );
		}

		bool isDepthInOut()const
		{
			return isDepthInput()
				&& isDepthOutput();
		}

		bool isStencilClearing()const
		{
			return hasFlag( Flag::StencilClearing );
		}

		bool isStencilInput()const
		{
			return hasFlag( Flag::StencilInput );
		}

		bool isStencilOutput()const
		{
			return hasFlag( Flag::StencilOutput );
		}

		bool isStencilInOut()const
		{
			return isStencilInput()
				&& isStencilOutput();
		}

		bool isDepthStencilInput()const
		{
			return isDepthInput()
				&& isStencilInput();
		}

		bool isDepthStencilOutput()const
		{
			return isDepthOutput()
				&& isStencilOutput();
		}

		bool isDepthStencilInOut()const
		{
			return isDepthInOut()
				&& isStencilInOut();
		}

		bool isTransferInput()const
		{
			return hasFlag( Flag::Transfer )
				&& hasFlag( Flag::Input );
		}

		bool isTransferOutput()const
		{
			return hasFlag( Flag::Transfer )
				&& hasFlag( Flag::Output );
		}
		/**@}*/
		/**
		*\brief
		*	Creates a default empty attachment.
		*/
		static Attachment createDefault( ImageViewId view )
		{
			return Attachment{ view };
		}
		/**
		*\name
		*	Members.
		*/
		/**@[*/
		FramePass * pass{};
		std::string name{};
		ImageViewId view{};
		VkAttachmentLoadOp loadOp{};
		VkAttachmentStoreOp storeOp{};
		VkAttachmentLoadOp stencilLoadOp{};
		VkAttachmentStoreOp stencilStoreOp{};
		VkImageLayout initialLayout{};
		VkFilter filter{};
		uint32_t binding{};
		VkClearValue clearValue{};
		VkPipelineColorBlendAttachmentState blendState = DefaultBlendState;
		/**@}*/

	private:
		Attachment( ImageViewId view );
		Attachment( FlagKind flags
			, FramePass & pass
			, std::string name
			, ImageViewId view
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkAttachmentLoadOp stencilLoadOp
			, VkAttachmentStoreOp stencilStoreOp
			, VkImageLayout initialLayout
			, uint32_t binding
			, VkFilter filter
			, VkClearValue clearValue
			, VkPipelineColorBlendAttachmentState blendState );

		void setFlag( Flag flag, bool set )
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
