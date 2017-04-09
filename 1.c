#include <stdbool.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>

#include <unistd.h>
#include <wiringPi.h>

#define OUT 14

int main(int argc, char *argv[])
{

    wiringPiSetupGpio();
    pinMode(OUT, OUTPUT);
    digitalWrite(OUT, 1);
    return 0;
}
