#include "Config.h"
#include "main.h"

#define CONF_POLL_INTERVAL 100;
Config g_config;

static inline void print_config(const Config& config)
{
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
          config.version,
          config.vtxMode,
          config.currFreq,
          config.channel,
          config.freqMode,
          config.pitMode,
          config.pitmodeInRange,
          config.pitmodeOutRange,
          config.pitmodeFreq,
          config.currPowerdB,
          config.unlocked);
};

int Config::load(void)
{
    g_disk.read(0, (uint8_t *)&g_config, sizeof(g_config));

    if (g_config.version != versionEEPROM)
    {
        return loaddefault();
    }
    print_config(*this);
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
    print_config(*this);
    return 0;
}
