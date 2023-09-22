#include "Common.hpp"

#include <RenderGraph/Attachment.hpp>
#include <RenderGraph/FrameGraph.hpp>
#include <RenderGraph/FramePass.hpp>
#include <RenderGraph/ImageData.hpp>
#include <RenderGraph/ResourceHandler.hpp>

namespace
{
	static constexpr crg::SamplerDesc defaultSamplerDesc{};

	void testSampledAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testSampledAttachment" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addSampledView( view, 1u );
		require( pass.images.size() == 1u );
		auto & attachment = pass.images[0];
		check( attachment.hasFlag( crg::Attachment::Flag::Image ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Input ) );
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Spl" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		check( attachment.binding == 1u );
		check( attachment.image.samplerDesc == defaultSamplerDesc );
		testEnd();
	}

	void testInStorageAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInStorageAttachment" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputStorageView( view, 1u );
		require( pass.images.size() == 1u );
		auto & attachment = pass.images[0];
		check( attachment.hasFlag( crg::Attachment::Flag::Image ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Input ) );
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/IStr" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		check( attachment.binding == 1u );
		testEnd();
	}

	void testOutStorageAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutStorageAttachment" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputStorageView( view, 1u );
		require( pass.images.size() == 1u );
		auto & attachment = pass.images[0];
		check( attachment.hasFlag( crg::Attachment::Flag::Image ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Output ) );
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/OStr" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		check( attachment.binding == 1u );
		testEnd();
	}

	void testInOutStorageAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutStorageAttachment" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInOutStorageView( view, 1u );
		require( pass.images.size() == 1u );
		auto & attachment = pass.images[0];
		check( attachment.hasFlag( crg::Attachment::Flag::Image ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Input ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Output ) );
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/IOStr" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		check( attachment.binding == 1u );
		testEnd();
	}

	void testInColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInColourAttachment" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputColourView( view );
		require( pass.images.size() == 1u );
		auto & attachment = pass.images[0];
		check( attachment.hasFlag( crg::Attachment::Flag::Image ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Input ) );
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Ic" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		testEnd();
	}

	void testOutColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutColourAttachment" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputColourView( view );
		require( pass.images.size() == 1u );
		auto & attachment = pass.images[0];
		check( attachment.hasFlag( crg::Attachment::Flag::Image ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Output ) );
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Oc" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		testEnd();
	}

	void testInOutColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutColourAttachment" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_R32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInOutColourView( view );
		require( pass.images.size() == 1u );
		auto & attachment = pass.images[0];
		check( attachment.hasFlag( crg::Attachment::Flag::Image ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Input ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Output ) );
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/IOc" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		testEnd();
	}

	void testInDepthAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInDepthAttachment" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputDepthView( view );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.hasFlag( crg::Attachment::Flag::Image ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Input ) );
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Id" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		testEnd();
	}

	void testOutDepthAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutDepthAttachment" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputDepthView( view );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.hasFlag( crg::Attachment::Flag::Image ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Output ) );
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Od" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		testEnd();
	}

	void testInOutDepthAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutDepthAttachment" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInOutDepthView( view );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.hasFlag( crg::Attachment::Flag::Image ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Input ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Output ) );
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/IOd" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		testEnd();
	}

	void testInDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInDepthStencilAttachment" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputDepthStencilView( view );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.hasFlag( crg::Attachment::Flag::Image ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Input ) );
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Ids" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		testEnd();
	}

	void testOutDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutDepthStencilAttachment" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputDepthStencilView( view );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.hasFlag( crg::Attachment::Flag::Image ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Output ) );
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Ods" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.view() == view );
		testEnd();
	}

	void testInOutDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutDepthStencilAttachment" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_D32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInOutDepthStencilView( view );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.hasFlag( crg::Attachment::Flag::Image ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Input ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Output ) );
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/IOds" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.view() == view );
		testEnd();
	}

	void testInStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInStencilAttachment" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputStencilView( view );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.hasFlag( crg::Attachment::Flag::Image ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Input ) );
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Is" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.view() == view );
		testEnd();
	}

	void testOutStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutStencilAttachment" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputStencilView( view );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.hasFlag( crg::Attachment::Flag::Image ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Output ) );
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Os" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_CLEAR );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.view() == view );
		testEnd();
	}

	void testInOutStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutStencilAttachment" );
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", VK_FORMAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInOutStencilView( view );
		require( !pass.images.empty() );
		auto & attachment = pass.images[0];
		check( attachment.hasFlag( crg::Attachment::Flag::Image ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Input ) );
		check( attachment.hasFlag( crg::Attachment::Flag::Output ) );
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/IOs" );
		check( attachment.image.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE );
		check( attachment.image.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE );
		check( attachment.image.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD );
		check( attachment.image.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE );
		check( attachment.view() == view );
		testEnd();
	}
}

int main( int argc, char ** argv )
{
	testSuiteBegin( "TestAttachment" );
	testSampledAttachment( testCounts );
	testInStorageAttachment( testCounts );
	testOutStorageAttachment( testCounts );
	testInOutStorageAttachment( testCounts );
	testInColourAttachment( testCounts );
	testOutColourAttachment( testCounts );
	testInOutColourAttachment( testCounts );
	testInDepthAttachment( testCounts );
	testOutDepthAttachment( testCounts );
	testInOutDepthAttachment( testCounts );
	testInDepthStencilAttachment( testCounts );
	testOutDepthStencilAttachment( testCounts );
	testInOutDepthStencilAttachment( testCounts );
	testInStencilAttachment( testCounts );
	testOutStencilAttachment( testCounts );
	testInOutStencilAttachment( testCounts );
	testSuiteEnd();
}
