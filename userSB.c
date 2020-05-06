/*
**
** File:	userSB.c
**
** Author:	Zach Jones (ztj3686@rit.edu)

**
** Contributor:
**
** Description:	User-level code for testing the audio SoundBlaster system.
**              This plays the sandstorm sound on loop to the sound card.
*/

#include "userSB.h"
#include "common.h"
#include "ulib.h"
#include "kmem.h"
#include "cio.h"

#define BUFF_SIZE   560000

// took these notes up an octave cause it sounds better
#define f_sharp_note 185.00 / 2
#define g_sharp_note 207.65 / 2
#define b_note       246.94 / 2
#define c_sharp_note 277.18 / 2

// pre-build the notes so it doesn't lagc_sharp_buff
static uint16* f_sharp_buff;
static uint16* g_sharp_buff;
static uint16*       b_buff;
static uint16* c_sharp_buff;
static uint16* silent_buff;


// computes the sign of a radian value.
static double sine( double x ) {
    // transformation that requires less approximation
    if (x > AUDIO_PI) {
        return -sine(x - AUDIO_PI);
    }
    double sum = x;

    double x2 = x * x;
    double bottom = 1;
    double top = x;

    for (double n = 3; n < 8; n += 2) {
        bottom *= n * ( n - 1 );
        top *= -1 * x2;
        sum += top / bottom;
    }

    return sum;
}


static double get_next_value(double hz) {
    static double current_radian = 0.0;

    current_radian += (hz / (double) AUDIO_HZ) * 2 * AUDIO_PI;
    if (current_radian > 2 * AUDIO_PI) {
        current_radian -= 2 * AUDIO_PI;
    }
    return sine(current_radian);
}

// fills a buffer with the buffer size number of samples.
static void fill_buff(uint16* buff_to_fill, double hz) {
    for (int i = 0; i < BUFF_SIZE; i++) {
        double value = ( get_next_value(hz) + 1.0 ) * 65535.0 / 2.0;
        buff_to_fill[i] =  (uint16) value;
    }
}

static int play_sound(uint16* buff_to_play, int division) {
    int count_to_play = BUFF_SIZE / division;
    // wait until we send all the bytes
    int posted = 0;
    do {
        int count = write (CHAN_SB, buff_to_play, count_to_play - posted);
        if (count == 0) {
            sleep(0); // yeild CPU
        } else if (count < 0) {
            // error, no device
            return count;
        }
        posted += count;
    } while (posted < count_to_play);

    // play silence the other half
    posted = 0;
    do {
        int count = write (CHAN_SB, silent_buff, count_to_play - posted);
        if (count == 0) {
            sleep(0); // yeild CPU
        } else if (count < 0) {
            // error, no device
            return count;
        }
        posted += count;
    } while (posted < count_to_play);

    return 0; // to shut up gcc
}

int mainSB( int argc, char* args ) {

    int num_pages = BUFF_SIZE * 2 / PAGE_SIZE + 1;
    f_sharp_buff = _kalloc_page(num_pages);
    g_sharp_buff = _kalloc_page(num_pages);
    b_buff       = _kalloc_page(num_pages);
    c_sharp_buff = _kalloc_page(num_pages);
    silent_buff = _kalloc_page(num_pages);

    // fill the buffers with notes
    fill_buff(f_sharp_buff, f_sharp_note);
    fill_buff(g_sharp_buff, g_sharp_note);
    fill_buff(c_sharp_buff, c_sharp_note);
    fill_buff(b_buff, b_note);
    for (int i = 0; i < BUFF_SIZE; i++) {
        silent_buff[i] = 0;
    }

    // notes are grouped into one quarter note beat
    // play measure 1
    int status = play_sound(g_sharp_buff, 16);
    if (status < 0) {
        // no hardware support, exit
        return 1;
    }
    play_sound(g_sharp_buff, 16);
    play_sound(g_sharp_buff, 16);
    play_sound(g_sharp_buff, 16);

    play_sound(g_sharp_buff, 8);
    play_sound(g_sharp_buff, 16);
    play_sound(g_sharp_buff, 16);

    play_sound(g_sharp_buff, 16);
    play_sound(g_sharp_buff, 16);
    play_sound(g_sharp_buff, 16);
    play_sound(g_sharp_buff, 16);

    play_sound(g_sharp_buff, 8);
    play_sound(c_sharp_buff, 16);
    play_sound(c_sharp_buff, 16);

    // loop forever
    for(;;) {
        // measure 2
        play_sound(c_sharp_buff, 16);
        play_sound(c_sharp_buff, 16);
        play_sound(c_sharp_buff, 16);
        play_sound(c_sharp_buff, 16);

        play_sound(c_sharp_buff, 8);
        play_sound(b_buff, 16);
        play_sound(b_buff, 16);

        play_sound(b_buff, 16);
        play_sound(b_buff, 16);
        play_sound(b_buff, 16);
        play_sound(b_buff, 16);

        play_sound(b_buff, 8);
        play_sound(f_sharp_buff, 16);
        play_sound(f_sharp_buff, 16);

        // measure 3, 4, 5
        for (int i = 3; i <= 5; i++) {
            play_sound(g_sharp_buff, 16);
            play_sound(g_sharp_buff, 16);
            play_sound(g_sharp_buff, 16);
            play_sound(g_sharp_buff, 16);

            play_sound(g_sharp_buff, 8);
            play_sound(g_sharp_buff, 16);
            play_sound(g_sharp_buff, 16);

            play_sound(g_sharp_buff, 16);
            play_sound(g_sharp_buff, 16);
            play_sound(g_sharp_buff, 16);
            play_sound(g_sharp_buff, 16);

            play_sound(g_sharp_buff, 8);
            play_sound(c_sharp_buff, 16);
            play_sound(c_sharp_buff, 16);
        }
    }

    // should never happen
    return 0;
}
