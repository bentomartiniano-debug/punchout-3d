#ifndef KORAMA_TOURNAMENT_HPP
#define KORAMA_TOURNAMENT_HPP

#include <array>
#include <optional>
#include <string>

namespace KoRama {

// Uma chave do torneio. -1 = ainda a definir; 8 = Batalhator.
struct BracketMatch {
    int p1 = -1;
    int p2 = -1;
    int winner = -1;
};

// Torneio Arcade: quartas, semifinal, final e o desafio do chefe (generateTournamentBracket e
// showVictoryRingCelebration do index.html). O jogador sempre luta na primeira chave de cada fase;
// as outras são decididas no cara ou coroa quando ele vence.
class Tournament {
public:
    static constexpr int kBoss = 8;
    static constexpr int kStages = 4;

    int player = -1;
    int stage = 0; // 1 quartas, 2 semifinal, 3 final, 4 chefe, 5 campeão; 0 = nenhum torneio
    std::array<BracketMatch, 4> quarter{};
    std::array<BracketMatch, 2> semi{};
    BracketMatch final{};
    BracketMatch boss{};

    // Sorteia as chaves com os 7 outros lutadores do elenco (usa std::rand).
    static Tournament create(int playerChar);

    bool active() const { return stage >= 1 && stage <= kStages; }
    bool champion() const { return stage == kStages + 1; }
    int currentOpponent() const; // -1 sem luta pendente
    void recordWin();            // o jogador venceu a luta da fase atual

    static const char* stageName(int stage); // "QUARTAS DE FINAL", ...

    // Linha do korama_save.txt: "campaign <jogador> <fase> <p1 p2 vencedor> × 8 chaves".
    std::string serialize() const;
    static std::optional<Tournament> parse(const std::string& line);

private:
    template <typename F> void forEachMatch(F f);
    template <typename F> void forEachMatch(F f) const;
};

} // namespace KoRama

#endif // KORAMA_TOURNAMENT_HPP
