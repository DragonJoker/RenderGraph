#include "Common.hpp"

#include <RenderGraph/Attachment.hpp>
#include <RenderGraph/FramePass.hpp>
#include <RenderGraph/FrameGraph.hpp>
#include <RenderGraph/ImageData.hpp>
#include <RenderGraph/Log.hpp>
#include <RenderGraph/ResourceHandler.hpp>

#include <fmt/base.h>

namespace
{
	TEST( FramePass, Log )
	{
		testBegin( "testLog" )
		auto callback = []( std::string_view msg, bool newLine )noexcept
			{
				if ( newLine )
					fmt::print( "{} Callback\n", msg.data() );
				else
					fmt::print( "{} Callback ", msg.data() );
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

		crg::Logger::setTraceCallback( nullptr );
		crg::Logger::setDebugCallback( nullptr );
		crg::Logger::setInfoCallback( nullptr );
		crg::Logger::setWarningCallback( nullptr );
		crg::Logger::setErrorCallback( nullptr );

		crg::Logger::logTrace( "No callback traceCoinNoNL ", false );
		crg::Logger::logTrace( "No callback traceCoinNL" );
		crg::Logger::logDebug( "No callback debugCoinNoNL ", false );
		crg::Logger::logDebug( "No callback debugCoinNL" );
		crg::Logger::logInfo( "No callback infoCoinNoNL ", false );
		crg::Logger::logInfo( "No callback infoCoinNL" );
		crg::Logger::logWarning( "No callback warningCoinNoNL ", false );
		crg::Logger::logWarning( "No callback warningCoinNL" );
		crg::Logger::logError( "No callback errorCoinNoNL ", false );
		crg::Logger::logError( "No callback errorCoinNL" );

		testEnd()
	}

	TEST( FramePass, RenderPass_1C )
	{
		testBegin( "testRenderPass_1C" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "1C", crg::RunnablePassCreator{} );
		auto rt = graph.createImage( test::createImage( "rt", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		pass.addOutputColourTarget( rtv );

		check( pass.getName() == "1C" )
		check( pass.getTargets().size() == 1u )
		check( pass.getTargets()[0]->view() == rtv )
		testEnd()
	}

	TEST( FramePass, RenderPass_2C )
	{
		testBegin( "testRenderPass_2C" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "2C", crg::RunnablePassCreator{} );
		auto rt1 = graph.createImage( test::createImage( "rt1", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv1 = graph.createView( test::createView( "rtv1", rt1 ) );
		pass.addOutputColourTarget( rtv1 );

		auto rt2 = graph.createImage( test::createImage( "rt2", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv2 = graph.createView( test::createView( "rtv2", rt2 ) );
		pass.addOutputColourTarget( rtv2 );

		check( pass.getName() == "2C" )
		check( pass.getTargets().size() == 2u )
		check( pass.getTargets()[0]->view() == rtv1 )
		check( pass.getTargets()[1]->view() == rtv2 )
		testEnd()
	}

	TEST( FramePass, RenderPass_0C_1I )
	{
		testBegin( "testRenderPass_0C_1I" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "0C_1I", crg::RunnablePassCreator{} );
		auto in = graph.createImage( test::createImage( "in", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto inv = graph.createView( test::createView( "inv", in ) );
		auto attach = crg::Attachment::createDefault( inv );
		pass.addInputSampled( attach, 1u );

		check( pass.getName() == "0C_1I" )
		check( pass.getSampled().size() == 1u )
		check( pass.getSampled().begin()->second.attach->view() == inv )
		testEnd()
	}

	TEST( FramePass, RenderPass_0C_2I )
	{
		testBegin( "testRenderPass_0C_2I" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "0C_2I", crg::RunnablePassCreator{} );
		auto in1 = graph.createImage( test::createImage( "in1", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto inv1 = graph.createView( test::createView( "inv1", in1 ) );
		auto attach1 = crg::Attachment::createDefault( inv1 );
		pass.addInputSampled( attach1, 1u );

		auto in2 = graph.createImage( test::createImage( "in2", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto inv2 = graph.createView( test::createView( "inv2", in2 ) );
		auto attach2 = crg::Attachment::createDefault( inv2 );
		pass.addInputSampled( attach2, 2u );

		check( pass.getName() == "0C_2I" )
		check( pass.getSampled().size() == 2u )
		check( pass.getSampled().begin()->second.attach->view() == inv1 )
		check( pass.getSampled().rbegin()->second.attach->view() == inv2 )
		testEnd()
	}

	TEST( FramePass, RenderPass_1C_1I )
	{
		testBegin( "testRenderPass_1C_1I" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "1C_1I", crg::RunnablePassCreator{} );
		auto rt = graph.createImage( test::createImage( "rt", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		pass.addOutputColourTarget( rtv );

		auto in = graph.createImage( test::createImage( "in", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto inv = graph.createView( test::createView( "inv", in ) );
		auto attach = crg::Attachment::createDefault( inv );
		pass.addInputSampled( attach, 1u );

		check( pass.getName() == "1C_1I" )
		check( pass.getTargets().size() == 1u )
		check( pass.getTargets()[0]->view() == rtv )
		check( pass.getSampled().size() == 1u )
		check( pass.getSampled().begin()->second.attach->view() == inv )
		testEnd()
	}

	TEST( FramePass, RenderPass_1C_2I )
	{
		testBegin( "testRenderPass_1C_2I" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "1C_2I", crg::RunnablePassCreator{} );
		auto rt = graph.createImage( test::createImage( "rt", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		pass.addOutputColourTarget( rtv );

		auto in1 = graph.createImage( test::createImage( "in1", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto inv1 = graph.createView( test::createView( "inv1", in1 ) );
		auto attach1 = crg::Attachment::createDefault( inv1 );
		pass.addInputSampled( attach1, 1u );

		auto in2 = graph.createImage( test::createImage( "in2", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto inv2 = graph.createView( test::createView( "inv2", in2 ) );
		auto attach2 = crg::Attachment::createDefault( inv2 );
		pass.addInputSampled( attach2, 2u );

		check( pass.getName() == "1C_2I" )
		check( pass.getTargets().size() == 1u )
		check( pass.getTargets()[0]->view() == rtv )
		check( pass.getSampled().size() == 2u )
		check( pass.getSampled().begin()->second.attach->view() == inv1 )
		check( pass.getSampled().rbegin()->second.attach->view() == inv2 )
		testEnd()
	}

	TEST( FramePass, RenderPass_2C_1I )
	{
		testBegin( "testRenderPass_2C_1I" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "2C_1I", crg::RunnablePassCreator{} );
		auto rt1 = graph.createImage( test::createImage( "rt1", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv1 = graph.createView( test::createView( "rtv1", rt1 ) );
		pass.addOutputColourTarget( rtv1 );

		auto rt2 = graph.createImage( test::createImage( "rt2", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv2 = graph.createView( test::createView( "rtv2", rt2 ) );
		pass.addOutputColourTarget( rtv2 );

		auto in = graph.createImage( test::createImage( "in", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto inv = graph.createView( test::createView( "inv", in ) );
		auto attach = crg::Attachment::createDefault( inv );
		pass.addInputSampled( attach, 1u );

		check( pass.getName() == "2C_1I" )
		check( pass.getTargets().size() == 2u )
		check( pass.getTargets()[0]->view() == rtv1 )
		check( pass.getTargets()[1]->view() == rtv2 )
		check( pass.getSampled().size() == 1u )
		check( pass.getSampled().begin()->second.attach->view() == inv )
		testEnd()
	}

	TEST( FramePass, RenderPass_2C_2I )
	{
		testBegin( "testRenderPass_2C_2I" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "2C_2I", crg::RunnablePassCreator{} );
		auto rt1 = graph.createImage( test::createImage( "rt1", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv1 = graph.createView( test::createView( "rtv1", rt1 ) );
		pass.addOutputColourTarget( rtv1 );

		auto rt2 = graph.createImage( test::createImage( "rt2", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv2 = graph.createView( test::createView( "rtv2", rt2 ) );
		pass.addOutputColourTarget( rtv2 );

		auto in1 = graph.createImage( test::createImage( "in1", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto inv1 = graph.createView( test::createView( "inv1", in1 ) );
		auto attach1 = crg::Attachment::createDefault( inv1 );
		pass.addInputSampled( attach1, 1u );

		auto in2 = graph.createImage( test::createImage( "in2", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto inv2 = graph.createView( test::createView( "inv2", in2 ) );
		auto attach2 = crg::Attachment::createDefault( inv2 );
		pass.addInputSampled( attach2, 2u );

		check( pass.getName() == "2C_2I" )
		check( pass.getTargets().size() == 2u )
		check( pass.getTargets()[0]->view() == rtv1 )
		check( pass.getTargets()[1]->view() == rtv2 )
		check( pass.getSampled().size() == 2u )
		check( pass.getSampled().begin()->second.attach->view() == inv1 )
		check( pass.getSampled().rbegin()->second.attach->view() == inv2 )
		testEnd()
	}
	
	TEST( FramePass, RenderPass_0C_DS )
	{
		testBegin( "testRenderPass_0C_DS" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "0C_DS", crg::RunnablePassCreator{} );
		auto ds = graph.createImage( test::createImage( "ds", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilTarget( dsv );

		check( pass.getName() == "0C_DS" )
		check( pass.getTargets().size() == 1u )
		check( pass.getTargets()[0]->view() == dsv )
		testEnd()
	}

	TEST( FramePass, RenderPass_1C_DS )
	{
		testBegin( "testRenderPass_1C_DS" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "1C_DS", crg::RunnablePassCreator{} );
		auto rt = graph.createImage( test::createImage( "rt", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		pass.addOutputColourTarget( rtv );

		auto ds = graph.createImage( test::createImage( "ds", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilTarget( dsv );

		check( pass.getName() == "1C_DS" )
		check( pass.getTargets().size() == 2u )
		check( pass.getTargets()[0]->view() == dsv )
		check( pass.getTargets()[1]->view() == rtv )
		testEnd()
	}

	TEST( FramePass, RenderPass_2C_DS )
	{
		testBegin( "testRenderPass_2C_DS" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "2C_DS", crg::RunnablePassCreator{} );
		auto rt1 = graph.createImage( test::createImage( "rt1", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv1 = graph.createView( test::createView( "rtv1", rt1 ) );
		pass.addOutputColourTarget( rtv1 );

		auto rt2 = graph.createImage( test::createImage( "rt2", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv2 = graph.createView( test::createView( "rtv2", rt2 ) );
		pass.addOutputColourTarget( rtv2 );

		auto ds = graph.createImage( test::createImage( "ds", crg::PixelFormat::eD32_SFLOAT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilTarget( dsv );

		check( pass.getName() == "2C_DS" )
		check( pass.getTargets().size() == 3u )
		check( pass.getTargets()[0]->view() == dsv )
		check( pass.getTargets()[1]->view() == rtv1 )
		check( pass.getTargets()[2]->view() == rtv2 )
		testEnd()
	}
	
	TEST( FramePass, RenderPass_0C_1I_DS )
	{
		testBegin( "testRenderPass_0C_1I_DS" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "0C_1I_DS", crg::RunnablePassCreator{} );
		auto in = graph.createImage( test::createImage( "in", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto inv = graph.createView( test::createView( "inv", in ) );
		auto attach = crg::Attachment::createDefault( inv );
		pass.addInputSampled( attach, 1u );

		auto ds = graph.createImage( test::createImage( "ds", crg::PixelFormat::eD32_SFLOAT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilTarget( dsv );

		check( pass.getName() == "0C_1I_DS" )
		check( pass.getTargets().size() == 1u )
		check( pass.getTargets()[0]->view() == dsv )
		check( pass.getSampled().size() == 1u )
		check( pass.getSampled().begin()->second.attach->view() == inv )
		testEnd()
	}

	TEST( FramePass, RenderPass_0C_2I_DS )
	{
		testBegin( "testRenderPass_0C_2I_DS" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "0C_2I_DS", crg::RunnablePassCreator{} );
		auto in1 = graph.createImage( test::createImage( "in1", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto inv1 = graph.createView( test::createView( "inv1", in1 ) );
		auto attach1 = crg::Attachment::createDefault( inv1 );
		pass.addInputSampled( attach1, 1u );

		auto in2 = graph.createImage( test::createImage( "in2", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto inv2 = graph.createView( test::createView( "inv2", in2 ) );
		auto attach2 = crg::Attachment::createDefault( inv2 );
		pass.addInputSampled( attach2, 2u );

		auto ds = graph.createImage( test::createImage( "ds", crg::PixelFormat::eD32_SFLOAT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilTarget( dsv );

		check( pass.getName() == "0C_2I_DS" )
		check( pass.getTargets().size() == 1u )
		check( pass.getTargets()[0]->view() == dsv )
		check( pass.getSampled().size() == 2u )
		check( pass.getSampled().begin()->second.attach->view() == inv1 )
		check( pass.getSampled().rbegin()->second.attach->view() == inv2 )
		testEnd()
	}

	TEST( FramePass, RenderPass_1C_1I_DS )
	{
		testBegin( "testRenderPass_1C_1I_DS" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "1C_1I_DS", crg::RunnablePassCreator{} );
		auto rt = graph.createImage( test::createImage( "rt", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		pass.addOutputColourTarget( rtv );

		auto in = graph.createImage( test::createImage( "in", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto inv = graph.createView( test::createView( "inv", in ) );
		auto attach = crg::Attachment::createDefault( inv );
		pass.addInputSampled( attach, 1u );

		auto ds = graph.createImage( test::createImage( "ds", crg::PixelFormat::eD32_SFLOAT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilTarget( dsv );

		check( pass.getName() == "1C_1I_DS" )
		check( pass.getTargets().size() == 2u )
		check( pass.getTargets()[0]->view() == dsv )
		check( pass.getTargets()[1]->view() == rtv )
		check( pass.getSampled().size() == 1u )
		check( pass.getSampled().begin()->second.attach->view() == inv )
		testEnd()
	}

	TEST( FramePass, RenderPass_1C_2I_DS )
	{
		testBegin( "testRenderPass_1C_2I_DS" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "1C_2I_DS", crg::RunnablePassCreator{} );
		auto rt = graph.createImage( test::createImage( "rt", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv = graph.createView( test::createView( "rtv", rt ) );
		pass.addOutputColourTarget( rtv );

		auto in1 = graph.createImage( test::createImage( "in1", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto inv1 = graph.createView( test::createView( "inv1", in1 ) );
		auto attach1 = crg::Attachment::createDefault( inv1 );
		pass.addInputSampled( attach1, 1u );

		auto in2 = graph.createImage( test::createImage( "in2", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto inv2 = graph.createView( test::createView( "inv2", in2 ) );
		auto attach2 = crg::Attachment::createDefault( inv2 );
		pass.addInputSampled( attach2, 2u );

		auto ds = graph.createImage( test::createImage( "ds", crg::PixelFormat::eD32_SFLOAT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilTarget( dsv );

		check( pass.getName() == "1C_2I_DS" )
		check( pass.getTargets().size() == 2u )
		check( pass.getTargets()[0]->view() == dsv )
		check( pass.getTargets()[1]->view() == rtv )
		check( pass.getSampled().size() == 2u )
		check( pass.getSampled().begin()->second.attach->view() == inv1 )
		check( pass.getSampled().rbegin()->second.attach->view() == inv2 )
		testEnd()
	}

	TEST( FramePass, RenderPass_2C_1I_DS )
	{
		testBegin( "testRenderPass_2C_1I_DS" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "2C_1I_DS", crg::RunnablePassCreator{} );
		auto rt1 = graph.createImage( test::createImage( "rt1", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv1 = graph.createView( test::createView( "rtv1", rt1 ) );
		pass.addOutputColourTarget( rtv1 );

		auto rt2 = graph.createImage( test::createImage( "rt2", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv2 = graph.createView( test::createView( "rtv2", rt2 ) );
		pass.addOutputColourTarget( rtv2 );

		auto in = graph.createImage( test::createImage( "in", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto inv = graph.createView( test::createView( "inv", in ) );
		auto attach = crg::Attachment::createDefault( inv );
		pass.addInputSampled( attach, 1u );

		auto ds = graph.createImage( test::createImage( "ds", crg::PixelFormat::eD32_SFLOAT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilTarget( dsv );

		check( pass.getName() == "2C_1I_DS" )
		check( pass.getTargets().size() == 3u )
		check( pass.getTargets()[0]->view() == dsv )
		check( pass.getTargets()[1]->view() == rtv1 )
		check( pass.getTargets()[2]->view() == rtv2 )
		check( pass.getSampled().size() == 1u )
		check( pass.getSampled().begin()->second.attach->view() == inv )
		testEnd()
	}

	TEST( FramePass, RenderPass_2C_2I_DS )
	{
		testBegin( "testRenderPass_2C_2I_DS" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto & pass = graph.createPass( "2C_2I_DS", crg::RunnablePassCreator{} );
		auto rt1 = graph.createImage( test::createImage( "rt1", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv1 = graph.createView( test::createView( "rtv1", rt1 ) );
		pass.addOutputColourTarget( rtv1 );

		auto rt2 = graph.createImage( test::createImage( "rt2", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto rtv2 = graph.createView( test::createView( "rtv2", rt2 ) );
		pass.addOutputColourTarget( rtv2 );

		auto in1 = graph.createImage( test::createImage( "in1", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto inv1 = graph.createView( test::createView( "inv1", in1 ) );
		auto attach1 = crg::Attachment::createDefault( inv1 );
		pass.addInputSampled( attach1, 1u );

		auto in2 = graph.createImage( test::createImage( "in2", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto inv2 = graph.createView( test::createView( "inv2", in2 ) );
		auto attach2 = crg::Attachment::createDefault( inv2 );
		pass.addInputSampled( attach2, 2u );

		auto ds = graph.createImage( test::createImage( "ds", crg::PixelFormat::eD32_SFLOAT ) );
		auto dsv = graph.createView( test::createView( "dsv", ds ) );
		pass.addOutputDepthStencilTarget( dsv );

		check( pass.getName() == "2C_2I_DS" )
		check( pass.getTargets().size() == 3u )
		check( pass.getTargets()[0]->view() == dsv )
		check( pass.getTargets()[1]->view() == rtv1 )
		check( pass.getTargets()[2]->view() == rtv2 )
		check( pass.getSampled().size() == 2u )
		check( pass.getSampled().begin()->second.attach->view() == inv1 )
		check( pass.getSampled().rbegin()->second.attach->view() == inv2 )
		testEnd()
	}
}

testSuiteMain()
