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
		CRG_API VkImageLayout getImageLayout( bool separateDepthStencilLayouts
			, bool isInput
			, bool isOutput )const;
		CRG_API VkAccessFlags getAccessMask( bool isInput
			, bool isOutput )const;
		CRG_API VkPipelineStageFlags getPipelineStageFlags( bool isCompute )const;

		FlagKind getFlags()const
		{
			return flags;
		}

		bool hasFlag( Flag flag )const
		{
			return Flag( flags & FlagKind( flag ) ) == flag;
		}

		bool isSampledView()const
		{
			return hasFlag( Flag::Sampled );
		}

		bool isStorageView()const
		{
			return hasFlag( Flag::Storage );
		}

		bool isTransferView()const
		{
			return hasFlag( Flag::Transfer );
		}

		bool isDepthAttach()const
		{
			return hasFlag( Flag::Depth );
		}

		bool isStencilAttach()const
		{
			return hasFlag( Flag::Stencil );
		}

		bool isDepthStencilAttach()const
		{
			return isDepthAttach() && isStencilAttach();
		}

		bool isClearingAttach()const
		{
			return hasFlag( Flag::Clearing );
		}

		bool isColourAttach()const
		{
			return !isSampledView()
				&& !isStorageView()
				&& !isTransferView()
				&& !isDepthAttach()
				&& !isStencilAttach();
		}

		bool isStencilClearingAttach()const
		{
			return hasFlag( Flag::StencilClearing );
		}

		bool isStencilInputAttach()const
		{
			return hasFlag( Flag::StencilInput );
		}

		bool isStencilOutputAttach()const
		{
			return hasFlag( Flag::StencilOutput );
		}
		/**@}*/

	public:
		ImageViewIdArray views;
		VkAttachmentLoadOp loadOp{};
		VkAttachmentStoreOp storeOp{};
		VkAttachmentLoadOp stencilLoadOp{};
		VkAttachmentStoreOp stencilStoreOp{};
		VkImageLayout initialLayout{};
		VkImageLayout finalLayout{};
		SamplerDesc samplerDesc{};
		VkClearValue clearValue{};
		VkPipelineColorBlendAttachmentState blendState = DefaultBlendState;

	private:
		CRG_API ImageAttachment();
		CRG_API ImageAttachment( ImageViewId view );
		CRG_API ImageAttachment( FlagKind flags
			, ImageViewIdArray views
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkAttachmentLoadOp stencilLoadOp
			, VkAttachmentStoreOp stencilStoreOp
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout
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
		CRG_API WriteDescriptorSet getWrite( uint32_t binding
			, uint32_t count )const;
		CRG_API VkAccessFlags getAccessMask( bool isInput
			, bool isOutput )const;
		CRG_API VkPipelineStageFlags getPipelineStageFlags( bool isCompute )const;

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

		Buffer buffer;
		VkBufferView view;
		VkDeviceSize offset;
		VkDeviceSize range;

	private:
		CRG_API BufferAttachment();
		CRG_API BufferAttachment( Buffer buffer );
		CRG_API BufferAttachment( FlagKind flags
			, Buffer buffer
			, VkDeviceSize offset
			, VkDeviceSize range );
		CRG_API BufferAttachment( FlagKind flags
			, Buffer buffer
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
		CRG_API VkAccessFlags getAccessMask()const;
		CRG_API VkPipelineStageFlags getPipelineStageFlags( bool isCompute )const;

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

		bool isSampledView()const
		{
			return isImage() && image.isSampledView();
		}

		bool isStorageView()const
		{
			return isImage() && image.isStorageView();
		}

		bool isTransferView()const
		{
			return isImage() && image.isTransferView();
		}

		bool isDepthAttach()const
		{
			return isImage() && image.isDepthAttach();
		}

		bool isStencilAttach()const
		{
			return isImage() && image.isStencilAttach();
		}

		bool isClearingAttach()const
		{
			return isImage() && image.isClearingAttach();
		}

		bool isColourAttach()const
		{
			return !isSampledView()
				&& !isStorageView()
				&& !isTransferView()
				&& !isDepthAttach()
				&& !isStencilAttach();
		}

		bool isColourClearingAttach()const
		{
			return isClearingAttach() && isColourAttach();
		}

		bool isColourInputAttach()const
		{
			return isInput() && isColourAttach();
		}

		bool isColourOutputAttach()const
		{
			return isOutput() && isColourAttach();
		}

		bool isColourInOutAttach()const
		{
			return isInput() && isOutput() && isColourAttach();
		}

		bool isDepthClearingAttach()const
		{
			return isClearingAttach() && isDepthAttach();
		}

		bool isDepthInputAttach()const
		{
			return isInput() && isDepthAttach();
		}

		bool isDepthOutputAttach()const
		{
			return isOutput() && isDepthAttach();
		}

		bool isDepthInOutAttach()const
		{
			return isInput() && isOutput() && isDepthAttach();
		}

		bool isStencilClearingAttach()const
		{
			return isImage() && image.isStencilClearingAttach();
		}

		bool isStencilInputAttach()const
		{
			return isImage() && image.isStencilInputAttach();
		}

		bool isStencilOutputAttach()const
		{
			return isImage() && image.isStencilOutputAttach();
		}

		bool isStencilInOutAttach()const
		{
			return isStencilInputAttach() && isStencilOutputAttach();
		}

		bool isDepthStencilInputAttach()const
		{
			return isDepthInputAttach() && isStencilInputAttach();
		}

		bool isDepthStencilOutputAttach()const
		{
			return isDepthOutputAttach() && isStencilOutputAttach();
		}

		bool isDepthStencilInOutAttach()const
		{
			return isDepthInOutAttach() && isStencilInOutAttach();
		}

		bool isTransferInputView()const
		{
			return isInput() && isTransferView();
		}

		bool isTransferOutputView()const
		{
			return isOutput() && isTransferView();
		}

		bool isStorageInputView()const
		{
			return isInput() && isStorageView();
		}

		bool isStorageOutputView()const
		{
			return isOutput() && isStorageView();
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
		static Attachment createDefault( Buffer buffer )
		{
			return Attachment{ buffer };
		}
		/**
		*\name
		*	Members.
		*/
		/**@[*/
		FramePass * pass{};
		uint32_t binding{};
		uint32_t count{};
		std::string name{};
		ImageAttachment image;
		BufferAttachment buffer;
		/**@}*/

		CRG_API Attachment( ImageViewId view
			, Attachment const & origin );

	private:
		CRG_API Attachment( ImageViewId view );
		CRG_API Attachment( Buffer buffer );
		CRG_API Attachment( FlagKind flags
			, FramePass & pass
			, uint32_t binding
			, uint32_t count
			, std::string name
			, ImageAttachment::FlagKind imageFlags
			, ImageViewIdArray views
			, VkAttachmentLoadOp loadOp
			, VkAttachmentStoreOp storeOp
			, VkAttachmentLoadOp stencilLoadOp
			, VkAttachmentStoreOp stencilStoreOp
			, VkImageLayout initialLayout
			, VkImageLayout finalLayout
			, SamplerDesc samplerDesc
			, VkClearValue clearValue
			, VkPipelineColorBlendAttachmentState blendState );
		CRG_API Attachment( FlagKind flags
			, FramePass & pass
			, uint32_t binding
			, uint32_t count
			, std::string name
			, BufferAttachment::FlagKind bufferFlags
			, Buffer buffer
			, VkDeviceSize offset
			, VkDeviceSize range );
		CRG_API Attachment( FlagKind flags
			, FramePass & pass
			, uint32_t binding
			, uint32_t count
			, std::string name
			, BufferAttachment::FlagKind bufferFlags
			, Buffer buffer
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
