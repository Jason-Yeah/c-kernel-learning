#include "home_theater_facade.hpp"
#include <iostream>
#include <string>

int main()
{
    DVDPlayer dvd;
    Projector projector;
    Amplifier amp;
    Screen screen;
    Lights lights;
    PopcornPopper popper;

    HomeTheaterFacade theater(dvd, projector, amp, screen, lights, popper);

    theater.watch_movie("Secret");
    theater.end_movie();

    return 0;
}
