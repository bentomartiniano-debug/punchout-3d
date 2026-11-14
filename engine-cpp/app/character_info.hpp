#ifndef KORAMA_CHARACTER_INFO_HPP
#define KORAMA_CHARACTER_INFO_HPP

#include "bridge_types.h"
#include <SFML/Graphics/Color.hpp>
#include <array>
#include <cstdint>

namespace KoRama {

// Ficha de cada lutador para menus e HUD (CANON_FIGHTERS, CHAR_INFO e CHAR_STATS do index.html).
// O índice é o char_id do motor.
struct CharacterInfo {
    const char* key;        // nome usado nos sprites exportados (portrait_<key>_front, ...)
    const char* name;
    const char* title;
    const char* archetype;
    const char* quote;
    sf::Color color;
    int forca, stamina, alcance, resistencia;
    bool hasFightSprites;   // spritesheet completa exportada; os demais entram na fase 5
    FighterSizeCategory defaultSize;
    const char* sizeLabel;
};

inline const std::array<CharacterInfo, 10> kRoster = {{
    {"jack", "JACK", "MÃOS DE COLMEIA", "ENXAME & DPS VELOZ",
     "O zumbido da colmeia anuncia o nocaute inevitável.", sf::Color(0xff, 0xd7, 0x00), 70, 80, 65, 75, true, FIGHTER_SIZE_PEQUENUXO, "PEQUENUXO"},
    {"frog", "KING FROG", "O REI DA LAMA", "PESO PESADO & ALCANCE",
     "O monarca do pântano domina com alcance tóxico e saltos sísmicos.", sf::Color(0x2e, 0xcc, 0x71), 90, 60, 75, 90, true, FIGHTER_SIZE_GRANDALHAO, "GRANDALHÃO"},
    {"joana", "JOANA", "CARA DE CRACA", "CONTROLE & RETALIAÇÃO",
     "Carapaça blindada das profundezas que esmaga com pinças de coral.", sf::Color(0xe6, 0x5c, 0x00), 80, 70, 90, 70, true, FIGHTER_SIZE_NORMAL, "NORMAL"},
    {"cremoso", "O CREMOSO", "SORVETEIRO DOCE", "TANQUE & REGENERAÇÃO",
     "Mestre da confeitaria que absorve impactos e retalia com calda quente.", sf::Color(0xfe, 0xf0, 0x8a), 75, 95, 60, 80, true, FIGHTER_SIZE_GRANDALHAO, "GRANDALHÃO"},
    {"valeria", "VALÉRIA VULCÂNICA", "A TITÃ DE LAVA", "PRESSÃO & COMBOS DE FOGO",
     "A fúria vulcânica que aquece seus punhos em ritmos implacáveis.", sf::Color(0xff, 0x45, 0x00), 95, 55, 80, 65, true, FIGHTER_SIZE_NORMAL, "NORMAL"},
    {"vivaldi", "MAESTRA VIVALDI", "A MUSICISTA ESPECTRAL", "CADÊNCIA & CONGELAMENTO",
     "Compositora virtuosa que rege o ringue com ritmo e gelo.", sf::Color(0x9d, 0x4e, 0xdd), 65, 80, 95, 60, true, FIGHTER_SIZE_NORMAL, "NORMAL"},
    {"espinha", "LADY ESPINHA", "CACTUS LUCHADORA", "REFLEXO & ESPINHOS",
     "Guerreira da caatinga cujo contra-ataque perfura qualquer defesa.", sf::Color(0x00, 0xf5, 0xd4), 80, 85, 80, 80, true, FIGHTER_SIZE_NORMAL, "NORMAL"},
    {"marcov", "DR. MARCOV", "ASTRONAUTA QUÂNTICO", "ZONER & TECNOLOGIA ESPACIAL",
     "Cientista orbital armado com propulsores de plasma e precisão cósmica.", sf::Color(0xd9, 0x04, 0x29), 90, 70, 70, 90, true, FIGHTER_SIZE_NORMAL, "NORMAL"},
    {"boss", "BATALHATOR 2000", "CAMPEÃO SUPREMO", "CHEFE SUPREMO",
     "O campeão invicto que rege a arena com fúria de aço.", sf::Color(0xff, 0x00, 0x55), 100, 100, 100, 100, false, FIGHTER_SIZE_GRANDALHAO, "GRANDALHÃO"},
    {"dummy", "O ESPANTALHO", "BONECO DE TREINO", "SACO DE PANCADA",
     "Pronto para receber pancada e absorver qualquer combo sem cair!", sf::Color(0xd9, 0x77, 0x06), 50, 100, 50, 100, false, FIGHTER_SIZE_NORMAL, "NORMAL"},
}};

inline const CharacterInfo& rosterInfo(uint32_t charId) {
    return kRoster[charId < kRoster.size() ? charId : 0];
}

struct SkinDef {
    const char* name;
    const char* tagline;
    const char* quote;
    sf::Color color;
};

inline const std::array<std::array<SkinDef, 4>, 10> kRosterSkins = {{
    // 0: Jack
    {{
        {"PADRÃO", "Apicultor Rústico", "O zumbido da colmeia anuncia o nocaute inevitável.", sf::Color(0xff, 0xd7, 0x00)},
        {"APICULTOR", "Traje de Apicultor Profissional", "Até a abelha mais doce sabe a hora exata de ferroar!", sf::Color(0xf8, 0xfa, 0xfc)},
        {"CAVALEIRO DO FAVO", "Paladino da Ordem da Rainha", "Pela Rainha, pelo Favo e pela Honra do Enxame!", sf::Color(0x47, 0x55, 0x69)},
        {"BIO-HAZARD MECH", "Engenharia Bio-Cibernética 2099", "Amostra contida. Nível de ameaça reduzido a zero.", sf::Color(0x00, 0xf5, 0xd4)}
    }},
    // 1: King Frog
    {{
        {"PADRÃO", "O Monarca do Pântano", "O monarca do pântano domina com alcance tóxico e saltos sísmicos.", sf::Color(0x2e, 0xcc, 0x71)},
        {"DOM SAPONE", "O Mafioso da Lama", "Você veio ao meu ringue no dia do casamento da minha filha...", sf::Color(0x18, 0x18, 0x1b)},
        {"GLADIADOR", "Imperador do Coliseu Anfíbio", "Veni, Vidi, Vici... e o pântano agora é meu império!", sf::Color(0xb4, 0x53, 0x09)},
        {"SAPO DARDO", "Xamã Dendrobates Asteca", "Um único toque da minha pele adormece o coração de um titã.", sf::Color(0x1d, 0x4e, 0xd8)}
    }},
    // 2: Joana
    {{
        {"PADRÃO", "Pirata Mutante das Profundezas", "Carapaça blindada das profundezas que esmaga com pinças de coral.", sf::Color(0xe6, 0x5c, 0x00)},
        {"PRÉ-MALDIÇÃO", "Pescadora do Mar do Norte", "O mar bravio do norte nunca me engoliu... e não vai ser você quem vai me arrastar pro fundo!", sf::Color(0xea, 0xb3, 0x08)},
        {"ALMIRANTE DO ABISMO", "Fantasma do Davy Jones", "Você agora pertence ao armário do Davy Jones...", sf::Color(0x0d, 0x94, 0x88)},
        {"STEAMPUNK DREDGER", "Corsária Industrial a Vapor", "Potência máxima de caldeira. O fundo do mar é nosso depósito de sucata!", sf::Color(0xf5, 0x9e, 0x0b)}
    }},
    // 3: O Cremoso
    {{
        {"PADRÃO", "Mestre da Confeitaria Tradicional", "Mestre da confeitaria que absorve impactos e retalia com calda quente.", sf::Color(0xfe, 0xf0, 0x8a)},
        {"ZEN MATCHA", "Mestre Zen Matcha Orgânico", "Respire fundo, sinta a energia da Terra... seu nocaute foi 100% natural, orgânico e sem crueldade.", sf::Color(0x84, 0xcc, 0x16)},
        {"NAPOLITANO GOURMET", "Três Sabores Nobres", "Três sabores, três rounds, um nocaute doce e perfeito!", sf::Color(0xf4, 0x3f, 0x5e)},
        {"SUNDAE DE FESTA", "Carnaval de Algodão Doce", "A festa acabou e sobrou só você derretido na lona!", sf::Color(0x38, 0xbd, 0xf8)}
    }},
    // 4: Valéria Vulcânica
    {{
        {"PADRÃO", "Dançarina Vulcânica de Frevo", "A fúria vulcânica que aquece seus punhos em ritmos implacáveis.", sf::Color(0xff, 0x45, 0x00)},
        {"RAINHA DO MARACATU", "Galo da Madrugada de Olinda", "No passo do frevo e no calor do Galo, ninguém fica de pé na minha frente!", sf::Color(0xfa, 0xcc, 0x15)},
        {"FEBRE VULCÂNICA", "Rainha da Pista", "O ringue ferve no ritmo da minha dança!", sf::Color(0xff, 0x4b, 0x9b)},
        {"FLAMENCO CALIENTE", "Bailarina das Chamas de Sevilha", "¡Olé! O ritmo do meu sangue arde mais forte que a sua guarda!", sf::Color(0xb9, 0x1c, 0x1c)}
    }},
    // 5: Maestra Vivaldi
    {{
        {"PADRÃO", "Compositora Espectral Virtuosa", "Compositora virtuosa que rege o ringue com ritmo e gelo.", sf::Color(0x9d, 0x4e, 0xdd)},
        {"VIVALDI HUMANA", "A Virtuosa de Veneza", "A verdadeira música não morre... ela pulsa viva em cada nota e em cada soco!", sf::Color(0x99, 0x1b, 0x1b)},
        {"ÓPERA FANTASMA", "La Scala Gótica de Milão", "O pano caiu para você. Bravo! Bravíssimo!", sf::Color(0x88, 0x13, 0x37)},
        {"AURORA BOREAL", "Rainha Glacial do Ártico", "O silêncio do gelo é a mais sublime das sinfonias.", sf::Color(0x38, 0xbd, 0xf8)}
    }},
    // 6: Lady Espinha
    {{
        {"PADRÃO", "Luchadora de Mandacaru", "Guerreira da caatinga cujo contra-ataque perfura qualquer defesa.", sf::Color(0x00, 0xf5, 0xd4)},
        {"EL MANTO DE ORO", "La Campeona Enmascarada", "¡Por el oro, por la gloria y por el honor de la máscara!", sf::Color(0xff, 0xd7, 0x00)},
        {"LA CATRINA", "Día de los Muertos", "Na morte como no ringue... ninguém escapa dos meus espinhos.", sf::Color(0xea, 0x58, 0x0c)},
        {"RAINHA DO DESERTO", "Flor do Faroeste", "O deserto floresce... e eu também sei acertar em cheio.", sf::Color(0xc9, 0x92, 0x54)}
    }},
    // 7: Dr. Marcov
    {{
        {"CIENTISTA", "Gênio do Laboratório Orbital", "Jaleco, punhos de primata e ciência aplicada ao nocaute.", sf::Color(0x4b, 0xb7, 0xb7)},
        {"PRIMATA PRIMAL", "Chimpanzé das Cavernas", "Ooga-booga! Macaco bate forte, você dorme no chão!", sf::Color(0xd9, 0x77, 0x06)},
        {"VOSTOK GAGARIN", "Herói Soviético CCCP", "Поехали! A Terra é azul... mas a sua cara vai ficar roxa!", sf::Color(0xdc, 0x26, 0x26)},
        {"ÁS DA AVIAÇÃO 1918", "Barão Vermelho Primata", "Nenhum caça me derruba nos céus... e nenhum boxeador me derruba no ringue!", sf::Color(0x78, 0x35, 0x0f)}
    }},
    // 8: Batalhator 2000
    {{
        {"PADRÃO", "Campeão Supremo & Nerd Operador", "O campeão invicto que rege a arena com fúria de aço.", sf::Color(0xff, 0x00, 0x55)},
        {"RETRO ARCADE 1984", "Gabinete Fliperama 8-Bit", "GG EZ! Ninguém quebra meu recorde na minha máquina!", sf::Color(0x06, 0xb6, 0xd4)},
        {"STEAMPUNK GOLEM", "Autômato Vitoriano a Vapor", "Pontual como a mecânica inglesa... e letal como uma caldeira pressurizada!", sf::Color(0xb4, 0x53, 0x09)},
        {"MECHA TOKUSATSU", "Robô Sentai & Piloto Cosplay", "A justiça triunfou! O monstro foi derrotado mais uma vez!", sf::Color(0xef, 0x44, 0x44)}
    }},
    // 9: O Espantalho
    {{
        {"PADRÃO", "Boneco de Feno e Retalhos", "Pronto para receber pancada e absorver qualquer combo sem cair!", sf::Color(0xd9, 0x77, 0x06)},
        {"CRASH TEST DUMMY", "Manequim de Colisão Automotiva", "Impacto absorvido com sucesso. Airbags não foram necessários.", sf::Color(0xfa, 0xcc, 0x15)},
        {"HOLO-DUMMY", "Manequim Cibernético VR", "Reiniciando simulação de combate. Pronto para o próximo round.", sf::Color(0x22, 0xc5, 0x5e)},
        {"HALLOWEEN PUMPKIN", "Espantalho Sinistro de Abóbora", "Doces ou travessuras? Você escolheu apanhar na noite de Halloween!", sf::Color(0xea, 0x58, 0x0c)}
    }}
}};

inline const SkinDef& rosterSkin(uint32_t charId, uint32_t skinId) {
    const uint32_t c = charId < kRosterSkins.size() ? charId : 0;
    const uint32_t s = skinId < 4 ? skinId : 0;
    return kRosterSkins[c][s];
}

} // namespace KoRama

#endif // KORAMA_CHARACTER_INFO_HPP
