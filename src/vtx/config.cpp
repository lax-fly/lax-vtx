#include "Config.h"
#include "main.h"

#define CONF_POLL_INTERVAL 100;
Config g_config;

int Config::load(void)
{
    g_disk.read(0, (uint8_t *)&g_config, sizeof(g_config));

    if (g_config.version != versionEEPROM)
    {
        return loaddefault();
    }
    return 0;
}

int Config::loaddefault(void)
{
    g_config.version = versionEEPROM;
    g_config.vtxMode = Protocol::BUTTON;
    g_config.currFreq = 5800;
    g_config.channel = 27;      // F4
    g_config.freqMode = 0;
    g_config.pitMode = 0;
    g_config.pitmodeInRange = 0;
    g_config.pitmodeOutRange = 0;
    g_config.pitmodeFreq = 5584;
    g_config.currPowerdB = 14;
    g_config.unlocked = 1;
    return 0;
}

int Config::save(void)
{
    g_disk.write(0, (uint8_t *)&g_config, sizeof(g_config));
    led_r.blink(4, 80);
    debug("\n"
          "version:        %d\n"
          "vtxMode:        %d\n"
          "currFreq:       %d\n"
          "channel:        %d\n"
          "freqMode:       %d\n"
          "pitMode:        %d\n"
          "pitmodeInRange: %d\n"
          "pitmodeOutRange:%d\n"
          "pitmodeFreq:    %d\n"
          "currPowerdB:    %d\n"
          "unlocked:       %d\n",
          g_config.version,
          g_config.vtxMode,
          g_config.currFreq,
          g_config.channel,
          g_config.freqMode,
          g_config.pitMode,
          g_config.pitmodeInRange,
          g_config.pitmodeOutRange,
          g_config.pitmodeFreq,
          g_config.currPowerdB,
          g_config.unlocked);
    return 0;
}
