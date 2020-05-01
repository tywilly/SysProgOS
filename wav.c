/*
** File:	wav.c
**
** Author:	Cody Burrows (cxb2114@rit.edu)
**
** Contributor:
**
** Functions in this file implement functionality for basic WAVE file support.
*/

#include "common.h"
#include "cio.h"
#include "wav.h"

// See if a chunk of data is a valid wav file or not
bool _wav_valid(char *data_start, char *data_end) {
    uint32 *ptr = (uint32 *) data_start;
    uint32 *limit = (uint32 *) data_end;

    if (ptr < limit && *ptr == 0x46464952) { 
        // found RIFF header 'R' = 0x52, 'I' = 0x49, 'F' = 0x46, 'F' = 0x46
        ptr += 2; // skip next longword
    } else {
        return false;
    }

    if (ptr < limit && *ptr == 0x45564157) {
        // found WAVE string 'W' = 0x57, 'A' = 0x41, 'V' = 0x56, 'E' = 0x45
        ptr++;
    } else {
        return false;
    }

    if (ptr < limit && *ptr == 0x20746d66) { // 'fmt '
        // probably good enough...
        return true;
    } else {
        return false;
    }
}

// See if a WAV file contains playable PCM audio
bool _wav_is_playable(char *data_start, char *data_end) {
    if (!_wav_valid(data_start, data_end)) {
        return false;
    }

    int32 sample_rate = _wav_sample_rate(data_start, data_end);

    char *fmt = data_start + WAV_AUDIO_FORMAT_OFFSET;
    return fmt < data_end && 
           *fmt == WAV_AUDIO_FORMAT_PCM &&
           _wav_num_channels(data_start, data_end) == 2 &&
           _wav_bits_per_sample(data_start, data_end) == 16 &&
           sample_rate > 0 &&
           sample_rate <= (1 << 16);
}

// Get the number of channels
uint8 _wav_num_channels(char *data_start, char *data_end) {
    if (!_wav_valid(data_start, data_end)) {
        return ERR_UNKNOWN_FMT;
    }

    char *n_channels = data_start + WAV_NUM_CHANNELS_OFFSET;
    if (n_channels < data_end) {
        return (uint8) *n_channels;
    } else {
        return ERR_UNKNOWN_FMT;
    }
}

// Get the sample rate
int32 _wav_sample_rate(char *data_start, char *data_end) {
    if (!_wav_valid(data_start, data_end)) {
        return ERR_UNKNOWN_FMT;
    }

    // flip endianness, and only take 16 bits.
    uint16 sample_rate = 0;
    char *rate = data_start + WAV_SAMPLE_RATE_OFFSET;
    for (uint8 i = 0; i < 4; ++i) {
        char *byte = rate + i;

        if (byte > data_end) {
            return ERR_UNKNOWN_FMT;
        }

        if (i < 2) {
            // take the first two bytes only
            sample_rate |= (((uint16) *byte) << (i * 8));
        } else {
            // make sure there aren't any more bits set...essentially checking
            // if greater than 65,535
            if (*byte != 0) {
                return WAV_UNSUPPORTED_RATE;
            }
        }
    }

    return (int32) sample_rate;
}

// See how many bits per sample
uint8 _wav_bits_per_sample(char *data_start, char *data_end) {
    if (!_wav_valid(data_start, data_end)) {
        return ERR_UNKNOWN_FMT;
    }

    char *bits_per_sample = data_start + WAV_BITS_PER_SAMPLE_OFFSET;
    if (bits_per_sample < data_end) {
        return (uint8) *bits_per_sample;
    } else {
        return ERR_UNKNOWN_FMT;
    }
}

// Get a pointer to the first sample
char *_wav_audio_start(char *data_start, char *data_end) {
    if (!_wav_valid(data_start, data_end)) {
        return ERR_UNKNOWN_FMT;
    }

    char *start = data_start + WAV_DATA_OFFSET;
    if (start < data_end) {
        return start;
    } else {
        return ERR_UNKNOWN_FMT;
    }
}
