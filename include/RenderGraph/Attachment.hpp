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
	*	An image attachment.
	*/
	struct ImageAttachment
	{
		friend struct Attachment;
		friend struct FramePass;
		/**
		*\brief
		*	The flags qualifying the attachment.
		*/
		using FlagKind = uint16_t;
		enum class Flag : FlagKind
		{
			None = 0x00,
			Sampled = 0x01 << 0,
			Storage = 0x01 << 1,
			Transfer = 0x01 << 2,
			Depth = 0x01 << 3,
			Stencil = 0x01 << 4,
			Clearing = 0x01 << 5,
			StencilClearing = 0x01 << 6,
			StencilInput = 0x01 << 7,
			StencilOutput = 0x01 << 8,
		};
		CRG_API ImageAttachment( ImageViewId view
			, ImageAttachment const & origin );
		/**
		*\name
		*	Getters.
		*/
		/**@{*/
		CRG_API ImageViewId view( uint32_t index = 0u )const;
		CRG_API VkDescriptorType getDescriptorType()const;

		FlagKind getFlags()const
		{
			return flags;
		}

		bool hasFlag( Flag flag )const
		{
			return Flag( flags & FlagKind( flag ) ) == flag;
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
		/**@}*/

	public:
		std::string name{};
		ImageViewIdArray views;
		VkAttachmentLoadOp loadOp{};
		VkAttachmentStoreOp storeOp{};
		VkAttachmentLoadOp stencilLoadOp{};
		VkAttachmentStoreOp stencilStoreOp{};
		VkImageLayout initialLayout{};
		SamplerDesc samplerDesc{};
		VkClearValue clearValue{};
		VkPipelineColorBlendAttachmentState blendState = DefaultBlendState;

	private:
		CRG_API ImageAttachment();
		CRG_API ImageAttachment( ImageViewId view );
		CRG_API ImageAttachment( FlagKind flags
			, std::string name
			, ImageViewIdArray views
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkAttachmentLoadOp stencilLoadOp
			, VkAttachmentStoreOp stencilStoreOp
			, VkImageLayout initialLayout
			, SamplerDesc samplerDesc
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

		friend CRG_API bool operator==( ImageAttachment const & lhs, ImageAttachment const & rhs );
	};
	/**
	*\brief
	*	A buffer (uniform or storage) attachment.
	*/
	struct BufferAttachment
	{
		friend struct Attachment;
		friend struct FramePass;
		/**
		*\brief
		*	The flags qualifying a buffer attachment.
		*/
		using FlagKind = uint16_t;
		enum class Flag : FlagKind
		{
			None = 0x00,
			Uniform = 0x01 << 0,
			Storage = 0x01 << 1,
			View = 0x01 << 2,
		};

		CRG_API VkDescriptorType getDescriptorType()const;
		CRG_API WriteDescriptorSet getWrite( uint32_t binding )const;

		FlagKind getFlags()const
		{
			return flags;
		}

		bool hasFlag( Flag flag )const
		{
			return Flag( flags & FlagKind( flag ) ) == flag;
		}

		bool isUniform()const
		{
			return hasFlag( Flag::Uniform );
		}

		bool isStorage()const
		{
			return hasFlag( Flag::Storage );
		}

		bool isView()const
		{
			return hasFlag( Flag::View );
		}

		bool isUniformView()const
		{
			return isUniform() && isView();
		}

		bool isStorageView()const
		{
			return isStorage() && isView();
		}

		VkBuffer buffer;
		VkBufferView view;
		VkDeviceSize offset;
		VkDeviceSize range;

	private:
		CRG_API BufferAttachment();
		CRG_API BufferAttachment( FlagKind flags
			, VkBuffer buffer
			, VkDeviceSize offset
			, VkDeviceSize range );
		CRG_API BufferAttachment( FlagKind flags
			, VkBuffer buffer
			, VkBufferView view
			, VkDeviceSize offset
			, VkDeviceSize range );

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

		friend CRG_API bool operator==( BufferAttachment const & lhs, BufferAttachment const & rhs );
	};
	/**
	*\brief
	*	An attachment to a pass.
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
			Image = 0x01 << 2,
			Buffer = 0x01 << 3,
		};
		/**
		*\name
		*	Getters.
		*/
		/**@{*/
		CRG_API ImageViewId view( uint32_t index = 0u )const;
		CRG_API VkImageLayout getImageLayout( bool separateDepthStencilLayouts )const;
		CRG_API VkDescriptorType getDescriptorType()const;
		CRG_API WriteDescriptorSet getBufferWrite()const;

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

		bool isImage()const
		{
			return hasFlag( Flag::Image );
		}

		bool isBuffer()const
		{
			return hasFlag( Flag::Buffer );
		}

		bool isUniformBuffer()const
		{
			return isBuffer() && buffer.isUniform();
		}

		bool isUniformBufferView()const
		{
			return isBuffer() && buffer.isUniformView();
		}

		bool isStorageBuffer()const
		{
			return isBuffer() && buffer.isStorage();
		}

		bool isStorageBufferView()const
		{
			return isBuffer() && buffer.isStorageView();
		}

		bool isBufferView()const
		{
			return isBuffer() && buffer.isView();
		}

		bool isSampled()const
		{
			return isImage() && image.isSampled();
		}

		bool isStorage()const
		{
			return isImage() && image.isStorage();
		}

		bool isTransfer()const
		{
			return isImage() && image.isTransfer();
		}

		bool isDepth()const
		{
			return isImage() && image.isDepth();
		}

		bool isStencil()const
		{
			return isImage() && image.isStencil();
		}

		bool isClearing()const
		{
			return isImage() && image.isClearing();
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
			return isImage() && image.isStencilClearing();
		}

		bool isStencilInput()const
		{
			return isImage() && image.isStencilInput();
		}

		bool isStencilOutput()const
		{
			return isImage() && image.isStencilOutput();
		}

		bool isStencilInOut()const
		{
			return isStencilInput() && isStencilOutput();
		}

		bool isDepthStencilInput()const
		{
			return isDepthInput() && isStencilInput();
		}

		bool isDepthStencilOutput()const
		{
			return isDepthOutput() && isStencilOutput();
		}

		bool isDepthStencilInOut()const
		{
			return isDepthInOut() && isStencilInOut();
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
		uint32_t binding{};
		ImageAttachment image;
		BufferAttachment buffer;
		/**@}*/

		CRG_API Attachment( ImageViewId view
			, Attachment const & origin );

	private:
		CRG_API Attachment( ImageViewId view );
		CRG_API Attachment( FlagKind flags
			, FramePass & pass
			, uint32_t binding
			, ImageAttachment::FlagKind imageFlags
			, std::string name
			, ImageViewIdArray views
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkAttachmentLoadOp stencilLoadOp
			, VkAttachmentStoreOp stencilStoreOp
			, VkImageLayout initialLayout
			, SamplerDesc samplerDesc
			, VkClearValue clearValue
			, VkPipelineColorBlendAttachmentState blendState );
		CRG_API Attachment( FlagKind flags
			, FramePass & pass
			, uint32_t binding
			, BufferAttachment::FlagKind bufferFlags
			, VkBuffer buffer
			, VkDeviceSize offset
			, VkDeviceSize range );
		CRG_API Attachment( FlagKind flags
			, FramePass & pass
			, uint32_t binding
			, BufferAttachment::FlagKind bufferFlags
			, VkBuffer buffer
			, VkBufferView view
			, VkDeviceSize offset
			, VkDeviceSize range );

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

	CRG_API bool operator==( BufferAttachment const & lhs, BufferAttachment const & rhs );
	CRG_API bool operator!=( BufferAttachment const & lhs, BufferAttachment const & rhs );
	CRG_API bool operator==( ImageAttachment const & lhs, ImageAttachment const & rhs );
	CRG_API bool operator!=( ImageAttachment const & lhs, ImageAttachment const & rhs );
	CRG_API bool operator==( Attachment const & lhs, Attachment const & rhs );
	CRG_API bool operator!=( Attachment const & lhs, Attachment const & rhs );

	CRG_API bool isDepthFormat( VkFormat fmt );
	CRG_API bool isStencilFormat( VkFormat fmt );
	CRG_API bool isColourFormat( VkFormat fmt );
	CRG_API bool isDepthStencilFormat( VkFormat fmt );
}
