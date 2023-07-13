#include "loglevels.h"


#ifndef DEFAULT_LOG_LEVEL
  #define DEFAULT_LOG_LEVEL LOG_VERBOSE
#endif

// Global configuration variable for all LOG?() macros.
int g_runtime_log_level = DEFAULT_LOG_LEVEL;

