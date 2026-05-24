/**
 * @file audio.h
 * @brief PC Speaker and PIT Channel 2 Driver Declarations
 *
 * Provides functions to generate square-wave tones matching game sounds
 * using x86 Programmable Interval Timer (PIT) Channel 2 and the PC speaker hardware.
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

/**
 * @brief Initializes the audio subsystem, making sure the speaker is silent.
 */
void audio_init(void);

/**
 * @brief Plays a continuous square-wave tone on the PC speaker.
 * @param frequency The desired frequency of the tone in Hertz (Hz).
 *                  If 0 is passed, the speaker is silenced.
 */
void audio_play_tone(uint32_t frequency);

/**
 * @brief Silences the PC speaker, stopping any playing tone.
 */
void audio_stop_tone(void);

/**
 * @brief Generates a tone at the specified frequency for a given duration.
 *        Note: This is a synchronous, blocking beep using a calibrated delay loop.
 * @param frequency The frequency of the tone in Hertz (Hz).
 * @param duration_ms The duration of the tone in milliseconds (ms).
 */
void audio_beep(uint32_t frequency, uint32_t duration_ms);

#endif /* AUDIO_H */
