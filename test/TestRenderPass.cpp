#include "Common.hpp"

#include <RenderGraph/Attachment.hpp>
#include <RenderGraph/FramePass.hpp>
#include <RenderGraph/FrameGraph.hpp>
#include <RenderGraph/ImageData.hpp>
#include <RenderGraph/Log.hpp>
#include <RenderGraph/ResourceHandler.hpp>

namespace
{
	void testLog( test::TestCounts & testCounts )
	{
		testBegin( "testLog" )
		auto callback = []( std::string const & msg, bool newLine )
		{
			std::cout << msg << "Callback";

			if ( newLine )
			{
				std::cout << "\n";
			}
			else
			{
				std::cout << " ";
			}
		};

		crg::Logger::setTraceCallback( callback );
		crg::Logger::setDebugCallback( callback );
		crg::Logger::setInfoCallback( callback );
		crg::Logger::setWarningCallback( callback );
		crg::Logger::setErrorCallback( callback );

		crg::Logger::logTrace( "traceCoinNoNL ", false );
		crg::Logger::logTrace( "traceCoinNL" );
		crg::Logger::logDebug( "debugCoinNoNL ", false );
		crg::Logger::logDebug( "debugCoinNL" );
		crg::Logger::logInfo( "infoCoinNoNL ", false );
		crg::Logger::logInfo( "infoCoinNL" );
		crg::Logger::logWarning( "warningCoinNoNL ", false );
		crg::Logger::logWarning( "warningCoinNL" );
		crg::Logger::logError( "errorCoinNoNL ", false );
		crg::Logger::logError( "errorCoinNL" );
		testEnd()
	}

	void testRenderPass_1C( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_1C" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "1C", crg::RunnablePassCreator{} );
		auto rt = graph.createImage( test::createImage( "rt", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		pass.addOutputColourView( rtv );

		check( pass.getName() == "1C" )
		check( pass.images.size() == 1u )
		check( pass.images[0].view() == rtv )
		testEnd()
	}

	void testRenderPass_2C( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_2C" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "2C", crg::RunnablePassCreator{} );
		auto rt1 = graph.createImage( test::createImage( "rt1", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv1 = graph.createView( test::createView( "rtv1", rt1 ) );
		pass.addOutputColourView( rtv1 );

		auto rt2 = graph.createImage( test::createImage( "rt2", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv2 = graph.createView( test::createView( "rtv2", rt2 ) );
		pass.addOutputColourView( rtv2 );

		check( pass.getName() == "2C" )
		check( pass.images.size() == 2u )
		check( pass.images[0].view() == rtv1 )
		check( pass.images[1].view() == rtv2 )
		testEnd()
	}

	void testRenderPass_0C_1I( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_0C_1I" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "0C_1I", crg::RunnablePassCreator{} );
		auto in = graph.createImage( test::createImage( "in", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv = graph.createView( test::createView( "inv", in ) );
		pass.addSampledView( inv, 1u );

		check( pass.getName() == "0C_1I" )
		check( pass.images.size() == 1u )
		check( pass.images[0].view() == inv )
		testEnd()
	}

	void testRenderPass_0C_2I( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_0C_2I" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "0C_2I", crg::RunnablePassCreator{} );
		auto in1 = graph.createImage( test::createImage( "in1", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv1 = graph.createView( test::createView( "inv1", in1 ) );
		pass.addSampledView( inv1, 1u );

		auto in2 = graph.createImage( test::createImage( "in2", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv2 = graph.createView( test::createView( "inv2", in2 ) );
		pass.addSampledView( inv2, 2u );

		check( pass.getName() == "0C_2I" )
		check( pass.images.size() == 2u )
		check( pass.images[0].view() == inv1 )
		check( pass.images[1].view() == inv2 )
		testEnd()
	}

	void testRenderPass_1C_1I( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_1C_1I" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "1C_1I", crg::RunnablePassCreator{} );
		auto rt = graph.createImage( test::createImage( "rt", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		pass.addOutputColourView( rtv );

		auto in = graph.createImage( test::createImage( "in", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv = graph.createView( test::createView( "inv", in ) );
		pass.addSampledView( inv, 1u );

		check( pass.getName() == "1C_1I" )
		check( pass.images.size() == 2u )
		check( pass.images[0].view() == rtv )
		check( pass.images[1].view() == inv )
		testEnd()
	}

	void testRenderPass_1C_2I( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_1C_2I" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
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

		check( pass.getName() == "1C_2I" )
		check( pass.images.size() == 3u )
		check( pass.images[0].view() == rtv )
		check( pass.images[1].view() == inv1 )
		check( pass.images[2].view() == inv2 )
		testEnd()
	}

	void testRenderPass_2C_1I( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_2C_1I" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
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

		check( pass.getName() == "2C_1I" )
		check( pass.images.size() == 3u )
		check( pass.images[0].view() == rtv1 )
		check( pass.images[1].view() == rtv2 )
		check( pass.images[2].view() == inv )
		testEnd()
	}

	void testRenderPass_2C_2I( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_2C_2I" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
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

		check( pass.getName() == "2C_2I" )
		check( pass.images.size() == 4u )
		check( pass.images[0].view() == rtv1 )
		check( pass.images[1].view() == rtv2 )
		check( pass.images[2].view() == inv1 )
		check( pass.images[3].view() == inv2 )
		testEnd()
	}
	
	void testRenderPass_0C_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_0C_DS" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "0C_DS", crg::RunnablePassCreator{} );
		auto ds = graph.createImage( test::createImage( "ds", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilView( dsv );

		check( pass.getName() == "0C_DS" )
		check( pass.images.size() == 1u )
		check( pass.images[0].view() == dsv )
		testEnd()
	}

	void testRenderPass_1C_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_1C_DS" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "1C_DS", crg::RunnablePassCreator{} );
		auto rt = graph.createImage( test::createImage( "rt", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		pass.addOutputColourView( rtv );

		auto ds = graph.createImage( test::createImage( "ds", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilView( dsv );

		check( pass.getName() == "1C_DS" )
		check( pass.images.size() == 2u )
		check( pass.images[0].view() == dsv )
		check( pass.images[1].view() == rtv )
		testEnd()
	}

	void testRenderPass_2C_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_2C_DS" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
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

		check( pass.getName() == "2C_DS" )
		check( pass.images.size() == 3u )
		check( pass.images[0].view() == dsv )
		check( pass.images[1].view() == rtv1 )
		check( pass.images[2].view() == rtv2 )
		testEnd()
	}
	
	void testRenderPass_0C_1I_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_0C_1I_DS" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "0C_1I_DS", crg::RunnablePassCreator{} );
		auto in = graph.createImage( test::createImage( "in", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto inv = graph.createView( test::createView( "inv", in ) );
		pass.addSampledView( inv, 1u );

		auto ds = graph.createImage( test::createImage( "ds", VK_FORMAT_D32_SFLOAT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilView( dsv );

		check( pass.getName() == "0C_1I_DS" )
		check( pass.images.size() == 2u )
		check( pass.images[0].view() == dsv )
		check( pass.images[1].view() == inv )
		testEnd()
	}

	void testRenderPass_0C_2I_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_0C_2I_DS" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
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

		check( pass.getName() == "0C_2I_DS" )
		check( pass.images.size() == 3u )
		check( pass.images[0].view() == dsv )
		check( pass.images[1].view() == inv1 )
		check( pass.images[2].view() == inv2 )
		testEnd()
	}

	void testRenderPass_1C_1I_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_1C_1I_DS" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
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

		check( pass.getName() == "1C_1I_DS" )
		check( pass.images.size() == 3u )
		check( pass.images[0].view() == dsv )
		check( pass.images[1].view() == rtv )
		check( pass.images[2].view() == inv )
		testEnd()
	}

	void testRenderPass_1C_2I_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_1C_2I_DS" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
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

		check( pass.getName() == "1C_2I_DS" )
		check( pass.images.size() == 4u )
		check( pass.images[0].view() == dsv )
		check( pass.images[1].view() == rtv )
		check( pass.images[2].view() == inv1 )
		check( pass.images[3].view() == inv2 )
		testEnd()
	}

	void testRenderPass_2C_1I_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_2C_1I_DS" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
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

		check( pass.getName() == "2C_1I_DS" )
		check( pass.images.size() == 4u )
		check( pass.images[0].view() == dsv )
		check( pass.images[1].view() == rtv1 )
		check( pass.images[2].view() == rtv2 )
		check( pass.images[3].view() == inv )
		testEnd()
	}

	void testRenderPass_2C_2I_DS( test::TestCounts & testCounts )
	{
		testBegin( "testRenderPass_2C_2I_DS" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
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

		check( pass.getName() == "2C_2I_DS" )
		check( pass.images.size() == 5u )
		check( pass.images[0].view() == dsv )
		check( pass.images[1].view() == rtv1 )
		check( pass.images[2].view() == rtv2 )
		check( pass.images[3].view() == inv1 )
		check( pass.images[4].view() == inv2 )
		testEnd()
	}
}

int main( int argc, char ** argv )
{
	testSuiteBegin( "TestRenderPass" )
	testLog( testCounts );
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
	testSuiteEnd()
}
