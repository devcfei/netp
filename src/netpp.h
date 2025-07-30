#pragma once

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <strsafe.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <winuser.h>
#elif __linux__
// Linux-specific includes
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#endif

// libevent
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <event2/thread.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>


// STL
#include <set>
#include <functional>



#include "netp.h"
#include "netpimpl.h"

// Logging macros - enabled in debug builds, disabled in release builds
#ifdef DEBUG_BUILD

#ifdef _WIN32
    // Debug build - logging enabled
    #define LOGF(...) fprintf(stderr, "[FATAL] " __VA_ARGS__)
    #define LOGE(...) fprintf(stderr, "[ERROR] " __VA_ARGS__)
    #define LOGW(...) fprintf(stderr, "[WARN]  " __VA_ARGS__)
    #define LOGI(...) fprintf(stderr, "[INFO]  " __VA_ARGS__)
    #define LOGV(...) fprintf(stderr, "[VERB]  " __VA_ARGS__)
    #define ASSERT(condition) if(!(condition)) { fprintf(stderr, "[ASSERT] Assertion failed: %s, file: %s, line: %d\n", #condition, __FILE__, __LINE__); abort(); }
#else
   // Debug build - logging enabled
   #define LOGF(...) fprintf(stderr, "[FATAL] " __VA_ARGS__); fprintf(stderr, "\n")
   #define LOGE(...) fprintf(stderr, "[ERROR] " __VA_ARGS__); fprintf(stderr, "\n")
   #define LOGW(...) fprintf(stderr, "[WARN]  " __VA_ARGS__); fprintf(stderr, "\n")
   #define LOGI(...) fprintf(stderr, "[INFO]  " __VA_ARGS__); fprintf(stderr, "\n")
   #define LOGV(...) fprintf(stderr, "[VERB]  " __VA_ARGS__); fprintf(stderr, "\n")
   #define ASSERT(condition) if(!(condition)) { fprintf(stderr, "[ASSERT] Assertion failed: %s, file: %s, line: %d\n", #condition, __FILE__, __LINE__); abort(); }

#endif

#else
    // Release build - logging disabled
    #define LOGF(...)
    #define LOGE(...)
    #define LOGW(...)
    #define LOGI(...)
    #define LOGV(...)
    #define ASSERT(condition)
#endif


