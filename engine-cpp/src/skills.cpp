#include "skills.hpp"
#include <cstdlib>
#include <string>
#include <vector>

namespace KoRama {

namespace {

using C = SkillCommand;
using T = SkillType;
using M = SkillMode;

// Construtor enxuto de efeitos: fx().dmg(20).burn(300, 3).
struct Fx {
    SkillEffect e;
    Fx& dmg(float v) { e.damage = v; return *this; }
    Fx& at(float v) { e.at = v; return *this; }
    Fx& hits(int v) { e.hits = v; return *this; }
    Fx& stun(int v) { e.stun = v; return *this; }
    Fx& slow(int v) { e.slow = v; return *this; }
    Fx& burn(int frames, float dps) { e.burn = frames; e.burnDps = dps; return *this; }
    Fx& blind(int v) { e.blind = v; return *this; }
    Fx& bleed(int v) { e.bleed = v; return *this; }
    Fx& mud(int v) { e.mud = v; return *this; }
    Fx& drain(float v) { e.staminaDrain = v; return *this; }
    Fx& push(float v) { e.push = v; return *this; }
    Fx& heal(float v) { e.heal = v; e.at = 0.0f; return *this; }
    Fx& mode(M m, int frames) { e.mode = m; e.modeFrames = frames; e.at = 0.0f; return *this; }
    Fx& air() { e.airborne = true; return *this; }
    operator SkillEffect() const { return e; }
};
Fx fx() { return Fx{}; }

// Textos e comandos do SKILL_DATABASE do index.html. Durações das animações de activateX().
const std::array<SkillDef, static_cast<size_t>(SkillId::Count) - 1> kSkills = {{
    // Jack Mãos de Colmeia (0) - Swarmer / DoT
    {SkillId::JackInfestation, "jack_infestation", 0, "Infestação de Abelhas", T::Command, C::BackForward, "SWARM",
     "Enxame de abelhas em órbita causando dano por segundo (3,2 HP/s por 6s).", 44},
    {SkillId::JackMolasses, "jack_molasses", 0, "Modo Melaço", T::Mode, C::BothPunches, "HONEY",
     "6s: sem corte de dano! Cada soco rápido aplica lentidão cumulativa no rival.", 28, fx().mode(M::None, 360)},
    {SkillId::JackHoneyWall, "jack_honey_wall", 0, "Favo Protetor", T::Command, C::DownDownLeft, "SHIELD",
     "Escudo de favo: -60% de dano por 3 golpes, recupera stamina e retarda quem bate.", 28},
    {SkillId::JackStingJab, "jack_sting_jab", 0, "Agulhada Rainha", T::Command, C::UpUpRight, "STING",
     "Ferrão que ignora a guarda: 15 HP e marca o alvo (o enxame fica nele por 2,5s).", 24, fx().dmg(15)},
    {SkillId::JackCombBurst, "jack_comb_burst", 0, "Explosão de Pólen", T::Command, C::DownUp, "POLLEN",
     "Soco de pólen: 12 HP e cegueira total de 2s forçando o rival a defender.", 26, fx().dmg(12).at(0.35f).blind(120)},
    {SkillId::JackRoyalJelly, "jack_royal_jelly", 0, "Geleia Real", T::Command, C::BackBackLeft, "HEAL",
     "Regenera 12 HP. Recarga de 8s.", 28, fx().heal(12)},
    {SkillId::JackBeekeeper, "jack_beekeeper", 0, "Apicultor", T::Passive, C::None, "PASSIVA",
     "Esquivas bem-sucedidas soltam uma abelha (1 HP/s por 3s residual).", 0},
    {SkillId::JackHoneySkin, "jack_honey_skin", 0, "Pele de Mel", T::Passive, C::None, "PASSIVA",
     "Vida < 45%: -35% dano sofrido; bloqueios e impactos recuperam stamina.", 0},
    {SkillId::JackQueenBlessing, "jack_queen_blessing", 0, "Bênção da Rainha", T::Passive, C::None, "PASSIVA",
     "Derrubar o adversário regenera 16 HP.", 0},

    // King Frog (1) - Zoner / Juggle & Sustain
    {SkillId::FrogTongue, "frog_tongue", 1, "Golpe de Língua", T::Command, C::BackForward, "REACH",
     "Língua gigante: 14 HP, empurra contra as cordas e deixa lento (-35% por 4s).", 24, fx().dmg(14).push(40).slow(240)},
    {SkillId::FrogPoisonSkin, "frog_poison_skin", 1, "Pele Venenosa", T::Mode, C::BothPunches, "VENOM",
     "Toxina por 6s: oponente envenenado (2,5 HP/s) e drena a vida dele para o Frog.", 28},
    {SkillId::FrogRoyalLeap, "frog_royal_leap", 1, "Salto Real Esmagador", T::Command, C::DownUp, "IMPACT",
     "Salta fora da tela e cai com soco duplo: 16 HP e atordoa.", 32, fx().dmg(16).stun(60).air()},
    {SkillId::FrogMudSlime, "frog_mud_slime", 1, "Cuspidela de Lama", T::Command, C::DownDownLeft, "SLOW",
     "Disparo de lama: 12 HP + poça no chão que prende pés (sem esquiva por 3s).", 26, fx().dmg(12).at(0.35f).mud(180).slow(180)},
    {SkillId::FrogFlyCatcher, "frog_fly_catcher", 1, "Captura de Moscas", T::Command, C::BackBackLeft, "HEAL",
     "Captura moscas com a língua: regenera 12 HP e limpa efeitos negativos. Recarga 8s.", 26, fx().heal(12)},
    {SkillId::FrogSwampSpout, "frog_swamp_spout", 1, "Gêiser do Pântano", T::Command, C::UpUpRight, "BURST",
     "Erupção de água dos pés ao peito: 12 HP e desestabiliza o oponente (abre brecha no chão).", 28, fx().dmg(12).at(0.45f).push(20)},
    {SkillId::FrogJumper, "frog_jumper", 1, "Saltador Anfíbio", T::Passive, C::None, "PASSIVA",
     "Esquivas laterais e recuos consomem 50% a menos de stamina.", 0},
    {SkillId::FrogSlipperySkin, "frog_slippery_skin", 1, "Pele Mucosa Deslizante", T::Passive, C::None, "PASSIVA",
     "30% de chance do golpe escorregar (metade do dano e recupera 12 de stamina).", 0},
    {SkillId::FrogAmphibianRage, "frog_amphibian_rage", 1, "Fúria Anfíbia", T::Passive, C::None, "PASSIVA",
     "Com stamina abaixo de 30%, os socos saem 25% mais rápidos e batem +3.", 0},

    // Baronesa Joana Estrela (2) - Juggernaut
    {SkillId::JoanaClawCrush, "joana_claw_crush", 2, "Tesoura Esmagadora", T::Command, C::ForwardForwardRight, "BREAK",
     "Golpe devastador da garra: 16 HP limpos, quebra a guarda e atordoa 25f.", 28, fx().dmg(16)},
    {SkillId::JoanaHookPull, "joana_hook_pull", 2, "Arrastão do Harpão", T::Command, C::BackForwardLeft, "REACH",
     "Fisga o oponente com o harpão: 15 HP e puxa para curta com vantagem de frames.", 28, fx().dmg(15).push(-60).stun(20)},
    {SkillId::JoanaCrustaceanShell, "joana_crustacean_shell", 2, "Barreira de Cracas", T::Mode, C::BothPunches, "SHIELD",
     "5s de placa de cracas: -70% de dano e devolve 35% ao agressor.", 28},
    {SkillId::JoanaCoralSpout, "joana_coral_spout", 2, "Impacto Coralino", T::Command, C::DownUp, "BURST",
     "Uppercut com corais pontiagudos: 14 HP e sangramento (2 HP/s por 5s).", 28, fx().dmg(14).at(0.4f).bleed(300)},
    {SkillId::JoanaAnchorSlam, "joana_anchor_slam", 2, "Onda da Âncora", T::Command, C::DownDownLeft, "STUN",
     "Impacto da âncora no chão: 14 HP e onda marinha que reduz velocidade por 4s.", 28, fx().dmg(14).at(0.4f).slow(240)},
    {SkillId::JoanaOceanBounty, "joana_ocean_bounty", 2, "Bálsamo de Anêmona", T::Command, C::BackBackLeft, "HEAL",
     "Nutrientes marinhos na carapaça: regenera 12 HP. Recarga de 8s.", 26, fx().heal(12)},
    {SkillId::JoanaAsymmetricArms, "joana_asymmetric_arms", 2, "Armamento Assimétrico", T::Passive, C::None, "PASSIVA",
     "A garra bate 25% mais forte; o gancho alcança 40% mais longe.", 0},
    {SkillId::JoanaCalcifiedArmor, "joana_calcified_armor", 2, "Carapaça Calcária", T::Passive, C::None, "PASSIVA",
     "Reduz todo dano frontal sofrido em 20%.", 0},
    {SkillId::JoanaDeepSeaTide, "joana_deep_sea_tide", 2, "Maré das Profundezas", T::Passive, C::None, "PASSIVA",
     "Modo Monstruoso (HP <= 40): Super Armor e socos recuperam 5 de stamina.", 0},

    // O Cremoso (3) - Revenge Tank / Heavy Counter
    {SkillId::CremosoSyrupBlast, "cremoso_syrup_blast", 3, "Explosão de Calda Quente", T::Command, C::ForwardForwardRight, "BURN",
     "Soco de calda fervente: 14 HP + queimadura de 2 HP/s por 4s (total 22 HP).", 26, fx().dmg(14).burn(240, 2.0f)},
    {SkillId::CremosoVanillaWhip, "cremoso_vanilla_whip", 3, "Tornado de Baunilha", T::Command, C::DownUp, "BURST",
     "Espiral de socos de sorvete: 15 HP e lentidão de 3,5s.", 32, fx().dmg(15).hits(3).at(0.3f).slow(210)},
    {SkillId::CremosoMeltMode, "cremoso_melt_mode", 3, "Modo Derretido", T::Mode, C::BothPunches, "ABSORB",
     "Corpo maleável por 6s: -50% de dano e 20% do dano recebido vira cura.", 36, fx().mode(M::Melt, 360)},
    {SkillId::CremosoConePunch, "cremoso_cone_punch", 3, "Estocada de Casquinha", T::Command, C::UpUpRight, "STING",
     "Casquinha que fura o bloqueio: 15 HP, e 21 HP em quem estiver na guarda.", 26, fx().dmg(15)},
    {SkillId::CremosoPuddleTrap, "cremoso_puddle_trap", 3, "Poça de Calda Grudenta", T::Command, C::DownDownLeft, "SLOW",
     "Soco pesado no chão: 14 HP + poça pegajosa que prende (sem esquiva por 3s).", 28, fx().dmg(14).at(0.4f).mud(180).slow(180)},
    {SkillId::CremosoCherryTopping, "cremoso_cherry_topping", 3, "Cereja do Bolo", T::Command, C::BackBackLeft, "HEAL",
     "Engole a cereja gigante do topo: regenera 12 HP. Recarga de 8s.", 28, fx().heal(12)},
    {SkillId::CremosoCreamCushion, "cremoso_cream_cushion", 3, "Amortecedor de Creme", T::Passive, C::None, "PASSIVA",
     "Bloqueio reduz 50% de chip damage e converte 20% do dano absorvido em stamina.", 0},
    {SkillId::CremosoFrozenStamina, "cremoso_frozen_stamina", 3, "Stamina Gelada", T::Passive, C::None, "PASSIVA",
     "Stamina regenera 35% mais rápido e a máxima não cai com golpes pesados.", 0},
    {SkillId::CremosoSweetRevenge, "cremoso_sweet_revenge", 3, "Vingança Doce", T::Passive, C::None, "PASSIVA",
     "Esquivas perfeitas curam 2 HP e o próximo soco bate 15% mais forte.", 0},

    // Valéria (4) - Glass Cannon / Pressão de Fogo
    {SkillId::ValeriaMagmaPunch, "valeria_magma_punch", 4, "Soco Magmático", T::Command, C::ForwardForwardRight, "BURN",
     "Soco de lava: 14 HP + queimadura de 2 HP/s por 3s (total 20 HP).", 26, fx().dmg(14).burn(180, 2.0f)},
    {SkillId::ValeriaVolcanicErupt, "valeria_volcanic_erupt", 4, "Erupção Solavanco", T::Command, C::DownUp, "KNOCKDOWN",
     "Pilar vulcânico sob o oponente: 16 HP e atordoa 1,2s.", 30, fx().dmg(16).at(0.45f).stun(72)},
    {SkillId::ValeriaHeatwaveGuard, "valeria_heatwave_guard", 4, "Escudo de Calor", T::Mode, C::BothPunches, "SHIELD",
     "Onda de calor por 5s: quem ataca erra 20% e se queima no contato.", 28, fx().mode(M::Heatwave, 300)},
    {SkillId::ValeriaLavaKickJab, "valeria_lava_kick_jab", 4, "Estocada de Lava", T::Command, C::UpUpRight, "STING",
     "Rocha vulcânica que ignora a defesa: 15 HP e deixa queimando (2 HP/s por 2s).", 24, fx().dmg(15).burn(120, 2.0f)},
    {SkillId::ValeriaAshCloud, "valeria_ash_cloud", 4, "Erupção de Cinzas", T::Command, C::DownDownLeft, "SLOW",
     "Soco rápido de cinzas: 12 HP e projeta cinzas nos olhos (cegueira por 3s).", 26, fx().dmg(12).at(0.35f).blind(180)},
    {SkillId::ValeriaPhoenixEmber, "valeria_phoenix_ember", 4, "Reagrupamento das Brasas", T::Command, C::BackBackLeft, "HEAL",
     "Absorve o calor do ringue: regenera 12 HP. Recarga de 8s.", 26, fx().heal(12)},
    {SkillId::ValeriaEmberDance, "valeria_ember_dance", 4, "Dança das Brasas", T::Passive, C::None, "PASSIVA",
     "Esquivas custam 0 de stamina e resetam delay de recuperação.", 0},
    {SkillId::ValeriaColdMagma, "valeria_cold_magma", 4, "Ignição Interna", T::Passive, C::None, "PASSIVA",
     "Ao zerar stamina, explode em chamas (8 HP no rival) e recupera 30 stamina (1x/round).", 0},
    {SkillId::ValeriaVolcanicRage, "valeria_volcanic_rage", 4, "Fúria Vulcânica", T::Passive, C::None, "PASSIVA",
     "Com pouca vida (35 ou menos), os socos batem 30% mais forte.", 0},

    // Maestra Vivaldi (5) - Rhythm Striker
    {SkillId::VivaldiCadenceStaccato, "vivaldi_cadence_staccato", 5, "Cadência Staccato", T::Command, C::ForwardForwardRight, "RHYTHM",
     "Cinco socos rítmicos em crescendo: 15 HP no total.", 34, fx().dmg(15).hits(5).at(0.2f)},
    {SkillId::VivaldiWinterAllegro, "vivaldi_winter_allegro", 5, "Inverno Allegro", T::Command, C::DownUp, "STUN",
     "Uppercut de gelo ectoplásmico: 14 HP e congela (atordoa 1,2s).", 28, fx().dmg(14).stun(72)},
    {SkillId::VivaldiFourSeasons, "vivaldi_four_seasons_mode", 5, "Modo Quatro Estações", T::Mode, C::BothPunches, "BOOST",
     "Altera o tempo por 6s: socos 35% mais rápidos e mais baratos.", 28, fx().mode(M::Seasons, 360)},
    {SkillId::VivaldiCrescendoJab, "vivaldi_crescendo_jab", 5, "Estocada em Crescendo", T::Command, C::UpUpRight, "STING",
     "Batuta que ignora a guarda e cresce com a sequência: 11 HP, +2 por acerto seguido (até 19).", 24, fx().dmg(15)},
    {SkillId::VivaldiAutumnRequiem, "vivaldi_autumn_requiem", 5, "Réquiem de Outono", T::Command, C::DownDownLeft, "DRAIN",
     "Golpe musical: 12 HP, drena 20 de stamina e deixa lento por 4s.", 28, fx().dmg(12).at(0.35f).drain(20).slow(240)},
    {SkillId::VivaldiSpringSonata, "vivaldi_spring_sonata", 5, "Sonata de Primavera", T::Command, C::BackBackLeft, "HEAL",
     "Acorde curativo: regenera 12 HP. Recarga de 8s.", 26, fx().heal(12)},
    {SkillId::VivaldiPerfectRhythm, "vivaldi_perfect_rhythm", 5, "Ritmo Perfeito", T::Passive, C::None, "PASSIVA",
     "Acertos seguidos batem 8% mais forte cada (até +40%).", 0},
    {SkillId::VivaldiPhantomCloak, "vivaldi_phantom_cloak", 5, "Manto Rítmico", T::Passive, C::None, "PASSIVA",
     "Acertar 3 golpes no ritmo correto concede 1 esquiva automática garantida.", 0},
    {SkillId::VivaldiSymphonicResonance, "vivaldi_symphonic_resonance", 5, "Ressonância Sinfônica", T::Passive, C::None, "PASSIVA",
     "Derrubar o oponente ou fechar 5 acertos de combo recupera 25 de stamina.", 0},

    // Lady Espinha (6) - Counter-Punker / Punição
    {SkillId::EspinhaNopalSlap, "espinha_nopal_slap", 6, "La Garra del Cacto", T::Command, C::ForwardForwardRight, "BREAK",
     "Tapa duplo de mandacaru que quebra a guarda: 15 HP e atordoa 72f.", 28, fx().dmg(15).hits(2).at(0.35f).stun(72)},
    {SkillId::EspinhaLuchaFlyer, "espinha_lucha_flyer", 6, "Gran Salto Luchador", T::Command, C::DownUp, "IMPACT",
     "Salto de lucha libre esmagando o oponente de cima: 16 HP.", 34, fx().dmg(16).at(0.82f).air()},
    {SkillId::EspinhaThornVeil, "espinha_thorn_veil", 6, "Velo de las Espinas", T::Mode, C::BothPunches, "SHIELD",
     "Cortina de espinhos por 6s: -50% de dano e espinhos refletem o dobro.", 28, fx().mode(M::ThornVeil, 360)},
    {SkillId::EspinhaNeedleJab, "espinha_needle_jab", 6, "Estocada de Mandacaru", T::Command, C::UpUpRight, "STING",
     "Espinho gigante que ignora o bloqueio: 15 HP e sangramento por 2s.", 24, fx().dmg(15).bleed(120)},
    {SkillId::EspinhaDesertWind, "espinha_desert_wind", 6, "Viento del Desierto", T::Command, C::DownDownLeft, "SLOW",
     "Girocóptero de cacto: 13 HP, lento por 4s e cegueira de 2s.", 28, fx().dmg(13).at(0.4f).slow(240).blind(120)},
    {SkillId::EspinhaFridaTears, "espinha_frida_tears", 6, "Lágrimas de Frida", T::Command, C::BackBackLeft, "HEAL",
     "Flores de dália da resiliência: regenera 12 HP. Recarga de 8s.", 26, fx().heal(12)},
    {SkillId::EspinhaThornRecoil, "espinha_thorn_recoil", 6, "Espinas de Lucha", T::Passive, C::None, "PASSIVA",
     "Quem bate na Lady Espinha recebe 15% do dano de volta.", 0},
    {SkillId::EspinhaRebelSpirit, "espinha_rebel_spirit", 6, "Espíritu Rebelde", T::Passive, C::None, "PASSIVA",
     "Contra-ataques causam +25% de dano.", 0},
    {SkillId::EspinhaVivaCaatinga, "espinha_viva_caatinga", 6, "Viva La Caatinga", T::Passive, C::None, "PASSIVA",
     "Quanto menor a vida, mais stamina regenera e mais baratos ficam os socos.", 0},

    // Dr. Marcov (7) - Heavy Kinetic / Brawler
    {SkillId::MarcovRocketThrust, "marcov_rocket_thrust", 7, "Propulsão Foguete", T::Command, C::ForwardForwardRight, "ROCKET",
     "Arrancada a jato com soco cinético: 15 HP e empurrão nas cordas.", 26, fx().dmg(15).push(60)},
    {SkillId::MarcovOrbitalLaser, "marcov_orbital_laser", 7, "Raio Orbital Sputnik", T::Command, C::DownUp, "BURST",
     "Laser azul do satélite sobre o oponente: 16 HP e atordoa 1,2s.", 30, fx().dmg(16).at(0.55f).stun(72)},
    {SkillId::MarcovPlasmaBarrier, "marcov_plasma_barrier", 7, "Escudo Sputnik", T::Mode, C::BothPunches, "BARRIER",
     "Barreira de plasma por 6s: -55% de dano e cada golpe sofrido recarrega stamina.", 28, fx().mode(M::Plasma, 360)},
    {SkillId::MarcovKineticOverdrive, "marcov_kinetic_overdrive", 7, "Impulso de Reação", T::Command, C::UpUpRight, "STING",
     "Soco a turbina que ignora a defesa: 15 HP, joga para as cordas e tira 15 de fôlego.", 24, fx().dmg(15).push(45).drain(15)},
    {SkillId::MarcovExhaustCloud, "marcov_exhaust_cloud", 7, "Fumaça de Exaustão", T::Command, C::DownDownLeft, "SLOW",
     "Disparo de escapamento: 12 HP de calor, drena 20 stamina e slow 4s.", 28, fx().dmg(12).at(0.4f).slow(240).drain(20)},
    {SkillId::MarcovOxygenRecharge, "marcov_oxygen_recharge", 7, "Célula de Oxigênio", T::Command, C::BackBackLeft, "HEAL",
     "Reserva de oxigênio do traje: regenera 12 HP. Recarga de 8s.", 26, fx().heal(12)},
    {SkillId::MarcovZeroGravity, "marcov_zero_gravity", 7, "Gravidade Zero", T::Passive, C::None, "PASSIVA",
     "Imune a empurrões e escapa da 1ª queda de cada round com 1 HP.", 0},
    {SkillId::MarcovThrusterCharge, "marcov_thruster_charge", 7, "Módulo Sputnik", T::Passive, C::None, "PASSIVA",
     "Socos carregados e especiais recuperam 20 de stamina.", 0},
    {SkillId::MarcovTitaniumSuit, "marcov_titanium_suit", 7, "Traje de Titânio", T::Passive, C::None, "PASSIVA",
     "Golpes pesados e críticos causam 25% menos dano.", 0},

    // Batalhator 2000 (8): Chefe Final
    {SkillId::BossDualSaw, "batalhator_dual_saw_vortex", 8, "Vórtice de Serras", T::Command, C::ForwardForwardRight, "SAW",
     "Serras circulares duplas arremessadas: 15 HP e sangramento.", 26, fx().dmg(15).at(0.5f).bleed(240)},
    {SkillId::BossOverclock, "batalhator_overclock_overdrive", 8, "Sobrecarga de Reator", T::Command, C::DownUp, "OVERDRIVE",
     "Onda térmica do reator: 16 HP e drena 25 de stamina.", 32, fx().dmg(16).at(0.4f).drain(25)},
    {SkillId::BossTitanium, "batalhator_titanium_plating", 8, "Blindagem de Titânio", T::Passive, C::None, "PASSIVA",
     "Fase 1: recebe 35% menos dano.", 0},
    {SkillId::BossEmp, "batalhator_emp_shockwave", 8, "Descarga PEM", T::Command, C::UpUpRight, "EMP",
     "Pulso eletromagnético na lona: 15 HP e paralisia.", 32, fx().dmg(15).at(0.45f).stun(70)},
    {SkillId::BossMissiles, "batalhator_rocket_barrage", 8, "Barragem de Mísseis", T::Command, C::DownDownLeft, "MISSILES",
     "4 micromísseis teleguiados: 16 HP no total.", 40, fx().dmg(16).at(0.35f).hits(4)},
    {SkillId::BossDynamo, "batalhator_berserk_dynamo", 8, "Dínamo Enfurecido", T::Passive, C::None, "PASSIVA",
     "Fase 2: 15% de cada golpe recebido volta como choque elétrico.", 0},
    {SkillId::BossHolo, "batalhator_holo_combo", 8, "Combo Holográfico", T::Command, C::BackForward, "HOLO",
     "Clones holográficos em sequência ultrarrápida: 16 HP e desorientação.", 36, fx().dmg(16).at(0.4f).hits(3).blind(120)},
    {SkillId::BossApocalypse, "batalhator_orbital_apocalypse", 8, "Raio Apocalíptico", T::Command, C::BothPunches, "LASER",
     "Feixe orbital de laser: 18 HP e drena 30 de stamina.", 40, fx().dmg(18).at(0.55f).drain(30)},
    {SkillId::BossQuantum, "batalhator_quantum_matrix", 8, "Matriz Quântica", T::Passive, C::None, "PASSIVA",
     "Fase 3: 35% de esquiva quântica e +4 HP a cada golpe que acerta.", 0},
}};

} // namespace

void SkillSystem::init() {}

const SkillDef* SkillSystem::get(SkillId id) {
    for (const SkillDef& s : kSkills) {
        if (s.id == id) return &s;
    }
    return nullptr;
}

SkillEntry SkillSystem::entryOf(const SkillDef& def) {
    if (def.type == SkillType::Passive) return SkillEntry::Quick;
    if (def.id == SkillId::FrogPoisonSkin) return SkillEntry::Flow;   // é modo, mas envenena o rival
    if (def.type == SkillType::Mode) return SkillEntry::Quick;
    const std::string tag = def.tag ? def.tag : "";
    for (const char* t : {"HEAL", "SHIELD"}) if (tag == t) return SkillEntry::Quick;
    for (const char* t : {"SLOW", "POLLEN", "DRAIN", "SWARM", "VENOM"}) if (tag == t) return SkillEntry::Flow;
    return SkillEntry::Full;
}

int SkillSystem::chainNeed(const SkillDef& def) {
    const SkillEntry e = entryOf(def);
    return e == SkillEntry::Full ? 3 : e == SkillEntry::Flow ? 2 : 0;
}

SkillFinisher SkillSystem::finisherOf(const SkillDef& def) {
    SkillFinisher f;
    switch (def.command) {
    case SkillCommand::BackForward: f.dir = INPUT_RIGHT; f.z = f.x = true; break;
    case SkillCommand::ForwardForwardRight: f.dir = INPUT_RIGHT; f.x = true; break;
    case SkillCommand::BackForwardLeft: f.dir = INPUT_RIGHT; f.z = true; break;
    case SkillCommand::DownUp: f.dir = INPUT_UP; f.z = true; break;
    case SkillCommand::UpUpRight: f.dir = INPUT_UP; f.x = true; break;
    case SkillCommand::DownDownLeft: f.dir = INPUT_DOWN; f.z = true; break;
    case SkillCommand::BackBackLeft: f.dir = INPUT_LEFT; f.z = true; break;
    case SkillCommand::BothPunches: f.both = true; break;
    case SkillCommand::None: break;
    }
    return f;
}

std::vector<CmdTok> SkillSystem::commandTokens(const SkillDef& def) {
    using T = CmdTok;
    const SkillEntry e = entryOf(def);
    if (e == SkillEntry::Quick) {   // comando simples de sempre
        switch (def.command) {
        case SkillCommand::BackForward: return {T::Left, T::Right, T::Plus, T::PunchR};
        case SkillCommand::DownUp: return {T::Down, T::Up, T::Plus, T::PunchR};
        case SkillCommand::BothPunches: return {T::PunchL, T::Plus, T::PunchR};
        case SkillCommand::UpUpRight: return {T::Up, T::Up, T::Plus, T::PunchR};
        case SkillCommand::DownDownLeft: return {T::Down, T::Down, T::Plus, T::PunchL};
        case SkillCommand::BackBackLeft: return {T::Left, T::Left, T::Plus, T::PunchL};
        case SkillCommand::ForwardForwardRight: return {T::Right, T::Right, T::Plus, T::PunchR};
        case SkillCommand::BackForwardLeft: return {T::Left, T::Right, T::Plus, T::PunchL};
        case SkillCommand::None: break;
        }
        return {};
    }
    // combo (as bolinhas) e depois o finalizador
    std::vector<CmdTok> out = {e == SkillEntry::Full ? T::Chain3 : T::Chain2};
    const SkillFinisher f = finisherOf(def);
    if (f.both) { out.insert(out.end(), {T::PunchL, T::Plus, T::PunchR}); return out; }
    out.push_back(f.dir == INPUT_UP ? T::Up : f.dir == INPUT_DOWN ? T::Down : f.dir == INPUT_LEFT ? T::Left : T::Right);
    out.push_back(T::Plus);
    out.push_back(f.x ? T::PunchR : T::PunchL);   // quando os dois valem, mostra o X
    return out;
}

const SkillDef* SkillSystem::getByKey(const std::string& key) {
    for (const SkillDef& s : kSkills) {
        if (key == s.key) return &s;
    }
    return nullptr;
}

const std::array<SkillDef, static_cast<size_t>(SkillId::Count) - 1>& SkillSystem::all() {
    return kSkills;
}

bool SkillSystem::hasSkills(uint32_t charId) {
    return charId <= 7;
}

SkillLoadout SkillSystem::defaultLoadout(uint32_t charId) {
    if (charId == 0) return {SkillId::JackInfestation, SkillId::JackMolasses, SkillId::JackBeekeeper};
    if (charId == 1) return {SkillId::FrogTongue, SkillId::FrogPoisonSkin, SkillId::FrogJumper};
    if (charId == 2) return {SkillId::JoanaClawCrush, SkillId::JoanaCrustaceanShell, SkillId::JoanaAsymmetricArms};
    if (charId == 3) return {SkillId::CremosoSyrupBlast, SkillId::CremosoMeltMode, SkillId::CremosoCreamCushion};
    if (charId == 4) return {SkillId::ValeriaMagmaPunch, SkillId::ValeriaHeatwaveGuard, SkillId::ValeriaEmberDance};
    if (charId == 5) return {SkillId::VivaldiCadenceStaccato, SkillId::VivaldiFourSeasons, SkillId::VivaldiPerfectRhythm};
    if (charId == 6) return {SkillId::EspinhaNopalSlap, SkillId::EspinhaThornVeil, SkillId::EspinhaThornRecoil};
    if (charId == 7) return {SkillId::MarcovRocketThrust, SkillId::MarcovPlasmaBarrier, SkillId::MarcovZeroGravity};
    return {};
}

SkillLoadout SkillSystem::bossLoadout(int stage) {
    if (stage <= 1) return {SkillId::BossDualSaw, SkillId::BossOverclock, SkillId::BossTitanium};
    if (stage == 2) return {SkillId::BossEmp, SkillId::BossMissiles, SkillId::BossDynamo};
    return {SkillId::BossHolo, SkillId::BossApocalypse, SkillId::BossQuantum};
}

SkillLoadout SkillSystem::randomLoadout(uint32_t charId) {
    std::vector<SkillId> specials, passives;
    for (const SkillDef& s : kSkills) {
        if (s.charId != charId) continue;
        (s.type == SkillType::Passive ? passives : specials).push_back(s.id);
    }
    SkillLoadout out;
    if (!specials.empty()) {
        size_t a = static_cast<size_t>(std::rand()) % specials.size();
        size_t b = specials.size() > 1 ? (a + 1 + static_cast<size_t>(std::rand()) % (specials.size() - 1)) % specials.size() : a;
        out.spec1 = specials[a];
        out.spec2 = specials[b];
    }
    if (!passives.empty()) out.passive = passives[static_cast<size_t>(std::rand()) % passives.size()];
    return out;
}

bool SkillLoadout::canCast(SkillId id) const {
    if (has(id)) return true;
    if (!allSpecials || id == SkillId::None) return false;
    const SkillDef* d = SkillSystem::get(id);
    return d && d->type != SkillType::Passive;
}

} // namespace KoRama
