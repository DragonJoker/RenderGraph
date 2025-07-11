/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "ImageViewData.hpp"

#include <limits>

#ifdef None
#undef None
#endif

namespace crg
{
	static constexpr uint32_t InvalidBindingId = std::numeric_limits< uint32_t >::max();

	struct SamplerDesc
	{
		FilterMode magFilter{ FilterMode::eNearest };
		FilterMode minFilter{ FilterMode::eNearest };
		MipmapMode mipmapMode{ MipmapMode::eNearest };
		WrapMode addressModeU{ WrapMode::eClampToEdge };
		WrapMode addressModeV{ WrapMode::eClampToEdge };
		WrapMode addressModeW{ WrapMode::eClampToEdge };
		float mipLodBias{ 0.0f };
		float minLod{ -500.0f };
		float maxLod{ 500.0f };

		explicit constexpr SamplerDesc( FilterMode magFilter = FilterMode::eNearest
			, FilterMode minFilter = FilterMode::eNearest
			, MipmapMode mipmapMode = MipmapMode::eNearest
			, WrapMode addressModeU = WrapMode::eClampToEdge
			, WrapMode addressModeV = WrapMode::eClampToEdge
			, WrapMode addressModeW = WrapMode::eClampToEdge
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

	private:
		friend bool operator==( SamplerDesc const & lhs, SamplerDesc const & rhs ) = default;
	};
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
			StencilClearing = 0x01 << 5,
			StencilInput = 0x01 << 6,
			StencilOutput = 0x01 << 7,
			Transition = 0x01 << 8,
			DepthStencil = Depth | Stencil,
			StencilInOut = StencilInput | StencilOutput,
		};
		/**
		*\name
		*	Getters.
		*/
		/**@{*/
		CRG_API ImageViewId view( uint32_t index = 0u )const;
		CRG_API ImageLayout getImageLayout( bool separateDepthStencilLayouts
			, bool isInput
			, bool isOutput )const;
		CRG_API AccessFlags getAccessMask( bool isInput
			, bool isOutput )const;
		CRG_API PipelineStageFlags getPipelineStageFlags( bool isCompute )const;

		FlagKind getFlags()const
		{
			return flags;
		}

		uint32_t getViewCount()const
		{
			return uint32_t( views.size() );
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

		bool isTransitionView()const
		{
			return hasFlag( Flag::Transition );
		}

		bool isDepthAttach()const
		{
			return hasFlag( Flag::Depth ) && !isTransitionView();
		}

		bool isStencilAttach()const
		{
			return hasFlag( Flag::Stencil ) && !isTransitionView();
		}

		bool isDepthStencilAttach()const
		{
			return isDepthAttach() && isStencilAttach();
		}

		bool isColourAttach()const
		{
			return !isSampledView()
				&& !isTransitionView()
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
		ImageViewIdArray views{};
		AttachmentLoadOp loadOp{};
		AttachmentStoreOp storeOp{};
		AttachmentLoadOp stencilLoadOp{};
		AttachmentStoreOp stencilStoreOp{};
		SamplerDesc samplerDesc{};
		ClearValue clearValue{};
		PipelineColorBlendAttachmentState blendState = DefaultBlendState;
		ImageLayout wantedLayout{};

	private:
		CRG_API ImageAttachment() = default;
		CRG_API explicit ImageAttachment( ImageViewId view );
		CRG_API ImageAttachment( FlagKind flags
			, ImageViewIdArray views
			, AttachmentLoadOp loadOp
			, AttachmentStoreOp storeOp
			, AttachmentLoadOp stencilLoadOp
			, AttachmentStoreOp stencilStoreOp
			, SamplerDesc samplerDesc
			, ClearValue clearValue
			, PipelineColorBlendAttachmentState blendState
			, ImageLayout wantedLayout );

		FlagKind flags{};

		friend bool operator==( ImageAttachment const & lhs
			, ImageAttachment const & rhs )
		{
			return lhs.flags == rhs.flags
				&& lhs.views == rhs.views
				&& lhs.loadOp == rhs.loadOp
				&& lhs.storeOp == rhs.storeOp
				&& lhs.stencilLoadOp == rhs.stencilLoadOp
				&& lhs.stencilStoreOp == rhs.stencilStoreOp
				&& lhs.samplerDesc == rhs.samplerDesc
				&& lhs.clearValue == rhs.clearValue
				&& lhs.blendState == rhs.blendState;
		}

	};
	struct BufferSubresourceRange
	{
		DeviceSize offset{};
		DeviceSize size{};
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
			Transfer = 0x01 << 2,
			View = 0x01 << 3,
			Transition = 0x01 << 4,
			UniformView = Uniform | View,
			StorageView = Storage | View,
			TransitionView = Transition | View,
		};

		CRG_API AccessFlags getAccessMask( bool isInput
			, bool isOutput )const;
		CRG_API PipelineStageFlags getPipelineStageFlags( bool isCompute )const;

		FlagKind getFlags()const
		{
			return flags;
		}

		bool hasFlag( Flag flag )const
		{
			return Flag( flags & FlagKind( flag ) ) == flag;
		}

		uint32_t getBufferCount()const
		{
			return uint32_t( buffer.getCount() );
		}

		bool isUniform()const
		{
			return hasFlag( Flag::Uniform );
		}

		bool isStorage()const
		{
			return hasFlag( Flag::Storage );
		}

		bool isTransfer()const
		{
			return hasFlag( Flag::Transfer );
		}

		bool isTransition()const
		{
			return hasFlag( Flag::Transition );
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

		bool isTransitionView()const
		{
			return isTransition() && isView();
		}

		Buffer buffer{ {}, std::string{} };
		VkBufferView view{};
		BufferSubresourceRange range{};

	private:
		CRG_API BufferAttachment() = default;
		CRG_API explicit BufferAttachment( Buffer buffer );
		CRG_API BufferAttachment( FlagKind flags
			, Buffer buffer
			, DeviceSize offset
			, DeviceSize range
			, AccessState access = {} );
		CRG_API BufferAttachment( FlagKind flags
			, Buffer buffer
			, VkBufferView view
			, DeviceSize offset
			, DeviceSize range
			, AccessState access = {} );

		FlagKind flags{};
		AccessState wantedAccess{};

		friend bool operator==( BufferAttachment const & lhs, BufferAttachment const & rhs )
		{
			return lhs.flags == rhs.flags
				&& lhs.buffer == rhs.buffer
				&& lhs.view == rhs.view
				&& lhs.range.offset == rhs.range.offset
				&& lhs.range.size == rhs.range.size;
		}
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
			NoTransition = 0x01 << 4,
			Clearable = 0x01 << 5,
			InOut = Input | Output,
		};
		/**
		*\name
		*	Getters.
		*/
		/**@{*/
		CRG_API uint32_t getViewCount()const;
		CRG_API uint32_t getBufferCount()const;
		CRG_API ImageViewId view( uint32_t index = 0u )const;
		CRG_API VkBuffer buffer( uint32_t index = 0u )const;
		CRG_API ImageLayout getImageLayout( bool separateDepthStencilLayouts )const;
		CRG_API AccessFlags getAccessMask()const;
		CRG_API PipelineStageFlags getPipelineStageFlags( bool isCompute )const;

		FlagKind getFlags()const
		{
			return flags;
		}

		bool hasFlag( Flag flag )const
		{
			return Flag( flags & FlagKind( flag ) ) == flag;
		}

		bool isNoTransition()const
		{
			return hasFlag( Flag::NoTransition );
		}

		bool isInput()const
		{
			return hasFlag( Flag::Input );
		}

		bool isOutput()const
		{
			return hasFlag( Flag::Output );
		}

		bool isInOut()const
		{
			return isInput() && isOutput();
		}

		bool isImage()const
		{
			return hasFlag( Flag::Image );
		}

		bool isBuffer()const
		{
			return hasFlag( Flag::Buffer );
		}

		bool isClearable()const
		{
			return hasFlag( Flag::Clearable );
		}

		bool isUniformBuffer()const
		{
			return isBuffer() && bufferAttach.isUniform();
		}

		bool isUniformBufferView()const
		{
			return isBuffer() && bufferAttach.isUniformView();
		}

		bool isStorageBuffer()const
		{
			return isBuffer() && bufferAttach.isStorage();
		}

		bool isTransferBuffer()const
		{
			return isBuffer() && bufferAttach.isTransfer();
		}

		bool isTransferInputBuffer()const
		{
			return isInput() && isTransferBuffer();
		}

		bool isTransferOutputBuffer()const
		{
			return isOutput() && isTransferBuffer();
		}

		bool isClearableBuffer()const
		{
			return isBuffer()
				&& isOutput()
				&& isClearable();
		}

		bool isClearableImage()const
		{
			return isImage()
				&& isOutput()
				&& isClearable();
		}

		bool isStorageBufferView()const
		{
			return isBuffer() && bufferAttach.isStorageView();
		}

		bool isTransitionBuffer()const
		{
			return isBuffer() && bufferAttach.isTransition();
		}

		bool isTransitionBufferView()const
		{
			return isBuffer() && bufferAttach.isTransitionView();
		}

		bool isBufferView()const
		{
			return isBuffer() && bufferAttach.isView();
		}

		bool isSampledView()const
		{
			return isImage() && imageAttach.isSampledView();
		}

		bool isStorageView()const
		{
			return isImage() && imageAttach.isStorageView();
		}

		bool isTransferView()const
		{
			return isImage() && imageAttach.isTransferView();
		}

		bool isTransitionView()const
		{
			return isImage() && imageAttach.isTransitionView();
		}

		bool isDepthAttach()const
		{
			return isImage() && imageAttach.isDepthAttach();
		}

		bool isStencilAttach()const
		{
			return isImage() && imageAttach.isStencilAttach();
		}

		bool isColourAttach()const
		{
			return !isSampledView()
				&& !isTransitionView()
				&& !isStorageView()
				&& !isTransferView()
				&& !isDepthAttach()
				&& !isStencilAttach();
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
			return isImage() && imageAttach.isStencilClearingAttach();
		}

		bool isStencilInputAttach()const
		{
			return isImage() && imageAttach.isStencilInputAttach();
		}

		bool isStencilOutputAttach()const
		{
			return isImage() && imageAttach.isStencilOutputAttach();
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

		BufferSubresourceRange const & getBufferRange()const
		{
			return bufferAttach.range;
		}

		SamplerDesc const & getSamplerDesc()const
		{
			return imageAttach.samplerDesc;
		}

		ClearValue const & getClearValue()const
		{
			return imageAttach.clearValue;
		}

		AttachmentLoadOp getLoadOp()const
		{
			return imageAttach.loadOp;
		}

		AttachmentLoadOp getStencilLoadOp()const
		{
			return imageAttach.stencilLoadOp;
		}

		AttachmentStoreOp getStoreOp()const
		{
			return imageAttach.storeOp;
		}

		AttachmentStoreOp getStencilStoreOp()const
		{
			return imageAttach.stencilStoreOp;
		}

		PipelineColorBlendAttachmentState getBlendState()const
		{
			return imageAttach.blendState;
		}
		/**@}*/
		/**
		*\brief
		*	Creates a default empty attachment.
		*/
		static Attachment createDefault( ImageViewId view )
		{
			return Attachment{ std::move( view ) };
		}
		static Attachment createDefault( Buffer buffer )
		{
			return Attachment{ std::move( buffer ) };
		}
		/**
		*\name
		*	Members.
		*/
		/**@[*/
		FramePass * pass{};
		uint32_t binding{};
		std::string name{};
		ImageAttachment imageAttach{};
		BufferAttachment bufferAttach{};
		/**@}*/

		CRG_API Attachment( ImageViewId view
			, Attachment const & origin );

	private:
		CRG_API explicit Attachment( ImageViewId view );
		CRG_API explicit Attachment( Buffer buffer );
		CRG_API Attachment( FlagKind flags
			, FramePass & pass
			, uint32_t binding
			, std::string name
			, ImageAttachment::FlagKind imageFlags
			, ImageViewIdArray views
			, AttachmentLoadOp loadOp
			, AttachmentStoreOp storeOp
			, AttachmentLoadOp stencilLoadOp
			, AttachmentStoreOp stencilStoreOp
			, SamplerDesc samplerDesc
			, ClearValue clearValue
			, PipelineColorBlendAttachmentState blendState
			, ImageLayout wantedLayout );
		CRG_API Attachment( FlagKind flags
			, FramePass & pass
			, uint32_t binding
			, std::string name
			, BufferAttachment::FlagKind bufferFlags
			, Buffer buffer
			, DeviceSize offset
			, DeviceSize range
			, AccessState wantedAccess );
		CRG_API Attachment( FlagKind flags
			, FramePass & pass
			, uint32_t binding
			, std::string name
			, BufferAttachment::FlagKind bufferFlags
			, Buffer buffer
			, VkBufferView view
			, DeviceSize offset
			, DeviceSize range
			, AccessState wantedAccess );

		FlagKind flags{};

		friend bool operator==( Attachment const & lhs
			, Attachment const & rhs )
		{
			return lhs.pass == rhs.pass
				&& lhs.flags == rhs.flags
				&& lhs.imageAttach == rhs.imageAttach
				&& lhs.bufferAttach == rhs.bufferAttach;
		}
	};
}
