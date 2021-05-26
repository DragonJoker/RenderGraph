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
	struct SamplerDesc
	{
		VkFilter magFilter;
		VkFilter minFilter;
		VkSamplerMipmapMode mipmapMode;
		VkSamplerAddressMode addressModeU;
		VkSamplerAddressMode addressModeV;
		VkSamplerAddressMode addressModeW;
		float mipLodBias;
		float minLod;
		float maxLod;

		explicit constexpr SamplerDesc( VkFilter magFilter = VK_FILTER_NEAREST
			, VkFilter minFilter = VK_FILTER_NEAREST
			, VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST
			, VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
			, VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
			, VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
			, float mipLodBias = 0.0f
			, float minLod = -500.0f
			, float maxLod = 500.0f )
			: magFilter{ magFilter }
			, minFilter{ minFilter }
			, mipmapMode{ mipmapMode }
			, addressModeU{ addressModeU }
			, addressModeV{ addressModeV }
			, addressModeW{ addressModeW }
			, mipLodBias{ mipLodBias }
			, minLod{ minLod }
			, maxLod{ maxLod }
		{
		}
	};
	CRG_API bool operator==( SamplerDesc const & lhs
		, SamplerDesc const & rhs );
	/**
	*\brief
	*	A sampled image, or an input/output colour/depth/stencil/depthstencil attachment.
	*/
	struct Attachment
	{
		friend struct FramePass;
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
		CRG_API VkImageLayout getImageLayout( bool separateDepthStencilLayouts )const;
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
		SamplerDesc samplerDesc{};
		uint32_t binding{};
		VkClearValue clearValue{};
		VkPipelineColorBlendAttachmentState blendState = DefaultBlendState;
		/**@}*/

	private:
		CRG_API Attachment( ImageViewId view );
		CRG_API Attachment( FlagKind flags
			, FramePass & pass
			, std::string name
			, ImageViewId view
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkAttachmentLoadOp stencilLoadOp
			, VkAttachmentStoreOp stencilStoreOp
			, VkImageLayout initialLayout
			, uint32_t binding
			, SamplerDesc samplerDesc
			, VkClearValue clearValue
			, VkPipelineColorBlendAttachmentState blendState );

		CRG_API void setFlag( Flag flag, bool set )
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

		friend CRG_API bool operator==( Attachment const & lhs, Attachment const & rhs );
	};

	CRG_API bool operator==( Attachment const & lhs, Attachment const & rhs );
	CRG_API bool operator!=( Attachment const & lhs, Attachment const & rhs );
}
