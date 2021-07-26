#include "DiscordService.h"
#include <stdexcept>

#ifdef CS_ENABLE_DISCORD_RPC
#include <discord_rpc.h>
#endif

using namespace cs;

static DiscordService* _instance;

DiscordService::DiscordService()
{
}

DiscordService::~DiscordService()
{
#ifdef CS_ENABLE_DISCORD_RPC
    Discord_Shutdown();
#endif
    _instance = nullptr;
}

void DiscordService::initialise(const char* applicationId, const char* steamAppId)
{
    if (_initialised)
    {
        throw std::runtime_error("DiscordService already initialised.");
    }

    if (_instance != nullptr)
    {
        throw std::runtime_error("Can not initialise multiple instances of DiscordService.");
    }

    _instance = this;
    _initialised = true;

#ifdef CS_ENABLE_DISCORD_RPC
    DiscordEventHandlers handlers = {};
    handlers.ready = [](const DiscordUser* request) {
        _instance->onReady();
    };
    handlers.disconnected = [](int errorCode, const char* message) {
        _instance->onDisconnected(errorCode, message);
    };
    handlers.errored = [](int errorCode, const char* message) {
        _instance->onErrored(errorCode, message);
    };
    Discord_Initialize(applicationId, &handlers, 1, steamAppId);
#endif
}

void DiscordService::update()
{
    throwIfNotInitialised();
#ifdef CS_ENABLE_DISCORD_RPC
    Discord_RunCallbacks();
#endif
}

void DiscordService::setPresence(const DiscordPresence& presence)
{
    throwIfNotInitialised();

#ifdef CS_ENABLE_DISCORD_RPC
    DiscordRichPresence discordPresence = {};
    discordPresence.largeImageKey = "logo";
    discordPresence.state = presence.state;
    discordPresence.details = presence.details;
    Discord_UpdatePresence(&discordPresence);
#endif
}

void DiscordService::throwIfNotInitialised()
{
    if (!_initialised)
    {
        throw std::runtime_error("DiscordService has not been initialised.");
    }
}
