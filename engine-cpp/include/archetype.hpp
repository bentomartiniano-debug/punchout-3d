#ifndef KORAMA_ARCHETYPE_HPP
#define KORAMA_ARCHETYPE_HPP

#include <cstdint>

namespace KoRama {

// O estilo de luta de cada lutador, valendo sem habilidade equipada (PLANO-COMBOS-E-ESQUIVA.md, arquétipos).
// A recompensa da esquiva e as estocadas de cada um ficam no motor (GameEngine::dodgeReward e skillDamage).
struct Archetype {
    const char* name;
    float reach;      // soma ao alcance dos socos (o rival só é atingido se a soma dos raios couber no alcance)
    float lunge;      // passo à frente máximo que o soco dá sozinho para alcançar
    float dealt;      // multiplica o dano que causa (socos e especiais)
    float taken;      // multiplica o dano que leva
    float punchPush;  // todo soco limpo empurra o rival isto a mais
    float pushMul;    // multiplica o empurrão dos socos pesados
    int beePerPunch;  // quadros de enxame que cada soco limpo acrescenta no rival (teto kBeeCap)
};

constexpr int kBeeCap = 240;

inline const Archetype& archetypeOf(uint32_t charId) {
    static const Archetype k[9] = {
        //                        alcance passo  dano  leva  empurra  ×emp  abelha
        {"Enxame",                  0.0f, 40.0f, 1.00f, 1.00f,  0.0f, 1.0f, 20},  // Jack: cada soco solta abelhas que ficam no rival
        {"Armadilheiro",            0.0f, 25.0f, 1.00f, 1.00f,  0.0f, 1.0f, 0},   // King Frog: prende e drena; quem erra nele fica lento
        {"Pesada",                -10.0f, 60.0f, 1.00f, 1.00f,  0.0f, 1.0f, 0},   // Joana: braço curto, mas fecha a distância; a esquiva carrega o próximo soco
        {"Contra-atacador",         0.0f, 30.0f, 1.00f, 1.00f,  0.0f, 1.0f, 0},   // Cremoso: espera, absorve e devolve
        {"Canhão de vidro",        -5.0f, 55.0f, 1.25f, 1.20f,  0.0f, 1.0f, 0},   // Valéria: bate mais e apanha mais
        {"Zoneadora",              30.0f, 15.0f, 1.00f, 1.00f, 10.0f, 1.0f, 0},   // Vivaldi: braço longo e cada soco afasta o rival
        {"Retaliadora",             0.0f, 40.0f, 1.00f, 1.00f,  0.0f, 1.0f, 0},   // Lady Espinha: quem erra nela se fura
        {"Controlador de posição", -5.0f, 40.0f, 1.00f, 1.00f,  0.0f, 1.5f, 0},   // Marcov: empurra para as cordas e guarda fôlego
        {"Chefe",                   0.0f, 40.0f, 1.00f, 1.00f,  0.0f, 1.0f, 0},
    };
    return k[charId < 9 ? charId : 8];
}

} // namespace KoRama

#endif // KORAMA_ARCHETYPE_HPP
