#pragma once

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



#define LOGF(...)
#define LOGE(...)
#define LOGW(...)
#define LOGI(...)
#define LOGV(...)
#define ASSERT


