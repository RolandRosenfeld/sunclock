/*
 * bitmaps for sun clock, X11 version; header file
 */

#ifdef	DEFINE_BITS
#define	EXTERN
#else
#define	EXTERN	extern
#endif

#ifdef __STDC__
#define CONST	const
#else
#define CONST
#endif

#define icon_map_width 128
#define icon_map_height 63
EXTERN CONST char icon_map_bits[]
#ifdef	DEFINE_BITS
#include "icon.bits.h"
#else
;
#endif

#define large_map_width 640
#define large_map_height 320
EXTERN CONST char large_map_bits[] 
#ifdef	DEFINE_BITS
#include "large.bits.h"
#else
;
#endif

#undef CONST
#undef EXTERN
