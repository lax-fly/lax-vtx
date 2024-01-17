// https://www.team-blacksheep.com/tbs_smartaudio_rev08.pdf

#pragma once

void smartaudioBuildSettingsPacket(void);
int smartaudioProcessSerial(void);
void smartaudioReset(void);

// class SmartAudio
// {
//     static uint8_t rxPacket[64];
//     static uint8_t txPacket[64];
//     uint8_t m_state = 0, in_idx, in_len;
// };
