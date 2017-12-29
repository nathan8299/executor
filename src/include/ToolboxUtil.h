#if !defined(_TOOLBOX_UTIL_H_)
#define _TOOLBOX_UTIL_H_

/*
 * Copyright 1986, 1989, 1990 by Abacus Research and Development, Inc.
 * All rights reserved.
 *

 */

#include "QuickDraw.h"
#include "SANE.h"

namespace Executor
{
#define sysPatListID 0

#define iBeamCursor 1
#define crossCursor 2
#define plusCursor 3
#define watchCursor 4

struct Int64Bit
{
    GUEST_STRUCT;
    GUEST<LONGINT> hiLong;
    GUEST<LONGINT> loLong;
};

#if 1 || !defined(__alpha)
typedef Pattern *PatPtr;
#else /* defined(__alpha) */
typedef char *PatPtr;
// FIXME: #warning "Bad PatPtr def ... 'cause of problems with my gcc mods -- ctm"
#endif /* defined(__alpha) */

typedef GUEST<PatPtr> *PatHandle;

/* DO NOT DELETE THIS LINE */
extern pascal trap Fract C_FracSqrt(Fract x);

extern pascal trap Fract C_FracSin(Fixed x);

extern Fixed FixATan2(LONGINT x, LONGINT y);
extern pascal trap Fixed C_FixAtan2(LONGINT x, LONGINT y);

extern pascal trap Fract C_FracCos(Fixed x);

extern pascal trap Fixed C_FixRatio(INTEGER n, INTEGER d);

extern pascal trap Fixed C_FixMul(Fixed a, Fixed b);

extern pascal trap INTEGER C_FixRound(Fixed x);

extern pascal trap StringHandle C_NewString(StringPtr s);

extern pascal trap void C_SetString(StringHandle h, StringPtr s);

extern pascal trap StringHandle C_GetString(INTEGER i);

extern void GetIndString(StringPtr s, INTEGER sid, INTEGER index);
extern pascal trap LONGINT C_Munger(Handle h, LONGINT off, Ptr p1,
                                    LONGINT len1, Ptr p2, LONGINT len2);
extern pascal trap void C_PackBits(GUEST<Ptr> *sp, GUEST<Ptr> *dp, INTEGER len);

extern pascal trap void C_UnpackBits(GUEST<Ptr> *sp, GUEST<Ptr> *dp, INTEGER len);

extern pascal trap BOOLEAN C_BitTst(Ptr bp, LONGINT bn);

extern pascal trap void C_BitSet(Ptr bp, LONGINT bn);

extern pascal trap void C_BitClr(Ptr bp, LONGINT bn);

extern pascal trap LONGINT C_BitAnd(LONGINT a, LONGINT b);

extern pascal trap LONGINT C_BitOr(LONGINT a, LONGINT b);

extern pascal trap LONGINT C_BitXor(LONGINT a, LONGINT b);

extern pascal trap LONGINT C_BitNot(LONGINT a);

extern pascal trap LONGINT C_BitShift(LONGINT a, INTEGER n);

extern pascal trap INTEGER C_HiWord(LONGINT a);

extern pascal trap INTEGER C_LoWord(LONGINT a);

extern pascal trap void C_LongMul(LONGINT a, LONGINT b, Int64Bit *c);

extern void ScreenRes(GUEST<INTEGER> *hp, GUEST<INTEGER> *vp);
extern pascal trap PatHandle C_GetPattern(INTEGER id);

extern void GetIndPattern(Byte *op, INTEGER plistid, INTEGER index);
extern pascal trap CursHandle C_GetCursor(INTEGER id);

extern pascal trap PicHandle C_GetPicture(INTEGER id);

extern pascal trap LONGINT C_DeltaPoint(Point a, Point b);

extern pascal trap Fixed C_SlopeFromAngle(INTEGER a);

extern pascal trap INTEGER C_AngleFromSlope(Fixed s);

extern pascal trap Fract C_FracMul(Fract x, Fract y);

extern pascal trap Fixed C_FixDiv(Fixed x, Fixed y);

extern pascal trap Fract C_FracDiv(Fract x, Fract y);

extern pascal trap Fixed C_Long2Fix(LONGINT x);

extern pascal trap LONGINT C_Fix2Long(Fixed x);

extern pascal trap Fract C_Fix2Frac(Fixed x);

extern pascal trap Fixed C_Frac2Fix(Fract x);

extern Extended Fix2X(Fixed x);
extern Fixed X2Fix(Extended *xp);
extern Extended Frac2X(Fract x);
extern Fract X2Frac(Extended *xp);
extern pascal trap void C_R_Fix2X(Fixed x, extended80 *ret);
extern pascal trap void C_R_Frac2X(Fract x, extended80 *ret);
extern pascal trap Fixed C_R_X2Fix(extended80 *x);
extern pascal trap Fract C_R_X2Frac(extended80 *x);
}

#endif /* _TOOLBOX_UTIL_H_ */
