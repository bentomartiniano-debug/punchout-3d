// app3d/render/fx_clips.hpp: os efeitos dos especiais gravados do vinil (tools3d/export/export_fx.html).
// fx_<lutador>.glb traz as peças (nós fx<i>); fx_<lutador>.kfx traz, por especial e por quadro, onde cada peça visível está em relação ao
// lutador parado no começo do golpe e a cor/opacidade dela. O tocador prende o efeito no lutador, virado para o rival, e estica para a
// frente até a distância do rival no jogo (no vinil o rival ficava a 1,75 m).
#pragma once
#include "model/model.hpp"
#include "skills.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include "fighter.hpp"
namespace KoRama {

struct FxEntry { uint16_t node = 0; glm::mat4 m{1.f}; glm::vec4 c{1.f}; };
struct FxClip { float dur = 0.f, fps = 30.f; std::vector<std::vector<FxEntry>> frames; std::vector<float> rootZ; };   // rootZ: quanto o lutador andou para a frente no vinil

struct FxLibrary {
    Model* model = nullptr; std::map<std::string, FxClip> clips; std::vector<int> nodeOf;   // nó do .kfx -> índice no Model
    std::vector<uint8_t> skip;   // peças gravadas que o jogo refaz em código (render/char_fx.hpp): não entram
    void hide(std::initializer_list<int> ids) { for (int id : ids) { if (size_t(id) >= skip.size()) skip.resize(size_t(id) + 1, 0); skip[size_t(id)] = 1; } }
    bool load(const std::string& kfxPath, Model& m) {
        model = &m; std::ifstream in(kfxPath, std::ios::binary); if (!in) return false;
        const std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()); size_t at = 0;
        const auto take = [&](void* dst, size_t n) { if (at + n > data.size()) return false; std::memcpy(dst, data.data() + at, n); at += n; return true; };
        char magic[4]; uint32_t nSkills = 0; if (!take(magic, 4) || (std::memcmp(magic, "KFX1", 4) != 0 && std::memcmp(magic, "KFX2", 4) != 0) || !take(&nSkills, 4)) return false; const bool v2 = magic[3] == '2';
        uint32_t maxNode = 0; std::vector<std::string> order;
        for (uint32_t s = 0; s < nSkills; s++) {
            uint8_t len = 0; if (!take(&len, 1)) return false; std::string id(len, '\0'); if (!take(id.data(), len)) return false;
            FxClip clip; uint32_t frames = 0; if (!take(&clip.dur, 4) || !take(&clip.fps, 4) || !take(&frames, 4)) return false; clip.frames.resize(frames); clip.rootZ.assign(frames, 0.f);
            for (size_t fi = 0; fi < clip.frames.size(); fi++) { auto& fr = clip.frames[fi]; if (v2 && !take(&clip.rootZ[fi], 4)) return false; uint16_t n = 0; if (!take(&n, 2)) return false; fr.resize(n);
                for (FxEntry& e : fr) { float a[12]; uint8_t c[4]; if (!take(&e.node, 2) || !take(a, 48) || !take(c, 4)) return false;
                    e.m = glm::mat4(glm::vec4(a[0], a[1], a[2], 0), glm::vec4(a[3], a[4], a[5], 0), glm::vec4(a[6], a[7], a[8], 0), glm::vec4(a[9], a[10], a[11], 1));
                    e.c = glm::vec4(c[0], c[1], c[2], c[3]) / 255.f; maxNode = std::max<uint32_t>(maxNode, e.node); } }
            clips[id] = std::move(clip); order.push_back(id);
        }
        // sobra da gravação: o vinil gravou os especiais em sequência, então o começo de um clipe ainda traz as peças do anterior que não tinham
        // sumido (a armadura, o véu, a bolha, o disco do derretido...). Essas peças saem do clipe até o quadro em que somem sozinhas
        for (size_t k = 1; k < order.size(); k++) { const FxClip& prev = clips[order[k - 1]]; FxClip& cur = clips[order[k]]; if (prev.frames.empty() || cur.frames.empty()) continue;
            std::vector<uint16_t> left; for (const FxEntry& e : cur.frames[0]) if (e.c.a >= 0.01f) for (const FxEntry& q : prev.frames.back()) if (q.node == e.node && q.c.a >= 0.01f) { left.push_back(e.node); break; }
            for (uint16_t node : left) for (auto& fr : cur.frames) { const auto it = std::find_if(fr.begin(), fr.end(), [&](const FxEntry& e) { return e.node == node && e.c.a >= 0.01f; }); if (it == fr.end()) break; fr.erase(it); } }
        nodeOf.assign(maxNode + 1, -1); for (uint32_t i = 0; i <= maxNode; i++) { const auto it = m.byName.find("fx" + std::to_string(i)); if (it != m.byName.end()) nodeOf[i] = it->second; }
        return true;
    }
};

// o modo do especial ainda está ativo no motor (armadura, véu, escudo...)
inline bool modeActive(const Fighter& f, SkillId id) {
    const StatusEffects& s = f.status; using I = SkillId;
    switch (id) { case I::JackMolasses: return s.molasses > 0; case I::JackHoneyWall: return s.honeyShield > 0; case I::FrogPoisonSkin: return s.poisonSkin > 0; case I::JoanaCrustaceanShell: return s.shell > 0; case I::CremosoMeltMode: return s.melt > 0;
    case I::ValeriaHeatwaveGuard: return s.heatwave > 0; case I::VivaldiFourSeasons: return s.seasons > 0; case I::EspinhaThornVeil: return s.thornVeil > 0; case I::MarcovPlasmaBarrier: return s.plasma > 0; default: return false; }
}
// especial do motor -> id do especial no vinil
inline const char* vinilSkillId(SkillId id) {
    using I = SkillId;
    switch (id) {
    case I::JackInfestation: return "swarm"; case I::JackMolasses: return "molasses"; case I::JackHoneyWall: return "wall"; case I::JackStingJab: return "sting"; case I::JackCombBurst: return "comb"; case I::JackRoyalJelly: return "jelly";
    case I::FrogTongue: return "tongue"; case I::FrogPoisonSkin: return "poison"; case I::FrogRoyalLeap: return "leap"; case I::FrogMudSlime: return "mud"; case I::FrogFlyCatcher: return "flies"; case I::FrogSwampSpout: return "geyser";
    case I::JoanaClawCrush: return "crush"; case I::JoanaHookPull: return "hook"; case I::JoanaCrustaceanShell: return "shell"; case I::JoanaCoralSpout: return "coral"; case I::JoanaAnchorSlam: return "anchor"; case I::JoanaOceanBounty: return "bounty";
    case I::CremosoSyrupBlast: return "syrup"; case I::CremosoVanillaWhip: return "whip"; case I::CremosoMeltMode: return "melt"; case I::CremosoConePunch: return "cone"; case I::CremosoPuddleTrap: return "puddle"; case I::CremosoCherryTopping: return "cherry";
    case I::ValeriaMagmaPunch: return "magma"; case I::ValeriaVolcanicErupt: return "erupt"; case I::ValeriaHeatwaveGuard: return "guard"; case I::ValeriaLavaKickJab: return "kick"; case I::ValeriaAshCloud: return "ash"; case I::ValeriaPhoenixEmber: return "phoenix";
    case I::VivaldiCadenceStaccato: return "staccato"; case I::VivaldiWinterAllegro: return "winter"; case I::VivaldiFourSeasons: return "seasons"; case I::VivaldiCrescendoJab: return "jab"; case I::VivaldiAutumnRequiem: return "requiem"; case I::VivaldiSpringSonata: return "spring";
    case I::EspinhaNopalSlap: return "slap"; case I::EspinhaLuchaFlyer: return "flyer"; case I::EspinhaThornVeil: return "veil"; case I::EspinhaNeedleJab: return "needle"; case I::EspinhaDesertWind: return "wind"; case I::EspinhaFridaTears: return "tears";
    case I::MarcovRocketThrust: return "rocket"; case I::MarcovOrbitalLaser: return "laser"; case I::MarcovPlasmaBarrier: return "plasma"; case I::MarcovKineticOverdrive: return "kinetic"; case I::MarcovExhaustCloud: return "exhaust"; case I::MarcovOxygenRecharge: return "oxygen";
    case I::BossDualSaw: return "saw"; case I::BossOverclock: return "overclock"; case I::BossEmp: return "emp"; case I::BossMissiles: return "missiles"; case I::BossHolo: return "holo"; case I::BossApocalypse: return "apocalypse";
    default: return nullptr;
    }
}

// toca um clipe preso a um lutador: durante o especial o tempo segue o progresso do golpe no motor; depois continua até o fim do clipe
struct FxPlayer {
    const FxLibrary* lib = nullptr; const FxClip* clip = nullptr; float t = 0.f; glm::mat4 frame{1.f}; float stretch = 1.f;
    SkillId skill = SkillId::None; bool inSkill = false, follow = false; float scale = 1.f;   // follow: modo (armadura, véu, escudo): acompanha o lutador e repete o fim enquanto o modo dura
    static constexpr float kVinilDist = 1.75f;   // distância do rival no vinil
    static constexpr float kFxSize = 1.5f;      // o vinil é visto de 3 m; a câmera da luta fica bem mais longe, então cada peça cresce
    bool has(const FxLibrary* l, SkillId id) const { const char* k = vinilSkillId(id); return l && k && l->clips.count(k); }
    // feet: centro do lutador no chão; fwd: para o rival (horizontal); dist: até o rival; scale: escala do modelo
    void start(const FxLibrary* l, SkillId id, const glm::vec3& feet, const glm::vec3& fwd, float dist, float scale) {
        lib = l; clip = nullptr; skill = id; inSkill = true; t = 0.f; const char* k = vinilSkillId(id); if (!l || !k) return; const auto it = l->clips.find(k); if (it == l->clips.end()) return; clip = &it->second;
        const SkillDef* d = SkillSystem::get(id); follow = (d && d->type == SkillType::Mode) || id == SkillId::JackHoneyWall; this->scale = scale;
        reframe(feet, fwd); stretch = follow ? 1.f : std::max(0.6f, dist / (kVinilDist * std::max(0.3f, scale)));   // o vinil põe o rival a 1,75 m
    }
    void reframe(const glm::vec3& feet, const glm::vec3& fwd) {
        const glm::vec3 f = glm::normalize(glm::vec3(fwd.x, 0.f, fwd.z) + glm::vec3(0, 0, 1e-5f)), right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), f));
        frame = glm::mat4(glm::vec4(right * scale, 0), glm::vec4(0, scale, 0, 0), glm::vec4(f * scale, 0), glm::vec4(feet, 1));
    }
    // progress: 0..1 do golpe no motor enquanto ele dura; fora do golpe, -1 (o clipe segue no tempo real)
    // modeOn: o modo deste especial ainda está ativo no motor (só vale para follow)
    void tick(float progress, bool modeOn = false) {
        if (!clip) return; if (progress >= 0.f && inSkill) { t = progress * clip->dur; return; }
        inSkill = false; t += 1.f / 60.f; const float end = float(clip->frames.size()) / clip->fps;
        if (follow) { if (!modeOn) { clip = nullptr; return; } const float loop0 = std::min(clip->dur, end - 0.2f), len = std::max(0.2f, end - loop0); if (t >= end) t = loop0 + std::fmod(t - loop0, len); return; }
        if (t >= end) clip = nullptr; }
    bool active() const { return clip != nullptr; }
    // põe as peças do quadro atual no Model (mundo, cor e visibilidade); devolve se há algo para desenhar
    // modos (follow): as peças gravadas em volta da cabeça e do tronco andam com eles. head/body levam do lugar de repouso (o lutador parado
    // como no vinil) ao lugar de agora, com inclinação, balanço e deformação; neckY/waistY são as alturas do pescoço e da cintura no repouso
    struct Anchor { glm::mat4 head{1.f}, body{1.f}; float neckY = 0.f, waistY = 0.f, push = 0.05f; bool on = false; };
    bool pose(Model& m) const { return pose(m, Anchor{}); }
    bool pose(Model& m, const Anchor& an) const {
        for (Node& n : m.nodes) { n.visible = false; n.tint = glm::vec4(-1.f); }
        if (!clip || clip->frames.empty()) return false; if (m.rootNode >= 0) m.nodes[size_t(m.rootNode)].visible = true;
        const size_t fi = std::min(clip->frames.size() - 1, size_t(t * clip->fps)); bool any = false;
        for (const FxEntry& e : clip->frames[fi]) { if (e.node >= lib->nodeOf.size() || lib->nodeOf[e.node] < 0 || e.c.a < 0.01f) continue; if (e.node < lib->skip.size() && lib->skip[e.node]) continue; Node& n = m.nodes[size_t(lib->nodeOf[e.node])];
            // a peça cresce no mesmo lugar; a trajetória estica para a frente até o rival, mas o avanço do próprio lutador no golpe não estica
            const float rz = clip->rootZ.empty() ? 0.f : clip->rootZ[fi]; glm::mat4 em = e.m; em[3].z = rz + (em[3].z - rz) * stretch; em[0] *= kFxSize; em[1] *= kFxSize; em[2] *= kFxSize; glm::mat4 fix(1.f);
            if (follow && an.on) { const glm::vec3 p(em[3]); const float hz = glm::length(glm::vec2(p.x, p.z));
                if (p.y > an.neckY * 0.9f && hz < 0.7f) { fix = an.head;
                    // enfeite do cabelo: um pouco para fora (a peruca 3D é mais cheia que a do vinil) e cresce ao entrar e encolhe ao sair
                    const glm::vec3 hc(0.f, an.neckY + 0.14f, 0.f); em[3] += glm::vec4(glm::normalize(p - hc + glm::vec3(1e-5f)) * an.push, 0.f);
                    const auto present = [&](size_t f) { for (const FxEntry& x : clip->frames[f]) if (x.node == e.node && x.c.a >= 0.01f) return true; return false; };
                    int back = 0, ahead = 0; while (back < 8 && fi >= size_t(back + 1) && present(fi - size_t(back) - 1)) back++; while (ahead < 8 && fi + size_t(ahead) + 1 < clip->frames.size() && present(fi + size_t(ahead) + 1)) ahead++;
                    float k = std::min(1.f, float(std::min(back, ahead) + 1) / 6.f); k = k * k * (3.f - 2.f * k); em[0] *= k; em[1] *= k; em[2] *= k; } }
            n.world = fix * frame * em; n.tint = e.c; n.visible = true; any = true; }
        return any;
    }
};
}
