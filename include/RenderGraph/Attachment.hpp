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
			Input = 0x01 << 0,
			Output = 0x01 << 1,
			Sampled = 0x01 << 2,
			Storage = 0x01 << 3,
			Transfer = 0x01 << 4,
			Depth = 0x01 << 5,
			Stencil = 0x01 << 6,
			Clearing = 0x01 << 7,
			StencilClearing = 0x01 << 8,
			StencilInput = 0x01 << 9,
			StencilOutput = 0x01 << 10,
		};
		/**
		*\name
		*	Getters.
		*/
		/**@{*/
		CRG_API ImageViewId view( uint32_t index = 0u )const;
		CRG_API VkImageLayout getImageLayout( bool separateDepthStencilLayouts )const;
		FlagKind getFlags()const
		{
			return flags;
		}
		
		bool hasFlag( Flag flag )const
		{
			return Flag( flags & FlagKind( flag ) ) == flag;
		}

		bool isInput()const
		{
			return hasFlag( Flag::Input );
		}

		bool isOutput()const
		{
			return hasFlag( Flag::Output );
		}

		bool isSampled()const
		{
			return hasFlag( Flag::Sampled );
		}

		bool isStorage()const
		{
			return hasFlag( Flag::Storage );
		}

		bool isTransfer()const
		{
			return hasFlag( Flag::Transfer );
		}

		bool isDepth()const
		{
			return hasFlag( Flag::Depth );
		}

		bool isStencil()const
		{
			return hasFlag( Flag::Stencil );
		}

		bool isAttachment()const
		{
			return !isSampled()
				&& !isStorage();
		}

		bool isClearing()const
		{
			return hasFlag( Flag::Clearing );
		}

		bool isColour()const
		{
			return !isSampled()
				&& !isStorage()
				&& !isTransfer()
				&& !isDepth()
				&& !isStencil();
		}

		bool isColourClearing()const
		{
			return isClearing() && isColour();
		}

		bool isColourInput()const
		{
			return isInput() && isColour();
		}

		bool isColourOutput()const
		{
			return isOutput() && isColour();
		}

		bool isColourInOut()const
		{
			return isInput() && isOutput() && isColour();
		}

		bool isDepthClearing()const
		{
			return isClearing() && isDepth();
		}

		bool isDepthInput()const
		{
			return isInput() && isDepth();
		}

		bool isDepthOutput()const
		{
			return isOutput() && isDepth();
		}

		bool isDepthInOut()const
		{
			return isInput() && isOutput() && isDepth();
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
			return isInput() && isTransfer();
		}

		bool isTransferOutput()const
		{
			return isOutput() && isTransfer();
		}

		bool isStorageInput()const
		{
			return isInput() && isStorage();
		}

		bool isStorageOutput()const
		{
			return isOutput() && isStorage();
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
		ImageViewIdArray views;
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

		CRG_API Attachment( ImageViewId view
			, Attachment const & origin );

	private:
		CRG_API Attachment( ImageViewId view );
		CRG_API Attachment( FlagKind flags
			, FramePass & pass
			, std::string name
			, ImageViewIdArray views
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

	CRG_API bool isDepthFormat( VkFormat fmt );
	CRG_API bool isStencilFormat( VkFormat fmt );
	CRG_API bool isColourFormat( VkFormat fmt );
	CRG_API bool isDepthStencilFormat( VkFormat fmt );
}
