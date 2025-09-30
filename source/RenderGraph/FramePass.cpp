/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "RenderGraph/FramePass.hpp"

#include "RenderGraph/FrameGraph.hpp"
#include "RenderGraph/RunnablePass.hpp"

#include <array>

namespace crg
{
	inline uint32_t constexpr ImplicitOffset = 1024U;
	inline uint32_t constexpr TransferOffset = 4096U;

	namespace fpass
	{
		static std::string adjustName( FramePass const & pass
			, std::string const & dataName )
		{
			auto result = pass.getGroupName() + "/" + dataName;
			uint32_t index = 0u;

			while ( result[index] == '/' )
			{
				++index;
			}

			return result.substr( index );
		}
	}

	FramePass::FramePass( FramePassGroup const & group
		, FrameGraph & graph
		, uint32_t id
		, std::string const & name
		, RunnablePassCreator runnableCreator )
		: m_group{ group }
		, m_graph{ graph }
		, m_id{ id }
		, m_runnableCreator{ std::move( runnableCreator ) }
		, m_name{ name }
	{
	}

	Attachment const * FramePass::getParentAttachment( Attachment const & attach )const
	{
		auto it = m_ownAttaches.find( &attach );
		return it != m_ownAttaches.end()
			? it->second.parent
			: nullptr;
	}

	void FramePass::addInputUniformBuffer( BufferViewIdArray buffers
		, uint32_t binding )
	{
		auto attachName = fpass::adjustName( *this, buffers.front().data->name ) + "/UB";
		auto attach = addOwnAttach( std::move( buffers )
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Input )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Uniform )
			, AccessState{}
			, nullptr );
		m_uniforms.try_emplace( binding, attach );
	}

	void FramePass::addInputSampledImage( ImageViewIdArray views
		, uint32_t binding
		, SamplerDesc samplerDesc )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/Spl";
		auto attach = addOwnAttach( std::move( views )
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Input )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Sampled )
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, ClearValue{}
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eShaderReadOnly
			, nullptr );
		m_sampled.try_emplace( binding, attach, std::move( samplerDesc ) );
	}

	void FramePass::addInputUniform( Attachment const & attachment
		, uint32_t binding )
	{
		auto attachName = fpass::adjustName( *this, attachment.buffer().data->name ) + "/UB";
		auto attach = addOwnAttach( attachment.bufferAttach.buffers
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Input )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Uniform )
			, AccessState{}
			, &attachment );
		m_uniforms.try_emplace( binding, attach );
	}

	void FramePass::addInputSampled( Attachment const & attachment
		, uint32_t binding
		, SamplerDesc samplerDesc )
	{
		auto attachName = fpass::adjustName( *this, attachment.view( 0 ).data->name ) + "/Spl";
		auto attach = addOwnAttach( attachment.imageAttach.views
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Input )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Sampled )
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, ClearValue{}
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eShaderReadOnly
			, &attachment );
		m_sampled.try_emplace( binding, attach, std::move( samplerDesc ) );
	}

	void FramePass::addInputStorageBuffer( BufferViewIdArray buffers
			, uint32_t binding )
	{
		auto attachName = fpass::adjustName( *this, buffers.front().data->name ) + "/SB";
		auto attach = addOwnAttach( std::move( buffers )
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Input )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Storage )
			, AccessState{}
		, nullptr );
		m_inputs.try_emplace( binding, attach );
	}

	void FramePass::addInputStorageImage( ImageViewIdArray views
		, uint32_t binding )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/IStr";
		auto attach = addOwnAttach( std::move( views )
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Input )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Storage )
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, ClearValue{}
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eGeneral
			, nullptr );
		m_inputs.try_emplace( binding, attach );
	}

	void FramePass::addInputStorage( Attachment const & attachment
		, uint32_t binding )
	{
		if ( attachment.isImage() )
		{
			auto attachName = fpass::adjustName( *this, attachment.view().data->name ) + "/IStr";
			auto attach = addOwnAttach( attachment.imageAttach.views
				, std::move( attachName )
				, Attachment::FlagKind( Attachment::Flag::Input )
				, ImageAttachment::FlagKind( ImageAttachment::Flag::Storage )
				, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
				, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
				, ClearValue{}
				, PipelineColorBlendAttachmentState{}
				, ImageLayout::eGeneral
				, &attachment );
			m_inputs.try_emplace( binding, attach );
		}
		else
		{
			auto attachName = fpass::adjustName( *this, attachment.buffer().data->name ) + "/IStr";
			auto attach = addOwnAttach( attachment.bufferAttach.buffers
				, std::move( attachName )
				, Attachment::FlagKind( Attachment::Flag::Input )
				, BufferAttachment::FlagKind( BufferAttachment::Flag::Storage )
				, AccessState{}
			, &attachment );
			m_inputs.try_emplace( binding, attach );
		}
	}

	Attachment const * FramePass::addInOutStorage( Attachment const & attachment
		, uint32_t binding )
	{
		Attachment const * result{};

		if ( attachment.isImage() )
		{
			auto attachName = fpass::adjustName( *this, attachment.view().data->name ) + "/IOStr";
			result = addOwnAttach( attachment.imageAttach.views
				, std::move( attachName )
				, Attachment::FlagKind( Attachment::Flag::InOut )
				, ImageAttachment::FlagKind( ImageAttachment::Flag::Storage )
				, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
				, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
				, ClearValue{}
				, PipelineColorBlendAttachmentState{}
				, ImageLayout::eGeneral
				, &attachment );
			m_inouts.try_emplace( binding, result );
		}
		else
		{
			auto attachName = fpass::adjustName( *this, attachment.buffer().data->name ) + "/IOStr";
			result = addOwnAttach( attachment.bufferAttach.buffers
				, std::move( attachName )
				, Attachment::FlagKind( Attachment::Flag::InOut )
				, BufferAttachment::FlagKind( BufferAttachment::Flag::Storage )
				, AccessState{}
			, &attachment );
			m_inouts.try_emplace( binding, result );
		}

		return result;
	}

	Attachment const * FramePass::addOutputStorageBuffer( BufferViewIdArray buffers
		, uint32_t binding )
	{
		auto attachName = fpass::adjustName( *this, buffers.front().data->name ) + "/OSB";
		auto result = addOwnAttach( std::move( buffers )
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Output )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Storage )
			, AccessState{}
		, nullptr );
		m_outputs.try_emplace( binding, result );
		return result;
	}

	Attachment const * FramePass::addClearableOutputStorageBuffer( BufferViewIdArray buffers
		, uint32_t binding )
	{
		auto attachName = fpass::adjustName( *this, buffers.front().data->name ) + "/OSB";
		auto result = addOwnAttach( std::move( buffers )
			, std::move( attachName )
			, ( Attachment::FlagKind( Attachment::Flag::Output ) | Attachment::FlagKind( Attachment::Flag::Clearable ) )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Storage )
			, AccessState{}
		, nullptr );
		m_outputs.try_emplace( binding, result );
		return result;
	}

	Attachment const * FramePass::addOutputStorageImage( ImageViewIdArray views
		, uint32_t binding )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/OStr";
		auto result = addOwnAttach( std::move( views )
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Output )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Storage )
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, ClearValue{}
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eGeneral
			, nullptr );
		m_outputs.try_emplace( binding, result );
		return result;
	}

	Attachment const * FramePass::addClearableOutputStorageImage( ImageViewIdArray views
		, uint32_t binding
		, ClearValue clearValue )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/COStr";
		auto result = addOwnAttach( std::move( views )
			, std::move( attachName )
			, ( Attachment::FlagKind( Attachment::Flag::Output ) | Attachment::FlagKind( Attachment::Flag::Clearable ) )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Storage )
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, std::move( clearValue )
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eGeneral
			, nullptr );
		m_outputs.try_emplace( binding, result );
		return result;
	}

	void FramePass::addInputTransferBuffer( BufferViewIdArray views )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/ITrf";
		auto attach = addOwnAttach( std::move( views )
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Input )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Transfer )
			, AccessState{}
			, nullptr );
		m_inputs.try_emplace( TransferOffset + uint32_t( m_inputs.size() ), attach );
	}

	void FramePass::addInputTransferImage( ImageViewIdArray views )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/ITrf";
		auto attach = addOwnAttach( std::move( views )
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Input )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Transfer )
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, ClearValue{}
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eTransferSrc
			, nullptr );
		m_inputs.try_emplace( TransferOffset + uint32_t( m_inputs.size() ), attach );
	}

	void FramePass::addInputTransfer( Attachment const & attachment )
	{
		if ( attachment.isImage() )
		{
			auto attachName = fpass::adjustName( *this, attachment.view().data->name ) + "/ITrf";
			auto attach = addOwnAttach( attachment.imageAttach.views
				, std::move( attachName )
				, Attachment::FlagKind( Attachment::Flag::Input )
				, ImageAttachment::FlagKind( ImageAttachment::Flag::Transfer )
				, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
				, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
				, ClearValue{}
				, PipelineColorBlendAttachmentState{}
				, ImageLayout::eTransferSrc
				, & attachment );
			m_inputs.try_emplace( TransferOffset + uint32_t( m_inputs.size() ), attach );
		}
		else
		{
			auto attachName = fpass::adjustName( *this, attachment.buffer().data->name ) + "/ITrf";
			auto attach = addOwnAttach( attachment.bufferAttach.buffers
				, std::move( attachName )
				, Attachment::FlagKind( Attachment::Flag::Input )
				, BufferAttachment::FlagKind( BufferAttachment::Flag::Transfer )
				, AccessState{}
				, &attachment );
			m_inputs.try_emplace( TransferOffset + uint32_t( m_inputs.size() ), attach );
		}
	}

	Attachment const * FramePass::addInOutTransfer( Attachment const & attachment
			, Attachment::Flag flag )
	{
		Attachment const * result{};

		if ( attachment.isImage() )
		{
			auto attachName = fpass::adjustName( *this, attachment.view().data->name ) + "/IOTrf";
			result = addOwnAttach( attachment.imageAttach.views
				, std::move( attachName )
				, Attachment::FlagKind( Attachment::FlagKind( Attachment::Flag::InOut ) | Attachment::FlagKind( flag ) )
				, ImageAttachment::FlagKind( ImageAttachment::Flag::Transfer )
				, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
				, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
				, ClearValue{}
				, PipelineColorBlendAttachmentState{}
				, ImageLayout::eTransferSrc
				, &attachment );
			m_inouts.try_emplace( TransferOffset + uint32_t( m_inouts.size() ), result );
		}
		else
		{
			auto attachName = fpass::adjustName( *this, attachment.buffer().data->name ) + "/IOTrf";
			result = addOwnAttach( attachment.bufferAttach.buffers
				, std::move( attachName )
				, Attachment::FlagKind( Attachment::FlagKind( Attachment::Flag::InOut ) | Attachment::FlagKind( flag ) )
				, BufferAttachment::FlagKind( BufferAttachment::Flag::Transfer )
				, AccessState{}
			, &attachment );
			m_inouts.try_emplace( TransferOffset + uint32_t( m_inouts.size() ), result );
		}

		return result;
	}

	Attachment const * FramePass::addOutputTransferBuffer( BufferViewIdArray buffers )
	{
		auto attachName = fpass::adjustName( *this, buffers.front().data->name ) + "/OTB";
		auto result = addOwnAttach( std::move( buffers )
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Output )
			, BufferAttachment::FlagKind( BufferAttachment::Flag::Transfer )
			, AccessState{}
		, nullptr );
		m_outputs.try_emplace( TransferOffset + uint32_t( m_outputs.size() ), result );
		return result;
	}

	Attachment const * FramePass::addOutputTransferImage( ImageViewIdArray views )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/OT";
		auto result = addOwnAttach( std::move( views )
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Output )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Transfer )
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, ClearValue{}
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eTransferDst
			, nullptr );
		m_outputs.try_emplace( TransferOffset + uint32_t( m_outputs.size() ), result );
		return result;
	}

	void FramePass::addInputColourTargetImage( ImageViewIdArray views )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/IRcl";
		auto result = addOwnAttach( std::move( views )
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Input )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::None )
			, AttachmentLoadOp::eLoad, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, ClearValue{ ClearColorValue{} }
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eColorAttachment
			, nullptr );
		m_targets.emplace_back( result );
	}

	void FramePass::addInputDepthTargetImage( ImageViewIdArray views )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/IRdp";
		auto result = addOwnAttach( std::move( views )
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Input )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Depth )
			, AttachmentLoadOp::eLoad, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, ClearValue{ ClearDepthStencilValue{} }
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eDepthStencilAttachment
			, nullptr );
		m_targets.emplace_back( result );
	}

	void FramePass::addInputStencilTargetImage( ImageViewIdArray views )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/IRst";
		auto result = addOwnAttach( std::move( views )
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Input )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::StencilInput ) | ImageAttachment::FlagKind( ImageAttachment::Flag::Stencil )
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eLoad, AttachmentStoreOp::eDontCare
			, ClearValue{ ClearDepthStencilValue{} }
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eDepthStencilAttachment
			, nullptr );
		m_targets.emplace_back( result );
	}

	void FramePass::addInputDepthStencilTargetImage( ImageViewIdArray views )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/IRds";
		auto result = addOwnAttach( std::move( views )
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Input )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::StencilInput ) | ImageAttachment::FlagKind( ImageAttachment::Flag::DepthStencil )
			, AttachmentLoadOp::eLoad, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eLoad, AttachmentStoreOp::eDontCare
			, ClearValue{ ClearDepthStencilValue{} }
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eDepthStencilAttachment
			, nullptr );
		m_targets.emplace_back( result );
	}

	void FramePass::addInputColourTarget( Attachment const & attachment )
	{
		auto attachName = fpass::adjustName( *this, attachment.view().data->name ) + "/IRcl";
		auto result = addOwnAttach( attachment.imageAttach.views
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Input )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::None )
			, AttachmentLoadOp::eLoad, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, ClearValue{ ClearColorValue{} }
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eColorAttachment
			, &attachment );
		m_targets.emplace_back( result );
	}

	void FramePass::addInputDepthTarget( Attachment const & attachment )
	{
		auto attachName = fpass::adjustName( *this, attachment.view().data->name ) + "/IRdp";
		auto attach = addOwnAttach( attachment.imageAttach.views
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Input )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Depth )
			, AttachmentLoadOp::eLoad, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, ClearValue{ ClearDepthStencilValue{} }
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eDepthStencilAttachment
			, &attachment );
		m_targets.emplace_back( attach );
	}

	void FramePass::addInputStencilTarget( Attachment const & attachment )
	{
		auto attachName = fpass::adjustName( *this, attachment.view().data->name ) + "/IRst";
		auto attach = addOwnAttach( attachment.imageAttach.views
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Input )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::StencilInput ) | ImageAttachment::FlagKind( ImageAttachment::Flag::Stencil )
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eLoad, AttachmentStoreOp::eDontCare
			, ClearValue{ ClearDepthStencilValue{} }
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eDepthStencilAttachment
			, &attachment );
		m_targets.emplace_back( attach );
	}

	void FramePass::addInputDepthStencilTarget( Attachment const & attachment )
	{
		auto attachName = fpass::adjustName( *this, attachment.view().data->name ) + "/IRds";
		auto attach = addOwnAttach( attachment.imageAttach.views
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Input )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::StencilInput ) | ImageAttachment::FlagKind( ImageAttachment::Flag::DepthStencil )
			, AttachmentLoadOp::eLoad, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eLoad, AttachmentStoreOp::eDontCare
			, ClearValue{ ClearDepthStencilValue{} }
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eDepthStencilAttachment
			, &attachment );
		m_targets.emplace_back( attach );
	}

	Attachment const * FramePass::addInOutColourTarget( Attachment const & attachment
		, PipelineColorBlendAttachmentState blendState )
	{
		auto attachName = fpass::adjustName( *this, attachment.view().data->name ) + "/IORcl";
		auto result = addOwnAttach( attachment.imageAttach.views
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::InOut )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::None )
			, AttachmentLoadOp::eLoad, AttachmentStoreOp::eStore
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, ClearValue{ ClearColorValue{} }
			, std::move( blendState )
			, ImageLayout::eColorAttachment
			, &attachment );
		m_targets.emplace_back( result );
		return result;
	}

	Attachment const * FramePass::addInOutDepthTarget( Attachment const & attachment )
	{
		auto attachName = fpass::adjustName( *this, attachment.view().data->name ) + "/IORdp";
		auto result = addOwnAttach( attachment.imageAttach.views
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::InOut )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Depth )
			, AttachmentLoadOp::eLoad, AttachmentStoreOp::eStore
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, ClearValue{ ClearDepthStencilValue{} }
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eDepthStencilAttachment
			, &attachment );
		m_targets.emplace_back( result );
		return result;
	}

	Attachment const * FramePass::addInOutStencilTarget( Attachment const & attachment )
	{
		auto attachName = fpass::adjustName( *this, attachment.view().data->name ) + "/IORst";
		auto result = addOwnAttach( attachment.imageAttach.views
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::InOut )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::StencilInOut ) | ImageAttachment::FlagKind( ImageAttachment::Flag::Stencil )
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eLoad, AttachmentStoreOp::eStore
			, ClearValue{ ClearDepthStencilValue{} }
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eDepthStencilAttachment
			, &attachment );
		m_targets.emplace_back( result );
		return result;
	}

	Attachment const * FramePass::addInOutDepthStencilTarget( Attachment const & attachment )
	{
		auto attachName = fpass::adjustName( *this, attachment.view().data->name ) + "/IORds";
		auto result = addOwnAttach( attachment.imageAttach.views
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::InOut )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::StencilInOut ) | ImageAttachment::FlagKind( ImageAttachment::Flag::DepthStencil )
			, AttachmentLoadOp::eLoad, AttachmentStoreOp::eStore
			, AttachmentLoadOp::eLoad, AttachmentStoreOp::eStore
			, ClearValue{ ClearDepthStencilValue{} }
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eDepthStencilAttachment
			, &attachment );
		m_targets.emplace_back( result );
		return result;
	}

	Attachment const * FramePass::addOutputColourTarget( ImageViewIdArray views
		, ClearColorValue clearValue )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/ORcl";
		auto result = addOwnAttach( std::move( views )
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Output )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::None )
			, AttachmentLoadOp::eClear, AttachmentStoreOp::eStore
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, ClearValue{ std::move( clearValue ) }
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eColorAttachment
			, nullptr );
		m_targets.emplace_back( result );
		return result;
	}

	Attachment const * FramePass::addOutputDepthTarget( ImageViewIdArray views
		, ClearDepthStencilValue clearValue )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/ORdp";
		auto result = addOwnAttach( std::move( views )
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Output )
			, ImageAttachment::FlagKind( ImageAttachment::Flag::Depth )
			, AttachmentLoadOp::eClear, AttachmentStoreOp::eStore
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, ClearValue{ std::move( clearValue ) }
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eDepthAttachment
			, nullptr );
		m_targets.emplace( m_targets.begin(), result );
		return result;
	}

	Attachment const * FramePass::addOutputStencilTarget( ImageViewIdArray views
		, ClearDepthStencilValue clearValue )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/ORst";
		auto result = addOwnAttach( std::move( views )
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Output )
			, ImageAttachment::FlagKind( ImageAttachment::FlagKind( ImageAttachment::Flag::StencilOutput ) | ImageAttachment::FlagKind( ImageAttachment::Flag::Stencil ) )
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eClear, AttachmentStoreOp::eStore
			, ClearValue{ std::move( clearValue ) }
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eStencilAttachment
			, nullptr );
		m_targets.emplace( m_targets.begin(), result );
		return result;
	}

	Attachment const * FramePass::addOutputDepthStencilTarget( ImageViewIdArray views
		, ClearDepthStencilValue clearValue )
	{
		auto attachName = fpass::adjustName( *this, views.front().data->name ) + "/ORds";
		auto result = addOwnAttach( std::move( views )
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Output )
			, ImageAttachment::FlagKind( ImageAttachment::FlagKind( ImageAttachment::Flag::StencilOutput ) | ImageAttachment::FlagKind( ImageAttachment::Flag::DepthStencil ) )
			, AttachmentLoadOp::eClear, AttachmentStoreOp::eStore
			, AttachmentLoadOp::eClear, AttachmentStoreOp::eStore
			, ClearValue{ std::move( clearValue ) }
			, PipelineColorBlendAttachmentState{}
			, ImageLayout::eDepthStencilAttachment
			, nullptr );
		m_targets.emplace( m_targets.begin(), result );
		return result;
	}

	void FramePass::addImplicit( Attachment const & attachment
		, AccessState wantedAccess )
	{
		auto attachName = fpass::adjustName( *this, attachment.buffer().data->name ) + "/Impl";
		auto attach = addOwnAttach( attachment.bufferAttach.buffers
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Input )
			, BufferAttachment::FlagKind( BufferAttachment::FlagKind( BufferAttachment::Flag::Transition ) | attachment.bufferAttach.getFormatFlags() )
			, std::move( wantedAccess )
			, &attachment );
		m_inputs.try_emplace( ImplicitOffset + uint32_t( m_inputs.size() ), attach );
	}

	void FramePass::addImplicit( Attachment const & attachment
		, ImageLayout wantedLayout )
	{
		auto attachName = fpass::adjustName( *this, attachment.view().data->name ) + "/Impl";
		auto attach = addOwnAttach( attachment.imageAttach.views
			, std::move( attachName )
			, Attachment::FlagKind( Attachment::Flag::Input )
			, ImageAttachment::FlagKind( ImageAttachment::FlagKind( ImageAttachment::Flag::Transition ) | attachment.imageAttach.getFormatFlags() )
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare
			, ClearValue{}
			, PipelineColorBlendAttachmentState{}
			, wantedLayout
			, &attachment );
		m_inputs.try_emplace( ImplicitOffset + uint32_t( m_inputs.size() ), attach );
	}

	RunnablePassPtr FramePass::createRunnable( GraphContext & context
		, RunnableGraph & pgraph )const
	{
		return m_runnableCreator( *this, context, pgraph );
	}

	std::string FramePass::getFullName()const
	{
		return m_group.getFullName() + "/" + getName();
	}

	std::string FramePass::getGroupName()const
	{
		return m_group.getName() + "/" + getName();
	}

	Attachment const * FramePass::addOwnAttach( ImageViewIdArray views, std::string attachName
		, Attachment::FlagKind flags, ImageAttachment::FlagKind imageFlags
		, AttachmentLoadOp loadOp, AttachmentStoreOp storeOp
		, AttachmentLoadOp stencilLoadOp, AttachmentStoreOp stencilStoreOp
		, ClearValue clearValue, PipelineColorBlendAttachmentState blendState
		, ImageLayout wantedLayout
		, Attachment const * parent )
	{
		if ( views.front().data->source.empty() )
			return addOwnAttach( new Attachment{ flags
					, *this, std::move( attachName )
					, imageFlags
					, std::move( views )
					, loadOp, storeOp
					, stencilLoadOp, stencilStoreOp
					, std::move( clearValue )
					, std::move( blendState )
					, wantedLayout
					, Attachment::Token{} }
				, parent );

		// Dispatch merged views sources in source attachs views
		std::vector< ImageViewIdArray > sourceAttachsViews;
		sourceAttachsViews.resize( views.front().data->source.size() );
		for ( auto view : views )
		{
			for ( uint32_t i = 0u; i < view.data->source.size(); ++i )
				sourceAttachsViews[i].push_back( view.data->source[i] );
		}

		// Use these views to create attachs
		uint32_t index{};
		std::vector< AttachmentPtr > sources;
		for ( auto & sourceViews : sourceAttachsViews )
		{
			sources.push_back( std::make_unique< Attachment >( flags
				, *this, attachName + std::to_string( index )
				, imageFlags
				, std::move( sourceViews )
				, loadOp, storeOp
				, stencilLoadOp, stencilStoreOp
				, clearValue, blendState
				, wantedLayout
				, Attachment::Token{} ) );
			++index;
		}

		// Create the resulting attach
		auto result = addOwnAttach( new Attachment{ flags
				, *this, std::move( attachName )
				, imageFlags
				, std::move( views )
				, loadOp, storeOp
				, stencilLoadOp, stencilStoreOp
				, std::move( clearValue )
				, std::move( blendState )
				, wantedLayout
				, Attachment::Token{} }
			, parent );
		result->pass = nullptr;

		// And set its sources
		if ( !parent || parent->source.empty() )
		{
			for ( auto & sourceAttach : sources )
				result->source.emplace_back( std::move( sourceAttach ) );
		}
		else
		{
			// If parent has sources, link the new attachment sources to the parent ones
			assert( parent->source.size() == sources.size() );
			for ( uint32_t i = 0; i < sources.size(); ++i )
			{
				auto source = addOwnAttach( sources[i].release()
					, ( parent->source[i].parent
						? parent->source[i].parent
						: parent->source[i].attach.get() ) );
				result->source.emplace_back( parent->source[i].attach.get(), source->pass, source->imageAttach );
			}

			result->initSources();
		}

		return result;
	}

	Attachment const * FramePass::addOwnAttach( BufferViewIdArray views, std::string attachName
		, Attachment::FlagKind flags, BufferAttachment::FlagKind bufferFlags
		, AccessState access
		, Attachment const * parent )
	{
		if ( views.front().data->source.empty() )
			return addOwnAttach( new Attachment{ flags
					, *this, std::move( attachName )
					, bufferFlags
					, std::move( views )
					, std::move( access )
					, Attachment::Token{} }
				, parent );

		// Dispatch merged views sources in source attachs views
		std::vector< BufferViewIdArray > sourceAttachsViews;
		sourceAttachsViews.resize( views.front().data->source.size() );
		for ( auto view : views )
		{
			for ( uint32_t i = 0u; i < view.data->source.size(); ++i )
				sourceAttachsViews[i].push_back( view.data->source[i] );
		}

		// Use these views to create attachs
		uint32_t index{};
		std::vector< AttachmentPtr > sources;
		for ( auto & sourceViews : sourceAttachsViews )
		{
			sources.push_back( std::make_unique< Attachment >( flags
				, *this, attachName + std::to_string( index )
				, bufferFlags
				, std::move( sourceViews )
				, access
				, Attachment::Token{} ) );
			++index;
		}

		// Create the resulting attach
		auto result = addOwnAttach( new Attachment{ flags
				, *this, std::move( attachName )
				, bufferFlags
				, std::move( views )
				, std::move( access )
				, Attachment::Token{} }
			, parent );
		result->pass = nullptr;

		// And set its sources
		if ( !parent || parent->source.empty() )
		{
			for ( auto & sourceAttach : sources )
				result->source.emplace_back( std::move( sourceAttach ) );
		}
		else
		{
			// If parent has sources, link the new attachment sources to the parent ones
			assert( parent->source.size() == sources.size() );
			for ( uint32_t i = 0; i < sources.size(); ++i )
			{
				auto source = addOwnAttach( sources[i].release()
					, ( parent->source[i].parent
						? parent->source[i].parent
						: parent->source[i].attach.get() ) );
				result->source.emplace_back( parent->source[i].attach.get(), source->pass, source->bufferAttach );
			}

			result->initSources();
		}

		return result;
	}

	Attachment * FramePass::addOwnAttach( Attachment * mine
		, Attachment const * parent )
	{
		auto & own = m_ownAttaches.try_emplace( mine ).first->second;
		own.mine.reset( mine );
		own.parent = parent;
		return mine;
	}
}
