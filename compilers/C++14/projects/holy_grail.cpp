/*
 * holy_grail.cpp
 *
 * The Bridge of Death scene from Monty Python and the Holy Grail: asks the
 * three questions and casts you into the gorge, unless the answer to the
 * third mentions both an African and a European swallow.  The verdict is
 * printed in blinking red (ANSI escape codes).
 * C++ version of documentation/projects/holy_grail.asm.
 *
 * Author:   Richard Lesh
 */
#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

static const char *const ANSI_RED = "\x1b[31m";
static const char *const ANSI_BLINK = "\x1b[5m";
static const char *const ANSI_RESET_BLINK = "\x1b[25m";
static const char *const ANSI_RESET_COLOR = "\x1b[39m";

// reads a line without its newline; empty at the end of the input
static std::string read_line()
{
    std::string line;
    if (!std::getline(std::cin, line)) line.clear();
    return line;
}

int main()
{
    std::cout << "STOP! Who would cross the Bridge of Death must answer me\n"
                 "these questions three, ere the other side he see.\n\n";
    std::cout << "BRIDGEKEEPER: What... is your name? ";
    std::string name = read_line();
    std::cout << "BRIDGEKEEPER: What... is your quest? ";
    std::string quest = read_line();
    std::cout << "BRIDGEKEEPER: What... is the air-speed velocity of an unladen swallow? ";
    std::string speed = read_line();
    std::cout << '\n';
    std::transform(speed.begin(), speed.end(), speed.begin(), [](char c) { return (char)std::tolower(c); });
    bool african = speed.find("african") != std::string::npos;
    bool european = speed.find("european") != std::string::npos;
    std::cout << ANSI_RED << ANSI_BLINK;
    if (african && european) std::cout << "BRIDGEKEEPER: Huh? I-- I don't know that... AAAAARGH!\n\n";
    else std::cout << "BRIDGEKEEPER: You are cast into the Gorge of Eternal Peril!\n\n";
    std::cout << ANSI_RESET_BLINK << ANSI_RESET_COLOR;
    return 0;
}
