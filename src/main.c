#include <stdint.h>
#include <gpib/gpib_user.h>
#include "gpib/gpib_ioctl.h"
#include <gpib/ib.h>
#include "lib/ib_internal.h"

#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <curses.h>

int v749;
int i;
int gpib0;
short int line;
char data[64];
char status = 0;
int ret;
short int srq = 0;
int exit_code = 0;
double accum;
struct timespec last_timestamp, current_timestamp;

char time_in_char[32];
char file_name[512];

char init[3][64] = { "O 1 A", "F 0 R0D2A", "I 1 A" };   // set I, Range 1E-12A, Integration time 10 sec

//char init1[] = "F 0 R0D2A";     // set I, Range 1E-12A, Integration time 10 sec
//char init2[] = "O 1 A";         // SRQ mode
//char init3[] = "I 1 A";         // Input ON



FILE *fd;

int main(void)
{
  initscr();
  cbreak();
  noecho();
  scrollok(stdscr, TRUE);
  nodelay(stdscr, TRUE);

  time_t tdate = time(NULL);
  clock_gettime(CLOCK_REALTIME, &last_timestamp);
  clock_gettime(CLOCK_REALTIME, &current_timestamp);

  strftime(time_in_char, sizeof(time_in_char), "%d_%m_%Y_%H:%M:%S", localtime(&tdate));

  strcat(file_name, "tmpfs/");
  strcat(file_name, time_in_char);
  strcat(file_name, ".log");

  fd = fopen(file_name, "w");

  gpib0 = ibfind("agi");
  v749 = ibfind("v749");
  ibtmo(v749, T30s);


  printw("Sending: interface clear\n");
  wrefresh(stdscr);

  ibsre(gpib0, 1);              // lock device
  ibclr(v749);
  ibwait(v749, CMPL);

  usleep(1500000);

  printw("Sending: trigger\n");
  ibtrg(v749);

  for (i = 0; i < sizeof(init) / sizeof(init[0]); i++)
  {

    printw("Sending: %s - %i\n", init[i], strlen(init[i]));
    wrefresh(stdscr);
    ibwrt(v749, init[i], strlen(init[i]));
    ibwait(v749, CMPL);
    usleep(1500000);

    while (srq != 1 && status != 0x40)
    {
      TestSRQ(gpib0, &srq);     // check SRQ line
      if(srq == 1)
      {
        ibrsp(v749, &status);   // check status byte
        ibtrg(v749);
        ibwait(v749, CMPL);
        if(status != 0x40)
          usleep(1500000);
      }
    }
    srq = 0;
    status = 0x0;
  }



  ibconfig(gpib0, IbcAUTOPOLL, 0);
  ibconfig(gpib0, IbcSPollTime, 8);


  printw("------------------------------------------------------\n");
  printw("Press 'q' to exit program and release measurement unit\n");
  printw("------------------------------------------------------\n");
  wrefresh(stdscr);

  usleep(4000000);


  while (exit_code == 0)
  {
    switch (getch())
    {
    case 'q':
      exit_code++;
      break;
    }


    while (accum < 500)         // synchronize 500 ms. cycle
    {
      clock_gettime(CLOCK_REALTIME, &current_timestamp);
      accum = ((current_timestamp.tv_sec - last_timestamp.tv_sec) + (current_timestamp.tv_nsec - last_timestamp.tv_nsec) / 1E9) * 1000;
    }
    last_timestamp.tv_sec = current_timestamp.tv_sec;
    last_timestamp.tv_nsec = current_timestamp.tv_nsec;
    accum = 0;

    while (srq == 0)
    {


      TestSRQ(gpib0, &srq);     // check SRQ line

      if(srq == 1)
      {
        ibrsp(v749, &status);   // read status byte
        printw("Status: 0x%x  ", status);

        if(status == 0x40)
        {
          ibrd(v749, data, sizeof(data));
          ibtrg(v749);
          ibwait(v749, CMPL);
          printw("Reading: %s", data);

          fprintf(fd, "%s", data);
          fflush(fd);
          syncfs(fileno(fd));

          memset(data, 0, sizeof(data));

        } else
        {
          printw("\n");
          usleep(200000);

        }
        wrefresh(stdscr);
      }
    }
    srq = 0;
  }

  ibsre(gpib0, 0);              // release device
  fclose(fd);
}
