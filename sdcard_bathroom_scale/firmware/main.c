/*
 * SDCard Bathroom Scale
 *
 * Copyright (C) Jorge Pinto aka Casainho, 2009.
 *
 *   casainho [at] gmail [dot] com
 *     www.casainho.net
 *
 * Released under the GPL Licence, Version 3
 */

#include "lpc210x.h"
#include "system.h"
#include "isrsupport.h"
#include "lcd.h"
#include "timers.h"
#include "ios.h"
#include "spi.h"
#include "./fatfs/diskio.h"
#include "./fatfs/ff.h"
#include "err.h"

#ifndef NULL
#define NULL    0
#endif

/* Global variables */
volatile unsigned long int           back_plane_a,
                                     back_plane_b,
                                     back_plane_c;

volatile unsigned short int timer1_counter = 0;

/*---------------------------------------------------------*/
/* User Provided RTC Function for FatFs module             */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support an RTC.                     */
/* This function is not required in read-only cfg.         */


unsigned long get_fattime ()
{
    /* Pack date and time into a DWORD variable */
    return    ((unsigned long)(2009 - 1980) << 25)
            | ((unsigned long) 01 << 21)
            | ((unsigned long) 01 << 16)
            | ((unsigned long) 0 << 11)
            | ((unsigned long) 0 << 5)
            | ((unsigned long) 0 >> 1);
}

int main (void)
{
    /* Initialize variables */
    volatile float weight = 0;
    unsigned char state = 0;

    FATFS fs;       /* Work area (file system object) for logical drive */
    FIL file;       /* file object */
    FRESULT res;    /* FatFs function common result code */


	/* Initialize the system */
    system_init ();

    /* Initialize the IOs */
    ios_init ();

    /* Initialize the LCD */
    lcd_init ();

    spi_init ();

    /* Initialize the Timer1 */
    timer1_init ();
    enableIRQ ();

    for (;;)
    {
        if (io_is_set(LCD_PIN_13))
        {
            /* Acquire the signals from the LCD input */
            back_plane_c = get_ios ();
            while (io_is_set(LCD_PIN_13)) ;

            timer1_counter = 18;
            while (timer1_counter) ;
            back_plane_b = get_ios ();

            timer1_counter = 35;
            while (timer1_counter) ;
            back_plane_a = get_ios ();

            lcd_send_command (DD_RAM_ADDR); /* LCD set first row */
            weight = get_weight (back_plane_a, back_plane_b, back_plane_c);
            lcd_send_char (' ');
            lcd_send_char (' ');
            lcd_send_char (' ');
            lcd_send_char (' ');
            lcd_send_float (weight, 3, 1);
            lcd_send_char (' ');
            lcd_send_char ('K');
            lcd_send_char ('g');

            state = 0;
        }

        else
        {
            switch (state)
            {
                case 0:
                timer1_counter = 500;
                state = 1;
                break;

                case 1:
                if (!timer1_counter)
                {
                   /* If weight are higher than 40kg, store it on the SD Card */
                    if (weight > 40)
                    {
                        /* Register a work area for logical drive 0 */
                        res = f_mount(0, &fs);
                        if (res)
                            die ("Err mount fs");

                        /* Open source file */
                        res = f_open(&file, "weight.csv", FA_OPEN_ALWAYS | FA_WRITE);
                        if (res)
                            die ("Err create file");

                        /* Move to end of the file to append data */
                        res = f_lseek(&file, file.fsize);

                        /* Write the weight value at end of file with a CSV */
                        res = f_printf(&file, "%d", (int) weight);
                        if (res == EOF)
                            die ("Err f_printf");

                        res = f_printf(&file, "%c", '.');
                        if (res == EOF)
                            die ("Err f_printf");

                        /* Write the weight value at end of file with a CSV */
                        res = f_printf(&file, "%d",
                                ((int) ((weight - ((int) weight)) * 10)));
                        if (res == EOF)
                            die ("Err f_printf");

                        res = f_printf(&file, "%c", ',\r');
                        if (res == EOF)
                            die ("Err f_printf");

                        /* Close the file */
                        res = f_close(&file);
                        if (res)
                            die ("Err close file");

                        /* Unregister a work area before discard it */
                        res = f_mount(0, NULL);
                        if (res)
                            die ("Err unmount fs");

                        lcd_send_command (DD_RAM_ADDR2); /* LCD set first row */
                        lcd_send_string ("  Weight saved  ");
                        weight = 0;
                        timer1_counter = 20000; /* wait 2 seconds */
                        while (timer1_counter) ;
                        break;
                    }

                    timer1_counter = 100;
                    while (timer1_counter) ;
                    lcd_send_command (CLR_DISP);
                }
                break;
            }
        }
    }

    /* Go to idle mode to save power. System leaves idle mode on interrupt. */
    /* UNCOMENT IN THE END - NOT POSSIBLE TO DEBUG WITH IDLE MODE */
#if 0
    system_go_idle ();
    }
#endif
}
