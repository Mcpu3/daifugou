#ifndef _GROUP5_H_
#define _GROUP5_H_

using namespace std;

#include "player.h"
#include "gamestatus.h"

class Group5 : public Player {
public:
    Group5(const char *name = "Group5") : Player(name) {}

public:
    ~Group5() {}

public:
    void ready();

public:
    bool follow(const GameStatus &, CardSet &);

public:
    bool approve(const GameStatus &);
};

#endif