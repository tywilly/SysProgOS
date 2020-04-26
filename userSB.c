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


static double get_next_value(void) {
    static double current_radian = 0.0;

    current_radian += TARGET_HZ / (double) AUDIO_HZ * 2 * AUDIO_PI;
    if (current_radian > 2 * AUDIO_PI) {
        current_radian -= 2 * AUDIO_PI;
    }
    return sine(current_radian);
}

static uint16 buff[256];
static int spot;

int mainSB( int argc, char* args ) {
    spot = 0;

    // loop forever
    for(;;) {
        // convert the sine wave to a uint16 spread over it's bounds
        double value = ( sine( get_next_value() ) + 1.0 ) * 65535.0 / 2.0;
        uint16 sample = (uint16) value;
        buff[spot] = sample;
        spot++;
        if (spot == 256) {
            spot = 0;
            // wait until we send all 256 bytes
            int posted = 0;
            do {
                int count = write (CHAN_SB, &buff, 256 - posted);
                if (count == 0) {
                    sleep(0); // yeild CPU
                }
                posted += count;
            } while (posted < 256);
        }
    }

    // should never happen
    return 0;
}
