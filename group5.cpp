#include <algorithm>
#include <future>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <utility>
#include <vector>

#include "card.h"
#include "cardset.h"
#include "player.h"
#include "gamestatus.h"
#include "group5.h"
#include "dealer.h"
using namespace std;

void Group5::ready() {
    dealer = Group5Dealer();
    dealer.regist(Group5("Me"));
    dealer.regist(Group5("Enemy"));
    dealer.newGame();
    passed = true;
    npass = 0;
    CardSet theDeck = CardSet();
    theDeck.setupDeck();
    for (int i = 0; i < theDeck.size(); i++) {
        if (hand.member(theDeck.at(i))) {
            dealer.player(0).inHand().insert(theDeck.at(i));
            dealer.dupHand(0).insert(theDeck.at(i));
        }
        else {
            dealer.player(1).inHand().insert(theDeck.at(i));
            dealer.dupHand(1).insert(theDeck.at(i));
        }
    }
}

bool Group5::follow(const GameStatus &gstat, CardSet &cs) {
    vector<pair<CardSet, long double>> cardsetsAndCosts;
    const vector<CardSet> followableCardset = getFollowableCardsets(gstat, hand, false);
    if (dealer.playerInTurn().getName() == "Enemy") {
        dealer.nextPlayer();
    }
    vector<future<long double>> costs;
    for (const CardSet &cardSet : followableCardset) {
        cardsetsAndCosts.emplace_back(cardSet, 0.0);
        costs.emplace_back(async(launch::async, [&, this]() { return depthFirstSearch(gstat, dealer, cardSet, passed); }));
    }
    for (int i = 0; i < cardsetsAndCosts.size(); i++) {
        cardsetsAndCosts.at(i).second = costs.at(i).get();
    }

    // シングルスレッド

    // for (future<pair<CardSet, long double>> &cardsetAndCost : cardsetsAndCosts) {
    //     cardsetAndCost.second = beamSearch(dealer, cardsetAndCost.first, passed, npass, 0);
    // }

    sort(cardsetsAndCosts.begin(), cardsetsAndCosts.end(), [](const pair<CardSet, long double> &a, const pair<CardSet, long double> &b) { return a.second < b.second; });
    cs.insert(cardsetsAndCosts.front().first);
    hand.remove(cardsetsAndCosts.front().first);
    return true;
}

bool Group5::approve(const GameStatus &gstat) {
    if ((gstat.playerID[gstat.turnIndex] == getId() && dealer.playerInTurn().getName() == "Enemy")
        || (gstat.playerID[gstat.turnIndex] != getId() && dealer.playerInTurn().getName() == "Me")) {
        dealer.nextPlayer();
    }
    if (passed && dealer.playerInTurnIsLeader()) {
        dealer.clearDiscardPile();
    }
    for (int i = 0; i < gstat.pile.size(); i++) {
        dealer.playerInTurn().inHand().remove(gstat.pile.at(i));
    }
    if (gstat.pile.isEmpty() || !dealer.accept(const_cast<CardSet &>(gstat.pile))) {
        if (!gstat.pile.isEmpty()) {
            dealer.putBackOpened(const_cast<CardSet &>(gstat.pile));
        }
        passed = true;
        npass++;
    }
    else {
        passed = false;
    }
    if (!passed) {
        dealer.setAsLeader();
        npass = 0;
    }
    return true;
}

long double Group5::depthFirstSearch(const GameStatus &gstat, Group5Dealer dealer, CardSet opened, bool passed) {
    if (passed && dealer.playerInTurnIsLeader()) {
        dealer.clearDiscardPile();
    }
    for (int i = 0; i < opened.size(); i++) {
        dealer.playerInTurn().inHand().remove(opened.at(i));
    }
    if (opened.isEmpty() || !dealer.accept(opened)) {
        if (!opened.isEmpty()) {
            dealer.putBackOpened(opened);
        }
        passed = true;
    }
    else {
        passed = false;
    }
    if (dealer.playerInTurn().isEmptyHanded() && dealer.isEmptyHanded(dealer.playerInTurn().getId())) {
        return evaluate(dealer);
    }
    if (!passed) {
        dealer.setAsLeader();
    }
    dealer.nextPlayer();
    vector<pair<CardSet, long double>> cardsetsAndCosts;
    const vector<CardSet> followableCardset = getFollowableCardsets(dealer.gameStatus(), dealer.playerInTurn().inHand(), true);
    for (const CardSet &cardSet : followableCardset) {
        cardsetsAndCosts.emplace_back(cardSet, 0.0);
    }
    for (pair<CardSet, long double> &cardsetAndCost : cardsetsAndCosts) {
        cardsetAndCost.second = depthFirstSearch(gstat, dealer, cardsetAndCost.first, passed);
    }
    if (cardsetsAndCosts.empty()) {
        return evaluate(dealer);
    }
    long double cost = accumulate(cardsetsAndCosts.begin(), cardsetsAndCosts.end(), 0.0, [](const long double &accumulation, const pair<CardSet, long double> &cardsetAndCost) { return accumulation + cardsetAndCost.second; }) / cardsetsAndCosts.size();
    return cost;
}

long double Group5::evaluate(Group5Dealer dealer) {
    long double cost = 0.0;
    for (int i = 0; i < dealer.player(0).inHand().size(); i++) {
        cost += 16.0 - dealer.player(0).inHand().at(i).strength();
    }
    for (int i = 0; i < dealer.player(1).inHand().size(); i++) {
        cost -= 16.0 - dealer.player(1).inHand().at(i).strength();
    }
    return cost;
}

vector<CardSet> Group5::getFollowableCardsets(const GameStatus &gstat, const CardSet &inHand, const bool &withoutNoCard) {
    vector<CardSet> followableCardsets;
    vector<CardSet> bucketsOfCardset(14);
    if (!withoutNoCard) {
        CardSet cardSet;
        followableCardsets.emplace_back(cardSet);
    }
    for (int i = 0; i < inHand.size(); i++) {
        if (!inHand.at(i).isJoker()) {
            bucketsOfCardset.at(inHand.at(i).getRank()).insert(inHand.at(i));
        }
        else {
            bucketsOfCardset.front().insert(inHand.at(i));
        }
    }
    if (gstat.pile.isEmpty()) {
        for (int i = 1; i < bucketsOfCardset.size(); i++) {
            for (int j = 1; j <= bucketsOfCardset.at(i).size(); j++) {
                CardSet cardSet;
                for (int k = 0; k < j; k++) {
                    cardSet.insert(bucketsOfCardset.at(i).at(k));
                }
                followableCardsets.emplace_back(cardSet);
            }
            if (!bucketsOfCardset.front().isEmpty()) {
                for (int j = 1; j <= min(bucketsOfCardset.at(i).size(), 3); j++) {
                    CardSet cardSet;
                    for (int k = 0; k < j; k++) {
                        cardSet.insert(bucketsOfCardset.at(i).at(k));
                    }
                    cardSet.insert(bucketsOfCardset.front().at(0));
                }
            }
        }
    }
    else {
        for (int i = 1; i < bucketsOfCardset.size(); i++) {
            if (!bucketsOfCardset.at(i).isEmpty()) {
                bool isGreaterThan = false;
                for (int j = 0; j < gstat.pile.size(); j++) {
                    isGreaterThan |= bucketsOfCardset.at(i).at(0).isGreaterThan(gstat.pile.at(j));
                }
                if (isGreaterThan) {
                    if (bucketsOfCardset.at(i).size() >= gstat.pile.size()) {
                        CardSet cardSet;
                        for (int j = 0; j < gstat.pile.size(); j++) {
                            cardSet.insert(bucketsOfCardset.at(i).at(j));
                        }
                        followableCardsets.emplace_back(cardSet);
                    }
                    if (!bucketsOfCardset.front().isEmpty()) {
                        if (gstat.pile.size() >= 2 && bucketsOfCardset.at(i).size() >= gstat.pile.size() - 1) {
                            CardSet cardSet;
                            for (int j = 0; j < gstat.pile.size() - 1; j++) {
                                cardSet.insert(bucketsOfCardset.at(i).at(j));
                            }
                            cardSet.insert(bucketsOfCardset.front().at(0));
                            followableCardsets.emplace_back(cardSet);
                        }
                    }
                }
            }
        }
    }
    if (!bucketsOfCardset.front().isEmpty()) {
        CardSet cardSet;
        cardSet.insert(bucketsOfCardset.front().at(0));
        followableCardsets.emplace_back(cardSet);
    }
    return followableCardsets;
}

int Group5Dealer::verbose = false;

Group5Dealer::Group5Dealer() : players(maxNumOfPlayers), id2player(maxNumOfPlayers), score(maxNumOfPlayers, vector<int>(maxNumOfPlayers + 1)), dupHands(maxNumOfPlayers) {
    numberOfPlayingPlayers = 0;
    numberOfParticipants = 0;
    turn = 0;
    pauper = 0;
    for (int i = 0; i < maxNumOfPlayers; i++){
      for (int j = 0; j <= maxNumOfPlayers; j++) {
        score[i][j] = 0;
      }
      dupHands[i].makeEmpty();
    }
    noMillionaire = true;
    return;
}

bool Group5Dealer::regist(Player pl) {
  //  std::cerr << "### registering of " << pl->playerName() << std::endl;
  //  std::cerr << "nop=" << numberOfParticipants << ",max=" << maxNumOfPlayers << std::endl;
  
  std::cerr << "Registering Player of " << pl.playerName() << " ... ";

    if (numberOfParticipants >= maxNumOfPlayers) {
      std::cerr << "Error: player registration exceeding the limit (" 
                << maxNumOfPlayers << ")." << std::endl;
      return false;
    }
    pl.setId(numberOfParticipants);
    players[numberOfParticipants] = pl;
    id2player[numberOfParticipants] = pl;
    numberOfParticipants++;
    pauper = 0;
    noMillionaire = true;

    std::cerr << "completed successfully." << std::endl;
    return true;
}

Player & Group5Dealer::currentLeader() {
    return players[leaderIndex];
}

bool Group5Dealer::playerInTurnIsLeader() {
    return leaderIndex == turn;
}

void Group5Dealer::newGame() {
    numberOfPlayingPlayers = howManyParticipants();
    if ( !noMillionaire )
        pauper = 0;
    for (int i = 0; i < numberOfParticipants; i++){
      players[i].clearHand();
      //      players[i]->ready();
      dupHands[i].makeEmpty();
    }
    clearDiscardPile();
    // dealAll();
    // for (int i = 0; i < numberOfParticipants; i++){
    //   players[i]->ready();
    // }
    setAsLeader();
}

void Group5Dealer::setAsLeader() {
    leaderIndex = turn;
}

void Group5Dealer::setAsLeader(int id) {
    turn = id;
    leaderIndex = turn;
}

bool Group5Dealer::deal(int c) {
    Card top;

    for (int p = 0; p < numberOfParticipants ; p++) {
        players[p].clearHand();
    }
    theDeck.setupDeck();
    theDeck.shuffle();
    for (int i = 0; i < c; i++) {
        for (int p = 0; p < numberOfPlayingPlayers; p++) {
            if ( theDeck.isEmpty() )
                break;
            theDeck.draw(top, 0);
            players[ (numberOfPlayingPlayers - 1 - p) % numberOfPlayingPlayers].takeCards(top);
            dupHands[players[(numberOfPlayingPlayers - 1 - p) % numberOfPlayingPlayers].getId()].insert(top);
        }
    }
    //    turn = 0;
    return true;
}

bool Group5Dealer::dealAll() {
    return deal(53);
}

void Group5Dealer::clearDiscardPile() {
    discarded.makeEmpty();
}

const CardSet & Group5Dealer::discardPile() {
    return discarded;
}

bool Group5Dealer::accept(CardSet & opened) {
        Card openedRank;

    if (! opened.subsetof(dupHands[players[turn].getId()])) {
    //   std::cout << "\t!!! You don't have " << opened
                // << "in your hand of " << dupHands[players[turn].getId()];
      return false;
    }

    if (discarded.isEmpty() && opened.isEmpty() )
      return false;  // regarded as "pass for empty discard pile."

    if (!discarded.isEmpty() && discarded.size() != opened.size())  // the number of cards must be match. no five cards w/ Jkr allowed.
      return false;
    
    if (!checkRankUniqueness(opened))
      return false;
    
    openedRank = getCardRank(opened);

    if (!discarded.isEmpty()) // 場にカードがでていないのであれば無条件に受理
      if (!openedRank.isGreaterThan(discardedRank)) 
        return false;
    

    // passed all the checks.

    discarded.makeEmpty();
    discarded.insert(opened);
    dupHands[players[turn].getId()].remove(opened);
    opened.makeEmpty();
    discardedRank=openedRank;

    return true;
}

Card Group5Dealer::getCardRank(CardSet & cs){
    for (int i = 0; i < cs.size(); i++) {
      if (!cs[i].isJoker()){return cs[i];}
    }
    return cs[0];
}

bool Group5Dealer::checkRankUniqueness(CardSet & cs) {
    int j = 0;
    if (cs.size() == 0)
        return false;
    if (cs[j].isJoker())
        j++;
    for (int i = j+1; i < cs.size(); i++) {
      if (cs[i].isJoker()){continue;} // 追加：途中にJkrを許す
      if (cs[j].getNumber() != cs[i].getNumber() )
        return false;
    }
    return true;
}

void Group5Dealer::showDiscardedToPlayers() {
    GameStatus gstat = gameStatus();
    for (int i = 0; i < numberOfPlayingPlayers; i++) {
        players[(turn + i) % numberOfPlayingPlayers].approve(gstat);
    }
    return; 
}

void Group5Dealer::withdrawPlayer(int i) {
    Player p;
    p = players[i];
    for ( ; i+1 < numberOfPlayingPlayers; i++) {
        players[i] = players[i+1];
    }
    if (pauper == numberOfPlayingPlayers) {
        players[i] = players[i+1];
        i++;
        pauper--;
    }
    players[i] = p;
    numberOfPlayingPlayers--;
    if (numberOfPlayingPlayers > 0) {
        turn = turn % numberOfPlayingPlayers;
        leaderIndex = leaderIndex % numberOfPlayingPlayers;
    }
}

Player & Group5Dealer::playerInTurnFinished() {
    //int i;
    bool bankrupt = false;
    
    Player * p = 0;
    if (numberOfPlayingPlayers == howManyParticipants()
        // the first finished person is not the millionaire 
            ) { 
        if (turn != howManyParticipants() - 1)
            bankrupt = true;
        withdrawPlayer(turn); // millionaire
        if ( bankrupt && !noMillionaire) {
            pauper = numberOfPlayingPlayers-1;
            withdrawPlayer(pauper);
        }
        // 次の１行をコメントアウトすると 都落ちなし
        // noMillionaire = false;
        return players[pauper];
    }
    withdrawPlayer(turn);
    show();
    return *p;
}


Player & Group5Dealer::player(int i) {
    return players[i];
}


int Group5Dealer::numberOfFinishedPlayers() {
    return howManyParticipants() - numberOfPlayingPlayers;
}

Player & Group5Dealer::playerInTurn() {
    return players[turn];
}

Player & Group5Dealer::nextPlayer() {
    turn = (turn+1) % numberOfPlayingPlayers;
    return players[turn];
}

void Group5Dealer::show() {
  std::cout << "===========" << std::endl;
  for (int p = 0; p < numberOfParticipants ; p++) {
    if ( p==numberOfPlayingPlayers)
      std::cout << "-----------" << std::endl;
    if ( p == leaderIndex )
      std::cout << "* ";
    else 
      std::cout << "  ";
    std::cout << players[p].toString() << std::endl;
    if (! players[p].inHand().equal(dupHands[players[p].getId()])) {
      std::cout << "*** error: " << players[p].getName()
                << "'s hand is different from the real hand. ***" << std::endl;
      std::cout << "   (real) : " << dupHands[players[p].getId()] << std::endl << std::endl;
    }
  }
  std::cout << "===========" << std::endl << std::endl;
}

void Group5Dealer::putBackOpened(CardSet & opened) {
  CardSet cs = opened.intersection(dupHands[players[turn].getId()]);
  players[turn].takeCards(cs);
}

////////////////////////////////////////////////////////////////////////
void Group5Dealer::updateScore() {
  int id, n = howManyParticipants();
  for (int i = 0; i < n; i++) {
    id = players[n - 1 - i].getId();
    // score[id][0] += (n - i);
    // score[id][0] += (n - i) * (n - i);
    score[id][0] += (i == 0) ? n + 1 : n - i;
    score[id][i+1]++;
  }
  // score[id][0] += n;
}

void Group5Dealer::printScore() {
  std::cout << "### Score ###" << std::endl;
  for (int i = 0; i < howManyParticipants(); i++) {
    std::cout << id2player[i].playerName() << ": "
              << score[id2player[i].getId()][0] << std::endl;
  }
}

void Group5Dealer::printResult() {
  int i, j, n = howManyParticipants();
  int tbl[n];
  int id, tmp;

  for (i = 0; i < n; i++) tbl[i] = i;

  for (i = n - 1; i > 0; i--) {
    for (j = 0; j < i; j++) {
      if (score[id2player[tbl[j]].getId()][0] < score[id2player[tbl[j+1]].getId()][0]) {
        tmp = tbl[j];
        tbl[j] = tbl[j+1];
        tbl[j+1] = tmp;
      }
    }
  }

  std::cerr << "### Final Result ###" << std::endl;
  std::cerr << std::setw(18) << " ";
  std::cerr << std::setw(10) << "Score  ";
  for (i = 0; i < n; i++) {
    std::string s;
    s = tostr(i+1);
    s += ((i == 0) ? "st" : ((i == 1) ? "nd" : ((i == 2) ? "rd" : "th")));
    std::cerr << std::setw(5) << s;
  }
  std::cerr << std::endl;
  std::cerr << "----------------------------";
  for (i = 0; i < n; i++) {
    std::cerr << "-----";
  }
  std::cerr << std::endl;

  for (i = 0; i < n; i++) {
    id = id2player[tbl[i]].getId();
    std::cerr << (i+1)
              << ((i == 0) ? "st" : ((i == 1) ? "nd" : ((i == 2) ? "rd" : "th")))
              << " place: " << std::setw(8) << id2player[tbl[i]].playerName() << ": "
              << std::setw(5) << score[id][0] << " : ";
    for (j = 0; j < n; j++) {
      std::cerr << std::setw(4) << score[id][j+1] << " ";
    }
    std::cerr << std::endl;
  }
}

bool Group5Dealer::isEmptyHanded(int playerId) {
  return dupHands[playerId].isEmpty();
}

////////////////////////////////////////////////////////////////////////
GameStatus Group5Dealer::gameStatus(void) const {
    GameStatus gstat;
    gstat.pile = discarded;
    gstat.turnIndex = turn;
    gstat.numPlayers = numberOfPlayingPlayers;
    gstat.numParticipants = numberOfParticipants;
    for (int i = 0; i < howManyParticipants(); i++) {
        gstat.numCards[i] = players[i].size();
        gstat.playerID[i] = players[i].getId();
        gstat.playerName[i] = players[i].playerName();
    }
    gstat.leaderIndex = leaderIndex;
    return gstat;
}

CardSet &Group5Dealer::dupHand(int i) {
    return dupHands[i];
}