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
double sine( double x ) {
    double sum = x;

    double x2 = x * x;
    double bottom = 1;
    double top = x;

    for (double n = 3; n < 20; n += 2) {
        bottom *= n * ( n - 1 );
        top *= -1 * x2;
        sum += top / bottom;
    }

    return sum;
}


double get_next_value(void) {
    static double current_radian = 0.0;

    current_radian += TARGET_HZ / (double) AUDIO_HZ * 2 * AUDIO_PI;
    if (current_radian > 2 * AUDIO_PI) {
        current_radian -= 2 * AUDIO_PI;
    }
    // TODO return the sine function of this value
    return sine(current_radian);
}

int mainSB( int argc, char* args ) {

    // loop forever
    for(;;) {
        // convert the sine wave to a uint16 spread over it's bounds
        double value = ( sine( get_next_value() ) + 1.0 ) * 65535.0;
        sb_write( ( uint16 ) value);
    }

    // should never happen
    return 0;
}
