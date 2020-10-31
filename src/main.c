#include "v7-49_read.h"
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <curses.h>

/* Mask definition for GPIB control lines */
#define DAV	0x1
#define NDAC	0x2
#define NRFD	0x4
#define IFC	0x8
#define REN	0x10
#define SRQ	0x20
#define ATNLINE	0x40
#define EOI	0x80

int v749;
int gpib0;
short int line;
char data[13];
int ret;
int exit_code = 0;
double accum;
struct timespec last_timestamp, current_timestamp;

char time_in_char[32];
char file_name[512];

char init1[] = "F 0 R0O1A";     // set I, Range 1E-12A, SRQ mode
char init2[] = "F 0 R0D2A";     // set I, Range 1E-12A, Integration time 10 sec
char init3[] = "F 0 R0I1A";     // set I, Range 1E-12A, Input ON

int fd;

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

  fd = open(file_name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);

  gpib0 = ibfind("agi");
  v749 = ibfind("v749");
  ibtmo(v749, T10s);

  printw("Sending: interface clear\n");
  wrefresh(stdscr);

  ibclr(v749);
  ibwait(v749, CMPL);
  ibsre(gpib0, 1);              // lock device
  sleep(5);


  printw("Sending: %s\n", init1);
  wrefresh(stdscr);
  ibwrt(v749, init1, strlen(init1));
  ibwait(v749, CMPL);

  sleep(15);


  printw("Sending: %s\n", init2);
  wrefresh(stdscr);
  ibwrt(v749, init2, strlen(init2));
  ibwait(v749, CMPL);

  sleep(15);


  printw("Sending: %s\n", init3);
  wrefresh(stdscr);
  ibwrt(v749, init3, strlen(init3));
  ibwait(v749, CMPL);

  sleep(15);


  printw("Sending: trigger\n");
  wrefresh(stdscr);
  ibtrg(v749);
  ibwait(v749, CMPL);


  printw("------------------------------------------------------\n");
  printw("Press 'q' to exit program and release measurement unit\n");
  printw("------------------------------------------------------\n");
  wrefresh(stdscr);


  while (exit_code == 0)
  {
    switch (getch())
    {
    case 'q':
      exit_code++;
      break;
    }

    iblines(gpib0, &line);
    if(((line >> 8) & SRQ) == 0)
    {
      ibrd(v749, data, 12);
      printw("Reading: %s\n", data);

      data[12] = '\n';
      data[13] = '\0';
      write(fd, data, sizeof(data));
      memset(data, 0, sizeof(data));

      ibtrg(v749);

      while (accum <= 500)      // synchronize 500 ms. cycle
      {
        clock_gettime(CLOCK_REALTIME, &current_timestamp);
        accum = ((current_timestamp.tv_sec - last_timestamp.tv_sec) + (current_timestamp.tv_nsec - last_timestamp.tv_nsec) / 1E9) * 1000;
      }
      last_timestamp.tv_sec = current_timestamp.tv_sec;
      last_timestamp.tv_nsec = current_timestamp.tv_nsec;
      accum = 0;
    }
  }

  ibsre(gpib0, 0);              // release device
  close(fd);
}
