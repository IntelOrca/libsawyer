#include <discord_rpc.h>
#include <gtest/gtest.h>

constexpr const char* APPLICATION_ID = "378612438200877056";
constexpr const char* STEAM_APP_ID = nullptr;

static void OnReady(const DiscordUser*)
{
}

static void OnDisconnected(int, const char*)
{
}

static void OnErrored(int, const char*)
{
}

TEST(DiscordTests, testInitialise)
{
    DiscordEventHandlers handlers = {};
    handlers.ready = OnReady;
    handlers.disconnected = OnDisconnected;
    handlers.errored = OnErrored;
    Discord_Initialize(APPLICATION_ID, &handlers, 1, STEAM_APP_ID);
}
