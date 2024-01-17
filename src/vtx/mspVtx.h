#pragma once

#include <stdint.h>

void mspQueryFlightController(uint32_t time_ms);
void mspBuildPacket(void);
int mspProcessSerial(void);
int mspUpdate(void);
void mspReset(void);
