// Button-controlled LED (in C), now truly standalone, controlling LED and button
// Same as tinkerHaWo35.c but using different pins: pin 23 for LED, pin 24 for button

// Compile: gcc  -o  t tut_button.c
// Run:     sudo ./t

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

// original code based in wiringPi library by Gordon Henderson
// #include "wiringPi.h"

// =======================================================
// Tunables
// PINs (based on BCM numbering)
#define LED 12
#define BUTTON 16
// delay for loop iterations (mainly), in ms
#define DELAY 200
// =======================================================

#ifndef	TRUE
#define	TRUE	(1==1)
#define	FALSE	(1==2)
#endif

#define	PAGE_SIZE		(4*1024)
#define	BLOCK_SIZE		(4*1024)

#define	INPUT			 0
#define	OUTPUT			 1

#define	LOW			 0
#define	HIGH			 1

static volatile unsigned int gpiobase ;
static volatile uint32_t *gpio ;


// Mask for the bottom 64 pins which belong to the Raspberry Pi
//	The others are available for the other devices

#define	PI_GPIO_MASK	(0xFFFFFFC0)

/* ------------------------------------------------------- */

int failure (int fatal, const char *message, ...)
{
  va_list argp ;
  char buffer [1024] ;

  if (!fatal) //  && wiringPiReturnCodes)
    return -1 ;

  va_start (argp, message) ;
  vsnprintf (buffer, 1023, message, argp) ;
  va_end (argp) ;

  fprintf (stderr, "%s", buffer) ;
  exit (EXIT_FAILURE) ;

  return 0 ;
}

/* Main ----------------------------------------------------------------------------- */

int main (void)
{
  int pinLED = LED, pinButton = BUTTON;
  int fSel, shift, pin,  clrOff, setOff, off;
  int   fd ;
  int   j;
  int theValue, thePin;
  unsigned int howLong = DELAY;
  uint32_t res; /* testing only */

  printf ("Raspberry Pi button controlled LED (button in %d, led out %d)\n", BUTTON, LED) ;

  if (geteuid () != 0)
    fprintf (stderr, "setup: Must be root. (Did you forget sudo?)\n") ;

  // -----------------------------------------------------------------------------
  // constants for RPi2
  gpiobase = 0x3F200000 ;

  // -----------------------------------------------------------------------------
  // memory mapping 
  // Open the master /dev/memory device

  if ((fd = open ("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC) ) < 0)
    return failure (FALSE, "setup: Unable to open /dev/mem: %s\n", strerror (errno)) ;

  // GPIO:
  gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, gpiobase) ;
  if ((int32_t)gpio == -1)
    return failure (FALSE, "setup: mmap (GPIO) failed: %s\n", strerror (errno)) ;

  // -----------------------------------------------------------------------------
  // setting the mode

  // controlling LED pin 12
  fSel =  1;    // GPIO 12 lives in register 2 (GPFSEL2)
  shift =  6;  // GPIO 12 sits in slot 3 of register 2, thus shift by 3*3 (3 bits per pin)
  // C version of setting LED
  *(gpio + fSel) = (*(gpio + fSel) & ~(7 << shift)) | (1 << shift) ;  // Sets bits to one = output

  // controlling button pin 24
  fSel =  1;    // GPIO 24 lives in register 2 (GPFSEL2)
  shift =  18;  // GPIO 24 sits in slot 4 of register 3, thus shift by 4*3 (3 bits per pin)
  // C version of mode input for button
  *(gpio + fSel) = *(gpio + fSel) & ~(7 << shift); // Sets bits to one = output

  // -----------------------------------------------------------------------------

  // now, start a loop, listening to pinButton and if set pressed, set pinLED
 fprintf(stderr, "starting loop ...\n");
 for (j=0; j<1000; j++)
  {
    if ((pinButton & 0xFFFFFFC0 /* PI_GPIO_MASK */) == 0)		// On-Board Pin; bottom 64 pins belong to the Pi
      {
	theValue = LOW ;

        if ((*(gpio + 13 /* GPLEV0 */) & (1 << (BUTTON & 31))) != 0)
          theValue = HIGH ;
        else
          theValue = LOW ;
      }
      else
      {
        fprintf(stderr, "only supporting on-board pins\n");          exit(1);
       }


    if ((pinLED & 0xFFFFFFC0 /*PI_GPIO_MASK */) == 0)		
      {
        if (theValue == HIGH) {
	  clrOff = 10; // GPCLR0 for pin 23
          *(gpio + clrOff) = 1 << (LED & 31) ;
        } else {
	  setOff = 7; // GPSET0 for pin 23
          *(gpio + setOff) = 1 << (LED & 31) ;
	}
      }
      else
      {
        fprintf(stderr, "only supporting on-board pins\n");          exit(1);
       }

    // INLINED delay
    {
      struct timespec sleeper, dummy ;

      // fprintf(stderr, "delaying by %d ms ...\n", howLong);
      sleeper.tv_sec  = (time_t)(howLong / 1000) ;
      sleeper.tv_nsec = (long)(howLong % 1000) * 1000000 ;

      nanosleep (&sleeper, &dummy) ;
    }
  }
 // Clean up: write LOW
 *(gpio + 7) = 1 << (12 & 31) ;

 fprintf(stderr, "end main.\n");
}
