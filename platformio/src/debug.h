#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG_EASYCOOL
#define DEBUG_EASYCOOL_PORT Serial

#ifdef DEBUG_EASYCOOL
#ifdef DEBUG_EASYCOOL_PORT
#define DEBUGV_EASYCOOL(...) DEBUG_EASYCOOL_PORT.printf( __VA_ARGS__ )
#endif
#endif

#ifndef DEBUG_EASYCOOL
#define DEBUGV_EASYCOOL(...)
#endif

#endif
