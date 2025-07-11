#include "Common.hpp"

#include <RenderGraph/Attachment.hpp>
#include <RenderGraph/FrameGraph.hpp>
#include <RenderGraph/FramePass.hpp>
#include <RenderGraph/ImageData.hpp>
#include <RenderGraph/ResourceHandler.hpp>

namespace
{
	constexpr crg::SamplerDesc defaultSamplerDesc{};

	void testSampledAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testSampledAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addSampledView( view, 1u );
		require( pass.images.size() == 1u )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( attachment.isInput() )
		check( !attachment.isOutput() )
		check( !attachment.isClearable() )
		check( !attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( !attachment.imageAttach.isDepthAttach() )
		check( !attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Spl" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.view() == view )
		check( attachment.binding == 1u )
		check( attachment.getSamplerDesc() == defaultSamplerDesc )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eShaderReadOnly )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eShaderReadOnly )
		testEnd()
	}

	void testImplicitColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testImplicitColourAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( crg::ImageData{ "Test", crg::ImageCreateFlags::eNone, crg::ImageType::e3D, crg::PixelFormat::eR32G32B32A32_SFLOAT, { 1024, 1024, 1024 }, crg::ImageUsageFlags::eSampled, 1u, 1u } );
		auto range = crg::getVirtualRange( image, crg::ImageViewType::e3D, { crg::ImageAspectFlags::eColor, 0u, 1u, 0u, 1u } );
		check( range.baseArrayLayer == 0 )
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addImplicitColourView( view, crg::ImageLayout::eShaderReadOnly );
		require( pass.images.size() == 1u )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( attachment.isInput() )
		check( !attachment.isOutput() )
		check( !attachment.isClearable() )
		check( attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( !attachment.imageAttach.isDepthAttach() )
		check( !attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Impl" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.view() == view )
		check( attachment.binding == crg::InvalidBindingId )
		check( attachment.getSamplerDesc() == defaultSamplerDesc )
		check( attachment.getImageLayout( false ) == attachment.imageAttach.wantedLayout )
		check( attachment.getImageLayout( true ) == attachment.imageAttach.wantedLayout )
		testEnd()
	}

	void testImplicitDepthAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testImplicitDepthAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eD32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addImplicitDepthView( view, crg::ImageLayout::ePreinitialized );
		require( pass.images.size() == 1u )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( attachment.isInput() )
		check( !attachment.isOutput() )
		check( !attachment.isClearable() )
		check( attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( attachment.imageAttach.hasFlag( crg::ImageAttachment::Flag::Depth ) )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( !attachment.imageAttach.isDepthAttach() )
		check( !attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Impl" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.view() == view )
		check( attachment.binding == crg::InvalidBindingId )
		check( attachment.getSamplerDesc() == defaultSamplerDesc )
		check( attachment.getImageLayout( false ) == attachment.imageAttach.wantedLayout )
		check( attachment.getImageLayout( true ) == attachment.imageAttach.wantedLayout )
		testEnd()
	}

	void testImplicitDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testImplicitDepthStencilAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addImplicitDepthStencilView( view, crg::ImageLayout::eShaderReadOnly );
		require( pass.images.size() == 1u )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( attachment.isInput() )
		check( !attachment.isOutput() )
		check( !attachment.isClearable() )
		check( attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( attachment.imageAttach.hasFlag( crg::ImageAttachment::Flag::Depth ) )
		check( attachment.imageAttach.hasFlag( crg::ImageAttachment::Flag::Stencil ) )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( !attachment.imageAttach.isDepthAttach() )
		check( !attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Impl" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.view() == view )
		check( attachment.binding == crg::InvalidBindingId )
		check( attachment.getSamplerDesc() == defaultSamplerDesc )
		check( attachment.getImageLayout( false ) == attachment.imageAttach.wantedLayout )
		check( attachment.getImageLayout( true ) == attachment.imageAttach.wantedLayout )
		testEnd()
	}

	void testInStorageAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInStorageAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputStorageView( view, 1u );
		require( pass.images.size() == 1u )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( attachment.isInput() )
		check( !attachment.isOutput() )
		check( !attachment.isClearable() )
		check( !attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( !attachment.imageAttach.isDepthAttach() )
		check( !attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/IStr" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.view() == view )
		check( attachment.binding == 1u )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eGeneral )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eGeneral )
		testEnd()
	}

	void testOutStorageAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutStorageAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputStorageView( view, 1u );
		require( pass.images.size() == 1u )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( !attachment.isInput() )
		check( attachment.isOutput() )
		check( !attachment.isClearable() )
		check( !attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( !attachment.imageAttach.isDepthAttach() )
		check( !attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/OStr" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.view() == view )
		check( attachment.binding == 1u )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eGeneral )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eGeneral )
		testEnd()
	}

	void testClearOutStorageAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testClearOutStorageAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addClearableOutputStorageView( view, 1u );
		require( pass.images.size() == 1u )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( !attachment.isInput() )
		check( attachment.isOutput() )
		check( attachment.isClearable() )
		check( !attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( !attachment.imageAttach.isDepthAttach() )
		check( !attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/COStr" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.view() == view )
		check( attachment.binding == 1u )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eGeneral )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eGeneral )
		testEnd()
	}

	void testInOutStorageAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutStorageAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInOutStorageView( view, 1u );
		require( pass.images.size() == 1u )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( attachment.isInput() )
		check( attachment.isOutput() )
		check( !attachment.isClearable() )
		check( !attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( !attachment.imageAttach.isDepthAttach() )
		check( !attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/IOStr" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.view() == view )
		check( attachment.binding == 1u )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eGeneral )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eGeneral )
		testEnd()
	}

	void testInTransferAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInTransferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addTransferInputView( view );
		require( pass.images.size() == 1u )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( attachment.isInput() )
		check( !attachment.isOutput() )
		check( !attachment.isClearable() )
		check( !attachment.isTransitionView() )
		check( attachment.isTransferView() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( !attachment.imageAttach.isDepthAttach() )
		check( !attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/It" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.view() == view )
		check( attachment.binding == crg::InvalidBindingId )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eTransferSrc )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eTransferSrc )
		testEnd()
	}

	void testOutTransferAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutTransferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addTransferOutputView( view );
		require( pass.images.size() == 1u )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( !attachment.isInput() )
		check( attachment.isOutput() )
		check( !attachment.isClearable() )
		check( !attachment.isTransitionView() )
		check( attachment.isTransferView() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( !attachment.imageAttach.isDepthAttach() )
		check( !attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Ot" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.view() == view )
		check( attachment.binding == crg::InvalidBindingId )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eTransferDst )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eTransferDst )
		testEnd()
	}

	void testInOutTransferAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutTransferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addTransferInOutView( view );
		require( pass.images.size() == 1u )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( attachment.isInput() )
		check( attachment.isOutput() )
		check( !attachment.isClearable() )
		check( !attachment.isTransitionView() )
		check( attachment.isTransferView() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( !attachment.imageAttach.isDepthAttach() )
		check( !attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/IOt" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.view() == view )
		check( attachment.binding == crg::InvalidBindingId )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eTransferDst )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eTransferDst )
		testEnd()
	}

	void testInColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInColourAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputColourView( view );
		require( pass.images.size() == 1u )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( attachment.isInput() )
		check( !attachment.isOutput() )
		check( !attachment.isClearable() )
		check( attachment.isColourAttach() )
		check( !attachment.isColourInOutAttach() )
		check( attachment.isColourInputAttach() )
		check( !attachment.isColourOutputAttach() )
		check( !attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( attachment.imageAttach.isColourAttach() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( !attachment.imageAttach.isDepthAttach() )
		check( !attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Ic" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eLoad )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.view() == view )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eColorAttachment )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eColorAttachment )
		testEnd()
	}

	void testOutColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutColourAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputColourView( view );
		require( pass.images.size() == 1u )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( !attachment.isInput() )
		check( attachment.isOutput() )
		check( !attachment.isClearable() )
		check( attachment.isColourAttach() )
		check( !attachment.isColourInOutAttach() )
		check( !attachment.isColourInputAttach() )
		check( attachment.isColourOutputAttach() )
		check( !attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( attachment.imageAttach.isColourAttach() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( !attachment.imageAttach.isDepthAttach() )
		check( !attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Oc" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eClear )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eStore )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.view() == view )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eColorAttachment )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eColorAttachment )
		testEnd()
	}

	void testInOutColourAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutColourAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eR32G32B32A32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInOutColourView( view );
		require( pass.images.size() == 1u )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( attachment.isInput() )
		check( attachment.isOutput() )
		check( !attachment.isClearable() )
		check( attachment.isColourAttach() )
		check( attachment.isColourInOutAttach() )
		check( attachment.isColourInputAttach() )
		check( attachment.isColourOutputAttach() )
		check( !attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( attachment.imageAttach.isColourAttach() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( !attachment.imageAttach.isDepthAttach() )
		check( !attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/IOc" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eLoad )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eStore )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.view() == view )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eColorAttachment )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eColorAttachment )
		testEnd()
	}

	void testInDepthAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInDepthAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eD32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputDepthView( view );
		require( !pass.images.empty() )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( attachment.isInput() )
		check( !attachment.isOutput() )
		check( !attachment.isClearable() )
		check( !attachment.isColourAttach() )
		check( !attachment.isColourInOutAttach() )
		check( !attachment.isColourInputAttach() )
		check( !attachment.isColourOutputAttach() )
		check( !attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( !attachment.imageAttach.isColourAttach() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( attachment.imageAttach.isDepthAttach() )
		check( !attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Id" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eLoad )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.view() == view )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eDepthStencilReadOnly )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eDepthReadOnly )
		testEnd()
	}

	void testOutDepthAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutDepthAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eD32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputDepthView( view );
		require( !pass.images.empty() )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( !attachment.isInput() )
		check( attachment.isOutput() )
		check( !attachment.isClearable() )
		check( !attachment.isColourAttach() )
		check( !attachment.isColourInOutAttach() )
		check( !attachment.isColourInputAttach() )
		check( !attachment.isColourOutputAttach() )
		check( !attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( !attachment.imageAttach.isColourAttach() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( attachment.imageAttach.isDepthAttach() )
		check( !attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Od" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eClear )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eStore )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.view() == view )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eDepthStencilAttachment )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eDepthAttachment )
		testEnd()
	}

	void testInOutDepthAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutDepthAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eD32_SFLOAT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInOutDepthView( view );
		require( !pass.images.empty() )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( attachment.isInput() )
		check( attachment.isOutput() )
		check( !attachment.isClearable() )
		check( !attachment.isColourAttach() )
		check( !attachment.isColourInOutAttach() )
		check( !attachment.isColourInputAttach() )
		check( !attachment.isColourOutputAttach() )
		check( !attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( !attachment.imageAttach.isColourAttach() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( attachment.imageAttach.isDepthAttach() )
		check( !attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/IOd" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eLoad )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eStore )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eDepthStencilAttachment )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eDepthAttachment )
		check( attachment.view() == view )
		testEnd()
	}

	void testInDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInDepthStencilAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputDepthStencilView( view );
		require( !pass.images.empty() )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( attachment.isInput() )
		check( !attachment.isOutput() )
		check( !attachment.isClearable() )
		check( !attachment.isColourAttach() )
		check( !attachment.isColourInOutAttach() )
		check( !attachment.isColourInputAttach() )
		check( !attachment.isColourOutputAttach() )
		check( !attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( !attachment.imageAttach.isColourAttach() )
		check( attachment.imageAttach.isDepthStencilAttach() )
		check( attachment.imageAttach.isDepthAttach() )
		check( attachment.imageAttach.isStencilAttach() )
		check( attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Ids" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eLoad )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eLoad )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.view() == view )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eDepthStencilReadOnly )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eDepthStencilReadOnly )
		testEnd()
	}

	void testOutDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutDepthStencilAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputDepthStencilView( view );
		require( !pass.images.empty() )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( !attachment.isInput() )
		check( attachment.isOutput() )
		check( !attachment.isClearable() )
		check( !attachment.isColourAttach() )
		check( !attachment.isColourInOutAttach() )
		check( !attachment.isColourInputAttach() )
		check( !attachment.isColourOutputAttach() )
		check( !attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( !attachment.imageAttach.isColourAttach() )
		check( attachment.imageAttach.isDepthStencilAttach() )
		check( attachment.imageAttach.isDepthAttach() )
		check( attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Ods" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eClear )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eStore )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eClear )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eStore )
		check( attachment.view() == view )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eDepthStencilAttachment )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eDepthStencilAttachment )
		testEnd()
	}

	void testInOutDepthStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutDepthStencilAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eD32_SFLOAT_S8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInOutDepthStencilView( view );
		require( !pass.images.empty() )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( attachment.isInput() )
		check( attachment.isOutput() )
		check( !attachment.isClearable() )
		check( !attachment.isColourAttach() )
		check( !attachment.isColourInOutAttach() )
		check( !attachment.isColourInputAttach() )
		check( !attachment.isColourOutputAttach() )
		check( !attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( !attachment.imageAttach.isColourAttach() )
		check( attachment.imageAttach.isDepthStencilAttach() )
		check( attachment.imageAttach.isDepthAttach() )
		check( attachment.imageAttach.isStencilAttach() )
		check( attachment.imageAttach.isStencilInputAttach() )
		check( attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/IOds" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eLoad )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eStore )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eLoad )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eStore )
		check( attachment.view() == view )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eDepthStencilAttachment )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eDepthStencilAttachment )
		testEnd()
	}

	void testInStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInStencilAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eS8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInputStencilView( view );
		require( !pass.images.empty() )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( attachment.isInput() )
		check( !attachment.isOutput() )
		check( !attachment.isClearable() )
		check( !attachment.isColourAttach() )
		check( !attachment.isColourInOutAttach() )
		check( !attachment.isColourInputAttach() )
		check( !attachment.isColourOutputAttach() )
		check( !attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( !attachment.imageAttach.isColourAttach() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( !attachment.imageAttach.isDepthAttach() )
		check( attachment.imageAttach.isStencilAttach() )
		check( attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Is" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eLoad )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.view() == view )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eDepthStencilReadOnly )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eStencilReadOnly )
		testEnd()
	}

	void testOutStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutStencilAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eS8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addOutputStencilView( view );
		require( !pass.images.empty() )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( !attachment.isInput() )
		check( attachment.isOutput() )
		check( !attachment.isClearable() )
		check( !attachment.isColourAttach() )
		check( !attachment.isColourInOutAttach() )
		check( !attachment.isColourInputAttach() )
		check( !attachment.isColourOutputAttach() )
		check( !attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( !attachment.imageAttach.isColourAttach() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( !attachment.imageAttach.isDepthAttach() )
		check( attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/Os" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eClear )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eStore )
		check( attachment.view() == view )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eDepthStencilAttachment )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eStencilAttachment )
		testEnd()
	}

	void testInOutStencilAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutStencilAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eS8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		pass.addInOutStencilView( view );
		require( !pass.images.empty() )
		auto const & attachment = pass.images[0];
		check( attachment.isImage() )
		check( attachment.isInput() )
		check( attachment.isOutput() )
		check( !attachment.isClearable() )
		check( !attachment.isColourAttach() )
		check( !attachment.isColourInOutAttach() )
		check( !attachment.isColourInputAttach() )
		check( !attachment.isColourOutputAttach() )
		check( !attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( !attachment.imageAttach.isColourAttach() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( !attachment.imageAttach.isDepthAttach() )
		check( attachment.imageAttach.isStencilAttach() )
		check( attachment.imageAttach.isStencilInputAttach() )
		check( attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.name == pass.getGroupName() + "/" + view.data->name + "/IOs" )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eDontCare )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eDontCare )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eLoad )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eStore )
		check( attachment.view() == view )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eDepthStencilAttachment )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eStencilAttachment )
		testEnd()
	}

	void testImageAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testImageAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		auto image = graph.createImage( test::createImage( "Test", crg::PixelFormat::eS8_UINT ) );
		auto view = graph.createView( test::createView( "Test", image ) );
		auto const attachment = crg::Attachment::createDefault( view );
		check( attachment.isImage() )
		check( !attachment.isInput() )
		check( !attachment.isOutput() )
		check( !attachment.isClearable() )
		check( attachment.isColourAttach() )
		check( !attachment.isColourInOutAttach() )
		check( !attachment.isColourInputAttach() )
		check( !attachment.isColourOutputAttach() )
		check( !attachment.isTransitionView() )
		check( !attachment.isTransferView() )
		check( attachment.imageAttach.isColourAttach() )
		check( !attachment.imageAttach.isDepthStencilAttach() )
		check( !attachment.imageAttach.isDepthAttach() )
		check( !attachment.imageAttach.isStencilAttach() )
		check( !attachment.imageAttach.isStencilInputAttach() )
		check( !attachment.imageAttach.isStencilOutputAttach() )
		check( attachment.getLoadOp() == crg::AttachmentLoadOp::eLoad )
		check( attachment.getStoreOp() == crg::AttachmentStoreOp::eStore )
		check( attachment.getStencilLoadOp() == crg::AttachmentLoadOp::eLoad )
		check( attachment.getStencilStoreOp() == crg::AttachmentStoreOp::eStore )
		check( attachment.view() == view )
		check( attachment.getImageLayout( false ) == crg::ImageLayout::eColorAttachment )
		check( attachment.getImageLayout( true ) == crg::ImageLayout::eColorAttachment )
		testEnd()
	}

	void testImplicitBufferAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testImplicitBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		crg::Buffer buffer{ VkBuffer( 1 ), "buffer" };
		pass.addImplicitBuffer( buffer, 0u, VK_WHOLE_SIZE, {} );
		require( !pass.buffers.empty() )
		auto const & attachment = pass.buffers[0];
		check( attachment.isBuffer() )
		check( attachment.isInput() )
		check( !attachment.isOutput() )
		check( !attachment.isBufferView() )
		check( !attachment.isTransferBuffer() )
		check( !attachment.isClearableBuffer() )
		check( !attachment.isStorageBuffer() )
		check( !attachment.isStorageBufferView() )
		check( !attachment.isUniformBuffer() )
		check( !attachment.isUniformBufferView() )
		check( attachment.isTransitionBuffer() )
		check( !attachment.isTransitionBufferView() )
		check( attachment.name == pass.getGroupName() + "/" + buffer.name + "/ImplB" )
		check( attachment.buffer() == buffer.buffer() )
		check( attachment.bufferAttach.buffer == buffer )
		testEnd()
	}

	void testUniformBufferAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testUniformBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		crg::Buffer buffer{ VkBuffer( 1 ), "buffer" };
		pass.addUniformBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );
		require( !pass.buffers.empty() )
		auto const & attachment = pass.buffers[0];
		check( attachment.isBuffer() )
		check( attachment.isInput() )
		check( !attachment.isOutput() )
		check( !attachment.isBufferView() )
		check( !attachment.isTransferBuffer() )
		check( !attachment.isClearableBuffer() )
		check( !attachment.isStorageBuffer() )
		check( !attachment.isStorageBufferView() )
		check( attachment.isUniformBuffer() )
		check( !attachment.isUniformBufferView() )
		check( !attachment.isTransitionBuffer() )
		check( !attachment.isTransitionBufferView() )
		check( attachment.name == pass.getGroupName() + "/" + buffer.name + "/UB" )
		check( attachment.buffer() == buffer.buffer() )
		check( attachment.bufferAttach.buffer == buffer )
		testEnd()
	}

	void testInputStorageBufferAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInputStorageBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		crg::Buffer buffer{ VkBuffer( 1 ), "buffer" };
		pass.addInputStorageBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );
		require( !pass.buffers.empty() )
		auto const & attachment = pass.buffers[0];
		check( attachment.isBuffer() )
		check( attachment.isInput() )
		check( !attachment.isOutput() )
		check( !attachment.isBufferView() )
		check( !attachment.isTransferBuffer() )
		check( !attachment.isClearableBuffer() )
		check( attachment.isStorageBuffer() )
		check( !attachment.isStorageBufferView() )
		check( !attachment.isUniformBuffer() )
		check( !attachment.isUniformBufferView() )
		check( !attachment.isTransitionBuffer() )
		check( !attachment.isTransitionBufferView() )
		check( attachment.name == pass.getGroupName() + "/" + buffer.name + "/ISB" )
		check( attachment.buffer() == buffer.buffer() )
		check( attachment.bufferAttach.buffer == buffer )
		testEnd()
	}

	void testOutputStorageBufferAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutputStorageBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		crg::Buffer buffer{ VkBuffer( 1 ), "buffer" };
		pass.addOutputStorageBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );
		require( !pass.buffers.empty() )
		auto const & attachment = pass.buffers[0];
		check( attachment.isBuffer() )
		check( !attachment.isInput() )
		check( attachment.isOutput() )
		check( !attachment.isBufferView() )
		check( !attachment.isTransferBuffer() )
		check( !attachment.isClearableBuffer() )
		check( attachment.isStorageBuffer() )
		check( !attachment.isStorageBufferView() )
		check( !attachment.isUniformBuffer() )
		check( !attachment.isUniformBufferView() )
		check( !attachment.isTransitionBuffer() )
		check( !attachment.isTransitionBufferView() )
		check( attachment.name == pass.getGroupName() + "/" + buffer.name + "/OSB" )
		check( attachment.buffer() == buffer.buffer() )
		check( attachment.bufferAttach.buffer == buffer )
		testEnd()
	}

	void testClearableOutputStorageBufferAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testClearableOutputStorageBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		crg::Buffer buffer{ VkBuffer( 1 ), "buffer" };
		pass.addClearableOutputStorageBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );
		require( !pass.buffers.empty() )
		auto const & attachment = pass.buffers[0];
		check( attachment.isBuffer() )
		check( !attachment.isInput() )
		check( attachment.isOutput() )
		check( !attachment.isBufferView() )
		check( !attachment.isTransferBuffer() )
		check( attachment.isClearableBuffer() )
		check( attachment.isStorageBuffer() )
		check( !attachment.isStorageBufferView() )
		check( !attachment.isUniformBuffer() )
		check( !attachment.isUniformBufferView() )
		check( !attachment.isTransitionBuffer() )
		check( !attachment.isTransitionBufferView() )
		check( attachment.name == pass.getGroupName() + "/" + buffer.name + "/OSB" )
		check( attachment.buffer() == buffer.buffer() )
		check( attachment.bufferAttach.buffer == buffer )
		testEnd()
	}

	void testInOutStorageBufferAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutStorageBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		crg::Buffer buffer{ VkBuffer( 1 ), "buffer" };
		pass.addInOutStorageBuffer( buffer, 0u, 0u, VK_WHOLE_SIZE );
		require( !pass.buffers.empty() )
		auto const & attachment = pass.buffers[0];
		check( attachment.isBuffer() )
		check( attachment.isInput() )
		check( attachment.isOutput() )
		check( !attachment.isBufferView() )
		check( !attachment.isTransferBuffer() )
		check( !attachment.isClearableBuffer() )
		check( attachment.isStorageBuffer() )
		check( !attachment.isStorageBufferView() )
		check( !attachment.isUniformBuffer() )
		check( !attachment.isUniformBufferView() )
		check( !attachment.isTransitionBuffer() )
		check( !attachment.isTransitionBufferView() )
		check( attachment.name == pass.getGroupName() + "/" + buffer.name + "/IOSB" )
		check( attachment.buffer() == buffer.buffer() )
		check( attachment.bufferAttach.buffer == buffer )
		testEnd()
	}

	void testImplicitBufferViewAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testImplicitBufferViewAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		crg::Buffer buffer{ VkBuffer( 1 ), "buffer" };
		auto view = VkBufferView( 2 );
		pass.addImplicitBufferView( buffer, view, 0u, VK_WHOLE_SIZE, {} );
		require( !pass.buffers.empty() )
		auto const & attachment = pass.buffers[0];
		check( attachment.isBuffer() )
		check( attachment.isInput() )
		check( !attachment.isOutput() )
		check( attachment.isBufferView() )
		check( !attachment.isTransferBuffer() )
		check( !attachment.isClearableBuffer() )
		check( !attachment.isStorageBuffer() )
		check( !attachment.isStorageBufferView() )
		check( !attachment.isUniformBuffer() )
		check( !attachment.isUniformBufferView() )
		check( attachment.isTransitionBuffer() )
		check( attachment.isTransitionBufferView() )
		check( attachment.name == pass.getGroupName() + "/" + buffer.name + "/ImplBV" )
		check( attachment.buffer() == buffer.buffer() )
		check( attachment.bufferAttach.view == view )
		check( attachment.bufferAttach.buffer == buffer )
		testEnd()
	}

	void testUniformBufferViewAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testUniformBufferViewAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		crg::Buffer buffer{ VkBuffer( 1 ), "buffer" };
		auto view = VkBufferView( 2 );
		pass.addUniformBufferView( buffer, view, 0u, 0u, VK_WHOLE_SIZE );
		require( !pass.buffers.empty() )
		auto const & attachment = pass.buffers[0];
		check( attachment.isBuffer() )
		check( attachment.isInput() )
		check( !attachment.isOutput() )
		check( attachment.isBufferView() )
		check( !attachment.isTransferBuffer() )
		check( !attachment.isClearableBuffer() )
		check( !attachment.isStorageBuffer() )
		check( !attachment.isStorageBufferView() )
		check( attachment.isUniformBuffer() )
		check( attachment.isUniformBufferView() )
		check( !attachment.isTransitionBuffer() )
		check( !attachment.isTransitionBufferView() )
		check( attachment.name == pass.getGroupName() + "/" + buffer.name + "/UBV" )
		check( attachment.buffer() == buffer.buffer() )
		check( attachment.bufferAttach.view == view )
		check( attachment.bufferAttach.buffer == buffer )
		testEnd()
	}

	void testInputStorageBufferViewAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInputStorageBufferViewAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		crg::Buffer buffer{ VkBuffer( 1 ), "buffer" };
		auto view = VkBufferView( 2 );
		pass.addInputStorageBufferView( buffer, view, 0u, 0u, VK_WHOLE_SIZE );
		require( !pass.buffers.empty() )
		auto const & attachment = pass.buffers[0];
		check( attachment.isBuffer() )
		check( attachment.isInput() )
		check( !attachment.isOutput() )
		check( attachment.isBufferView() )
		check( !attachment.isTransferBuffer() )
		check( !attachment.isClearableBuffer() )
		check( attachment.isStorageBuffer() )
		check( attachment.isStorageBufferView() )
		check( !attachment.isUniformBuffer() )
		check( !attachment.isUniformBufferView() )
		check( !attachment.isTransitionBuffer() )
		check( !attachment.isTransitionBufferView() )
		check( attachment.name == pass.getGroupName() + "/" + buffer.name + "/ISBV" )
		check( attachment.buffer() == buffer.buffer() )
		check( attachment.bufferAttach.view == view )
		check( attachment.bufferAttach.buffer == buffer )
		testEnd()
	}

	void testOutputStorageBufferViewAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutputStorageBufferViewAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		crg::Buffer buffer{ VkBuffer( 1 ), "buffer" };
		auto view = VkBufferView( 2 );
		pass.addOutputStorageBufferView( buffer, view, 0u, 0u, VK_WHOLE_SIZE );
		require( !pass.buffers.empty() )
		auto const & attachment = pass.buffers[0];
		check( attachment.isBuffer() )
		check( !attachment.isInput() )
		check( attachment.isOutput() )
		check( attachment.isBufferView() )
		check( !attachment.isTransferBuffer() )
		check( !attachment.isClearableBuffer() )
		check( attachment.isStorageBuffer() )
		check( attachment.isStorageBufferView() )
		check( !attachment.isUniformBuffer() )
		check( !attachment.isUniformBufferView() )
		check( !attachment.isTransitionBuffer() )
		check( !attachment.isTransitionBufferView() )
		check( attachment.name == pass.getGroupName() + "/" + buffer.name + "/OSBV" )
		check( attachment.buffer() == buffer.buffer() )
		check( attachment.bufferAttach.view == view )
		check( attachment.bufferAttach.buffer == buffer )
		testEnd()
	}

	void testClearableOutputStorageBufferViewAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testClearableOutputStorageBufferViewAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		crg::Buffer buffer{ VkBuffer( 1 ), "buffer" };
		auto view = VkBufferView( 2 );
		pass.addClearableOutputStorageBufferView( buffer, view, 0u, 0u, VK_WHOLE_SIZE );
		require( !pass.buffers.empty() )
		auto const & attachment = pass.buffers[0];
		check( attachment.isBuffer() )
		check( !attachment.isInput() )
		check( attachment.isOutput() )
		check( attachment.isBufferView() )
		check( !attachment.isTransferBuffer() )
		check( attachment.isClearableBuffer() )
		check( attachment.isStorageBuffer() )
		check( attachment.isStorageBufferView() )
		check( !attachment.isUniformBuffer() )
		check( !attachment.isUniformBufferView() )
		check( !attachment.isTransitionBuffer() )
		check( !attachment.isTransitionBufferView() )
		check( attachment.name == pass.getGroupName() + "/" + buffer.name + "/OSBV" )
		check( attachment.buffer() == buffer.buffer() )
		check( attachment.bufferAttach.view == view )
		check( attachment.bufferAttach.buffer == buffer )
		testEnd()
	}

	void testInOutStorageBufferViewAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutStorageBufferViewAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		crg::Buffer buffer{ VkBuffer( 1 ), "buffer" };
		auto view = VkBufferView( 2 );
		pass.addInOutStorageBufferView( buffer, view, 0u, 0u, VK_WHOLE_SIZE );
		require( !pass.buffers.empty() )
		auto const & attachment = pass.buffers[0];
		check( attachment.isBuffer() )
		check( attachment.isInput() )
		check( attachment.isOutput() )
		check( attachment.isBufferView() )
		check( !attachment.isTransferBuffer() )
		check( !attachment.isClearableBuffer() )
		check( attachment.isStorageBuffer() )
		check( attachment.isStorageBufferView() )
		check( !attachment.isUniformBuffer() )
		check( !attachment.isUniformBufferView() )
		check( !attachment.isTransitionBuffer() )
		check( !attachment.isTransitionBufferView() )
		check( attachment.name == pass.getGroupName() + "/" + buffer.name + "/IOSBV" )
		check( attachment.buffer() == buffer.buffer() )
		check( attachment.bufferAttach.view == view )
		check( attachment.bufferAttach.buffer == buffer )
		testEnd()
	}

	void testInputTransferBufferAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInputTransferBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		crg::Buffer buffer{ VkBuffer( 1 ), "buffer" };
		pass.addTransferInputBuffer( buffer, 0u, VK_WHOLE_SIZE );
		require( !pass.buffers.empty() )
		auto const & attachment = pass.buffers[0];
		check( attachment.isBuffer() )
		check( attachment.isInput() )
		check( !attachment.isOutput() )
		check( !attachment.isBufferView() )
		check( attachment.isTransferBuffer() )
		check( !attachment.isClearableBuffer() )
		check( !attachment.isStorageBuffer() )
		check( !attachment.isStorageBufferView() )
		check( !attachment.isUniformBuffer() )
		check( !attachment.isUniformBufferView() )
		check( !attachment.isTransitionBuffer() )
		check( !attachment.isTransitionBufferView() )
		check( attachment.name == pass.getGroupName() + "/" + buffer.name + "/ITB" )
		check( attachment.buffer() == buffer.buffer() )
		check( attachment.bufferAttach.buffer == buffer )
		testEnd()
	}

	void testOutputTransferBufferAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testOutputTransferBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		crg::Buffer buffer{ VkBuffer( 1 ), "buffer" };
		pass.addTransferOutputBuffer( buffer, 0u, VK_WHOLE_SIZE );
		require( !pass.buffers.empty() )
		auto const & attachment = pass.buffers[0];
		check( attachment.isBuffer() )
		check( !attachment.isInput() )
		check( attachment.isOutput() )
		check( !attachment.isBufferView() )
		check( attachment.isTransferBuffer() )
		check( !attachment.isClearableBuffer() )
		check( !attachment.isStorageBuffer() )
		check( !attachment.isStorageBufferView() )
		check( !attachment.isUniformBuffer() )
		check( !attachment.isUniformBufferView() )
		check( !attachment.isTransitionBuffer() )
		check( !attachment.isTransitionBufferView() )
		check( attachment.name == pass.getGroupName() + "/" + buffer.name + "/OTB" )
		check( attachment.buffer() == buffer.buffer() )
		check( attachment.bufferAttach.buffer == buffer )
		testEnd()
	}

	void testInOutTransferBufferAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testInOutTransferBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::FramePass & pass = graph.createPass( "test", crg::RunnablePassCreator{} );
		crg::Buffer buffer{ VkBuffer( 1 ), "buffer" };
		pass.addTransferInOutBuffer( buffer, 0u, VK_WHOLE_SIZE );
		require( !pass.buffers.empty() )
		auto const & attachment = pass.buffers[0];
		check( attachment.isBuffer() )
		check( attachment.isInput() )
		check( attachment.isOutput() )
		check( !attachment.isBufferView() )
		check( attachment.isTransferBuffer() )
		check( !attachment.isClearableBuffer() )
		check( !attachment.isStorageBuffer() )
		check( !attachment.isStorageBufferView() )
		check( !attachment.isUniformBuffer() )
		check( !attachment.isUniformBufferView() )
		check( !attachment.isTransitionBuffer() )
		check( !attachment.isTransitionBufferView() )
		check( attachment.name == pass.getGroupName() + "/" + buffer.name + "/IOTB" )
		check( attachment.buffer() == buffer.buffer() )
		check( attachment.bufferAttach.buffer == buffer )
		testEnd()
	}

	void testBufferAttachment( test::TestCounts & testCounts )
	{
		testBegin( "testBufferAttachment" )
		crg::ResourceHandler handler;
		crg::FrameGraph graph{ handler, testCounts.testName };
		crg::Buffer buffer{ VkBuffer( 1 ), "buffer" };
		auto const attachment = crg::Attachment::createDefault( buffer );
		check( attachment.isBuffer() )
		check( !attachment.isInput() )
		check( !attachment.isOutput() )
		check( !attachment.isBufferView() )
		check( !attachment.isTransferBuffer() )
		check( !attachment.isClearableBuffer() )
		check( !attachment.isStorageBuffer() )
		check( !attachment.isStorageBufferView() )
		check( !attachment.isUniformBuffer() )
		check( !attachment.isUniformBufferView() )
		check( !attachment.isTransitionBuffer() )
		check( !attachment.isTransitionBufferView() )
		check( attachment.buffer() == buffer.buffer() )
		check( attachment.bufferAttach.buffer == buffer )
		testEnd()
	}
}

int main( int argc, char ** argv )
{
	testSuiteBegin( "TestAttachment" )
	testSampledAttachment( testCounts );
	testImplicitColourAttachment( testCounts );
	testImplicitDepthAttachment( testCounts );
	testImplicitDepthStencilAttachment( testCounts );
	testInStorageAttachment( testCounts );
	testOutStorageAttachment( testCounts );
	testClearOutStorageAttachment( testCounts );
	testInOutStorageAttachment( testCounts );
	testInTransferAttachment( testCounts );
	testOutTransferAttachment( testCounts );
	testInOutTransferAttachment( testCounts );
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
	testImageAttachment( testCounts );
	testImplicitBufferAttachment( testCounts );
	testUniformBufferAttachment( testCounts );
	testInputStorageBufferAttachment( testCounts );
	testOutputStorageBufferAttachment( testCounts );
	testClearableOutputStorageBufferAttachment( testCounts );
	testInOutStorageBufferAttachment( testCounts );
	testImplicitBufferViewAttachment( testCounts );
	testUniformBufferViewAttachment( testCounts );
	testInputStorageBufferViewAttachment( testCounts );
	testOutputStorageBufferViewAttachment( testCounts );
	testClearableOutputStorageBufferViewAttachment( testCounts );
	testInOutStorageBufferViewAttachment( testCounts );
	testInputTransferBufferAttachment( testCounts );
	testOutputTransferBufferAttachment( testCounts );
	testInOutTransferBufferAttachment( testCounts );
	testBufferAttachment( testCounts );
	testSuiteEnd()
}
