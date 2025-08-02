/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/Attachment.hpp"
#include "RenderGraph/BufferData.hpp"
#include "RenderGraph/BufferViewData.hpp"
#include "RenderGraph/Exception.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/ImageViewData.hpp"
#include "RenderGraph/WriteDescriptorSet.hpp"

#include <cassert>
#include <cstring>

namespace crg
{
	//*********************************************************************************************

	BufferAttachment::BufferAttachment( BufferViewIdArray views )
		: buffers{ std::move( views ) }
		, flags{ FlagKind( buffers.front().data->info.format == PixelFormat::eUNDEFINED
			? Flag::None
			: Flag::View ) }
	{
	}

	BufferAttachment::BufferAttachment( FlagKind flags
		, BufferViewIdArray views
		, AccessState access )
		: buffers{ std::move( views ) }
		, flags{ FlagKind( flags
			| FlagKind( buffers.front().data->info.format == PixelFormat::eUNDEFINED ? Flag::None : Flag::View ) ) }
		, wantedAccess{ std::move( access ) }
	{
	}

	BufferViewId BufferAttachment::buffer( uint32_t index )const
	{
		return buffers.size() == 1u
			? buffers.front()
			: buffers[index];
	}

	AccessFlags BufferAttachment::getAccessMask( bool isInput
		, bool isOutput )const
	{
		AccessFlags result{ 0u };

		if ( isTransition() )
		{
			result = wantedAccess.access;
		}
		else if ( isStorage() )
		{
			if ( isInput )
			{
				result |= AccessFlags::eShaderRead;
			}

			if ( isOutput )
			{
				result |= AccessFlags::eShaderWrite;
			}
		}
		else if ( isTransfer() )
		{
			if ( isInput )
			{
				result |= AccessFlags::eTransferRead;
			}

			if ( isOutput )
			{
				result |= AccessFlags::eTransferWrite;
			}
		}
		else
		{
			result |= AccessFlags::eShaderRead;
		}

		return result;
	}

	PipelineStageFlags BufferAttachment::getPipelineStageFlags( bool isCompute )const
	{
		PipelineStageFlags result{ 0u };

		if ( isTransition() )
		{
			result = wantedAccess.pipelineStage;
		}
		else if ( isTransfer() )
		{
			result |= PipelineStageFlags::eTransfer;
		}
		else if ( isCompute )
		{
			result |= PipelineStageFlags::eComputeShader;
		}
		else
		{
			result |= PipelineStageFlags::eFragmentShader;
		}

		return result;
	}

	uint32_t BufferAttachment::getBufferCount()const
	{
		return uint32_t( buffers.size() );
	}

	//*********************************************************************************************

	ImageAttachment::ImageAttachment( ImageViewIdArray views )
		: views{std::move( views )  }
	{
	}

	ImageAttachment::ImageAttachment( FlagKind flags
		, ImageViewIdArray views
		, AttachmentLoadOp loadOp
		, AttachmentStoreOp storeOp
		, AttachmentLoadOp stencilLoadOp
		, AttachmentStoreOp stencilStoreOp
		, ClearValue clearValue
		, PipelineColorBlendAttachmentState blendState
		, ImageLayout wantedLayout )
		: views{ std::move( views ) }
		, loadOp{ loadOp }
		, storeOp{ storeOp }
		, stencilLoadOp{ stencilLoadOp }
		, stencilStoreOp{ stencilStoreOp }
		, clearValue{ std::move( clearValue ) }
		, blendState{ std::move( blendState ) }
		, wantedLayout{ wantedLayout }
		, flags{ flags }
	{
		assert( ( checkFlag( getAspectFlags( view() ), ImageAspectFlags::eColor ) && isColourFormat( getFormat( view() ) ) )
			|| ( checkFlag( getAspectFlags( view() ), ImageAspectFlags::eDepthStencil ) && isDepthStencilFormat( getFormat( view() ) ) )
			|| ( checkFlag( getAspectFlags( view() ), ImageAspectFlags::eDepth ) && isDepthFormat( getFormat( view() ) ) )
			|| ( checkFlag( getAspectFlags( view() ), ImageAspectFlags::eStencil ) && isStencilFormat( getFormat( view() ) ) ) );
		assert( ( !isSampledView() && !isTransitionView() && !isTransferView() )
			|| ( ( this->loadOp == AttachmentLoadOp::eDontCare )
				&& ( this->storeOp == AttachmentStoreOp::eDontCare )
				&& ( this->stencilLoadOp == AttachmentLoadOp::eDontCare )
				&& ( this->stencilStoreOp == AttachmentStoreOp::eDontCare ) ) );
	}

	ImageViewId ImageAttachment::view( uint32_t index )const
	{
		return views.size() == 1u
			? views.front()
			: views[index];
	}

	ImageLayout ImageAttachment::getImageLayout( bool separateDepthStencilLayouts
		, bool isInput
		, bool isOutput )const
	{
		auto result = ImageLayout::eReadOnly;

		if ( isSampledView() )
		{
			result = ImageLayout::eShaderReadOnly;
		}
		else if ( isTransitionView() )
		{
			result = wantedLayout;
		}
		else if ( isStorageView() )
		{
			result = ImageLayout::eGeneral;
		}
		else if ( isTransferView() )
		{
			if ( isOutput )
				result = ImageLayout::eTransferDst;
			else if ( isInput )
				result = ImageLayout::eTransferSrc;
		}
		else if ( isColourTarget() )
		{
			result = ImageLayout::eColorAttachment;
		}
#if VK_KHR_separate_depth_stencil_layouts
		else if ( separateDepthStencilLayouts )
		{
			if ( isDepthStencilTarget() )
			{
				if ( isOutput )
					result = ImageLayout::eDepthStencilAttachment;
				else if ( isInput )
					result = ImageLayout::eDepthStencilReadOnly;
			}
			else if ( isStencilTarget() )
			{
				if ( isOutput && isStencilOutputTarget() )
					result = ImageLayout::eStencilAttachment;
				else if ( isInput && isStencilInputTarget() )
					result = ImageLayout::eStencilReadOnly;
			}
			else if ( isDepthTarget() )
			{
				if ( isOutput )
					result = ImageLayout::eDepthAttachment;
				else if ( isInput )
					result = ImageLayout::eDepthReadOnly;
			}
		}
		else
#endif
		{
			if ( isOutput
				&& ( isDepthTarget() || isStencilOutputTarget() ) )
			{
				result = ImageLayout::eDepthStencilAttachment;
			}
			else if ( isInput
				&& ( isDepthTarget() || isStencilInputTarget() ) )
			{
				result = ImageLayout::eDepthStencilReadOnly;
			}
		}

		return result;
	}

	AccessFlags ImageAttachment::getAccessMask( bool isInput
		, bool isOutput )const
	{
		AccessFlags result{ 0u };

		if ( isSampledView() )
		{
			result |= AccessFlags::eShaderRead;
		}
		else if ( isTransitionView() )
		{
			result |= crg::getAccessMask( wantedLayout );
		}
		else if ( isStorageView() )
		{
			if ( isInput )
				result |= AccessFlags::eShaderRead;
			if ( isOutput )
				result |= AccessFlags::eShaderWrite;
		}
		else if ( isTransferView() )
		{
			if ( isInput )
				result |= AccessFlags::eTransferRead;
			if ( isOutput )
				result |= AccessFlags::eTransferWrite;
		}
		else if ( isDepthTarget() || isStencilTarget() )
		{
			if ( isInput )
				result |= AccessFlags::eDepthStencilAttachmentRead;
			if ( isOutput )
				result |= AccessFlags::eDepthStencilAttachmentWrite;
		}
		else
		{
			if ( isInput )
				result |= AccessFlags::eColorAttachmentRead;
			if ( isOutput )
				result |= AccessFlags::eColorAttachmentWrite;
		}

		return result;
	}

	PipelineStageFlags ImageAttachment::getPipelineStageFlags( bool isCompute )const
	{
		PipelineStageFlags result{ 0u };

		if ( isSampledView() )
		{
			result |= PipelineStageFlags::eFragmentShader;
		}
		else if ( isTransitionView() )
		{
			result |= getStageMask( wantedLayout );
		}
		else if ( isStorageView() )
		{
			if ( isCompute )
				result |= PipelineStageFlags::eComputeShader;
			else
				result |= PipelineStageFlags::eFragmentShader;
		}
		else if ( isTransferView() )
		{
			result |= PipelineStageFlags::eTransfer;
		}
		else if ( isDepthTarget() || isStencilTarget() )
		{
			result |= PipelineStageFlags::eLateFragmentTests;
		}
		else
		{
			result |= PipelineStageFlags::eColorAttachmentOutput;
		}

		return result;
	}

	//*********************************************************************************************

	Attachment::Attachment( Attachment const & rhs )
		: pass{ rhs.pass }
		, name{ rhs.name }
		, imageAttach{ rhs.imageAttach }
		, bufferAttach{ rhs.bufferAttach }
		, flags{ rhs.flags }
	{
	}

	Attachment & Attachment::operator=( Attachment const & rhs )
	{
		pass = rhs.pass;
		name = rhs.name;
		imageAttach = rhs.imageAttach;
		bufferAttach = rhs.bufferAttach;
		flags = rhs.flags;

		return *this;
	}

	Attachment::Attachment( ImageViewId view
		, Attachment const & origin )
		: pass{ origin.pass }
		, name{ origin.name + view.data->name }
		, imageAttach{ origin.imageAttach }
		, flags{ origin.flags }
	{
		imageAttach.views = { view };
	}

	Attachment::Attachment( BufferViewId view
		, Attachment const & origin )
		: pass{ origin.pass }
		, name{ origin.name + view.data->name }
		, bufferAttach{ origin.bufferAttach }
		, flags{ origin.flags }
	{
		bufferAttach.buffers = { view };
	}

	Attachment::Attachment( ImageViewIdArray views )
		: imageAttach{ std::move( views ) }
		, flags{ FlagKind( Attachment::Flag::Image ) }
	{
	}

	Attachment::Attachment( BufferViewIdArray views )
		: bufferAttach{ std::move( views ) }
		, flags{ FlagKind( Attachment::Flag::Buffer ) }
	{
	}

	Attachment::Attachment( FlagKind flags
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
		, Token )
		: pass{ &pass }
		, name{ std::move( name ) }
		, imageAttach{ imageFlags
			, std::move( views )
			, loadOp
			, storeOp
			, stencilLoadOp
			, stencilStoreOp
			, std::move( clearValue )
			, std::move( blendState )
			, wantedLayout }
		, flags{ FlagKind( flags
			| FlagKind( Flag::Image )
			| ( loadOp == AttachmentLoadOp::eLoad
				? FlagKind( Flag::Input )
				: FlagKind( Flag::None ) )
			| ( storeOp == AttachmentStoreOp::eStore
				? FlagKind( Flag::Output )
				: FlagKind( Flag::None ) )
			| ( stencilLoadOp == AttachmentLoadOp::eLoad
				? FlagKind( Flag::Input )
				: FlagKind( Flag::None ) )
			| ( stencilStoreOp == AttachmentStoreOp::eStore
				? FlagKind( Flag::Output )
				: FlagKind( Flag::None ) ) ) }
	{
	}

	Attachment::Attachment( FlagKind flags
		, FramePass const & pass
		, std::string name
		, BufferAttachment::FlagKind bufferFlags
		, BufferViewIdArray views
		, AccessState wantedAccess
		, Token )
		: pass{ &pass }
		, name{ std::move( name ) }
		, bufferAttach{ bufferFlags, std::move( views ), std::move( wantedAccess ) }
		, flags{ FlagKind( flags
			| FlagKind( Flag::Buffer ) ) }
	{
	}

	Attachment::Attachment( FlagKind flags
		, std::string name
		, FramePass const * pass
		, ImageAttachment attach
		, Token )
		: pass{ pass }
		, name{ std::move( name ) }
		, imageAttach{ std::move( attach ) }
		, flags{ flags }
	{
	}

	Attachment::Attachment( FlagKind flags
		, std::string name
		, FramePass const * pass
		, BufferAttachment attach
		, Token )
		: pass{ pass }
		, name{ std::move( name ) }
		, bufferAttach{ std::move( attach ) }
		, flags{ flags }
	{
	}

	uint32_t Attachment::getViewCount()const
	{
		return isImage()
			? imageAttach.getViewCount()
			: uint32_t{};
	}

	uint32_t Attachment::getBufferCount()const
	{
		return isBuffer()
			? bufferAttach.getBufferCount()
			: uint32_t{};
	}

	ImageViewId Attachment::view( uint32_t index )const
	{
		return isImage()
			? imageAttach.view( index )
			: ImageViewId{};
	}

	BufferViewId Attachment::buffer( uint32_t index )const
	{
		return isBuffer()
			? bufferAttach.buffer( index )
			: BufferViewId{};
	}

	ImageLayout Attachment::getImageLayout( bool separateDepthStencilLayouts )const
	{
		assert( isImage() );
		return imageAttach.getImageLayout( separateDepthStencilLayouts
			, isInput()
			, isOutput() );
	}

	AccessFlags Attachment::getAccessMask()const
	{
		if ( isImage() )
		{
			return imageAttach.getAccessMask( isInput()
				, isOutput() );
		}

		return bufferAttach.getAccessMask( isInput()
			, isOutput() );
	}

	PipelineStageFlags Attachment::getPipelineStageFlags( bool isCompute )const
	{
		if ( isImage() )
		{
			return imageAttach.getPipelineStageFlags( isCompute );
		}

		return bufferAttach.getPipelineStageFlags( isCompute );
	}

	Attachment const * Attachment::getSource( uint32_t index )const
	{
		if ( index > 0 && index >= source.size() )
			CRG_Exception( "Invalid index" );

		if ( source.empty() )
			return this;

		return source[index].attach.get();
	}

	void Attachment::initSources()
	{
		for ( uint32_t index = 0u; index < source.size(); ++index )
		{
			Source & attachSource = source[index];
			if ( isImage() )
			{
				attachSource.attach = std::make_unique< Attachment >( flags
					, name + std::to_string( index )
					, attachSource.pass
					, *attachSource.imageAttach
					, Token{} );
				attachSource.imageAttach = &attachSource.attach->imageAttach;
			}
			else
			{
				attachSource.attach = std::make_unique< Attachment >( flags
					, name + std::to_string( index )
					, attachSource.pass
					, *attachSource.bufferAttach
					, Token{} );
				attachSource.bufferAttach = &attachSource.attach->bufferAttach;
			}
		}
	}

	//*********************************************************************************************
}
