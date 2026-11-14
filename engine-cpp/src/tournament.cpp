#include "tournament.hpp"
#include <cstdlib>
#include <sstream>
#include <utility>
#include <vector>

namespace KoRama {

namespace {

constexpr int kRosterSize = 8;

int coinFlip(const BracketMatch& m) {
    return std::rand() % 2 == 0 ? m.p1 : m.p2;
}

} // namespace

template <typename F> void Tournament::forEachMatch(F f) {
    for (BracketMatch& m : quarter) f(m);
    for (BracketMatch& m : semi) f(m);
    f(final);
    f(boss);
}

template <typename F> void Tournament::forEachMatch(F f) const {
    for (const BracketMatch& m : quarter) f(m);
    for (const BracketMatch& m : semi) f(m);
    f(final);
    f(boss);
}

Tournament Tournament::create(int playerChar) {
    std::vector<int> rest;
    for (int c = 0; c < kRosterSize; ++c) {
        if (c != playerChar) rest.push_back(c);
    }
    for (std::size_t i = rest.size() - 1; i > 0; --i) {
        std::size_t j = static_cast<std::size_t>(std::rand()) % (i + 1);
        std::swap(rest[i], rest[j]);
    }
    Tournament t;
    t.player = playerChar;
    t.stage = 1;
    t.quarter = {{{playerChar, rest[0], -1}, {rest[1], rest[2], -1}, {rest[3], rest[4], -1}, {rest[5], rest[6], -1}}};
    t.boss = {-1, kBoss, -1};
    return t;
}

int Tournament::currentOpponent() const {
    switch (stage) {
    case 1: return quarter[0].p2;
    case 2: return semi[0].p2;
    case 3: return final.p2;
    case 4: return boss.p2;
    default: return -1;
    }
}

void Tournament::recordWin() {
    switch (stage) {
    case 1:
        quarter[0].winner = player;
        for (std::size_t i = 1; i < quarter.size(); ++i) quarter[i].winner = coinFlip(quarter[i]);
        semi[0] = {player, quarter[1].winner, -1};
        semi[1] = {quarter[2].winner, quarter[3].winner, -1};
        break;
    case 2:
        semi[0].winner = player;
        semi[1].winner = coinFlip(semi[1]);
        final = {player, semi[1].winner, -1};
        break;
    case 3:
        final.winner = player;
        boss = {player, kBoss, -1};
        break;
    case 4:
        boss.winner = player;
        break;
    default:
        return;
    }
    stage++;
}

const char* Tournament::stageName(int stage) {
    switch (stage) {
    case 1: return "QUARTAS DE FINAL";
    case 2: return "SEMIFINAL";
    case 3: return "FINAL DO TORNEIO";
    case 4: return "DESAFIO DO BOSS FINAL";
    case 5: return "CAMPEÃO ABSOLUTO";
    default: return "";
    }
}

std::string Tournament::serialize() const {
    std::ostringstream out;
    out << "campaign " << player << ' ' << stage;
    forEachMatch([&](const BracketMatch& m) { out << ' ' << m.p1 << ' ' << m.p2 << ' ' << m.winner; });
    return out.str();
}

std::optional<Tournament> Tournament::parse(const std::string& line) {
    std::istringstream in(line);
    std::string tag;
    Tournament t;
    if (!(in >> tag >> t.player >> t.stage) || tag != "campaign") return std::nullopt;
    bool ok = true;
    t.forEachMatch([&](BracketMatch& m) { ok = ok && static_cast<bool>(in >> m.p1 >> m.p2 >> m.winner); });
    auto valid = [](int c) { return c >= -1 && c <= kBoss; };
    t.forEachMatch([&](const BracketMatch& m) { ok = ok && valid(m.p1) && valid(m.p2) && valid(m.winner); });
    if (!ok || t.player < 0 || t.player >= kRosterSize || !t.active()) return std::nullopt;
    return t;
}

} // namespace KoRama
