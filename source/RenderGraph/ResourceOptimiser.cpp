/*
This file belongs to FrameGraph.
See LICENSE file in root folder.
*/
#include "ResourceOptimiser.hpp"

#include "RenderGraph/AttachmentTransition.hpp"
#include "RenderGraph/Exception.hpp"
#include "RenderGraph/FramePass.hpp"
#include "RenderGraph/ImageData.hpp"

#include <algorithm>
#include <cassert>

namespace crg
{
	namespace builder
	{
		namespace resopt
		{
			//static void markAttachment( Attachment & attachment
			//	, Attachment::Flag toMark )
			//{
			//	if ( attachment.hasFlag( toMark ) )
			//	{
			//		attachment.setUnique();
			//	}
			//}
			//
			//static void markAttachment( std::optional< Attachment > & attachment
			//	, Attachment::Flag toMark )
			//{
			//	if ( attachment )
			//	{
			//		markAttachment( *attachment, toMark );
			//	}
			//}
			//
			//static void markAttachments( AttachmentArray & attachments
			//	, Attachment::Flag toMark )
			//{
			//	for ( auto attachment : attachments )
			//	{
			//		markAttachment( attachment, toMark );
			//	}
			//}
			//
			//static void markPassAttachments( FramePass & pass
			//	, Attachment::Flag toMark )
			//{
			//	markAttachments( pass.sampled, toMark );
			//	markAttachments( pass.colourInOuts, toMark );
			//	markAttachment( pass.depthStencilInOut, toMark );
			//}

			//static void markPassesAttachments( FramePassSet & passes
			//	, Attachment::Flag toMark )
			//{
			//	for ( auto pass : passes )
			//	{
			//		markPassAttachments( *pass, toMark );
			//	}
			//}

			template< typename DataT >
			using IdDataMap = std::map< DataT *, std::vector< Id< DataT > > >;

			template< typename DataT >
			static IdDataMap< DataT > sortPerData( IdDataOwnerCont< DataT > const & images )
			{
				IdDataMap< DataT > result;
				return result;
			}
		}

		ImageIdAliasMap optimiseImages( ImageIdDataOwnerCont const & images
			, FramePassDependencies const & dependencies
			, RootNode const & root )
		{
			ImageIdAliasMap result;
			// We mark root and leaf nodes images as non mergeable.
			auto roots = retrieveRoots( dependencies );
			auto leaves = retrieveLeafs( dependencies );
			//resopt::markPassesAttachments( roots, Attachment::Flag::Output );
			//resopt::markPassesAttachments( leaves, Attachment::Flag::Input );

			// We'll need to see which image we can merge, given their ImageData,
			// and their flags.
			resopt::sortPerData( images );

			return ImageIdAliasMap{};
		}

		ImageViewIdAliasMap optimiseImageViews( ImageViewIdDataOwnerCont const & imageViews
			, FramePassDependencies const & dependencies
			, RootNode const & root )
		{
			ImageViewIdAliasMap result;
			return result;
		}
	}
}
