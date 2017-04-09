#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

#include <stdbool.h>
#include <wiringPi.h>

#include <errno.h>

#define OUT 14
#define NANO(x) x*1000
#define WIDTH 100
#define GAP 3*WIDTH

#define errExit(msg) { perror(msg); exit(EXIT_FAILURE); }

struct itimerspec newt = {{0, NANO(WIDTH)}, {0, NANO(GAP)}};
struct itimerspec second = {{0,0}, {1,0}};
struct itimerspec zerot = {{0,0}, {0,0}};
struct itimerspec timer2_time = {{0,0}, {0,0}};

short sequence_cnt = 0;
short cnt3 = 0;

unsigned int overrun_cnt = 0;

void timer1_init();
void timer2_init();
void timer1_start(long delay_before, long cycle);
void timer1_stop();
void timer2_sleep(int sec, int nsec);
void timer2_stop();

#define STOP 2
unsigned char start_seq[] = {
  0, 0, 1, 1, 0, 1, STOP
};
unsigned char *seq_ptr = NULL;

unsigned char outbit;
static void
handler(int sig, siginfo_t *si, void *uc)
{
  if (cnt3 == 0) {
    outbit = *(seq_ptr++);
    if (outbit == STOP) {
      timer1_stop();
      timer2_stop();
    }
  }

  switch(cnt3) {
    case 0:
    digitalWrite(OUT, 1);
    break;

    case 1:
    if (!outbit) {
      digitalWrite(OUT, 0);
    }
    break;

    case 2:
    if (outbit) {
      digitalWrite(OUT, 0);
    }
    break;
  }

  cnt3 = (cnt3+1)%3;

  timer_t *tidp = si->si_value.sival_ptr;
  int or = timer_getoverrun(*tidp);
  if (or > 0) {
    ++overrun_cnt;
  }
}

void sigint(int a)
{
    exit(0);
}

void exit_handler(void) {
	digitalWrite(OUT, 0);
}

timer_t timerid;
timer_t timerlong_id;
struct sigevent sev;
struct sigaction sa;

void timer1_init() {
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = handler;
  sigemptyset(&sa.sa_mask);

  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo = SIGRTMIN;
  sev.sigev_value.sival_ptr = &timerid;

  if (sigaction(sev.sigev_signo, &sa, NULL) == -1) {
    errExit("sigaction");
  }

  if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
    errExit("timer_create");
  }
}

void timer2_init() {
  if (timer_create(CLOCK_REALTIME, &sev, &timerlong_id) == -1) {
    errExit("timer_create2");
  }
}

void timer1_start(long delay_before, long cycle) {
  overrun_cnt = 0;
  cnt3 = 0;
  newt.it_value.tv_nsec = NANO(delay_before);
  newt.it_interval.tv_nsec = NANO(cycle);
  if (timer_settime(timerid, 0, &newt, NULL) == -1) {
    errExit("timer_settime");
  }
}

void timer1_stop() {
  if (timer_settime(timerid, 0, &zerot, NULL) == -1) {
    errExit("timer_settime");
  }
}

void timer2_sleep(int sec, int nsec) {
  second.it_value.tv_sec = sec;
  second.it_value.tv_nsec = nsec;
  if (timer_settime(timerlong_id, 0, &second, NULL) == -1) {
    errExit("timer_settime2");
  }
  do {
    timer_gettime(timerlong_id, &timer2_time);
    //printf("%d %d\n", timer2_time.it_value.tv_sec, timer2_time.it_value.tv_nsec);
    // sleep for timeout left
    pselect (0, NULL, NULL, NULL, &timer2_time.it_value, NULL);
  } while(timer2_time.it_value.tv_sec  != 0
      || (timer2_time.it_value.tv_nsec != 0));
}

void timer2_stop() {
  if (timer_settime(timerlong_id, 0, &zerot, NULL) == -1) {
    errExit("timer_settime");
  }
}

unsigned char play_buffer[10000];

unsigned char *
append_sequence(unsigned char *out_seq, unsigned char *in_seq) {
  while (*in_seq != STOP) {
    *(out_seq++) = *(in_seq++);
  }
  return out_seq;
}

unsigned char *
append_char(unsigned char *out_seq, unsigned char c) {
  int i;
  for (i = 0; i < 8; ++i) {
    *(out_seq++) = (c&128) ? 1 : 0;
    c <<= 1;
  }
  return out_seq;
}

void play_sequence(char *message) {
  seq_ptr = append_sequence(play_buffer, start_seq);

  char *cp = message;
  while (*cp) {
    seq_ptr = append_char(seq_ptr, *(cp++));
  }
  *seq_ptr = STOP;

  seq_ptr = play_buffer;

  // start with long high pulse
  digitalWrite(OUT, 1);
  timer2_sleep(0, NANO(GAP));
  digitalWrite(OUT, 0);
  timer1_start(GAP, WIDTH);
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    fputs("Give string to send as an argument.\n", stderr);
    exit(1);
  }
  signal(SIGINT, sigint);
  wiringPiSetupGpio();
  piHiPri(99);

  pinMode(OUT, OUTPUT);
  atexit(exit_handler);

  timer1_init();
  timer2_init();

  int try_count = 5;
  short success;
  do
  {
    play_sequence(argv[1]);
    timer2_sleep(1, 0);
    timer1_stop();
    digitalWrite(OUT, 0);

    success = overrun_cnt == 0;
    if (!success) {
      printf("Overrun count: %d\n", overrun_cnt);
      timer2_sleep(0, NANO(100000));
    }
    --try_count;
  } while (!success && try_count > 0);

  if (success) {
    exit(0);
  } else {
    puts("Maximum number of tries reached. Giving up.");
    exit(2);
  }
}
