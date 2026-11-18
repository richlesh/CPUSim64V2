#include <stdio.h>

typedef struct {
    const char *language;
    const char *greeting;
} Greeting;

int main(void) {
    Greeting greetings[] = {
        { "English",  "Hello, World!" },
        { "Spanish",  "¡Hola, Mundo!" },
        { "French",   "Bonjour, le monde !" },
        { "Greek",    "Γεια σου, Κόσμε!" },
        { "Russian",  "Привет, мир!" },
        { "Japanese", "こんにちは、世界！" },
        { "Chinese",  "你好，世界！" },
    };

    int count = sizeof(greetings) / sizeof(greetings[0]);

    for (int i = 0; i < count; i++) {
        printf("%-10s %s\n", greetings[i].language, greetings[i].greeting);
    }

    return 0;
}