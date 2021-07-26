#pragma once

#include <cstdint>
#include <limits>

namespace cs
{
    struct DiscordPresence
    {
        const char* state{};
        const char* details{};
    };

    class DiscordService
    {
    private:
        bool _initialised{};

    public:
        DiscordService();
        virtual ~DiscordService();

        void initialise(const char* applicationId, const char* steamAppId);
        void update();
        void setPresence(const DiscordPresence& presence);

    protected:
        virtual void onReady(){};
        virtual void onDisconnected(int errorCode, const char* message){};
        virtual void onErrored(int errorCode, const char* message){};

    private:
        void throwIfNotInitialised();
    };
}
