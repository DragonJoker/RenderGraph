/*
This file belongs to RenderGraph.
See LICENSE file in root folder.
*/
#include "ResourceOptimiser.hpp"

#include "RenderGraph/Exception.hpp"
#include "RenderGraph/RenderPass.hpp"

#include <algorithm>

namespace crg
{
	namespace builder
	{
		namespace
		{
			void markAttachment( Attachment & attachment
				, Attachment::Flag toMark )
			{
				if ( attachment.hasFlag( toMark ) )
				{
					attachment.setUnique();
				}
			}
			
			void markAttachment( std::optional< Attachment > & attachment
				, Attachment::Flag toMark )
			{
				if ( attachment )
				{
					markAttachment( *attachment, toMark );
				}
			}
			
			void markAttachments( AttachmentArray & attachments
				, Attachment::Flag toMark )
			{
				for ( auto attachment : attachments )
				{
					markAttachment( attachment, toMark );
				}
			}
			
			void markPassAttachments( RenderPass & pass
				, Attachment::Flag toMark )
			{
				markAttachments( pass.sampled, toMark );
				markAttachments( pass.colourInOuts, toMark );
				markAttachment( pass.depthStencilInOut, toMark );
			}

			void markPassesAttachments( RenderPassSet & passes
				, Attachment::Flag toMark )
			{
				for ( auto pass : passes )
				{
					markPassAttachments( *pass, toMark );
				}
			}

			template< typename DataT >
			using IdDataMap = std::map< DataT *, std::vector< Id< DataT > >;

			template< typename DataT >
			IdDataMap< DataT > sortPerData( IdDataOwnerCont< DataT > const & images )
			{
				IdDataMap< DataT >;
			}
		}

		ImageIdAliasMap optimiseImages( ImageIdDataOwnerCont const & images
			, RenderPassPtrArray const & passes
			, RenderPassDependenciesArray const & dependencies
			, AttachmentTransitionArray const & transitions
			, RootNode const & root )
		{
			ImageIdAliasMap result;
			// We mark root and leaf nodes images as non mergeable.
			auto roots = retrieveRoots( passes, dependencies );
			auto leaves = retrieveLeafs( passes, dependencies );
			markPassesAttachments( roots, Attachment::Flag::Output );
			markPassesAttachments( leaves, Attachment::Flag::Input );

			// We'll need to see which image we can merge, given their ImageData,
			// and their flags.
			sortPerData( images );

			return result;
		}

		ImageViewIdAliasMap optimiseImageViews( ImageViewIdDataOwnerCont const & imageViews
			, RenderPassPtrArray const & passes
			, RenderPassDependenciesArray const & dependencies
			, AttachmentTransitionArray const & transitions
			, RootNode const & root )
		{
			ImageViewIdAliasMap result;
			return result;
		}
	}
}
