#include <iostream>
#include <string>

struct Greeting {
    std::string language;
    std::string text;
};

int main() {
    const Greeting greetings[] = {
        { "English",  "Hello, world!" },
        { "Spanish",  "¡Hola, mundo!" },
        { "French",   "Bonjour, le monde !" },
        { "German",   "Hallo, Welt!" },
        { "Russian",  "Привет, мир!" },
        { "Japanese", "こんにちは、世界！" },
        { "Chinese",  "你好，世界！" },
    };

    for (const Greeting &g : greetings) {
        std::cout << g.language << ": " << g.text << '\n';
    }

    return 0;
}
