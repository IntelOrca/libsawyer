#include <gtest/gtest.h>
#include <sawyer/DiscordService.h>

constexpr const char* APPLICATION_ID = "378612438200877056";
constexpr const char* STEAM_APP_ID = nullptr;

TEST(DiscordServiceTests, testInitialise)
{
    cs::DiscordService service;
    service.initialise(APPLICATION_ID, STEAM_APP_ID);
}

TEST(DiscordServiceTests, setPresence)
{
    cs::DiscordService service;
    service.initialise(APPLICATION_ID, STEAM_APP_ID);

    cs::DiscordPresence presence;
    presence.state = "test";
    presence.details = "test details";
    service.setPresence(presence);
}

TEST(DiscordServiceTests, testInitialiseTwice)
{
    cs::DiscordService service;
    service.initialise(APPLICATION_ID, STEAM_APP_ID);
    ASSERT_THROW(service.initialise(APPLICATION_ID, STEAM_APP_ID), std::runtime_error);
}

TEST(DiscordServiceTests, testInitialiseMany)
{
    cs::DiscordService serviceA, serviceB;
    serviceA.initialise(APPLICATION_ID, STEAM_APP_ID);
    ASSERT_THROW(serviceB.initialise(APPLICATION_ID, STEAM_APP_ID), std::runtime_error);
}
