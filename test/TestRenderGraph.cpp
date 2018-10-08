#include "Window.hpp"

#include <RenderGraph/RenderGraph.hpp>

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

	void testNoPass( test::AppInstance & inst )
	{
		testBegin( "testNoPass" );
		crg::RenderGraph graph{ *inst.device };
		checkThrow( graph.compile() );

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

		checkNoThrow( graph.add( pass ) );
		checkNoThrow( graph.remove( pass ) );
		checkThrow( graph.compile() );
		testEnd();
	}

	void testOnePass( test::AppInstance & inst )
	{
		testBegin( "testOnePass" );
		crg::RenderGraph graph{ *inst.device };
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

		checkNoThrow( graph.add( pass ) );
		check( graph.compile() );
		testEnd();
	}

	void testDuplicateName( test::AppInstance & inst )
	{
		testBegin( "testDuplicateName" );
		crg::RenderGraph graph{ *inst.device };
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
		checkNoThrow( graph.add( pass ) );
		checkThrow( graph.add( pass ) );
		checkNoThrow( graph.remove( pass ) );
		testEnd();
	}

	void testWrongRemove( test::AppInstance & inst )
	{
		testBegin( "testWrongRemove" );
		crg::RenderGraph graph{ *inst.device };
		auto rt = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv = createView( *rt );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv );
		crg::RenderPass pass1
		{
			"1C",
			{},
			{ rtAttach },
		};
		crg::RenderPass pass2
		{
			"2C",
			{},
			{ rtAttach },
		};
		checkNoThrow( graph.add( pass1 ) );
		checkThrow( graph.remove( pass2 ) );
		checkNoThrow( graph.remove( pass1 ) );
		testEnd();
	}

	void testOneDependency( test::AppInstance & inst )
	{
		testBegin( "testOneDependency" );
		crg::RenderGraph graph{ *inst.device };
		auto rt = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto rtv = createView( *rt );
		auto rtAttach = crg::Attachment::createColour( "RT"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *rtv );
		auto inAttach = crg::Attachment::createInput( "IN"
			, *rtv );
		auto out = createImage( inst, ashes::Format::eR32G32B32A32_SFLOAT );
		auto outv = createView( *rt );
		auto outAttach = crg::Attachment::createColour( "OUT"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *outv );
		crg::RenderPass pass1
		{
			"1C",
			{},
			{ rtAttach },
		};
		crg::RenderPass pass2
		{
			"2C",
			{ inAttach },
			{ outAttach },
		};
		checkNoThrow( graph.add( pass1 ) );
		checkNoThrow( graph.add( pass2 ) );
		check( graph.compile() );
		testEnd();
	}

	void testDeferred( test::AppInstance & inst )
	{
		testBegin( "testDeferred" );
		auto d = createImage( inst, ashes::Format::eD32_SFLOAT );
		auto dv = createView( *d );
		auto p = createImage( inst, ashes::Format::eR32G32B32_SFLOAT );
		auto pv = createView( *p );
		auto n = createImage( inst, ashes::Format::eR32G32B32_SFLOAT );
		auto nv = createView( *n );
		auto c = createImage( inst, ashes::Format::eR32G32B32_SFLOAT );
		auto cv = createView( *c );
		auto l = createImage( inst, ashes::Format::eR32G32B32_SFLOAT );
		auto lv = createView( *l );
		auto dtAttach = crg::Attachment::createDepthStencil( "DepthTarget"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, ashes::AttachmentLoadOp::eDontCare
			, ashes::AttachmentStoreOp::eDontCare
			, *dv );
		auto dsAttach = crg::Attachment::createInput( "DepthSampled"
			, *dv );
		auto ptAttach = crg::Attachment::createColour( "PositionsTarget"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *pv );
		auto psAttach = crg::Attachment::createInput( "PositionsSampled"
			, *pv );
		auto ntAttach = crg::Attachment::createColour( "NormalsTarget"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *nv );
		auto nsAttach = crg::Attachment::createInput( "NormalsSampled"
			, *nv );
		auto ctAttach = crg::Attachment::createColour( "ColoursTarget"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *cv );
		auto csAttach = crg::Attachment::createInput( "ColoursSampled"
			, *cv );
		auto ltAttach = crg::Attachment::createColour( "LightingTarget"
			, ashes::AttachmentLoadOp::eClear
			, ashes::AttachmentStoreOp::eStore
			, *lv );
		crg::RenderPass depthPrepass
		{
			"depthPrepass",
			{},
			{ dtAttach },
		};
		crg::RenderPass geometryPass
		{
			"geometryPass",
			{ dsAttach },
			{ ptAttach, ntAttach, ctAttach },
		};
		crg::RenderPass lightingPass
		{
			"lightingPass",
			{ dsAttach, psAttach, nsAttach, csAttach },
			{ ltAttach },
		};
		crg::RenderGraph graph{ *inst.device };
		checkNoThrow( graph.add( depthPrepass ) );
		checkNoThrow( graph.add( geometryPass ) );
		checkNoThrow( graph.add( lightingPass ) );
		check( graph.compile() );
		testEnd();
	}
}

int main( int argc, char ** argv )
{
	test::AppInstance inst;
	testSuiteBegin( "TestRenderGraph", inst );
	//testNoPass( inst );
	//testOnePass( inst );
	//testDuplicateName( inst );
	//testWrongRemove( inst );
	//testOneDependency( inst );
	testDeferred( inst );
	testSuiteEnd();
}
