#pragma once

// Pull in the correct header file
#if defined(__MCUXPRESSO)
// Morpheus target
#include "minGlue-FatFs.h"
#else 
// "Offline" testing
#include "minGlue-posix.h"
#endif
