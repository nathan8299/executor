/* Copyright 1995 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

#include "rsys/common.h"

#include "FontMgr.h"

using namespace Executor;

static bool outline_preferred_p = false;

P1(PUBLIC pascal trap, void, SetOutlinePreferred,
   Boolean, _outline_preferred_p)
{
    outline_preferred_p = _outline_preferred_p;
}

P0(PUBLIC pascal trap, Boolean, GetOutlinePreferred)
{
    return outline_preferred_p;
}

P2(PUBLIC pascal trap, Boolean, IsOutline,
   Point, numer, Point, denom)
{
    return false;
}

P9(PUBLIC pascal trap, OSErr, OutlineMetrics,
   int16_t, byte_count, Ptr, text,
   Point, numer, Point, denom,
   int16_t *, y_max, int16_t *, y_min,
   Fixed *, aw_array, Fixed *, lsb_array,
   Rect *, bounds_array)
{
    warning_unimplemented(NULL_STRING);
    /* ### paramErr */
    return -50;
}

static bool preserve_glyph_p = false;

P1(PUBLIC pascal trap, void, SetPreserveGlyph,
   Boolean, _preserve_glyph_p)
{
    preserve_glyph_p = _preserve_glyph_p;
}

P0(PUBLIC pascal trap, Boolean, GetPreserveGlyph)
{
    return preserve_glyph_p;
}

P0(PUBLIC pascal trap, OSErr, FlushFonts)
{
    warning_unimplemented(NULL_STRING);
    /* ### paramErr */
    return -50;
}
