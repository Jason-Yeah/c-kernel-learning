#pragma once

#include "systems.hpp"
#include <iostream>
#include <string>

class HomeTheaterFacade
{
public:
    HomeTheaterFacade(DVDPlayer &dvd, Projector &proj, Amplifier &amp,
                      Screen &scr, Lights &light, PopcornPopper &pop)
        : dvd_(dvd), projector_(proj), amp_(amp), screen_(scr), lights_(light),
          popper_(pop)
    {
    }

    void watch_movie(const std::string &movie)
    {
        std::cout << "===== 准备观影 =====" << std::endl;
        popper_.TurnOn();
        lights_.Dim(10);
        screen_.Down();
        projector_.TurnOn();
        projector_.SetWideScreen();
        amp_.TurnOn();
        amp_.SetVolume(20);
        dvd_.TurnOn();
        dvd_.Play(movie);
        std::cout << "===== 开始观影！=====" << std::endl << std::endl;
    }

    void end_movie()
    {
        std::cout << "===== 观影结束 =====" << std::endl;
        popper_.TurnOff();
        lights_.Restore();
        screen_.Up();
        projector_.TurnOff();
        amp_.TurnOff();
        dvd_.TurnOff();
        std::cout << "===== 已全部关闭 =====" << std::endl;
    }

private:
    DVDPlayer &dvd_;

    Projector &projector_;

    Amplifier &amp_;

    Screen &screen_;

    Lights &lights_;

    PopcornPopper &popper_;
};
