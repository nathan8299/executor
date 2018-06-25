#if !defined(__NOTMAC__)
#define __NOTMAC__
/*
 * Copyright 1990 by Abacus Research and Development, Inc.
 * All rights reserved.
 *
 * Question. What do the things in this header file have in common???
 */

#if !defined(USE_WINDOWS_NOT_MAC_TYPEDEFS_AND_DEFINES)

#include "rsys/commonevt.h"
namespace Executor
{
extern char ROMlib_startdir[];

extern std::string ROMlib_ConfigurationFolder;
extern std::string ROMlib_PublicDirectoryMap;
extern std::string ROMlib_PrivateDirectoryMap;
extern std::string ROMlib_ScreenDumpFile;
extern std::string ROMlib_appname;
#if defined(WIN32)
extern char ROMlib_start_drive;
#endif

extern BOOLEAN ROMlib_shouldalarm(void);

extern void querypointerX(LONGINT *xp, LONGINT *yp, LONGINT *modp);
extern void autorepeatonX(void);
extern void PutScrapX(OSType type, LONGINT length, char *p, int scrap_cnt);
extern LONGINT GetScrapX(OSType type, Handle h);
}
#endif

#endif /* __NOTMAC__ */
