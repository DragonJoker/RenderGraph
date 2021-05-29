#include "Common.hpp"

#include <RenderGraph/Attachment.hpp>
#include <RenderGraph/FramePass.hpp>
#include <RenderGraph/FrameGraph.hpp>
#include <RenderGraph/ImageData.hpp>

namespace
{
	void testRenderPass_1C( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_1C" );
		crg::FrameGraph graph{ "test" };
		auto & pass = graph.createPass( "1C", crg::RunnablePassCreator{} );
		auto rt = graph.createImage( test::createImage( "rt", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		pass.addOutputColourView( rtv );

		check( pass.name == "1C" );
		check( pass.sampled.empty() );
		check( pass.colourInOuts.size() == 1u );
		check( pass.colourInOuts[0].view() == rtv );
		check( pass.depthStencilInOut == std::nullopt );
		testEnd();
	}

	void testRenderPass_2C( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_2C" );
		crg::FrameGraph graph{ "test" };
		auto & pass = graph.createPass( "2C", crg::RunnablePassCreator{} );
		auto rt1 = graph.createImage( test::createImage( "rt1", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv1 = graph.createView( test::createView( "rtv1", rt1 ) );
		pass.addOutputColourView( rtv1 );

		auto rt2 = graph.createImage( test::createImage( "rt2", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv2 = graph.createView( test::createView( "rtv2", rt2 ) );
		pass.addOutputColourView( rtv2 );

		check( pass.name == "2C" );
		check( pass.sampled.empty() );
		check( pass.colourInOuts.size() == 2u );
		check( pass.colourInOuts[0].view() == rtv1 );
		check( pass.colourInOuts[1].view() == rtv2 );
		check( pass.depthStencilInOut == std::nullopt );
		testEnd();
	}

	void testRenderPass_0C_1I( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_0C_1I" );
		crg::FrameGraph graph{ "test" };
		auto & pass = graph.createPass( "0C_1I", crg::RunnablePassCreator{} );
		auto in = graph.createImage( test::createImage( "in", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv = graph.createView( test::createView( "inv", in ) );
		pass.addSampledView( inv, 1u );

		check( pass.name == "0C_1I" );
		check( pass.sampled.size() == 1u );
		check( pass.sampled[0].view() == inv );
		check( pass.colourInOuts.empty() );
		check( pass.depthStencilInOut == std::nullopt );
		testEnd();
	}

	void testRenderPass_0C_2I( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_0C_2I" );
		crg::FrameGraph graph{ "test" };
		auto & pass = graph.createPass( "0C_2I", crg::RunnablePassCreator{} );
		auto in1 = graph.createImage( test::createImage( "in1", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv1 = graph.createView( test::createView( "inv1", in1 ) );
		pass.addSampledView( inv1, 1u );

		auto in2 = graph.createImage( test::createImage( "in2", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv2 = graph.createView( test::createView( "inv2", in2 ) );
		pass.addSampledView( inv2, 2u );

		check( pass.name == "0C_2I" );
		check( pass.sampled.size() == 2u );
		check( pass.sampled[0].view() == inv1 );
		check( pass.sampled[1].view() == inv2 );
		check( pass.colourInOuts.empty() );
		check( pass.depthStencilInOut == std::nullopt );
		testEnd();
	}

	void testRenderPass_1C_1I( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_1C_1I" );
		crg::FrameGraph graph{ "test" };
		auto & pass = graph.createPass( "1C_1I", crg::RunnablePassCreator{} );
		auto rt = graph.createImage( test::createImage( "rt", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		pass.addOutputColourView( rtv );

		auto in = graph.createImage( test::createImage( "in", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv = graph.createView( test::createView( "inv", in ) );
		pass.addSampledView( inv, 1u );

		check( pass.name == "1C_1I" );
		check( pass.sampled.size() == 1u );
		check( pass.sampled[0].view() == inv );
		check( pass.colourInOuts.size() == 1u );
		check( pass.colourInOuts[0].view() == rtv );
		check( pass.depthStencilInOut == std::nullopt );
		testEnd();
	}

	void testRenderPass_1C_2I( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_1C_2I" );
		crg::FrameGraph graph{ "test" };
		auto & pass = graph.createPass( "1C_2I", crg::RunnablePassCreator{} );
		auto rt = graph.createImage( test::createImage( "rt", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		pass.addOutputColourView( rtv );

		auto in1 = graph.createImage( test::createImage( "in1", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv1 = graph.createView( test::createView( "inv1", in1 ) );
		pass.addSampledView( inv1, 1u );

		auto in2 = graph.createImage( test::createImage( "in2", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv2 = graph.createView( test::createView( "inv2", in2 ) );
		pass.addSampledView( inv2, 2u );

		check( pass.name == "1C_2I" );
		check( pass.sampled.size() == 2u );
		check( pass.sampled[0].view() == inv1 );
		check( pass.sampled[1].view() == inv2 );
		check( pass.colourInOuts.size() == 1u );
		check( pass.colourInOuts[0].view() == rtv );
		check( pass.depthStencilInOut == std::nullopt );
		testEnd();
	}

	void testRenderPass_2C_1I( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_2C_1I" );
		crg::FrameGraph graph{ "test" };
		auto & pass = graph.createPass( "2C_1I", crg::RunnablePassCreator{} );
		auto rt1 = graph.createImage( test::createImage( "rt1", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv1 = graph.createView( test::createView( "rtv1", rt1 ) );
		pass.addOutputColourView( rtv1 );

		auto rt2 = graph.createImage( test::createImage( "rt2", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv2 = graph.createView( test::createView( "rtv2", rt2 ) );
		pass.addOutputColourView( rtv2 );

		auto in = graph.createImage( test::createImage( "in", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv = graph.createView( test::createView( "inv", in ) );
		pass.addSampledView( inv, 1u );

		check( pass.name == "2C_1I" );
		check( pass.sampled.size() == 1u );
		check( pass.sampled[0].view() == inv );
		check( pass.colourInOuts.size() == 2u );
		check( pass.colourInOuts[0].view() == rtv1 );
		check( pass.colourInOuts[1].view() == rtv2 );
		check( pass.depthStencilInOut == std::nullopt );
		testEnd();
	}

	void testRenderPass_2C_2I( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_2C_2I" );
		crg::FrameGraph graph{ "test" };
		auto & pass = graph.createPass( "2C_2I", crg::RunnablePassCreator{} );
		auto rt1 = graph.createImage( test::createImage( "rt1", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv1 = graph.createView( test::createView( "rtv1", rt1 ) );
		pass.addOutputColourView( rtv1 );

		auto rt2 = graph.createImage( test::createImage( "rt2", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv2 = graph.createView( test::createView( "rtv2", rt2 ) );
		pass.addOutputColourView( rtv2 );

		auto in1 = graph.createImage( test::createImage( "in1", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv1 = graph.createView( test::createView( "inv1", in1 ) );
		pass.addSampledView( inv1, 1u );

		auto in2 = graph.createImage( test::createImage( "in2", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv2 = graph.createView( test::createView( "inv2", in2 ) );
		pass.addSampledView( inv2, 2u );

		check( pass.name == "2C_2I" );
		check( pass.sampled.size() == 2u );
		check( pass.sampled[0].view() == inv1 );
		check( pass.sampled[1].view() == inv2 );
		check( pass.colourInOuts.size() == 2u );
		check( pass.colourInOuts[0].view() == rtv1 );
		check( pass.colourInOuts[1].view() == rtv2 );
		check( pass.depthStencilInOut == std::nullopt );
		testEnd();
	}
	
	void testRenderPass_0C_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_0C_DS" );
		crg::FrameGraph graph{ "test" };
		auto & pass = graph.createPass( "0C_DS", crg::RunnablePassCreator{} );
		auto ds = graph.createImage( test::createImage( "ds", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilView( dsv );

		check( pass.name == "0C_DS" );
		check( pass.sampled.empty() );
		check( pass.colourInOuts.empty() == 1u );
		check( pass.depthStencilInOut != std::nullopt );
		check( pass.depthStencilInOut.value().view() == dsv );
		testEnd();
	}

	void testRenderPass_1C_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_1C_DS" );
		crg::FrameGraph graph{ "test" };
		auto & pass = graph.createPass( "1C_DS", crg::RunnablePassCreator{} );
		auto rt = graph.createImage( test::createImage( "rt", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		pass.addOutputColourView( rtv );

		auto ds = graph.createImage( test::createImage( "ds", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilView( dsv );

		check( pass.name == "1C_DS" );
		check( pass.sampled.empty() );
		check( pass.colourInOuts.size() == 1u );
		check( pass.colourInOuts[0].view() == rtv );
		check( pass.depthStencilInOut != std::nullopt );
		check( pass.depthStencilInOut.value().view() == dsv );
		testEnd();
	}

	void testRenderPass_2C_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_2C_DS" );
		crg::FrameGraph graph{ "test" };
		auto & pass = graph.createPass( "2C_DS", crg::RunnablePassCreator{} );
		auto rt1 = graph.createImage( test::createImage( "rt1", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv1 = graph.createView( test::createView( "rtv1", rt1 ) );
		pass.addOutputColourView( rtv1 );

		auto rt2 = graph.createImage( test::createImage( "rt2", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv2 = graph.createView( test::createView( "rtv2", rt2 ) );
		pass.addOutputColourView( rtv2 );

		auto ds = graph.createImage( test::createImage( "ds", VK_FORMAT_D32_SFLOAT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilView( dsv );

		check( pass.name == "2C_DS" );
		check( pass.sampled.empty() );
		check( pass.colourInOuts.size() == 2u );
		check( pass.colourInOuts[0].view() == rtv1 );
		check( pass.colourInOuts[1].view() == rtv2 );
		check( pass.depthStencilInOut != std::nullopt );
		check( pass.depthStencilInOut.value().view() == dsv );
		testEnd();
	}
	
	void testRenderPass_0C_1I_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_0C_1I_DS" );
		crg::FrameGraph graph{ "test" };
		auto & pass = graph.createPass( "0C_1I_DS", crg::RunnablePassCreator{} );
		auto in = graph.createImage( test::createImage( "in", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv = graph.createView( test::createView( "inv", in ) );
		pass.addSampledView( inv, 1u );

		auto ds = graph.createImage( test::createImage( "ds", VK_FORMAT_D32_SFLOAT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilView( dsv );

		check( pass.name == "0C_1I_DS" );
		check( pass.sampled.size() == 1u );
		check( pass.sampled[0].view() == inv );
		check( pass.colourInOuts.empty() );
		check( pass.depthStencilInOut != std::nullopt );
		check( pass.depthStencilInOut.value().view() == dsv );
		testEnd();
	}

	void testRenderPass_0C_2I_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_0C_2I_DS" );
		crg::FrameGraph graph{ "test" };
		auto & pass = graph.createPass( "0C_2I_DS", crg::RunnablePassCreator{} );
		auto in1 = graph.createImage( test::createImage( "in1", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv1 = graph.createView( test::createView( "inv1", in1 ) );
		pass.addSampledView( inv1, 1u );

		auto in2 = graph.createImage( test::createImage( "in2", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv2 = graph.createView( test::createView( "inv2", in2 ) );
		pass.addSampledView( inv2, 2u );

		auto ds = graph.createImage( test::createImage( "ds", VK_FORMAT_D32_SFLOAT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilView( dsv );

		check( pass.name == "0C_2I_DS" );
		check( pass.sampled.size() == 2u );
		check( pass.sampled[0].view() == inv1 );
		check( pass.sampled[1].view() == inv2 );
		check( pass.colourInOuts.empty() );
		check( pass.depthStencilInOut != std::nullopt );
		check( pass.depthStencilInOut.value().view() == dsv );
		testEnd();
	}

	void testRenderPass_1C_1I_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_1C_1I_DS" );
		crg::FrameGraph graph{ "test" };
		auto & pass = graph.createPass( "1C_1I_DS", crg::RunnablePassCreator{} );
		auto rt = graph.createImage( test::createImage( "rt", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		pass.addOutputColourView( rtv );

		auto in = graph.createImage( test::createImage( "in", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv = graph.createView( test::createView( "inv", in ) );
		pass.addSampledView( inv, 1u );

		auto ds = graph.createImage( test::createImage( "ds", VK_FORMAT_D32_SFLOAT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilView( dsv );

		check( pass.name == "1C_1I_DS" );
		check( pass.sampled.size() == 1u );
		check( pass.sampled[0].view() == inv );
		check( pass.colourInOuts.size() == 1u );
		check( pass.colourInOuts[0].view() == rtv );
		check( pass.depthStencilInOut != std::nullopt );
		check( pass.depthStencilInOut.value().view() == dsv );
		testEnd();
	}

	void testRenderPass_1C_2I_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_1C_2I_DS" );
		crg::FrameGraph graph{ "test" };
		auto & pass = graph.createPass( "1C_2I_DS", crg::RunnablePassCreator{} );
		auto rt = graph.createImage( test::createImage( "rt", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		pass.addOutputColourView( rtv );

		auto in1 = graph.createImage( test::createImage( "in1", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv1 = graph.createView( test::createView( "inv1", in1 ) );
		pass.addSampledView( inv1, 1u );

		auto in2 = graph.createImage( test::createImage( "in2", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv2 = graph.createView( test::createView( "inv2", in2 ) );
		pass.addSampledView( inv2, 2u );

		auto ds = graph.createImage( test::createImage( "ds", VK_FORMAT_D32_SFLOAT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilView( dsv );

		check( pass.name == "1C_2I_DS" );
		check( pass.sampled.size() == 2u );
		check( pass.sampled[0].view() == inv1 );
		check( pass.sampled[1].view() == inv2 );
		check( pass.colourInOuts.size() == 1u );
		check( pass.colourInOuts[0].view() == rtv );
		check( pass.depthStencilInOut != std::nullopt );
		check( pass.depthStencilInOut.value().view() == dsv );
		testEnd();
	}

	void testRenderPass_2C_1I_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_2C_1I_DS" );
		crg::FrameGraph graph{ "test" };
		auto & pass = graph.createPass( "2C_1I_DS", crg::RunnablePassCreator{} );
		auto rt1 = graph.createImage( test::createImage( "rt1", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv1 = graph.createView( test::createView( "rtv1", rt1 ) );
		pass.addOutputColourView( rtv1 );

		auto rt2 = graph.createImage( test::createImage( "rt2", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv2 = graph.createView( test::createView( "rtv2", rt2 ) );
		pass.addOutputColourView( rtv2 );

		auto in = graph.createImage( test::createImage( "in", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv = graph.createView( test::createView( "inv", in ) );
		pass.addSampledView( inv, 1u );

		auto ds = graph.createImage( test::createImage( "ds", VK_FORMAT_D32_SFLOAT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilView( dsv );

		check( pass.name == "2C_1I_DS" );
		check( pass.sampled.size() == 1u );
		check( pass.sampled[0].view() == inv );
		check( pass.colourInOuts.size() == 2u );
		check( pass.colourInOuts[0].view() == rtv1 );
		check( pass.colourInOuts[1].view() == rtv2 );
		check( pass.depthStencilInOut != std::nullopt );
		check( pass.depthStencilInOut.value().view() == dsv );
		testEnd();
	}

	void testRenderPass_2C_2I_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_2C_2I_DS" );
		crg::FrameGraph graph{ "test" };
		auto & pass = graph.createPass( "2C_2I_DS", crg::RunnablePassCreator{} );
		auto rt1 = graph.createImage( test::createImage( "rt1", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv1 = graph.createView( test::createView( "rtv1", rt1 ) );
		pass.addOutputColourView( rtv1 );

		auto rt2 = graph.createImage( test::createImage( "rt2", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv2 = graph.createView( test::createView( "rtv2", rt2 ) );
		pass.addOutputColourView( rtv2 );

		auto in1 = graph.createImage( test::createImage( "in1", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv1 = graph.createView( test::createView( "inv1", in1 ) );
		pass.addSampledView( inv1, 1u );

		auto in2 = graph.createImage( test::createImage( "in2", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv2 = graph.createView( test::createView( "inv2", in2 ) );
		pass.addSampledView( inv2, 2u );

		auto ds = graph.createImage( test::createImage( "ds", VK_FORMAT_D32_SFLOAT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilView( dsv );

		check( pass.name == "2C_2I_DS" );
		check( pass.sampled.size() == 2u );
		check( pass.sampled[0].view() == inv1 );
		check( pass.sampled[1].view() == inv2 );
		check( pass.colourInOuts.size() == 2u );
		check( pass.colourInOuts[0].view() == rtv1 );
		check( pass.colourInOuts[1].view() == rtv2 );
		check( pass.depthStencilInOut != std::nullopt );
		check( pass.depthStencilInOut.value().view() == dsv );
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
