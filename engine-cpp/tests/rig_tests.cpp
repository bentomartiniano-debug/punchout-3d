// tests/rig_tests.cpp: o rig converte o estado do motor em transformações, sem janela.
#include "render/particles.hpp"
#include "rig/rig.hpp"
#include "render/impact_fx.hpp"
#include "rig/ropes.hpp"
#include <algorithm>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <cmath>
#include <cstdio>
using namespace KoRama;
static int fails = 0;
static void near(const char* what, float a, float b) { if (std::fabs(a - b) > 1e-3f) { std::printf("FALHOU %s: %f != %f\n", what, a, b); fails++; } }
int main() {
    Fighter p(4, true), e(4, false);
    p.worldPos = {0, 0, -125}; p.facing = 0;
    e.worldPos = {0, 0, 125}; e.facing = kRigPi;
    p.gloveL = {-32, 64, 12, 54, 54, 0, 0}; p.gloveR = {32, 64, 100, 54, 54, 0, 0};
    e.gloveL = p.gloveL; e.gloveR = p.gloveR;
    RigRest rest; rest.WAIST = 0.61f; rest.NECK = 0.92f; rest.HIPS = 0.63f; rest.legLen = 0.39f;
    rest.hip[0] = {-0.3f, 0.45f, 0}; rest.hip[1] = {0.3f, 0.45f, 0};
    rest.foot[0] = {-0.3f, 0.06f, 0}; rest.foot[1] = {0.3f, 0.06f, 0};
    const FighterPose a = computePose(p, rest, 0), b = computePose(e, rest, 0);
    near("jogador z", a.root[3][2], -125 * kWorldUnit - kSpread);
    near("oponente z", b.root[3][2], 125 * kWorldUnit + kSpread);
    near("jogador olha +z", (a.root * glm::vec4(0, 0, 1, 0)).z, 1);
    near("oponente olha -z", (b.root * glm::vec4(0, 0, 1, 0)).z, -1);
    near("luva L vai ao lado +1", a.glove[1].x, 32 * kGloveX);
    near("luva R vai ao lado -1", a.glove[0].x, -32 * kGloveX);
    near("altura da luva", a.glove[1].y, rest.handY + (64 - p.baseGloveL.y) * kRigUnit);
    near("soco vai à frente", a.glove[0].z, kGloveZ0 + 88 * kRigUnit + 2.f * kSpread);
    p.offsetY = -16; const FighterPose c = computePose(p, rest, 0);
    near("agacha", c.upperPos.y, rest.WAIST - 16 * kRigUnit);
    near("pé fica no chão", c.footPos[0].y, rest.foot[0].y);
    near("guarda: pé de trás", computePose(e, rest, 0).footPos[1].z, -kStanceBack);
    near("perna alcança o pé", c.legPos[0].y + c.legDirLen[0].y, rest.hip[0].y - rest.legLen);
    const FighterPose d = computePose(p, rest, 1);
    // nocauteada: não deita; fica de pé escorada nas cordas, inclinada para trás e com os braços sobre a corda
    if ((d.root * glm::vec4(0, 1, 0, 0)).y < 0.9f) { std::printf("FALHOU deitou no chão\n"); fails++; }
    if ((d.root * glm::vec4(0, 1, 0, 0)).z >= -0.1f) { std::printf("FALHOU não inclinou para trás\n"); fails++; }
    // a luva fica em cima da corda de cima (no mundo, já com o corpo inclinado), não dentro dela
    near("luva em cima da corda", (d.root * glm::vec4(d.glove[0], 1)).y - d.root[3][1], RingRopes::kHeights[2] + kGloveDrape);
    { RigRest big = rest; big.scale = 1.4f; PoseCtx high; high.ropeTop = 1.5f; const FighterPose g = computePose(p, big, 1, high);
      const float y = (g.root * glm::vec4(g.glove[0] * big.scale, 1)).y - g.root[3][1]; if (std::fabs(y - (1.5f + kGloveDrape)) > 0.06f) { std::printf("FALHOU luva com escala e corda alta: %f\n", y); fails++; } }
    if (d.glove[0].z >= 0 || d.glove[1].z >= 0) { std::printf("FALHOU braços não vão para a corda atrás\n"); fails++; }
    // passos: o pé fica plantado no chão enquanto o corpo anda pouco; longe demais ele dá um passo, levanta e chega lá
    Fighter wk(4, true); wk.worldPos = {0, 0, -125}; RigRest rw = rest; FootStepper fs;
    FighterPose w0 = computePose(wk, rw, 0); fs.tick(w0, rw); const glm::vec3 foot0 = w0.footPos[1];
    wk.worldPos = {0, 0, -123}; FighterPose w1 = computePose(wk, rw, 0); fs.tick(w1, rw);
    near("pé plantado fica no mundo", w1.footPos[1].z, foot0.z - 2 * (kWorldUnit + kSpread / 125.f));   // perto do meio o afastamento extra cresce com o raio
    wk.worldPos = {0, 0, -90}; float maxLift = 0; bool moved = false; FighterPose w2;
    for (int i = 0; i < 40; i++) { w2 = computePose(wk, rw, 0); fs.tick(w2, rw); maxLift = std::max({maxLift, w2.footPos[0].y - rw.foot[0].y, w2.footPos[1].y - rw.foot[1].y}); if (fs.t[0] >= 0 || fs.t[1] >= 0) moved = true; }
    if (!moved || maxLift < 0.05f) { std::printf("FALHOU não deu passo: %f\n", maxLift); fails++; }
    near("o passo chega no lugar", w2.footPos[1].z, computePose(wk, rw, 0).footPos[1].z);
    near("a perna acompanha o pé", w2.legPos[1].z + w2.legDirLen[1].z, w2.footPos[1].z);
    // a perna sai do quadril girado (como no vinil), e não do lugar parado
    Fighter tw(4, true); tw.gloveL.z = 150; tw.gloveR.z = 12; const FighterPose tp = computePose(tw, rest, 0);
    if (std::fabs(tp.legPos[0].z - tp.hipsPos.z) < 0.02f) { std::printf("FALHOU perna não gira com o quadril\n"); fails++; }
    // a perna fica dentro da saia até a barra, parada, girando e com o soco (medidas da saia da Valéria em valeria.js)
    RigRest rv = rest; rv.HIPS = 0.629f; rv.legLen = 0.392f; rv.hip[0] = {-0.306f, 0.449f, 0}; rv.hip[1] = {0.306f, 0.449f, 0}; rv.foot[0] = {-0.306f, 0.057f, 0}; rv.foot[1] = {0.306f, 0.057f, 0};
    for (int pose = 0; pose < 3; pose++) { Fighter lg(4, true); if (pose == 1) { lg.gloveL.z = 180; lg.gloveR.z = 12; } if (pose == 2) { lg.gloveL.z = 12; lg.gloveR.z = 180; lg.bodySkew = 0.3f; }
        const FighterPose lp = computePose(lg, rv, 0); const glm::quat inv = glm::inverse(glm::quat(glm::vec3(lp.lean * 0.45f, lp.twist * 0.5f, lp.roll * 0.55f)));
        for (int i = 0; i < 2; i++) for (float u = 0; u <= 1.001f; u += 0.1f) { const glm::vec3 w = lp.legPos[i] + lp.legDirLen[i] * u; if (w.y < 0.27f) continue; const glm::vec3 q2 = inv * (w - lp.hipsPos); const float h = (0.449f - (q2.y + rv.HIPS)) / (0.449f - 0.253f), half = 0.33f + (0.498f - 0.33f) * std::clamp(h, 0.f, 1.f); const float rr = std::hypot((std::fabs(q2.x) + 0.05f) / half, (std::fabs(q2.z) + 0.05f) / (half * 0.74f)); if (rr > 1.f) { std::printf("FALHOU perna fura a saia: pose %d lado %d u %.1f (%.2f)\n", pose, i, u, rr); fails++; } } }
    // defesa: a sombrinha abre aos poucos e fecha ao soltar
    Fighter bl(4, true); PoseSmoother bs; bs.tick(bl); near("guarda fechada", bs.open, 0);
    bl.state = FIGHTER_BLOCK; bs.tick(bl); if (!(bs.open > 0 && bs.open < 0.5f)) { std::printf("FALHOU abriu de uma vez: %f\n", bs.open); fails++; }
    for (int i = 0; i < 30; i++) bs.tick(bl);
    near("aberta na defesa", bs.open, 1);
    bl.state = FIGHTER_IDLE; for (int i = 0; i < 40; i++) bs.tick(bl);
    near("fecha ao soltar", bs.open, 0);
    // peças soltas: ficam para trás quando o corpo anda, com limite, e voltam
    LagSpring lg; lg.tick(glm::vec3(0.f)); for (int i = 1; i < 5; i++) lg.tick(glm::vec3(0, 0, i * 0.02f));
    if (!(lg.o.z < -0.001f)) { std::printf("FALHOU sem atraso: %f\n", lg.o.z); fails++; }
    for (int i = 0; i < 400; i++) lg.tick(glm::vec3(0, 0, 0.08f));
    if (glm::length(lg.o) > 0.002f) { std::printf("FALHOU atraso não volta: %f\n", glm::length(lg.o)); fails++; }
    // nas cordas os dois lutadores animam igual: o balanço vem do tempo e o motor (que só balança o oponente) não conta
    Fighter fa(4, true), fb(4, false); fb.offsetX = 8; fb.bodySkew = 0.2f; fb.offsetY = -6;
    PoseCtx h0, h1; h0.hangT = 40.f; h1.hangT = 90.f;
    const FighterPose pa = computePose(fa, rest, 1, h0), pb = computePose(fb, rest, 1, h0), pc2 = computePose(fa, rest, 1, h1);
    near("jogadora e oponente iguais x", pa.upperPos.x, pb.upperPos.x); near("jogadora e oponente iguais y", pa.upperPos.y, pb.upperPos.y); near("jogadora e oponente iguais roll", pa.roll, pb.roll);
    if (std::fabs(pa.neckTilt - pc2.neckTilt) + std::fabs(pa.upperPos.x - pc2.upperPos.x) < 0.01f) { std::printf("FALHOU congelada nas cordas\n"); fails++; }
    PoseCtx hrope; hrope.ropeBounce = 1.f; const float bz = (computePose(fa, rest, 1, hrope).root[3] - computePose(fa, rest, 1).root[3]).z;
    if (!(bz > 0.2f)) { std::printf("FALHOU sem tranco ao bater na corda: %f\n", bz); fails++; }
    // perto do meio a escala é a da luta; no fim do voo (raio 232) o corpo chega na corda, em qualquer ângulo
    Fighter st(4, true); st.worldPos = {0, 0, -180}; near("de pé não estica", rigPos(st).z, -180 * kWorldUnit - kSpread);   // kSpread: o afastamento extra para a câmera
    Fighter fl(4, true); fl.state = FIGHTER_KNOCKDOWN; fl.worldPos = {0, 0, -232}; near("voo chega na corda", rigPos(fl).z, -(RingRopes::kHalf - kRopeGap));
    fl.worldPos = {164.05f, 0, 164.05f}; const glm::vec3 diag = rigPos(fl); near("na diagonal vai à quina", std::max(std::fabs(diag.x), std::fabs(diag.z)), RingRopes::kHalf - kRopeGap);
    // ao levantar a posição desliza de volta, sem teleportar
    fl.worldPos = {0, 0, -232}; PoseSmoother back; back.tick(fl); fl.state = FIGHTER_IDLE; fl.worldPos = {0, 0, -125}; back.tick(fl);
    if (!(back.pos.z < -2.f)) { std::printf("FALHOU teleportou ao levantar: %f\n", back.pos.z); fails++; }
    for (int i = 0; i < 90; i++) back.tick(fl);
    near("volta para a luta", back.pos.z, -125 * kWorldUnit - kSpread);
    PoseCtx pc; pc.hasPos = true; pc.pos = {0.5f, 0, 0.25f}; near("usa a posição suavizada", computePose(fl, rest, 0, pc).root[3][0], 0.5f);
    // cordas: cedem para fora onde o corpo encosta, a de cima mais, presas nos postes, e voltam
    RingRopes ro; const glm::vec2 body(0, -RingRopes::kHalf - 0.2f);   // tronco escorado, passando da linha das cordas
    for (int i = 0; i < 120; i++) ro.tick(&body, 1, 0.3f);
    if (!(ro.d[0][2] > 0.3f && ro.d[0][2] >= ro.d[0][0] - 1e-4f)) { std::printf("FALHOU corda não cedeu: %f %f\n", ro.d[0][2], ro.d[0][0]); fails++; }
    // corpo escorado inclinado para trás: a de cima cede mais, e nenhuma corda fica dentro do tronco (abre na hora, sem esperar a mola)
    { RingRopes rl; RopeBody b; b.base = glm::vec2(0, -RingRopes::kHalf + 0.1f); b.lean = glm::vec2(0, -0.48f); b.r = 0.45f; rl.tick(&b, 1);
      for (int r = 0; r < rl.rows; r++) { const float need = std::min(RingRopes::kMaxBulge, -(b.base.y + b.lean.y * rl.heights[r]) - RingRopes::kHalf + b.r); if (rl.d[0][r] < need - 1e-4f) { std::printf("FALHOU corda dentro do corpo: fileira %d %f < %f\n", r, rl.d[0][r], need); fails++; } }
      if (!(rl.d[0][2] > rl.d[0][0])) { std::printf("FALHOU corpo inclinado: a de cima não cedeu mais\n"); fails++; } }
    near("não cede nos outros lados", ro.d[2][2], 0);
    near("presa no poste", ro.point(0, 2, 0).z, -RingRopes::kHalf);
    if (!(ro.point(0, 2, 0.5f).z < -RingRopes::kHalf - 0.25f)) { std::printf("FALHOU corda não foi para fora: %f\n", ro.point(0, 2, 0.5f).z); fails++; }
    for (int i = 0; i < 200; i++) ro.tick(nullptr, 0, 0.3f);
    if (std::fabs(ro.d[0][2]) > 0.01f) { std::printf("FALHOU corda não voltou\n"); fails++; }
    // a luva para no corpo do alvo
    Fighter q(4, true); q.worldPos = {0, 0, -125}; q.gloveR = {20, 100, 180, 54, 54, 0, 0}; q.gloveL = {-14, 68, 10, 54, 54, 0, 0};
    PoseCtx ctx; ctx.reachLimit = 1.3f;
    const FighterPose hit = computePose(q, rest, 0, ctx);
    near("luva para no alvo", hit.glove[0].z, 1.3f);
    near("luva amassa ao bater", hit.gloveScale[0].z, 0.7f);
    near("a outra luva não amassa", hit.gloveScale[1].z, 1.f);
    near("sem alvo segue reto", computePose(q, rest, 0).glove[0].z, kGloveZ0 + 168 * kRigUnit + 2.f * kSpread);   // no soco a luva cobre o afastamento extra
    // giro de mão: o soco gira a luva e cada golpe tem o seu arco
    Fighter j(4, true); j.isLeftPunch = false; j.state = FIGHTER_JAB; j.maxTimer = 100; j.timer = 50;
    const FighterPose jab = computePose(j, rest, 0);
    if (std::fabs(jab.gloveEuler[0].z) < 0.5f) { std::printf("FALHOU jab sem giro de mão\n"); fails++; }
    near("a mão parada não gira", jab.gloveEuler[1].z, 0);
    j.state = FIGHTER_HOOK; j.timer = 75; const float hookIni = computePose(j, rest, 0).gloveEuler[0].y; j.timer = 35; const float hookFim = computePose(j, rest, 0).gloveEuler[0].y;
    if (!(hookIni < -0.3f && hookFim > 0.3f)) { std::printf("FALHOU gancho não varre: %f %f\n", hookIni, hookFim); fails++; }
    j.state = FIGHTER_UPPER; j.timer = 75; const float upIni = computePose(j, rest, 0).gloveEuler[0].x; j.timer = 35; const float upFim = computePose(j, rest, 0).gloveEuler[0].x;
    if (!(upIni > 0.2f && upFim < -0.3f)) { std::printf("FALHOU uppercut não sobe: %f %f\n", upIni, upFim); fails++; }
    // deformação: amassa de uma vez no golpe, balança e volta
    ImpactSpring sp; sp.kick(0.8f); near("amassa na hora", sp.s, 0.8f);
    bool crossed = false; for (int i = 0; i < 90; i++) { sp.tick(); if (sp.s < -0.02f) crossed = true; }
    if (!crossed) { std::printf("FALHOU a mola não balança\n"); fails++; }
    if (std::fabs(sp.s) > 0.02f) { std::printf("FALHOU a mola não volta: %f\n", sp.s); fails++; }
    PoseCtx sq; sq.squash = 1.f; Fighter h(4, false); h.hitKind = 1; const FighterPose hs = computePose(h, rest, 0, sq);
    if (!(hs.headScale.y < 0.8f && hs.headScale.x > 1.15f)) { std::printf("FALHOU cabeça não amassa\n"); fails++; }
    near("corpo fica quase igual no golpe no rosto", hs.bodyScale.y > 0.9f ? 1.f : 0.f, 1);
    h.hitKind = 3; const FighterPose hb = computePose(h, rest, 0, sq);
    if (!(hb.bodyScale.y < 0.85f)) { std::printf("FALHOU corpo não amassa\n"); fails++; }
    // impacto: mesmas palavras e tempos do jogo 2D
    ImpactFx fx; fx.spawn(0, 4, true); near("leve dura 16", float(fx.timer), 16);
    if (std::string(fx.word()) != "TUM!") { std::printf("FALHOU palavra leve\n"); fails++; }
    fx.spawn(2, 0, false); near("contra-ataque dura 26", float(fx.timer), 26);
    if (std::string(fx.word()) != "KA-POW!" || fx.byPlayer) { std::printf("FALHOU contra-ataque\n"); fails++; }
    for (int i = 0; i < 26; i++) fx.tick();
    if (fx.alive) { std::printf("FALHOU impacto não termina\n"); fails++; }
    // as mãos deslizam: a troca de pose do motor não teleporta a luva
    Fighter g(4, true); g.gloveL = {-32, 64, 12, 54, 54, 0, 0}; g.gloveR = {32, 64, 12, 54, 54, 0, 0};
    PoseSmoother sm; sm.tick(g); near("começa na pose", sm.view.gloveL.y, 64);
    g.gloveL.y = 110; g.gloveR.y = 110; sm.tick(g);
    if (!(sm.view.gloveL.y > 64 && sm.view.gloveL.y < 80)) { std::printf("FALHOU a luva teleportou: %f\n", sm.view.gloveL.y); fails++; }
    for (int i = 0; i < 80; i++) sm.tick(g);
    near("a luva chega lá", sm.view.gloveL.y, 110);
    g.state = FIGHTER_JAB; g.isLeftPunch = true; g.gloveL.z = 112; g.gloveR.z = 112; sm.tick(g);
    if (!(sm.view.gloveL.z > 50)) { std::printf("FALHOU o soco ficou lento: %f\n", sm.view.gloveL.z); fails++; }
    if (!(sm.view.gloveL.z > sm.view.gloveR.z + 20)) { std::printf("FALHOU a mão do soco não é mais rápida\n"); fails++; }
    g.offsetX = 40; sm.tick(g); if (!(sm.view.offsetX > 0 && sm.view.offsetX < 40)) { std::printf("FALHOU o corpo teleportou: %f\n", sm.view.offsetX); fails++; }
    // sombrinha: parada não gira; no soco gira só a da mão que soca, e fica onde parou
    Fighter us(4, true); PoseSmoother um; um.tick(us); um.tick(us); near("parada não gira", um.spin[1], 0);
    us.state = FIGHTER_JAB; us.isLeftPunch = true; us.maxTimer = 20; us.timer = 10; um.tick(us);
    if (!(um.spin[1] > 0.f)) { std::printf("FALHOU a sombrinha do soco não girou: %f\n", um.spin[1]); fails++; }
    near("a outra mão não gira", um.spin[0], 0);
    const float stopped = um.spin[1]; us.state = FIGHTER_IDLE; um.tick(us); near("acabou o soco, para", um.spin[1], stopped);
    // estouro: cada golpe tem a sua poeira; a faísca sobe, cai deixando rastro e some
    { const auto dust = [](const Particles& b) { glm::vec3 c(0.f); int k = 0; for (const Particle& q : b.list) if (q.kind == kPuff) { c += q.p; k++; } return glm::vec4(c / float(std::max(k, 1)), float(k)); };
      Particles a, b; a.burst(glm::vec3(0, 1, 0), 1, 1); b.burst(glm::vec3(0, 1, 0), 1, 2);
      if (glm::length(dust(a) - dust(b)) < 0.05f) { std::printf("FALHOU a poeira saiu igual em dois golpes\n"); fails++; }
      const auto firstSpark = [](const Particles& p) -> const Particle* { for (const Particle& q : p.list) if (q.kind == kSpark) return &q; return nullptr; };
      Particles f; f.burst(glm::vec3(0, 1, 0), 1, 3); const Particle* s0 = firstSpark(f); const float life = s0->life; float peak = s0->p.y, y = peak; for (int i = 0; i < 20; i++) { f.tick(); const Particle* s = firstSpark(f); y = s->p.y; peak = std::max(peak, y); }
      if (!(firstSpark(f)->trailN >= 8)) { std::printf("FALHOU a faísca sem rastro\n"); fails++; }
      if (!(life >= 30.f)) { std::printf("FALHOU a faísca dura pouco: %f\n", life); fails++; }
      f.burst(glm::vec3(0, 0.1f, 0), 1, 4); for (int i = 0; i < 200; i++) f.tick(); for (const Particle& q : f.list) if (q.kind == kSpark) { std::printf("FALHOU a faísca não sumiu\n"); fails++; break; } }
    // tecido: o corpo acelera e o pano fica para trás; parado de novo, volta perto do lugar
    { ClothSim cs; const glm::mat4 I(1.f); glm::vec3 pos(0.f); cs.tick(pos, I, 0.f, 0.f); cs.tick(pos, I, 0.f, 0.f); for (int i = 0; i < 6; i++) { pos.z += 0.02f * float(i); cs.tick(pos, I, 0.f, 0.f); }
      if (!(cs.o[0].z < -0.005f)) { std::printf("FALHOU o pano não ficou para trás: %f\n", cs.o[0].z); fails++; }
      float peak = 0.f; for (int i = 0; i < 300; i++) { cs.tick(pos, I, 0.f, 0.f); if (i < 30) peak = std::max(peak, glm::length(cs.o[0])); }
      if (!(glm::length(cs.o[0]) < 0.03f)) { std::printf("FALHOU o pano não voltou: %f\n", glm::length(cs.o[0])); fails++; } }
    // colmeia: a boca (Z da peça) olha para a frente na guarda
    { Fighter hf(4, true); RigRest hr; hr.glove = GloveMode::Hive; const FighterPose hp = computePose(hf, hr, 0, PoseCtx{}); const glm::vec3 mouth = hp.gloveRot[0] * glm::vec3(0, 0, 1); if (!(mouth.z > 0.8f)) { std::printf("FALHOU a colmeia virada para trás: %f\n", mouth.z); fails++; } }
    // queda na quina: fica na corda, longe do poste, nos quatro cantos; no meio da corda nada muda
    { Fighter ck(4, true); ck.state = FIGHTER_KNOCKDOWN; const float side = RingRopes::kHalf - kCornerClear;
      for (const float sx : {-1.f, 1.f}) for (const float sz : {-1.f, 1.f}) { ck.worldPos = {sx * 164.05f, 0, sz * 164.05f}; const glm::vec3 q = rigPos(ck); if (!(std::min(std::fabs(q.x), std::fabs(q.z)) <= side + 1e-3f)) { std::printf("FALHOU a queda entrou na quina: %f %f\n", q.x, q.z); fails++; } if (!(std::max(std::fabs(q.x), std::fabs(q.z)) > side)) { std::printf("FALHOU a queda não chegou na corda: %f %f\n", q.x, q.z); fails++; } }
      ck.worldPos = {0, 0, 232}; near("meio da corda nao muda", rigPos(ck).z, RingRopes::kHalf - kRopeGap); }
    // salto (offsetY do motor): os pés sobem com o corpo, encolhidos, em vez de ficarem no chão esticando a perna; tronco e quadril sobem juntos
    { Fighter jp(6, false); jp.worldPos = {0, 0, 125}; jp.offsetY = 200; RigRest rj = rest; FootStepper fj; FighterPose pj = computePose(jp, rj, 0); fj.tick(pj, rj);
      const float up = 200 * kRigUnit, feet = pj.footPos[0].y - rj.foot[0].y;
      if (!(feet > up - 0.1f && feet < up + 0.15f)) { std::printf("FALHOU pés no salto: %f (corpo %f)\n", feet, up); fails++; }
      if (!(glm::length(pj.legDirLen[0]) < rj.legLen + 0.05f)) { std::printf("FALHOU perna esticou no salto: %f\n", glm::length(pj.legDirLen[0])); fails++; }
      const float gap = (pj.upperPos.y - pj.hipsPos.y) - (rj.WAIST - rj.HIPS); if (std::fabs(gap) > 0.05f) { std::printf("FALHOU corpo dividido no salto: %f\n", gap); fails++; } }
    // bota acima do chão no modelo (Espinha: 0,29 m): de pé não é salto, o pé continua dando passo
    { Fighter wk2(6, true); wk2.worldPos = {0, 0, -125}; RigRest rb = rest; rb.foot[0].y = rb.foot[1].y = 0.29f; FootStepper fb; FighterPose b0 = computePose(wk2, rb, 0); fb.tick(b0, rb);
      wk2.worldPos = {0, 0, -90}; bool stepped = false; for (int i = 0; i < 40; i++) { FighterPose bq = computePose(wk2, rb, 0); fb.tick(bq, rb); if (fb.t[0] >= 0 || fb.t[1] >= 0) stepped = true; }
      if (!stepped) { std::printf("FALHOU bota alta não dá passo\n"); fails++; } }
    // esquiva rápida: o pé plantado não fica mais longe que a folga do lugar de guarda
    { Fighter dg(6, true); dg.worldPos = {0, 0, -125}; RigRest rd = rest; FootStepper fd; FighterPose q0 = computePose(dg, rd, 0); fd.tick(q0, rd);
      dg.offsetX = 60; FighterPose q1 = computePose(dg, rd, 0); const glm::vec3 home = q1.footPos[0]; fd.tick(q1, rd);
      const float d = glm::length(glm::vec2(q1.footPos[0].x - home.x, q1.footPos[0].z - home.z)); if (d > kFootSlack + 1e-3f) { std::printf("FALHOU pé longe da guarda na esquiva: %f\n", d); fails++; } }
    // perna reta (Espinha): passo mais cedo, o pé fica mais perto da guarda
    { Fighter dg(6, true); dg.worldPos = {0, 0, -125}; RigRest rd = rest; rd.stance = 0.5f; rd.hipIn = 1.f; FootStepper fd; FighterPose q0 = computePose(dg, rd, 0); fd.tick(q0, rd);
      dg.offsetX = 60; FighterPose q1 = computePose(dg, rd, 0); const glm::vec3 home = q1.footPos[0]; fd.tick(q1, rd);
      const float d = glm::length(glm::vec2(q1.footPos[0].x - home.x, q1.footPos[0].z - home.z)); if (d > kFootSlack * 0.55f + 1e-3f) { std::printf("FALHOU perna reta longe da guarda: %f\n", d); fails++; } }
    // tentando levantar na contagem: continua escorada (sem meia-pose dura), mas o corpo sobe e a cabeça se ergue
    { Fighter kd(6, true); kd.worldPos = {0, 0, -125}; PoseCtx c0, c1; c1.rise = 1.f; c0.hangT = c1.hangT = 0.f; float best = -1.f, tilt0 = 0.f, tilt1 = 0.f;
      for (int k = 0; k < 140; k++) { c0.hangT = c1.hangT = float(k); const FighterPose a = computePose(kd, rest, 1.f, c0), b = computePose(kd, rest, 1.f, c1); best = std::max(best, b.root[3].y - a.root[3].y); tilt0 += a.neckTilt; tilt1 += b.neckTilt; }
      if (!(best > 0.06f)) { std::printf("FALHOU esforço não sobe: %f\n", best); fails++; }
      if (!(tilt1 < tilt0)) { std::printf("FALHOU cabeça não ergue no esforço\n"); fails++; } }
    if (!fails) std::printf("rig ok\n");
    return fails ? 1 : 0;
}
