/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/Attachment.hpp"
#include "RenderGraph/ImageData.hpp"
#include "RenderGraph/ImageViewData.hpp"
#include "RenderGraph/WriteDescriptorSet.hpp"

#include <cassert>
#include <cstring>

namespace crg
{
	//*********************************************************************************************

	BufferAttachment::BufferAttachment( Buffer buffer )
		: buffer{ std::move( buffer ) }
	{
	}

	BufferAttachment::BufferAttachment( FlagKind flags
		, Buffer buffer
		, DeviceSize offset
		, DeviceSize range
		, AccessState access )
		: buffer{ std::move( buffer ) }
		, range{ offset, range }
		, flags{ flags }
		, wantedAccess{ std::move( access ) }
	{
	}

	BufferAttachment::BufferAttachment( FlagKind flags
		, Buffer buffer
		, VkBufferView view
		, DeviceSize offset
		, DeviceSize range
		, AccessState access )
		: buffer{ std::move( buffer ) }
		, view{ view }
		, range{ offset, range }
		, flags{ flags }
		, wantedAccess{ std::move( access ) }
	{
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

	//*********************************************************************************************

	ImageAttachment::ImageAttachment( ImageViewId view )
		: views{ 1u, view }
	{
	}

	ImageAttachment::ImageAttachment( FlagKind flags
		, ImageViewIdArray views
		, AttachmentLoadOp loadOp
		, AttachmentStoreOp storeOp
		, AttachmentLoadOp stencilLoadOp
		, AttachmentStoreOp stencilStoreOp
		, SamplerDesc samplerDesc
		, ClearValue clearValue
		, PipelineColorBlendAttachmentState blendState
		, ImageLayout wantedLayout )
		: views{ std::move( views ) }
		, loadOp{ loadOp }
		, storeOp{ storeOp }
		, stencilLoadOp{ stencilLoadOp }
		, stencilStoreOp{ stencilStoreOp }
		, samplerDesc{ std::move( samplerDesc ) }
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
		else if ( isColourAttach() )
		{
			result = ImageLayout::eColorAttachment;
		}
#if VK_KHR_separate_depth_stencil_layouts
		else if ( separateDepthStencilLayouts )
		{
			if ( isDepthStencilAttach() )
			{
				if ( isOutput )
					result = ImageLayout::eDepthStencilAttachment;
				else if ( isInput )
					result = ImageLayout::eDepthStencilReadOnly;
			}
			else if ( isStencilAttach() )
			{
				if ( isOutput && isStencilOutputAttach() )
					result = ImageLayout::eStencilAttachment;
				else if ( isInput && isStencilInputAttach() )
					result = ImageLayout::eStencilReadOnly;
			}
			else if ( isDepthAttach() )
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
				&& ( isDepthAttach() || isStencilOutputAttach() ) )
			{
				result = ImageLayout::eDepthStencilAttachment;
			}
			else if ( isInput
				&& ( isDepthAttach() || isStencilInputAttach() ) )
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
		else if ( isDepthAttach() || isStencilAttach() )
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
		else if ( isDepthAttach() || isStencilAttach() )
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

	Attachment::Attachment( ImageViewId view
		, Attachment const & origin )
		: pass{ origin.pass }
		, binding{ origin.binding }
		, name{ origin.name + view.data->name }
		, imageAttach{ origin.imageAttach }
		, bufferAttach{ origin.bufferAttach }
		, flags{ origin.flags }
	{
	}

	Attachment::Attachment( ImageViewId view )
		: imageAttach{ std::move( view ) }
		, flags{ FlagKind( Attachment::Flag::Image ) }
	{
	}

	Attachment::Attachment( Buffer buffer )
		: bufferAttach{ std::move( buffer ) }
		, flags{ FlagKind( Attachment::Flag::Buffer ) }
	{
	}

	Attachment::Attachment( FlagKind flags
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
		, ImageLayout wantedLayout )
		: pass{ &pass }
		, binding{ binding }
		, name{ std::move( name ) }
		, imageAttach{ imageFlags
			, std::move( views )
			, loadOp
			, storeOp
			, stencilLoadOp
			, stencilStoreOp
			, std::move( samplerDesc )
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
		, FramePass & pass
		, uint32_t binding
		, std::string name
		, BufferAttachment::FlagKind bufferFlags
		, Buffer buffer
		, DeviceSize offset
		, DeviceSize range
		, AccessState wantedAccess )
		: pass{ &pass }
		, binding{ binding }
		, name{ std::move( name ) }
		, bufferAttach{ bufferFlags, std::move( buffer ), offset, range, std::move( wantedAccess ) }
		, flags{ FlagKind( flags
			| FlagKind( Flag::Buffer ) ) }
	{
	}

	Attachment::Attachment( FlagKind flags
		, FramePass & pass
		, uint32_t binding
		, std::string name
		, BufferAttachment::FlagKind bufferFlags
		, Buffer buffer
		, VkBufferView view
		, DeviceSize offset
		, DeviceSize range
		, AccessState wantedAccess )
		: pass{ &pass }
		, binding{ binding }
		, name{ std::move( name ) }
		, bufferAttach{ bufferFlags, std::move( buffer ), view, offset, range, std::move( wantedAccess ) }
		, flags{ FlagKind( flags
			| FlagKind( Flag::Buffer ) ) }
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

	VkBuffer Attachment::buffer( uint32_t index )const
	{
		return isBuffer()
			? bufferAttach.buffer.buffer( index )
			: VkBuffer{};
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

	//*********************************************************************************************
}
