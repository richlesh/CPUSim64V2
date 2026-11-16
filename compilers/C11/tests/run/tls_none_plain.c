/* no threads, no _Thread_local: nothing but the ordinary start-up code */
#include <stdio.h>
int g = 3;
int main(void) { g++; printf("g=%d\n", g); return 0; }
