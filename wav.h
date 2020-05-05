/*
** File:	wav.h
**
** Author:	Cody Burrows (cxb2114@rit.edu)
**
** Contributor:
**
** This file contains declarations of helpful functions for making sense of
** wav data.
*/

#ifndef WAV_H
#define WAV_H

#define ERR_UNKNOWN_FMT         0
#define WAV_UNSUPPORTED_RATE   -1

#define WAV_AUDIO_FORMAT_OFFSET     20
#define WAV_AUDIO_FORMAT_PCM        1

#define WAV_NUM_CHANNELS_OFFSET     22
#define WAV_SAMPLE_RATE_OFFSET      24
#define WAV_BITS_PER_SAMPLE_OFFSET  34
#define WAV_DATA_OFFSET             44

/**
  * Return true if the chunk of data is a valid RIFF/WAVE file.
  *
  * data_start: A pointer to the first byte of data.
  * data_end: A pointer to the last byte of data.
  */
bool _wav_valid(char *data_start, char *data_end);

/**
  * Determine if a chunk of data contains playable PCM data.
  *
  * data_start: A pointer to the first byte of data.
  * data_end: A pointer to the last byte of data.
  */
bool _wav_is_playable(char *data_start, char *data_end);

/**
  * Determine the number of channels represented in a chunk of data. If the
  * data format is unknown, ERR_UNKNOWN_FMT will be returned.
  *
  * data_start: A pointer to the first byte of data.
  * data_end: A pointer to the last byte of data.
  */
uint8 _wav_num_channels(char *data_start, char *data_end);

/**
  * Determine the sample rate of a WAV file. The rate will be returned. If
  * the rate is outside supported range of the system, WAV_UNSUPPORTED_RATE
  * will be returned. If the data format is unknown, ERR_UNKNOWN_FMT will be 
  * returned.
  *
  * data_start: A pointer to the first byte of data.
  * data_end: A pointer to the last byte of data.
  */
int32 _wav_sample_rate(char *data_start, char *data_end);

/**
  * Return the number of bits per sample in the audio data. If the chunk of data
  * does not appear to be a RIFF/WAVE file, then ERR_UNKNOWN_FMT will be
  * returned.
  *
  * data_start: A pointer to the first byte of data.
  * data_end: A pointer to the last byte of data.
  */
uint8 _wav_bits_per_sample(char *data_start, char *data_end);

/**
  * Return a pointer to the first audio sample in the WAV file. If the chunk of
  * data does not appear to be a RIFF/WAVE file, NULL will be returned.
  *
  * data_start: A pointer to the first byte of data.
  * data_end: A pointer to the last byte of data.
  */
char *_wav_audio_start(char *data_start, char *data_end);

#endif
