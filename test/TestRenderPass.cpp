#include "Common.hpp"

#include <RenderGraph/Attachment.hpp>
#include <RenderGraph/ImageData.hpp>
#include <RenderGraph/RenderPass.hpp>

namespace
{
	void testRenderPass_1C( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_1C" );
		auto rt = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto rtv = test::makeId( test::createView( rt ) );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv );

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

	void testRenderPass_2C( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_2C" );
		auto rt1 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto rtv1 = test::makeId( test::createView( rt1) );
		auto rtAttach1 = crg::Attachment::createColour( "RT1"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv1 );

		auto rt2 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto rtv2 = test::makeId( test::createView( rt2) );
		auto rtAttach2 = crg::Attachment::createColour( "RT2"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv2 );

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

	void testRenderPass_0C_1I( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_0C_1I" );
		auto in = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto inv = test::makeId( test::createView( in) );
		auto inAttach = crg::Attachment::createInput( "IN"
			, inv );

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

	void testRenderPass_0C_2I( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_0C_2I" );
		auto in1 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto inv1 = test::makeId( test::createView( in1) );
		auto inAttach1 = crg::Attachment::createInput( "IN1"
			, inv1 );

		auto in2 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto inv2 = test::makeId( test::createView( in2) );
		auto inAttach2 = crg::Attachment::createInput( "IN2"
			, inv2 );

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

	void testRenderPass_1C_1I( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_1C_1I" );
		auto rt = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto rtv = test::makeId( test::createView( rt) );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv );

		auto in = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto inv = test::makeId( test::createView( in) );
		auto inAttach = crg::Attachment::createInput( "IN"
			, inv );

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

	void testRenderPass_1C_2I( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_1C_2I" );
		auto rt = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto rtv = test::makeId( test::createView( rt) );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv );

		auto in1 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto inv1 = test::makeId( test::createView( in1) );
		auto inAttach1 = crg::Attachment::createInput( "IN1"
			, inv1 );

		auto in2 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto inv2 = test::makeId( test::createView( in2) );
		auto inAttach2 = crg::Attachment::createInput( "IN2"
			, inv2 );

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

	void testRenderPass_2C_1I( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_2C_1I" );
		auto rt1 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto rtv1 = test::makeId( test::createView( rt1) );
		auto rtAttach1 = crg::Attachment::createColour( "RT1"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv1 );

		auto rt2 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto rtv2 = test::makeId( test::createView( rt2) );
		auto rtAttach2 = crg::Attachment::createColour( "RT2"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv2 );

		auto in = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto inv = test::makeId( test::createView( in) );
		auto inAttach = crg::Attachment::createInput( "IN"
			, inv );

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

	void testRenderPass_2C_2I( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_2C_2I" );
		auto rt1 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto rtv1 = test::makeId( test::createView( rt1) );
		auto rtAttach1 = crg::Attachment::createColour( "RT1"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv1 );

		auto rt2 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto rtv2 = test::makeId( test::createView( rt2) );
		auto rtAttach2 = crg::Attachment::createColour( "RT2"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv2 );

		auto in1 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto inv1 = test::makeId( test::createView( in1) );
		auto inAttach1 = crg::Attachment::createInput( "IN1"
			, inv1 );

		auto in2 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto inv2 = test::makeId( test::createView( in2) );
		auto inAttach2 = crg::Attachment::createInput( "IN2"
			, inv2 );

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
	
	void testRenderPass_0C_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_0C_DS" );
		auto ds = test::createImage( VK_FORMAT_D32_SFLOAT );
		auto dsv = test::makeId( test::createView( ds) );
		auto dsAttach = crg::Attachment::createDepthStencil( "DS"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, dsv );

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

	void testRenderPass_1C_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_1C_DS" );
		auto rt = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto rtv = test::makeId( test::createView( rt) );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv );

		auto ds = test::createImage( VK_FORMAT_D32_SFLOAT );
		auto dsv = test::makeId( test::createView( ds) );
		auto dsAttach = crg::Attachment::createDepthStencil( "DS"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, dsv );

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

	void testRenderPass_2C_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_2C_DS" );
		auto rt1 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto rtv1 = test::makeId( test::createView( rt1) );
		auto rtAttach1 = crg::Attachment::createColour( "RT1"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv1 );

		auto rt2 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto rtv2 = test::makeId( test::createView( rt2) );
		auto rtAttach2 = crg::Attachment::createColour( "RT2"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv2 );

		auto ds = test::createImage( VK_FORMAT_D32_SFLOAT );
		auto dsv = test::makeId( test::createView( ds) );
		auto dsAttach = crg::Attachment::createDepthStencil( "DS"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, dsv );

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
	
	void testRenderPass_0C_1I_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_0C_1I_DS" );
		auto in = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto inv = test::makeId( test::createView( in) );
		auto inAttach = crg::Attachment::createInput( "IN"
			, inv );

		auto ds = test::createImage( VK_FORMAT_D32_SFLOAT );
		auto dsv = test::makeId( test::createView( ds) );
		auto dsAttach = crg::Attachment::createDepthStencil( "DS"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, dsv );

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

	void testRenderPass_0C_2I_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_0C_2I_DS" );
		auto in1 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto inv1 = test::makeId( test::createView( in1) );
		auto inAttach1 = crg::Attachment::createInput( "IN1"
			, inv1 );

		auto in2 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto inv2 = test::makeId( test::createView( in2) );
		auto inAttach2 = crg::Attachment::createInput( "IN2"
			, inv2 );

		auto ds = test::createImage( VK_FORMAT_D32_SFLOAT );
		auto dsv = test::makeId( test::createView( ds) );
		auto dsAttach = crg::Attachment::createDepthStencil( "DS"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, dsv );

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

	void testRenderPass_1C_1I_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_1C_1I_DS" );
		auto rt = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto rtv = test::makeId( test::createView( rt) );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv );

		auto in = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto inv = test::makeId( test::createView( in) );
		auto inAttach = crg::Attachment::createInput( "IN"
			, inv );

		auto ds = test::createImage( VK_FORMAT_D32_SFLOAT );
		auto dsv = test::makeId( test::createView( ds) );
		auto dsAttach = crg::Attachment::createDepthStencil( "DS"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, dsv );

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

	void testRenderPass_1C_2I_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_1C_2I_DS" );
		auto rt = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto rtv = test::makeId( test::createView( rt) );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv );

		auto in1 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto inv1 = test::makeId( test::createView( in1) );
		auto inAttach1 = crg::Attachment::createInput( "IN1"
			, inv1 );

		auto in2 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto inv2 = test::makeId( test::createView( in2) );
		auto inAttach2 = crg::Attachment::createInput( "IN2"
			, inv2 );

		auto ds = test::createImage( VK_FORMAT_D32_SFLOAT );
		auto dsv = test::makeId( test::createView( ds) );
		auto dsAttach = crg::Attachment::createDepthStencil( "DS"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, dsv );

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

	void testRenderPass_2C_1I_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_2C_1I_DS" );
		auto rt1 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto rtv1 = test::makeId( test::createView( rt1) );
		auto rtAttach1 = crg::Attachment::createColour( "RT1"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv1 );

		auto rt2 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto rtv2 = test::makeId( test::createView( rt2) );
		auto rtAttach2 = crg::Attachment::createColour( "RT2"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv2 );

		auto in = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto inv = test::makeId( test::createView( in) );
		auto inAttach = crg::Attachment::createInput( "IN"
			, inv );

		auto ds = test::createImage( VK_FORMAT_D32_SFLOAT );
		auto dsv = test::makeId( test::createView( ds) );
		auto dsAttach = crg::Attachment::createDepthStencil( "DS"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, dsv );

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

	void testRenderPass_2C_2I_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_2C_2I_DS" );
		auto rt1 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto rtv1 = test::makeId( test::createView( rt1) );
		auto rtAttach1 = crg::Attachment::createColour( "RT1"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv1 );

		auto rt2 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto rtv2 = test::makeId( test::createView( rt2) );
		auto rtAttach2 = crg::Attachment::createColour( "RT2"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, rtv2 );

		auto in1 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto inv1 = test::makeId( test::createView( in1) );
		auto inAttach1 = crg::Attachment::createInput( "IN1"
			, inv1 );

		auto in2 = test::createImage( VK_FORMAT_R32G32B32A32_SFLOAT );
		auto inv2 = test::makeId( test::createView( in2) );
		auto inAttach2 = crg::Attachment::createInput( "IN2"
			, inv2 );

		auto ds = test::createImage( VK_FORMAT_D32_SFLOAT );
		auto dsv = test::makeId( test::createView( ds) );
		auto dsAttach = crg::Attachment::createDepthStencil( "DS"
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, VK_ATTACHMENT_LOAD_OP_CLEAR
			, VK_ATTACHMENT_STORE_OP_STORE
			, dsv );

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
	testSuiteBegin( "TestRenderPass" );
	testRenderPass_1C( testCounts );
	testRenderPass_2C( testCounts );
	testRenderPass_0C_1I( testCounts );
	testRenderPass_0C_2I( testCounts );
	testRenderPass_1C_1I( testCounts );
	testRenderPass_1C_2I( testCounts );
	testRenderPass_2C_1I( testCounts );
	testRenderPass_2C_2I( testCounts );
	testRenderPass_0C_DS( testCounts );
	testRenderPass_1C_DS( testCounts );
	testRenderPass_2C_DS( testCounts );
	testRenderPass_0C_1I_DS( testCounts );
	testRenderPass_0C_2I_DS( testCounts );
	testRenderPass_1C_1I_DS( testCounts );
	testRenderPass_1C_2I_DS( testCounts );
	testRenderPass_2C_1I_DS( testCounts );
	testRenderPass_2C_2I_DS( testCounts );
	testSuiteEnd();
}
