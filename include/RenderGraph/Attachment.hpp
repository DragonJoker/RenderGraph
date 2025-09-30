/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "BufferViewData.hpp"
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

		FlagKind getFormatFlags()const
		{
			return FlagKind( flags & FlagKind( Flag::DepthStencil ) );
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

		bool isDepthTarget()const
		{
			return hasFlag( Flag::Depth ) && !isTransitionView();
		}

		bool isStencilTarget()const
		{
			return hasFlag( Flag::Stencil ) && !isTransitionView();
		}

		bool isDepthStencilTarget()const
		{
			return isDepthTarget() && isStencilTarget();
		}

		bool isColourTarget()const
		{
			return !isSampledView()
				&& !isTransitionView()
				&& !isStorageView()
				&& !isTransferView()
				&& !isDepthTarget()
				&& !isStencilTarget();
		}

		bool isStencilClearingTarget()const
		{
			return hasFlag( Flag::StencilClearing );
		}

		bool isStencilInputTarget()const
		{
			return hasFlag( Flag::StencilInput );
		}

		bool isStencilOutputTarget()const
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
		ClearValue clearValue{};
		PipelineColorBlendAttachmentState blendState = DefaultBlendState;
		ImageLayout wantedLayout{};
		FlagKind flags{};

	private:
		CRG_API ImageAttachment() = default;
		CRG_API explicit ImageAttachment( ImageViewIdArray view );
		CRG_API ImageAttachment( FlagKind flags
			, ImageViewIdArray views
			, AttachmentLoadOp loadOp
			, AttachmentStoreOp storeOp
			, AttachmentLoadOp stencilLoadOp
			, AttachmentStoreOp stencilStoreOp
			, ClearValue clearValue
			, PipelineColorBlendAttachmentState blendState
			, ImageLayout wantedLayout );

		friend bool operator==( ImageAttachment const & lhs
			, ImageAttachment const & rhs )noexcept
		{
			return lhs.flags == rhs.flags
				&& lhs.views == rhs.views
				&& lhs.loadOp == rhs.loadOp
				&& lhs.storeOp == rhs.storeOp
				&& lhs.stencilLoadOp == rhs.stencilLoadOp
				&& lhs.stencilStoreOp == rhs.stencilStoreOp
				&& lhs.clearValue == rhs.clearValue
				&& lhs.blendState == rhs.blendState;
		}

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

		CRG_API BufferViewId buffer( uint32_t index = 0u )const;
		CRG_API AccessFlags getAccessMask( bool isInput
			, bool isOutput )const;
		CRG_API PipelineStageFlags getPipelineStageFlags( bool isCompute )const;

		CRG_API uint32_t getBufferCount()const;

		FlagKind getFlags()const
		{
			return flags;
		}

		FlagKind getFormatFlags()const
		{
			return FlagKind( flags & FlagKind( Flag::View ) );
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

	public:
		BufferViewIdArray buffers;
		FlagKind flags{};
		AccessState wantedAccess{};

	private:
		CRG_API BufferAttachment() = default;
		CRG_API explicit BufferAttachment( BufferViewIdArray view );
		CRG_API BufferAttachment( FlagKind flags
			, BufferViewIdArray views
			, AccessState access = {} );

		friend bool operator==( BufferAttachment const & lhs
			, BufferAttachment const & rhs )noexcept
		{
			return lhs.flags == rhs.flags
				&& lhs.buffers == rhs.buffers;
		}
	};
	/**
	*\brief
	*	An attachment to a pass.
	*/
	struct Attachment
	{
		friend struct FramePass;
		friend class FrameGraph;

		class Token
		{
			friend struct Attachment;
			friend struct FramePass;

		private:
			Token() noexcept = default;
		};

		struct Source
		{
			Source( Attachment const * parent
				, FramePass const * pass
				, ImageAttachment const & attach )
				: parent{ parent }
				, pass{ pass }
				, imageAttach{ &attach }
			{
			}

			Source( Attachment const * parent
				, FramePass const * pass
				, BufferAttachment const & attach )
				: parent{ parent }
				, pass{ pass }
				, bufferAttach{ &attach }
			{
			}

			explicit Source( AttachmentPtr sourceAttach )
				: pass{ sourceAttach->pass }
				, imageAttach{ sourceAttach->isImage() ? &sourceAttach->imageAttach : nullptr }
				, bufferAttach{ sourceAttach->isBuffer() ? &sourceAttach->bufferAttach : nullptr }
				, attach{ std::move( sourceAttach ) }
			{
			}

			Attachment const * parent{};
			FramePass const * pass{};
			ImageAttachment const * imageAttach{};
			BufferAttachment const * bufferAttach{};
			AttachmentPtr attach;
		};
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

		CRG_API Attachment( Attachment const & rhs );
		CRG_API Attachment & operator=( Attachment const & rhs );
		CRG_API Attachment( Attachment && rhs )noexcept = default;
		CRG_API Attachment & operator=( Attachment && rhs )noexcept = default;
		/**
		*\name
		*	Getters.
		*/
		/**@{*/
		CRG_API uint32_t getViewCount()const;
		CRG_API uint32_t getBufferCount()const;
		CRG_API ImageViewId view( uint32_t index = 0u )const;
		CRG_API BufferViewId buffer( uint32_t index = 0u )const;
		CRG_API ImageLayout getImageLayout( bool separateDepthStencilLayouts )const;
		CRG_API AccessFlags getAccessMask()const;
		CRG_API PipelineStageFlags getPipelineStageFlags( bool isCompute )const;
		CRG_API Attachment const * getSource( uint32_t index )const;

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

		bool isSampledImageView()const
		{
			return isImage() && imageAttach.isSampledView();
		}

		bool isStorageImageView()const
		{
			return isImage() && imageAttach.isStorageView();
		}

		bool isTransferImageView()const
		{
			return isImage() && imageAttach.isTransferView();
		}

		bool isTransitionImageView()const
		{
			return isImage() && imageAttach.isTransitionView();
		}

		bool isDepthImageTarget()const
		{
			return isImage() && imageAttach.isDepthTarget();
		}

		bool isStencilImageTarget()const
		{
			return isImage() && imageAttach.isStencilTarget();
		}

		bool isColourImageTarget()const
		{
			return !isSampledImageView()
				&& !isTransitionImageView()
				&& !isStorageImageView()
				&& !isTransferImageView()
				&& !isDepthImageTarget()
				&& !isStencilImageTarget();
		}

		bool isColourInputImageTarget()const
		{
			return isInput() && isColourImageTarget();
		}

		bool isColourOutputImageTarget()const
		{
			return isOutput() && isColourImageTarget();
		}

		bool isColourInOutImageTarget()const
		{
			return isInput() && isOutput() && isColourImageTarget();
		}

		bool isDepthInputImageTarget()const
		{
			return isInput() && isDepthImageTarget();
		}

		bool isDepthOutputImageTarget()const
		{
			return isOutput() && isDepthImageTarget();
		}

		bool isDepthInOutImageTarget()const
		{
			return isInput() && isOutput() && isDepthImageTarget();
		}

		bool isStencilClearingImageTarget()const
		{
			return isImage() && imageAttach.isStencilClearingTarget();
		}

		bool isStencilInputImageTarget()const
		{
			return isImage() && imageAttach.isStencilInputTarget();
		}

		bool isStencilOutputImageTarget()const
		{
			return isImage() && imageAttach.isStencilOutputTarget();
		}

		bool isStencilInOutImageTarget()const
		{
			return isStencilInputImageTarget() && isStencilOutputImageTarget();
		}

		bool isDepthStencilInputImageTarget()const
		{
			return isDepthInputImageTarget() && isStencilInputImageTarget();
		}

		bool isDepthStencilOutputImageTarget()const
		{
			return isDepthOutputImageTarget() && isStencilOutputImageTarget();
		}

		bool isDepthStencilInOutImageTarget()const
		{
			return isDepthInOutImageTarget() && isStencilInOutImageTarget();
		}

		bool isTransferInputImageView()const
		{
			return isInput() && isTransferImageView();
		}

		bool isTransferOutputImageView()const
		{
			return isOutput() && isTransferImageView();
		}

		bool isStorageInputImageView()const
		{
			return isInput() && isStorageImageView();
		}

		bool isStorageOutputImageView()const
		{
			return isOutput() && isStorageImageView();
		}

		BufferSubresourceRange const & getBufferRange()const
		{
			return getSubresourceRange( bufferAttach.buffer() );
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
		static Attachment createDefault( ImageViewIdArray views )
		{
			return Attachment{ std::move( views ) };
		}
		static Attachment createDefault( BufferViewIdArray views )
		{
			return Attachment{ std::move( views ) };
		}
		static Attachment createDefault( ImageViewId view )
		{
			return createDefault( ImageViewIdArray{ view } );
		}
		static Attachment createDefault( BufferViewId view )
		{
			return createDefault( BufferViewIdArray{ view } );
		}
		/**
		*\name
		*	Members.
		*/
		/**@[*/
		FramePass const * pass{};
		std::string name{};
		ImageAttachment imageAttach{};
		BufferAttachment bufferAttach{};
		std::vector< Source > source{};
		FlagKind flags{};
		/**@}*/

		CRG_API Attachment( ImageViewId view
			, Attachment const & origin );
		CRG_API Attachment( BufferViewId view
			, Attachment const & origin );
		CRG_API explicit Attachment( ImageViewIdArray view );
		CRG_API explicit Attachment( BufferViewIdArray view );

		Attachment( FlagKind flags
			, std::string name
			, FramePass const * pass
			, ImageAttachment attach
			, Token token );
		Attachment( FlagKind flags
			, std::string name
			, FramePass const * pass
			, BufferAttachment attach
			, Token token );
		Attachment( FlagKind flags
			, FramePass const & pass
			, std::string name
			, ImageAttachment::FlagKind imageFlags
			, ImageViewIdArray views
			, AttachmentLoadOp loadOp
			, AttachmentStoreOp storeOp
			, AttachmentLoadOp stencilLoadOp
			, AttachmentStoreOp stencilStoreOp
			, ClearValue clearValue
			, PipelineColorBlendAttachmentState blendState
			, ImageLayout wantedLayout
			, Token token );
		Attachment( FlagKind flags
			, FramePass const & pass
			, std::string name
			, BufferAttachment::FlagKind bufferFlags
			, BufferViewIdArray views
			, AccessState wantedAccess
			, Token token );

	private:
		void initSources();

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
