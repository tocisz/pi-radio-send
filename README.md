# pi-radio-send
Send given string by modulating one output pin.

Using real time POSIX clock capability: timer_create, timer_settime, timer_gettime, timer_getoverrun
to created PWM where 2/3 means 1 and 1/3 means 0.
Get overrun function is used to detect delays and repeat transmission when overrun is detected.
