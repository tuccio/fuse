#pragma once

namespace fuse
{

#if UNICODE

	typedef wchar_t char_t;

#define FUSE_WIDE_LITERAL(Literal) L ## Literal
#define FUSE_LITERAL(Literal) FUSE_WIDE_LITERAL(Literal)

#else

	typedef char char_t;
	FUSE_LITERAL(Literal) Literal

#endif

}