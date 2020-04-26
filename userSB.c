/*
**
** File:	userSB.c
**
** Author:	Zach Jones
**
** Contributor:
**
** Description:	User-level code for testing the audio SoundBlaster system.
**              This one sends a single tone of A4 on the piano = 440 hz.
*/

#include "userSB.h"
#include "common.h"
#include "ulib.h"
#include "kmem.h"
#include "cio.h"

#define BUFF_SIZE   480000

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

static void play_sound(uint16* buff_to_play, int division) {
    int count_to_play = BUFF_SIZE / division;
    // wait until we send all the bytes
    int posted = 0;
    do {
        int count = write (CHAN_SB, buff_to_play, count_to_play - posted);
        if (count == 0) {
            sleep(0); // yeild CPU
        }
        posted += count;
    } while (posted < count_to_play);
}

int mainSB( int argc, char* args ) {

    int num_pages = BUFF_SIZE * 2 / PAGE_SIZE + 1;
    f_sharp_buff = _kalloc_page(num_pages);
    g_sharp_buff = _kalloc_page(num_pages);
    b_buff       = _kalloc_page(num_pages);
    c_sharp_buff = _kalloc_page(num_pages);

    __cio_printf("%d\n", num_pages);

    __cio_printf("%x\n", g_sharp_buff);

    // fill the 4 buffers with notes
    fill_buff(f_sharp_buff, f_sharp_note);
    fill_buff(g_sharp_buff, g_sharp_note);
    fill_buff(c_sharp_buff, c_sharp_note);
    fill_buff(b_buff, b_note);

    // loop forever
    for(;;) {
        //play_sound(f_sharp_buff);
        // measure 1 (grouped into a full beat)
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

        // play_sound(b_buff);
        // play_sound(c_sharp_buff);
    }

    // should never happen
    return 0;
}
