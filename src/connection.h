#pragma once

#include <memory>
#include <string>
#include <functional>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include "netp.h"

namespace netp {
namespace impl {

enum class ConnectionState {
    Initial,
    Connected,
    Failed,
    Closed
};

} // namespace impl
} // namespace netp 