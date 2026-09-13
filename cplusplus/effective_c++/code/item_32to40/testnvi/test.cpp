#include <iostream>

class GameCharacter
{
public:
    int healthValue() const // non-virtual; first called
    {
        std::cout << "GameCharacter::healthValue\n";

        return doHealthValue();
    }

private:
    virtual int doHealthValue() const
    {
        std::cout << "GameCharacter::doHealthValue\n";
        return 100;
    }
};

class EvilBadGuy : public GameCharacter
{
private:
    int doHealthValue() const override
    {
        std::cout << "EvilBadGuy::doHealthValue\n";
        return 80;
    }
};

int main()
{
    EvilBadGuy badGuy;

    GameCharacter *p = &badGuy;

    std::cout << p->healthValue() << '\n';
}