#include <iostream>

class GamePlayer {
public:
    static const int NumTurns = 5;
    int scores[NumTurns];

    enum { NumTurnsEnum = 10 };
    int healthValue[NumTurnsEnum];
};

// definition of the static const member variable
const int GamePlayer::NumTurns;

int main()
{
    std::cout << "Number of turns (const): " << GamePlayer::NumTurns << std::endl;

    const int* p = &GamePlayer::NumTurns;
    std::cout << "Number of turns (pointer): " << *p << std::endl;

    std::cout << "Number of turns (enum): " << GamePlayer::NumTurnsEnum << std::endl;

    return 0;
}