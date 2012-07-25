/* Exercise 23-4: Modify the program in Listing 23-5 (ptmr_sigev_signal.c) to use sigwaitinfo() instead
of a signal handler.
 */

/**********************************************************************\
*                Copyright (C) Michael Kerrisk, 2010.                  *
*                                                                      *
* This program is free software. You may use, modify, and redistribute *
* it under the terms of the GNU Affero General Public License as       *
* published by the Free Software Foundation, either version 3 or (at   *
* your option) any later version. This program is distributed without  *
* any warranty. See the file COPYING for details.                      *
\**********************************************************************/

/* ptmr_sigev_signal.c

   This program demonstrates the use of signals as the notification mechanism
   for expirations of a POSIX timer. Each of the program's command-line
   arguments specifies the initial value and interval for a POSIX timer. The
   format of these arguments is defined by the function itimerspecFromStr().

   The program establishes a handler for the timer notification signal, creates
   and arms one timer for each command-line argument, and then pauses. Each
   timer expiration causes the generation of a signal, and, when invoked, the
   signal handler displays information about the timer expiration.

   Kernel support for Linux timers is provided since Linux 2.6. On older
   systems, an incomplete user-space implementation of POSIX timers
   was provided in glibc.
*/
#define _POSIX_C_SOURCE 199309
#include <signal.h>
#include <time.h>
#include "curr_time.h"                  /* Declares currTime() */
#include "itimerspec_from_str.h"        /* Declares itimerspecFromStr() */
#include "tlpi_hdr.h"

#define TIMER_SIG SIGUSR1              /* Our timer notification signal */

int
main(int argc, char *argv[])
{
    struct itimerspec ts;
    struct sigevent   sev;
    timer_t *tidlist;
    int j;
    sigset_t timer_set, blocked_set;
    siginfo_t si;

    if (argc < 2)
        usageErr("%s secs[/nsecs][:int-secs[/int-nsecs]]...\n", argv[0]);

    tidlist = calloc(argc - 1, sizeof(timer_t));
    if (tidlist == NULL)
        errExit("malloc");

    /* Create and start one timer for each command-line argument */

    sev.sigev_notify = SIGEV_SIGNAL;    /* Notify via signal */
    sev.sigev_signo = TIMER_SIG;        /* Notify using this signal */

    for (j = 0; j < argc - 1; j++) {
        itimerspecFromStr(argv[j + 1], &ts);

        sev.sigev_value.sival_ptr = &tidlist[j];
                /* Allows handler to get ID of this timer */

        if (timer_create(CLOCK_REALTIME, &sev, &tidlist[j]) == -1)
            errExit("timer_create");
        printf("Timer ID: %ld (%s)\n", (long) tidlist[j], argv[j + 1]);

        if (timer_settime(tidlist[j], 0, &ts, NULL) == -1)
            errExit("timer_settime");
    }

    sigemptyset(&timer_set);
    sigaddset(&timer_set, TIMER_SIG);

    sigprocmask(SIG_BLOCK, &timer_set, NULL);

    for (;;) {
      if (sigwaitinfo(&timer_set, &si) == -1) {
        errExit("sigwaitinfo\n");
      }
      timer_t *tidptr;

      tidptr = si.si_value.sival_ptr;

      printf("[%s] Got signal %d\n", currTime("%T"), si.si_signo);
      printf("    *sival_ptr         = %ld\n", (long) *tidptr);
      printf("    timer_getoverrun() = %d\n", timer_getoverrun(*tidptr));
    }
}