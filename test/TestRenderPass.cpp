#include "Window.hpp"

#include <RenderGraph/Attachment.hpp>
#include <RenderGraph/RenderPass.hpp>

#include <RenderPass/AttachmentDescription.hpp>

namespace
{
	ashes::TexturePtr createImage( test::AppInstance const & inst
		, ashes::Format format )
	{
		ashes::ImageCreateInfo imageCreate{};
		imageCreate.extent = { inst.width, inst.height, 1u };
		imageCreate.format = format;
		imageCreate.imageType = ashes::TextureType::e2D;
		imageCreate.usage = ashes::ImageUsageFlag::eColourAttachment
			| ashes::ImageUsageFlag::eSampled;
		return inst.device->createTexture( imageCreate, ashes::MemoryPropertyFlag::eDeviceLocal );
	}

	ashes::TextureViewPtr createView( ashes::Texture const & image )
	{
		ashes::ImageViewCreateInfo viewCreate{};
		viewCreate.format = image.getFormat();
		viewCreate.viewType = ashes::TextureViewType::e2D;
		viewCreate.subresourceRange.aspectMask = getAspectMask( viewCreate.format );
		return image.createView( viewCreate );
	}

	void testRenderPass_1C( test::AppInstance & inst )
	{
		testBegin( "testRenderPass_1C" );
		auto rt = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv = createView( *rt );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv );

		crg::RenderPass pass
		{
			"1C",
			{},
			{ rtAttach },
		};

		check( pass.name == "1C" );
		check( pass.inputs.empty() );
		check( pass.colourOutputs.size() == 1u );
		check( pass.colourOutputs[0] == rtAttach );
		check( pass.depthStencilOutput == std::nullopt );
		testEnd();
	}

	void testRenderPass_2C( test::AppInstance & inst )
	{
		testBegin( "testRenderPass_2C" );
		auto rt1 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv1 = createView( *rt1 );
		auto rtAttach1 = crg::Attachment::createColour( "RT1"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv1 );

		auto rt2 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv2 = createView( *rt2 );
		auto rtAttach2 = crg::Attachment::createColour( "RT2"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv2 );

		crg::RenderPass pass
		{
			"2C",
			{},
			{ rtAttach1, rtAttach2 },
		};

		check( pass.name == "2C" );
		check( pass.inputs.empty() );
		check( pass.colourOutputs.size() == 2u );
		check( pass.colourOutputs[0] == rtAttach1 );
		check( pass.colourOutputs[1] == rtAttach2 );
		check( pass.depthStencilOutput == std::nullopt );
		testEnd();
	}

	void testRenderPass_0C_1I( test::AppInstance & inst )
	{
		testBegin( "testRenderPass_0C_1I" );
		auto in = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto inv = createView( *in );
		auto inAttach = crg::Attachment::createInput( "IN"
			, *inv );

		crg::RenderPass pass
		{
			"0C_1I",
			{ inAttach },
			{},
		};

		check( pass.name == "0C_1I" );
		check( pass.inputs.size() == 1u );
		check( pass.inputs[0] == inAttach );
		check( pass.colourOutputs.empty() );
		check( pass.depthStencilOutput == std::nullopt );
		testEnd();
	}

	void testRenderPass_0C_2I( test::AppInstance & inst )
	{
		testBegin( "testRenderPass_0C_2I" );
		auto in1 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto inv1 = createView( *in1 );
		auto inAttach1 = crg::Attachment::createInput( "IN1"
			, *inv1 );

		auto in2 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto inv2 = createView( *in2 );
		auto inAttach2 = crg::Attachment::createInput( "IN2"
			, *inv2 );

		crg::RenderPass pass
		{
			"0C_2I",
			{ inAttach1, inAttach2 },
			{},
		};

		check( pass.name == "0C_2I" );
		check( pass.inputs.size() == 2u );
		check( pass.inputs[0] == inAttach1 );
		check( pass.inputs[1] == inAttach2 );
		check( pass.colourOutputs.empty() );
		check( pass.depthStencilOutput == std::nullopt );
		testEnd();
	}

	void testRenderPass_1C_1I( test::AppInstance & inst )
	{
		testBegin( "testRenderPass_1C_1I" );
		auto rt = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv = createView( *rt );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv );

		auto in = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto inv = createView( *in );
		auto inAttach = crg::Attachment::createInput( "IN"
			, *inv );

		crg::RenderPass pass
		{
			"1C_1I",
			{ inAttach },
			{ rtAttach },
		};

		check( pass.name == "1C_1I" );
		check( pass.inputs.size() == 1u );
		check( pass.inputs[0] == inAttach );
		check( pass.colourOutputs.size() == 1u );
		check( pass.colourOutputs[0] == rtAttach );
		check( pass.depthStencilOutput == std::nullopt );
		testEnd();
	}

	void testRenderPass_1C_2I( test::AppInstance & inst )
	{
		testBegin( "testRenderPass_1C_2I" );
		auto rt = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv = createView( *rt );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv );

		auto in1 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto inv1 = createView( *in1 );
		auto inAttach1 = crg::Attachment::createInput( "IN1"
			, *inv1 );

		auto in2 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto inv2 = createView( *in2 );
		auto inAttach2 = crg::Attachment::createInput( "IN2"
			, *inv2 );

		crg::RenderPass pass
		{
			"1C_2I",
			{ inAttach1, inAttach2 },
			{ rtAttach },
		};

		check( pass.name == "1C_2I" );
		check( pass.inputs.size() == 2u );
		check( pass.inputs[0] == inAttach1 );
		check( pass.inputs[1] == inAttach2 );
		check( pass.colourOutputs.size() == 1u );
		check( pass.colourOutputs[0] == rtAttach );
		check( pass.depthStencilOutput == std::nullopt );
		testEnd();
	}

	void testRenderPass_2C_1I( test::AppInstance & inst )
	{
		testBegin( "testRenderPass_2C_1I" );
		auto rt1 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv1 = createView( *rt1 );
		auto rtAttach1 = crg::Attachment::createColour( "RT1"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv1 );

		auto rt2 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv2 = createView( *rt2 );
		auto rtAttach2 = crg::Attachment::createColour( "RT2"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv2 );

		auto in = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto inv = createView( *in );
		auto inAttach = crg::Attachment::createInput( "IN"
			, *inv );

		crg::RenderPass pass
		{
			"2C_1I",
			{ inAttach },
			{ rtAttach1, rtAttach2 },
		};

		check( pass.name == "2C_1I" );
		check( pass.inputs.size() == 1u );
		check( pass.inputs[0] == inAttach );
		check( pass.colourOutputs.size() == 2u );
		check( pass.colourOutputs[0] == rtAttach1 );
		check( pass.colourOutputs[1] == rtAttach2 );
		check( pass.depthStencilOutput == std::nullopt );
		testEnd();
	}

	void testRenderPass_2C_2I( test::AppInstance & inst )
	{
		testBegin( "testRenderPass_2C_2I" );
		auto rt1 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv1 = createView( *rt1 );
		auto rtAttach1 = crg::Attachment::createColour( "RT1"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv1 );

		auto rt2 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv2 = createView( *rt2 );
		auto rtAttach2 = crg::Attachment::createColour( "RT2"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv2 );

		auto in1 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto inv1 = createView( *in1 );
		auto inAttach1 = crg::Attachment::createInput( "IN1"
			, *inv1 );

		auto in2 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto inv2 = createView( *in2 );
		auto inAttach2 = crg::Attachment::createInput( "IN2"
			, *inv2 );

		crg::RenderPass pass
		{
			"2C_2I",
			{ inAttach1, inAttach2 },
			{ rtAttach1, rtAttach2 },
		};

		check( pass.name == "2C_2I" );
		check( pass.inputs.size() == 2u );
		check( pass.inputs[0] == inAttach1 );
		check( pass.inputs[1] == inAttach2 );
		check( pass.colourOutputs.size() == 2u );
		check( pass.colourOutputs[0] == rtAttach1 );
		check( pass.colourOutputs[1] == rtAttach2 );
		check( pass.depthStencilOutput == std::nullopt );
		testEnd();
	}
	
	void testRenderPass_0C_DS( test::AppInstance & inst )
	{
		testBegin( "testRenderPass_0C_DS" );
		auto ds = createImage( inst, ashes::Format::eD32_SFLOAT );
		auto dsv = createView( *ds );
		auto dsAttach = crg::Attachment::createDepthStencil( "DS"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *dsv );

		crg::RenderPass pass
		{
			"0C_DS",
			{},
			{},
			dsAttach,
		};

		check( pass.name == "0C_DS" );
		check( pass.inputs.empty() );
		check( pass.colourOutputs.empty() == 1u );
		check( pass.depthStencilOutput != std::nullopt );
		check( pass.depthStencilOutput.value() == dsAttach );
		testEnd();
	}

	void testRenderPass_1C_DS( test::AppInstance & inst )
	{
		testBegin( "testRenderPass_1C_DS" );
		auto rt = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv = createView( *rt );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv );

		auto ds = createImage( inst, ashes::Format::eD32_SFLOAT );
		auto dsv = createView( *ds );
		auto dsAttach = crg::Attachment::createDepthStencil( "DS"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *dsv );

		crg::RenderPass pass
		{
			"1C_DS",
			{},
			{ rtAttach },
			dsAttach,
		};

		check( pass.name == "1C_DS" );
		check( pass.inputs.empty() );
		check( pass.colourOutputs.size() == 1u );
		check( pass.colourOutputs[0] == rtAttach );
		check( pass.depthStencilOutput != std::nullopt );
		check( pass.depthStencilOutput.value() == dsAttach );
		testEnd();
	}

	void testRenderPass_2C_DS( test::AppInstance & inst )
	{
		testBegin( "testRenderPass_2C_DS" );
		auto rt1 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv1 = createView( *rt1 );
		auto rtAttach1 = crg::Attachment::createColour( "RT1"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv1 );

		auto rt2 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv2 = createView( *rt2 );
		auto rtAttach2 = crg::Attachment::createColour( "RT2"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv2 );

		auto ds = createImage( inst, ashes::Format::eD32_SFLOAT );
		auto dsv = createView( *ds );
		auto dsAttach = crg::Attachment::createDepthStencil( "DS"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *dsv );

		crg::RenderPass pass
		{
			"2C_DS",
			{},
			{ rtAttach1, rtAttach2 },
			dsAttach,
		};

		check( pass.name == "2C_DS" );
		check( pass.inputs.empty() );
		check( pass.colourOutputs.size() == 2u );
		check( pass.colourOutputs[0] == rtAttach1 );
		check( pass.colourOutputs[1] == rtAttach2 );
		check( pass.depthStencilOutput != std::nullopt );
		check( pass.depthStencilOutput.value() == dsAttach );
		testEnd();
	}
	
	void testRenderPass_0C_1I_DS( test::AppInstance & inst )
	{
		testBegin( "testRenderPass_0C_1I_DS" );
		auto in = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto inv = createView( *in );
		auto inAttach = crg::Attachment::createInput( "IN"
			, *inv );

		auto ds = createImage( inst, ashes::Format::eD32_SFLOAT );
		auto dsv = createView( *ds );
		auto dsAttach = crg::Attachment::createDepthStencil( "DS"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *dsv );

		crg::RenderPass pass
		{
			"0C_1I_DS",
			{ inAttach },
			{},
			dsAttach,
		};

		check( pass.name == "0C_1I_DS" );
		check( pass.inputs.size() == 1u );
		check( pass.inputs[0] == inAttach );
		check( pass.colourOutputs.empty() );
		check( pass.depthStencilOutput != std::nullopt );
		check( pass.depthStencilOutput.value() == dsAttach );
		testEnd();
	}

	void testRenderPass_0C_2I_DS( test::AppInstance & inst )
	{
		testBegin( "testRenderPass_0C_2I_DS" );
		auto in1 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto inv1 = createView( *in1 );
		auto inAttach1 = crg::Attachment::createInput( "IN1"
			, *inv1 );

		auto in2 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto inv2 = createView( *in2 );
		auto inAttach2 = crg::Attachment::createInput( "IN2"
			, *inv2 );

		auto ds = createImage( inst, ashes::Format::eD32_SFLOAT );
		auto dsv = createView( *ds );
		auto dsAttach = crg::Attachment::createDepthStencil( "DS"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *dsv );

		crg::RenderPass pass
		{
			"0C_2I_DS",
			{ inAttach1, inAttach2 },
			{},
			dsAttach,
		};

		check( pass.name == "0C_2I_DS" );
		check( pass.inputs.size() == 2u );
		check( pass.inputs[0] == inAttach1 );
		check( pass.inputs[1] == inAttach2 );
		check( pass.colourOutputs.empty() );
		check( pass.depthStencilOutput != std::nullopt );
		check( pass.depthStencilOutput.value() == dsAttach );
		testEnd();
	}

	void testRenderPass_1C_1I_DS( test::AppInstance & inst )
	{
		testBegin( "testRenderPass_1C_1I_DS" );
		auto rt = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv = createView( *rt );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv );

		auto in = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto inv = createView( *in );
		auto inAttach = crg::Attachment::createInput( "IN"
			, *inv );

		auto ds = createImage( inst, ashes::Format::eD32_SFLOAT );
		auto dsv = createView( *ds );
		auto dsAttach = crg::Attachment::createDepthStencil( "DS"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *dsv );

		crg::RenderPass pass
		{
			"1C_1I_DS",
			{ inAttach },
			{ rtAttach },
			dsAttach,
		};

		check( pass.name == "1C_1I_DS" );
		check( pass.inputs.size() == 1u );
		check( pass.inputs[0] == inAttach );
		check( pass.colourOutputs.size() == 1u );
		check( pass.colourOutputs[0] == rtAttach );
		check( pass.depthStencilOutput != std::nullopt );
		check( pass.depthStencilOutput.value() == dsAttach );
		testEnd();
	}

	void testRenderPass_1C_2I_DS( test::AppInstance & inst )
	{
		testBegin( "testRenderPass_1C_2I_DS" );
		auto rt = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv = createView( *rt );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv );

		auto in1 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto inv1 = createView( *in1 );
		auto inAttach1 = crg::Attachment::createInput( "IN1"
			, *inv1 );

		auto in2 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto inv2 = createView( *in2 );
		auto inAttach2 = crg::Attachment::createInput( "IN2"
			, *inv2 );

		auto ds = createImage( inst, ashes::Format::eD32_SFLOAT );
		auto dsv = createView( *ds );
		auto dsAttach = crg::Attachment::createDepthStencil( "DS"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *dsv );

		crg::RenderPass pass
		{
			"1C_2I_DS",
			{ inAttach1, inAttach2 },
			{ rtAttach },
			dsAttach,
		};

		check( pass.name == "1C_2I_DS" );
		check( pass.inputs.size() == 2u );
		check( pass.inputs[0] == inAttach1 );
		check( pass.inputs[1] == inAttach2 );
		check( pass.colourOutputs.size() == 1u );
		check( pass.colourOutputs[0] == rtAttach );
		check( pass.depthStencilOutput != std::nullopt );
		check( pass.depthStencilOutput.value() == dsAttach );
		testEnd();
	}

	void testRenderPass_2C_1I_DS( test::AppInstance & inst )
	{
		testBegin( "testRenderPass_2C_1I_DS" );
		auto rt1 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv1 = createView( *rt1 );
		auto rtAttach1 = crg::Attachment::createColour( "RT1"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv1 );

		auto rt2 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv2 = createView( *rt2 );
		auto rtAttach2 = crg::Attachment::createColour( "RT2"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv2 );

		auto in = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto inv = createView( *in );
		auto inAttach = crg::Attachment::createInput( "IN"
			, *inv );

		auto ds = createImage( inst, ashes::Format::eD32_SFLOAT );
		auto dsv = createView( *ds );
		auto dsAttach = crg::Attachment::createDepthStencil( "DS"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *dsv );

		crg::RenderPass pass
		{
			"2C_1I_DS",
			{ inAttach },
			{ rtAttach1, rtAttach2 },
			dsAttach,
		};

		check( pass.name == "2C_1I_DS" );
		check( pass.inputs.size() == 1u );
		check( pass.inputs[0] == inAttach );
		check( pass.colourOutputs.size() == 2u );
		check( pass.colourOutputs[0] == rtAttach1 );
		check( pass.colourOutputs[1] == rtAttach2 );
		check( pass.depthStencilOutput != std::nullopt );
		check( pass.depthStencilOutput.value() == dsAttach );
		testEnd();
	}

	void testRenderPass_2C_2I_DS( test::AppInstance & inst )
	{
		testBegin( "testRenderPass_2C_2I_DS" );
		auto rt1 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv1 = createView( *rt1 );
		auto rtAttach1 = crg::Attachment::createColour( "RT1"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv1 );

		auto rt2 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv2 = createView( *rt2 );
		auto rtAttach2 = crg::Attachment::createColour( "RT2"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv2 );

		auto in1 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto inv1 = createView( *in1 );
		auto inAttach1 = crg::Attachment::createInput( "IN1"
			, *inv1 );

		auto in2 = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto inv2 = createView( *in2 );
		auto inAttach2 = crg::Attachment::createInput( "IN2"
			, *inv2 );

		auto ds = createImage( inst, ashes::Format::eD32_SFLOAT );
		auto dsv = createView( *ds );
		auto dsAttach = crg::Attachment::createDepthStencil( "DS"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *dsv );

		crg::RenderPass pass
		{
			"2C_2I_DS",
			{ inAttach1, inAttach2 },
			{ rtAttach1, rtAttach2 },
			dsAttach,
		};

		check( pass.name == "2C_2I_DS" );
		check( pass.inputs.size() == 2u );
		check( pass.inputs[0] == inAttach1 );
		check( pass.inputs[1] == inAttach2 );
		check( pass.colourOutputs.size() == 2u );
		check( pass.colourOutputs[0] == rtAttach1 );
		check( pass.colourOutputs[1] == rtAttach2 );
		check( pass.depthStencilOutput != std::nullopt );
		check( pass.depthStencilOutput.value() == dsAttach );
		testEnd();
	}
}

int main( int argc, char ** argv )
{
	test::AppInstance inst;
	testSuiteBegin( "TestRenderPass", inst );
	testRenderPass_1C( inst );
	testRenderPass_2C( inst );
	testRenderPass_0C_1I( inst );
	testRenderPass_0C_2I( inst );
	testRenderPass_1C_1I( inst );
	testRenderPass_1C_2I( inst );
	testRenderPass_2C_1I( inst );
	testRenderPass_2C_2I( inst );
	testRenderPass_0C_DS( inst );
	testRenderPass_1C_DS( inst );
	testRenderPass_2C_DS( inst );
	testRenderPass_0C_1I_DS( inst );
	testRenderPass_0C_2I_DS( inst );
	testRenderPass_1C_1I_DS( inst );
	testRenderPass_1C_2I_DS( inst );
	testRenderPass_2C_1I_DS( inst );
	testRenderPass_2C_2I_DS( inst );
	testSuiteEnd();
}
