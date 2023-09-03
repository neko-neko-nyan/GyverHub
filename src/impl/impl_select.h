#pragma once
#include "macro.hpp"

#define GHI_IMPL_SELECT

#if GHC_MQTT_IMPL == GHC_IMPL_ASYNC
# include "mqtt/async.h"
#elif GHC_MQTT_IMPL == GHC_IMPL_SYNC
# include "mqtt/sync.h"
#elif GHC_MQTT_IMPL == GHC_IMPL_NATIVE
# include "mqtt/native.h"
#elif GHC_MQTT_IMPL == GHC_IMPL_NONE
class HubMQTT {};
#else
# error GHC_MQTT_IMPL misconfigured
#endif


#if GHC_HTTP_IMPL == GHC_IMPL_ASYNC
# include "http/async.h"
# include "websocket/async.h"
#elif GHC_HTTP_IMPL == GHC_IMPL_SYNC
# include "http/sync.h"
# include "websocket/sync.h"
#elif GHC_HTTP_IMPL == GHC_IMPL_NATIVE
# include "http/native.h"
# include "websocket/native.h"
#elif GHC_HTTP_IMPL == GHC_IMPL_NONE
class HubHTTP {};
class HubWS {};
#else
# error GHC_HTTP_IMPL misconfigured
#endif

#undef GHI_IMPL_SELECT
