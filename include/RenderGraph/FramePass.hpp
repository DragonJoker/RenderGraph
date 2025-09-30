/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#pragma once

#include "RenderGraph/Attachment.hpp"
#include "RenderGraph/WriteDescriptorSet.hpp"

#include <functional>
#include <optional>
#include <unordered_map>

namespace crg
{
	using RunnablePassCreator = std::function< RunnablePassPtr( FramePass const &
		, GraphContext &
		, RunnableGraph & ) >;

	struct FramePass
	{
	public:
		struct SampledAttachment
		{
			SampledAttachment( Attachment const * attach, SamplerDesc sampler )noexcept
				: attach{ attach }
				, sampler{ std::move( sampler ) }
			{
			}

			Attachment const * attach;
			SamplerDesc sampler;
		};

	public:
		/**
		*\name
		*	Dependencies.
		*/
		/**@{*/
		/**
		*\brief
		*	Gets the attachment parent from the givent one.
		*\param[in] attach
		*	The child attachment.
		*/
		CRG_API Attachment const * getParentAttachment( Attachment const & attach )const;
		/**@}*/
#pragma region Attachments
		/**
		*\name
		*	Attachments
		*/
		/**@{*/
#	pragma region Uniform
		/**
		*\name
		*	Uniform
		*/
		/**@{*/
		/**
		*\brief
		*	Creates a uniform buffer multi-pass attachment.
		*/
		CRG_API void addInputUniformBuffer( BufferViewIdArray buffers
			, uint32_t binding );
		/**
		*\brief
		*	Creates a uniform buffer single-pass attachment.
		*/
		void addInputUniformBuffer( BufferViewId buffer
			, uint32_t binding )
		{
			addInputUniformBuffer( BufferViewIdArray{ buffer }, binding );
		}
		/**
		*\brief
		*	Creates a sampled image multi-pass attachment.
		*/
		CRG_API void addInputSampledImage( ImageViewIdArray views
			, uint32_t binding
			, SamplerDesc samplerDesc = SamplerDesc{} );
		/**
		*\brief
		*	Creates a sampled image single-pass attachment.
		*/
		void addInputSampledImage( ImageViewId view
			, uint32_t binding
			, SamplerDesc samplerDesc = SamplerDesc{} )
		{
			addInputSampledImage( ImageViewIdArray{ view }, binding, std::move( samplerDesc ) );
		}
		/**
		*\brief
		*	Creates an input uniform attachment.
		*/
		CRG_API void addInputUniform( Attachment const & attach
			, uint32_t binding );
		/**
		*\brief
		*	Creates a sampled image attachment.
		*/
		CRG_API void addInputSampled( Attachment const & attach
			, uint32_t binding
			, SamplerDesc samplerDesc = SamplerDesc{} );
		/**@}*/
#	pragma endregion
#	pragma region Storage
		/**
		*\name
		*	Storage
		*/
		/**@{*/
		/**
		*\brief
		*	Creates a storage buffer multi-pass attachment.
		*/
		CRG_API void addInputStorageBuffer( BufferViewIdArray buffers
			, uint32_t binding );
		/**
		*\brief
		*	Creates a storage buffer single-pass attachment.
		*/
		void addInputStorageBuffer( BufferViewId buffer
			, uint32_t binding )
		{
			addInputStorageBuffer( BufferViewIdArray{ buffer }, binding );
		}
		/**
		*\brief
		*	Creates an input storage attachment.
		*/
		CRG_API void addInputStorageImage( ImageViewIdArray views
			, uint32_t binding );
		/**
		*\brief
		*	Creates an input storage attachment.
		*/
		void addInputStorageImage( ImageViewId view
			, uint32_t binding )
		{
			addInputStorageImage( ImageViewIdArray{ view }, binding );
		}
		/**
		*\brief
		*	Creates an input storage attachment.
		*/
		CRG_API void addInputStorage( Attachment const & attach
			, uint32_t binding );
		/**
		*\brief
		*	Creates an input/output storage attachment.
		*/
		CRG_API Attachment const * addInOutStorage( Attachment const & attach
			, uint32_t binding );
		/**
		*\brief
		*	Creates an output storage buffer multi-pass attachment.
		*/
		CRG_API Attachment const * addOutputStorageBuffer( BufferViewIdArray buffers
			, uint32_t binding );
		/**
		*\brief
		*	Creates an output storage buffer single-pass attachment.
		*/
		Attachment const * addOutputStorageBuffer( BufferViewId buffer
			, uint32_t binding )
		{
			return addOutputStorageBuffer( BufferViewIdArray{ buffer }, binding );
		}
		/**
		*\brief
		*	Creates a storage buffer multi-pass attachment that will be cleared a the beginning of the pass.
		*/
		CRG_API Attachment const * addClearableOutputStorageBuffer( BufferViewIdArray buffers
			, uint32_t binding );
		/**
		*\brief
		*	Creates a storage buffer single-pass attachment that will be cleared a the beginning of the pass.
		*/
		Attachment const * addClearableOutputStorageBuffer( BufferViewId buffer
			, uint32_t binding )
		{
			return addClearableOutputStorageBuffer( BufferViewIdArray{ buffer }, binding );
		}
		/**
		*\brief
		*	Creates a storage image multi-pass attachment.
		*/
		CRG_API Attachment const * addOutputStorageImage( ImageViewIdArray view
			, uint32_t binding );
		/**
		*\brief
		*	Creates a storage image single-pass attachment.
		*/
		Attachment const * addOutputStorageImage( ImageViewId view
			, uint32_t binding )
		{
			return addOutputStorageImage( ImageViewIdArray{ view }
			, binding );
		}
		/**
		*\brief
		*	Creates a storage image multi-pass attachment.
		*/
		CRG_API Attachment const * addClearableOutputStorageImage( ImageViewIdArray view
			, uint32_t binding
			, ClearValue clearValue = ClearValue{} );
		/**
		*\brief
		*	Creates a storage image single-pass attachment.
		*/
		Attachment const * addClearableOutputStorageImage( ImageViewId view
			, uint32_t binding
			, ClearValue clearValue = ClearValue{} )
		{
			return addClearableOutputStorageImage( ImageViewIdArray{ view }
				, binding
				, std::move( clearValue ) );
		}
		/**@}*/
#	pragma endregion
#	pragma region Transfer
		/**
		*\name
		*	Transfer
		*/
		/**@{*/
		/**
		*\brief
		*	Creates a transfer input external buffer.
		*/
		CRG_API void addInputTransferBuffer( BufferViewIdArray views );
		/**
		*\brief
		*	Creates a transfer input external buffer.
		*/
		void addInputTransferBuffer( BufferViewId view )
		{
			addInputTransferBuffer( BufferViewIdArray{ view } );
		}
		/**
		*\brief
		*	Creates a transfer input external image.
		*/
		CRG_API void addInputTransferImage( ImageViewIdArray views );
		/**
		*\brief
		*	Creates a transfer input external image.
		*/
		void addInputTransferImage( ImageViewId view )
		{
			addInputTransferImage( ImageViewIdArray{ view } );
		}
		/**
		*\brief
		*	Creates a transfer input attachment.
		*/
		CRG_API void addInputTransfer( Attachment const & attach );
		/**
		*\brief
		*	Creates a transfer input/output attachment.
		*/
		CRG_API Attachment const * addInOutTransfer( Attachment const & attach
			, Attachment::Flag flag = {} );
		/**
		*\brief
		*	Creates a transfer output buffer multi-pass attachment.
		*/
		CRG_API Attachment const * addOutputTransferBuffer( BufferViewIdArray buffers );
		/**
		*\brief
		*	Creates a transfer output buffer single-pass attachment.
		*/
		Attachment const * addOutputTransferBuffer( BufferViewId buffer )
		{
			return addOutputTransferBuffer( BufferViewIdArray{ buffer } );
		}
		/**
		*\brief
		*	Creates a transfer output multi-pass attachment.
		*/
		CRG_API Attachment const * addOutputTransferImage( ImageViewIdArray view );
		/**
		*\brief
		*	Creates an transfer output single-pass attachment.
		*/
		Attachment const * addOutputTransferImage( ImageViewId view )
		{
			return addOutputTransferImage( ImageViewIdArray{ view } );
		}
		/**@}*/
#	pragma endregion
#	pragma region Target
		/**
		*\name
		*	Target
		*/
		/**@{*/
		/**
		*\brief
		*	Creates an input colour attachment.
		*/
		CRG_API void addInputColourTargetImage( ImageViewIdArray views );
		/**
		*\brief
		*	Creates an input colour attachment.
		*/
		void addInputColourTargetImage( ImageViewId view )
		{
			return addInputColourTargetImage( ImageViewIdArray{ view } );
		}
		/**
		*\brief
		*	Creates an input depth attachment.
		*/
		CRG_API void addInputDepthTargetImage( ImageViewIdArray views );
		/**
		*\brief
		*	Creates an input depth attachment.
		*/
		void addInputDepthTargetImage( ImageViewId view )
		{
			return addInputDepthTargetImage( ImageViewIdArray{ view } );
		}
		/**
		*\brief
		*	Creates an input stencil attachment.
		*/
		CRG_API void addInputStencilTargetImage( ImageViewIdArray views );
		/**
		*\brief
		*	Creates an input stencil attachment.
		*/
		void addInputStencilTargetImage( ImageViewId view )
		{
			return addInputStencilTargetImage( ImageViewIdArray{ view } );
		}
		/**
		*\brief
		*	Creates an input depth and stencil attachment.
		*/
		CRG_API void addInputDepthStencilTargetImage( ImageViewIdArray views );
		/**
		*\brief
		*	Creates an input depth and stencil attachment.
		*/
		void addInputDepthStencilTargetImage( ImageViewId view )
		{
			return addInputDepthStencilTargetImage( ImageViewIdArray{ view } );
		}
		/**
		*\brief
		*	Creates an input colour attachment.
		*/
		CRG_API void addInputColourTarget( Attachment const & attach );
		/**
		*\brief
		*	Creates an input depth attachment.
		*/
		CRG_API void addInputDepthTarget( Attachment const & attach );
		/**
		*\brief
		*	Creates an input stencil attachment.
		*/
		CRG_API void addInputStencilTarget( Attachment const & attach );
		/**
		*\brief
		*	Creates an input depth and stencil attachment.
		*/
		CRG_API void addInputDepthStencilTarget( Attachment const & attach );
		/**
		*\brief
		*	Creates an in/out colour attachment.
		*/
		CRG_API Attachment const * addInOutColourTarget( Attachment const & attach
			, PipelineColorBlendAttachmentState blendState = DefaultBlendState );
		/**
		*\brief
		*	Creates an in/out depth attachment.
		*/
		CRG_API Attachment const * addInOutDepthTarget( Attachment const & attach );
		/**
		*\brief
		*	Creates an in/out stencil attachment.
		*/
		CRG_API Attachment const * addInOutStencilTarget( Attachment const & attach );
		/**
		*\brief
		*	Creates an in/out depth and stencil attachment.
		*/
		CRG_API Attachment const * addInOutDepthStencilTarget( Attachment const & attach );
		/**
		*\brief
		*	Creates an output colour multi-pass attachment.
		*/
		CRG_API Attachment const * addOutputColourTarget( ImageViewIdArray views
			, ClearColorValue clearValue = ClearColorValue{} );
		/**
		*\brief
		*	Creates an output colour single-pass attachment.
		*/
		Attachment const * addOutputColourTarget( ImageViewId view
			, ClearColorValue clearValue = ClearColorValue{} )
		{
			return addOutputColourTarget( ImageViewIdArray{ view }
			, std::move( clearValue ) );
		}
		/**
		*\brief
		*	Creates an output depth multi-pass attachment.
		*/
		CRG_API Attachment const * addOutputDepthTarget( ImageViewIdArray views
			, ClearDepthStencilValue clearValue = ClearDepthStencilValue{} );
		/**
		*\brief
		*	Creates an output depth single-pass attachment.
		*/
		Attachment const * addOutputDepthTarget( ImageViewId view
			, ClearDepthStencilValue clearValue = ClearDepthStencilValue{} )
		{
			return addOutputDepthTarget( ImageViewIdArray{ view }
			, std::move( clearValue ) );
		}
		/**
		*\brief
		*	Creates an output stencil multi-pass attachment.
		*/
		CRG_API Attachment const * addOutputStencilTarget( ImageViewIdArray views
			, ClearDepthStencilValue clearValue = ClearDepthStencilValue{} );
		/**
		*\brief
		*	Creates an output stencil single-pass attachment.
		*/
		Attachment const * addOutputStencilTarget( ImageViewId view
			, ClearDepthStencilValue clearValue = ClearDepthStencilValue{} )
		{
			return addOutputStencilTarget( ImageViewIdArray{ view }
			, std::move( clearValue ) );
		}
		/**
		*\brief
		*	Creates an output depth and stencil multi-pass attachment.
		*/
		CRG_API Attachment const * addOutputDepthStencilTarget( ImageViewIdArray views
			, ClearDepthStencilValue clearValue = ClearDepthStencilValue{} );
		/**
		*\brief
		*	Creates an output depth and stencil single-pass attachment.
		*/
		Attachment const * addOutputDepthStencilTarget( ImageViewId view
			, ClearDepthStencilValue clearValue = ClearDepthStencilValue{} )
		{
			return addOutputDepthStencilTarget( ImageViewIdArray{ view }
			, std::move( clearValue ) );
		}
		/**@}*/
		/**@}*/
#	pragma endregion
#	pragma region Implicit
		/**
		*\name
		*	Implicit
		*/
		/**@{*/
		/**
		*\brief
		*	Creates an implicit attachment.
		*\remarks
		*	This attachment will only be used to compute dependencies, and is considered an input, in that goal.
		*/
		CRG_API void addImplicit( Attachment const & attach
			, AccessState wantedAccess );
		/**
		*\brief
		*	Creates an implicit attachment.
		*\remarks
		*	This attachment will only be used to compute dependencies, and is considered an input, in that goal.
		*/
		CRG_API void addImplicit( Attachment const & attach
			, ImageLayout wantedLayout );
		/**@}*/
#	pragma endregion
		/**@}*/
#pragma endregion
		/**
		*\name
		*	Graph compilation.
		*/
		/**@{*/
		CRG_API RunnablePassPtr createRunnable( GraphContext & context
			, RunnableGraph & graph )const;
		/**@}*/

		CRG_API std::string getFullName()const;
		CRG_API std::string getGroupName()const;

		std::string const & getName()const
		{
			return m_name;
		}

		auto begin()const
		{
			return m_ownAttaches.cbegin();
		}

		auto end()const
		{
			return m_ownAttaches.cend();
		}

		FramePassGroup const & getGroup()const noexcept
		{
			return m_group;
		}

		FrameGraph const & getGraph()const noexcept
		{
			return m_graph;
		}

		uint32_t getId()const noexcept
		{
			return m_id;
		}

		std::map< uint32_t, Attachment const * > const & getUniforms()const noexcept
		{
			return m_uniforms;
		}

		std::map< uint32_t, SampledAttachment > const & getSampled()const noexcept
		{
			return m_sampled;
		}

		std::map< uint32_t, Attachment const * > const & getInputs()const noexcept
		{
			return m_inputs;
		}

		std::map< uint32_t, Attachment const * > const & getInouts()const noexcept
		{
			return m_inouts;
		}

		std::map< uint32_t, Attachment const * > const & getOutputs()const noexcept
		{
			return m_outputs;
		}

		std::vector< Attachment const * > const & getTargets()const noexcept
		{
			return m_targets;
		}

	protected:
		friend struct FramePassGroup;
		/**
		*\name
		*	Construction.
		*/
		/**@{*/
		CRG_API FramePass( FramePassGroup const & group
			, FrameGraph & graph
			, uint32_t id
			, std::string const & name
			, RunnablePassCreator runnableCreator );
		/**@}*/

	private:
		CRG_API Attachment const * addColourTarget( std::string const & name
			, Attachment::FlagKind flags
			, ImageViewIdArray views
			, AttachmentLoadOp loadOp
			, AttachmentStoreOp storeOp
			, ImageLayout wantedLayout = ImageLayout::eUndefined
			, ClearColorValue clearValue = ClearColorValue{}
			, PipelineColorBlendAttachmentState blendState = DefaultBlendState );
		CRG_API Attachment const * addDepthTarget( std::string const & name
			, Attachment::FlagKind flags
			, ImageViewIdArray views
			, AttachmentLoadOp loadOp
			, AttachmentStoreOp storeOp
			, ImageLayout wantedLayout = ImageLayout::eUndefined
			, ClearDepthStencilValue clearValue = ClearDepthStencilValue{} );
		CRG_API Attachment const * addStencilTarget( std::string const & name
			, Attachment::FlagKind flags
			, ImageAttachment::FlagKind stencilFlags
			, ImageViewIdArray views
			, AttachmentLoadOp stencilLoadOp
			, AttachmentStoreOp stencilStoreOp
			, ImageLayout wantedLayout = ImageLayout::eUndefined
			, ClearDepthStencilValue clearValue = ClearDepthStencilValue{} );
		CRG_API Attachment const * addDepthStencilTarget( std::string const & name
			, Attachment::FlagKind flags
			, ImageAttachment::FlagKind stencilFlags
			, ImageViewIdArray views
			, AttachmentLoadOp loadOp
			, AttachmentStoreOp storeOp
			, AttachmentLoadOp stencilLoadOp
			, AttachmentStoreOp stencilStoreOp
			, ImageLayout wantedLayout = ImageLayout::eUndefined
			, ClearDepthStencilValue clearValue = ClearDepthStencilValue{} );

		Attachment const * addOwnAttach( ImageViewIdArray views
			, std::string attachName
			, Attachment::FlagKind flags
			, ImageAttachment::FlagKind imageFlags
			, AttachmentLoadOp loadOp
			, AttachmentStoreOp storeOp
			, AttachmentLoadOp stencilLoadOp
			, AttachmentStoreOp stencilStoreOp
			, ClearValue clearValue
			, PipelineColorBlendAttachmentState blendState
			, ImageLayout wantedLayout
			, Attachment const * parent );
		Attachment const * addOwnAttach( BufferViewIdArray views
			, std::string attachName
			, Attachment::FlagKind flags
			, BufferAttachment::FlagKind bufferFlags
			, AccessState access
			, Attachment const * parent );
		Attachment * addOwnAttach( Attachment * mine
			, Attachment const * parent );

	private:
		FramePassGroup const & m_group;
		FrameGraph & m_graph;
		uint32_t m_id;
		std::map< uint32_t, Attachment const * > m_uniforms;
		std::map< uint32_t, SampledAttachment > m_sampled;
		std::map< uint32_t, Attachment const * > m_inputs;
		std::map< uint32_t, Attachment const * > m_inouts;
		std::map< uint32_t, Attachment const * > m_outputs;
		std::vector< Attachment const * > m_targets;
		RunnablePassCreator m_runnableCreator;
		std::string m_name;
		struct OwnAttachment
		{
			AttachmentPtr mine{};
			Attachment const * parent{};
		};
		std::unordered_map< Attachment const *, OwnAttachment > m_ownAttaches;
	};
}
