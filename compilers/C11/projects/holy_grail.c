/*
 * holy_grail.c
 *
 * The Bridge of Death scene from Monty Python and the Holy Grail: asks the
 * three questions and casts you into the gorge, unless the answer to the
 * third mentions both an African and a European swallow.  The verdict is
 * printed in blinking red (ANSI escape codes).
 * C11 version of documentation/projects/holy_grail.asm.
 *
 * Author:   Richard Lesh
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define ANSI_RED "\x1b[31m"
#define ANSI_BLINK "\x1b[5m"
#define ANSI_RESET_BLINK "\x1b[25m"
#define ANSI_RESET_COLOR "\x1b[39m"

/* reads a line without its newline */
static void read_line(char *buf, int size)
{
    size_t n;
    if (!fgets(buf, size, stdin)) {
        buf[0] = 0;
        return;
    }
    n = strlen(buf);
    if (n > 0 && buf[n - 1] == '\n') buf[n - 1] = 0;
}

int main(void)
{
    char name[200], quest[200], speed[200];
    int i, african, european;
    fputs("STOP! Who would cross the Bridge of Death must answer me\nthese questions three, ere the other side he see.", stdout);
    putchar('\n');
    putchar('\n');
    fputs("BRIDGEKEEPER: What... is your name? ", stdout);
    read_line(name, sizeof name);
    fputs("BRIDGEKEEPER: What... is your quest? ", stdout);
    read_line(quest, sizeof quest);
    fputs("BRIDGEKEEPER: What... is the air-speed velocity of an unladen swallow? ", stdout);
    read_line(speed, sizeof speed);
    putchar('\n');
    for (i = 0; speed[i]; i++) speed[i] = (char)tolower(speed[i]);
    african = strstr(speed, "african") != NULL;
    european = strstr(speed, "european") != NULL;
    fputs(ANSI_RED, stdout);
    fputs(ANSI_BLINK, stdout);
    if (african && european) fputs("BRIDGEKEEPER: Huh? I-- I don't know that... AAAAARGH!\n\n", stdout);
    else fputs("BRIDGEKEEPER: You are cast into the Gorge of Eternal Peril!\n\n", stdout);
    fputs(ANSI_RESET_BLINK, stdout);
    fputs(ANSI_RESET_COLOR, stdout);
    return 0;
}
