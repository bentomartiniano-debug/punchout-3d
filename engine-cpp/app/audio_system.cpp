#include "audio_system.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>

namespace KoRama {

namespace {

constexpr unsigned kRate = 44100;
constexpr float kTwoPi = 6.28318531f;

// ---------------------------------------------------------------------------
// Sintetizador offline com a semântica do WebAudio
// ---------------------------------------------------------------------------

enum class Wave { Sine, Square, Saw, Triangle, Noise };
enum class Filter { None, LowPass, HighPass, BandPass };

// AudioParam: setValueAtTime, linearRampToValueAtTime e exponentialRampToValueAtTime.
// As rampas começam no evento anterior, como no navegador.
class Param {
public:
    Param& set(float v, float t) { ev_.push_back({Kind::Set, t, v}); return *this; }
    Param& lin(float v, float t) { ev_.push_back({Kind::Linear, t, v}); return *this; }
    Param& exp(float v, float t) { ev_.push_back({Kind::Exp, t, v}); return *this; }
    bool empty() const { return ev_.empty(); }

    float at(float t) const {
        if (ev_.empty()) return 0.0f;
        float v = ev_[0].v, t0 = ev_[0].t;
        for (std::size_t i = 1; i < ev_.size(); ++i) {
            const Ev& e = ev_[i];
            if (t < e.t) {
                if (e.k == Kind::Set) return v;
                float u = (t - t0) / std::max(1e-6f, e.t - t0);
                if (e.k == Kind::Linear) return v + (e.v - v) * u;
                if (v <= 0.0f || e.v <= 0.0f) return v; // rampa exponencial exige valores positivos
                return v * std::pow(e.v / v, u);
            }
            v = e.v;
            t0 = e.t;
        }
        return v;
    }

private:
    enum class Kind { Set, Linear, Exp };
    struct Ev { Kind k; float t; float v; };
    std::vector<Ev> ev_;
};

struct Voice {
    Wave wave = Wave::Sine;
    float start = 0.0f, stop = 0.0f;
    Param freq, gain;
    Filter filter = Filter::None;
    Param cutoff;
    float q = 0.707f;
};

// Biquad do "Audio EQ Cookbook" (o mesmo modelo do BiquadFilterNode); passa-faixa com pico de 0 dB.
struct Biquad {
    float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    float process(Filter type, float in, float freq, float q) {
        freq = std::clamp(freq, 10.0f, kRate * 0.45f);
        const float w0 = kTwoPi * freq / kRate, cw = std::cos(w0), alpha = std::sin(w0) / (2.0f * std::max(0.05f, q));
        float b0, b1, b2;
        if (type == Filter::LowPass) { b0 = (1 - cw) / 2; b1 = 1 - cw; b2 = (1 - cw) / 2; }
        else if (type == Filter::HighPass) { b0 = (1 + cw) / 2; b1 = -(1 + cw); b2 = (1 + cw) / 2; }
        else { b0 = alpha; b1 = 0; b2 = -alpha; }
        const float a0 = 1 + alpha, a1 = -2 * cw, a2 = 1 - alpha;
        float out = (b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2) / a0;
        x2 = x1; x1 = in; y2 = y1; y1 = out;
        return out;
    }
};

void renderVoice(const Voice& v, std::vector<float>& out, std::mt19937& rng) {
    std::uniform_real_distribution<float> noise(-1.0f, 1.0f);
    Biquad bq;
    float phase = 0.0f;
    // Ruído branco num filtro estreito perde quase toda a energia (no JS a lava e a onda mal se ouvem):
    // compensa pelo corte inicial, até 6×.
    float makeup = 1.0f;
    if (v.wave == Wave::Noise && (v.filter == Filter::LowPass || v.filter == Filter::BandPass)) {
        makeup = std::clamp(std::sqrt(5000.0f / std::max(1.0f, v.cutoff.at(v.start))), 1.0f, 6.0f);
    }
    const std::size_t from = static_cast<std::size_t>(std::max(0.0f, v.start) * kRate);
    const std::size_t to = std::min(out.size(), static_cast<std::size_t>(v.stop * kRate));
    for (std::size_t i = from; i < to; ++i) {
        const float t = static_cast<float>(i) / kRate;
        float s;
        if (v.wave == Wave::Noise) {
            s = noise(rng);
        } else {
            phase += v.freq.at(t) / kRate;
            phase -= std::floor(phase);
            switch (v.wave) {
            case Wave::Sine: s = std::sin(kTwoPi * phase); break;
            case Wave::Square: s = phase < 0.5f ? 1.0f : -1.0f; break;
            case Wave::Saw: s = 2.0f * phase - 1.0f; break;
            default: s = 1.0f - 4.0f * std::fabs(phase - 0.5f); break; // triângulo
            }
        }
        if (v.filter != Filter::None) s = bq.process(v.filter, s, v.cutoff.at(t), v.q);
        out[i] += s * v.gain.at(t) * makeup;
    }
}

std::vector<float> render(const std::vector<Voice>& voices, float master = 1.0f) {
    float length = 0.0f;
    for (const Voice& v : voices) length = std::max(length, v.stop);
    std::vector<float> out(static_cast<std::size_t>(length * kRate) + 1, 0.0f);
    std::mt19937 rng(1234u);
    for (const Voice& v : voices) renderVoice(v, out, rng);
    for (float& s : out) s *= master;
    return out;
}

bool toBuffer(const std::vector<float>& samples, sf::SoundBuffer& buffer) {
    std::vector<std::int16_t> pcm(samples.size());
    for (std::size_t i = 0; i < samples.size(); ++i) {
        // Limitador suave acima de 0,9 em vez de cortar seco.
        float s = samples[i], a = std::fabs(s);
        if (a > 0.9f) s = std::copysign(0.9f + 0.1f * std::tanh((a - 0.9f) / 0.1f), s);
        pcm[i] = static_cast<std::int16_t>(s * 32000.0f);
    }
    return buffer.loadFromSamples(pcm.data(), pcm.size(), 1, kRate, {sf::SoundChannel::Mono});
}

// Atalhos para escrever os sons como no JS.
Voice osc(Wave w, float start, float stop) {
    Voice v;
    v.wave = w;
    v.start = start;
    v.stop = stop;
    return v;
}

Voice noiseVoice(float start, float stop, Filter f, float q = 0.707f) {
    Voice v = osc(Wave::Noise, start, stop);
    v.filter = f;
    v.q = q;
    return v;
}

// Oscilador com volume caindo exponencialmente (o padrão mais comum do SoundFX).
Voice blip(Wave w, float f0, float f1, float dur, float vol, bool expFreq = true, float start = 0.0f) {
    Voice v = osc(w, start, start + dur);
    v.freq.set(f0, start);
    if (f1 > 0.0f) {
        if (expFreq) v.freq.exp(f1, start + dur);
        else v.freq.lin(f1, start + dur);
    }
    v.gain.set(vol, start).exp(0.001f, start + dur);
    return v;
}

// ---------------------------------------------------------------------------
// Banco de sons (SoundFX do index.html)
// ---------------------------------------------------------------------------

std::vector<Voice> soundVoices(Sfx id, int param) {
    using W = Wave;
    std::vector<Voice> v;
    switch (id) {
    case Sfx::PunchLight:
    case Sfx::PunchHeavy: {
        const bool heavy = id == Sfx::PunchHeavy;
        const float charge = std::clamp(param / 10.0f, 0.0f, 1.0f);
        Voice n = noiseVoice(0.0f, 0.15f, Filter::LowPass);
        n.cutoff.set(heavy ? 400.0f : 800.0f, 0.0f).exp(100.0f, 0.15f);
        n.gain.set(0.6f + charge * 0.4f, 0.0f).exp(0.01f, 0.15f);
        v.push_back(n);
        if (heavy || charge > 0.3f) {
            Voice o = osc(W::Triangle, 0.0f, 0.2f);
            o.freq.set(150.0f + charge * 50.0f, 0.0f).exp(30.0f, 0.2f);
            o.gain.set(0.8f, 0.0f).exp(0.01f, 0.2f);
            v.push_back(o);
        }
        break;
    }
    case Sfx::Block: v.push_back(blip(W::Square, 180, 60, 0.08f, 0.4f)); break;
    case Sfx::Dodge: v.push_back(blip(W::Sine, 300, 600, 0.1f, 0.2f)); break;
    case Sfx::ChargeTick: v.push_back(blip(W::Sine, 220.0f + param / 10.0f * 440.0f, 0, 0.05f, 0.05f)); break;
    case Sfx::Bell: {
        // Gongo de boxe clássico: bronze fundido pesado com modos inarmônicos e batimento
        const float f0 = 520.0f; // Tom bronze médio encorpado
        struct Mode { float freq; float vol; float dur; };
        const Mode modes[] = {
            {f0, 0.70f, 2.4f},
            {f0 + 2.7f, 0.60f, 2.2f},         // Shimmer / batimento acústico vibrante
            {f0 * 1.188f, 0.50f, 1.8f},       // Terça menor ressonante
            {f0 * 1.512f, 0.45f, 1.5f},       // Quinta metálica
            {f0 * 1.996f, 0.40f, 1.2f},       // Oitava nominal
            {f0 * 2.534f, 0.32f, 0.8f},       // Harmônico superior
            {f0 * 3.981f, 0.25f, 0.5f},       // Anel agudo de sino
            {f0 * 5.412f, 0.18f, 0.35f}       // Brilho cortante de impacto
        };
        for (const auto& m : modes) {
            Voice o = osc(W::Sine, 0.0f, m.dur);
            o.freq.set(m.freq, 0.0f);
            o.gain.set(m.vol, 0.0f).exp(0.001f, m.dur);
            v.push_back(o);
        }
        // Estalo de ataque do batedor mecânico de metal (clapper strike transient)
        Voice strike = osc(W::Sine, 0.0f, 0.035f);
        strike.freq.set(3400.0f, 0.0f).exp(800.0f, 0.035f);
        strike.gain.set(0.65f, 0.0f).exp(0.01f, 0.035f);
        v.push_back(strike);

        Voice click = noiseVoice(0.0f, 0.025f, Filter::HighPass, 2.0f);
        click.cutoff.set(2800.0f, 0.0f);
        click.gain.set(0.55f, 0.0f).exp(0.01f, 0.025f);
        v.push_back(click);
        break;
    }
    case Sfx::ComboHit: {
        const float pitch = 260.0f * std::pow(1.05946f, static_cast<float>(std::min(22, param + 2)));
        Voice o = osc(param > 7 ? W::Saw : W::Triangle, 0.0f, 0.12f);
        o.freq.set(pitch, 0.0f).exp(pitch * 1.25f, 0.08f);
        o.gain.set(0.22f, 0.0f).exp(0.001f, 0.12f);
        v.push_back(o);
        break;
    }
    case Sfx::Impact: {
        Voice o = osc(W::Saw, 0.0f, 0.4f);
        o.freq.set(120, 0).exp(40, 0.3f);
        o.gain.set(0.6f, 0).exp(0.001f, 0.4f);
        v.push_back(o);
        break;
    }
    case Sfx::Zap: v.push_back(blip(W::Saw, 800, 100, 0.25f, 0.35f)); break;
    case Sfx::Rebuild: {
        std::mt19937 rng(77u);
        for (int i = 0; i < 3; ++i) {
            float t = i * 0.08f;
            v.push_back(blip(i % 2 == 0 ? W::Saw : W::Square, 150.0f + (rng() % 400), 0, 0.07f, 0.25f, true, t));
        }
        break;
    }
    case Sfx::CrowdCheer:
    case Sfx::CrowdBoo: {
        const bool cheer = id == Sfx::CrowdCheer;
        const float len = cheer ? 0.8f : 1.0f, peak = cheer ? 0.22f : 0.18f;
        Voice n = noiseVoice(0.0f, len, Filter::BandPass, cheer ? 0.6f : 0.4f);
        n.cutoff.set(cheer ? 1800.0f : 600.0f, 0.0f);
        n.gain.set(0.01f, 0.0f).lin(peak, cheer ? 0.12f : 0.2f).set(peak, cheer ? 0.35f : 0.5f).exp(0.001f, len);
        v.push_back(n);
        break;
    }
    case Sfx::CrowdGasp: {
        Voice n = noiseVoice(0.0f, 0.35f, Filter::BandPass, 1.2f);
        n.cutoff.set(2500, 0);
        n.gain.set(0.15f, 0).exp(0.001f, 0.35f);
        v.push_back(n);
        break;
    }
    case Sfx::RefereeCount: {
        const float base = 500.0f + param * 80.0f;
        Voice o = osc(W::Triangle, 0.0f, 0.2f);
        o.freq.set(base, 0).set(base * 1.2f, 0.06f);
        o.gain.set(0.35f, 0).exp(0.001f, 0.2f);
        v.push_back(o);
        if (param >= 8) {
            Voice o2 = osc(W::Square, 0.05f, 0.18f);
            o2.freq.set(base * 1.5f, 0.05f);
            o2.gain.set(0.15f, 0.05f).exp(0.001f, 0.18f);
            v.push_back(o2);
        }
        break;
    }
    case Sfx::Whip: v.push_back(blip(W::Saw, 800, 150, 0.15f, 0.35f)); break;
    case Sfx::Slurp: {
        Voice o = osc(W::Sine, 0, 0.28f);
        o.freq.set(220, 0).exp(650, 0.18f).exp(180, 0.28f);
        o.gain.set(0.3f, 0).exp(0.001f, 0.28f);
        v.push_back(o);
        break;
    }
    case Sfx::ClawSnap: v.push_back(blip(W::Triangle, 320, 80, 0.12f, 0.4f)); break;
    case Sfx::HarpoonChain: {
        Voice n = noiseVoice(0, 0.25f, Filter::BandPass, 4.0f);
        n.cutoff.set(3200, 0);
        n.gain.set(0.3f, 0).exp(0.001f, 0.25f);
        v.push_back(n);
        break;
    }
    case Sfx::SyrupBoil: {
        Voice o = osc(W::Sine, 0, 0.22f);
        o.freq.set(160, 0).lin(340, 0.1f).lin(120, 0.22f);
        o.gain.set(0.35f, 0).exp(0.001f, 0.22f);
        v.push_back(o);
        break;
    }
    case Sfx::CreamWhip: {
        Voice n = noiseVoice(0, 0.28f, Filter::BandPass, 1.8f);
        n.cutoff.set(1400, 0).lin(600, 0.28f);
        n.gain.set(0.35f, 0).exp(0.001f, 0.28f);
        v.push_back(n);
        break;
    }
    case Sfx::SawBuzz: {
        Voice o = osc(W::Saw, 0, 0.35f);
        o.freq.set(320, 0).lin(780, 0.15f).lin(450, 0.35f);
        o.gain.set(0.4f, 0).exp(0.001f, 0.35f);
        v.push_back(o);
        break;
    }
    case Sfx::ReactorPulse: {
        Voice o = osc(W::Triangle, 0, 0.45f);
        o.freq.set(80, 0).lin(240, 0.2f).exp(40, 0.45f);
        o.gain.set(0.5f, 0).exp(0.001f, 0.45f);
        v.push_back(o);
        break;
    }
    case Sfx::EmpShock:
        v.push_back(blip(W::Saw, 1800, 120, 0.25f, 0.4f));
        v.push_back(blip(W::Square, 900, 60, 0.25f, 0.4f));
        break;
    case Sfx::MissileBarrage:
        for (int m = 0; m < 4; ++m) v.push_back(blip(W::Saw, 400.0f + m * 150.0f, 80, 0.12f, 0.3f, true, m * 0.08f));
        break;
    case Sfx::ArcadeCombo: {
        const float freqs[] = {440.0f, 554.37f, 659.25f, 880.0f, 1108.73f, 1318.5f};
        for (int i = 0; i < 6; ++i) v.push_back(blip(W::Square, freqs[i], 0, 0.04f, 0.2f, true, i * 0.045f));
        break;
    }
    case Sfx::ApocalypseLaser: {
        v.push_back(blip(W::Saw, 3200, 90, 0.6f, 0.55f));
        Voice o = osc(W::Sine, 0, 0.6f);
        o.freq.set(60, 0).lin(180, 0.3f).exp(30, 0.6f);
        o.gain.set(0.55f, 0).exp(0.001f, 0.6f);
        v.push_back(o);
        break;
    }
    case Sfx::RocketThrust: {
        Voice n = noiseVoice(0, 0.35f, Filter::LowPass);
        n.cutoff.set(800, 0).lin(2400, 0.15f).exp(200, 0.35f);
        n.gain.set(0.45f, 0).exp(0.001f, 0.35f);
        v.push_back(n);
        break;
    }
    case Sfx::OrbitalLaser:
        v.push_back(blip(W::Saw, 2200, 180, 0.38f, 0.35f));
        v.push_back(blip(W::Sine, 4400, 360, 0.38f, 0.35f));
        break;
    case Sfx::PlasmaShield: {
        Voice o = osc(W::Sine, 0, 0.3f);
        o.freq.set(300, 0).lin(750, 0.15f).lin(300, 0.3f);
        o.gain.set(0.3f, 0).exp(0.001f, 0.3f);
        v.push_back(o);
        break;
    }
    case Sfx::CactusSlap: {
        Voice n = noiseVoice(0, 0.22f, Filter::BandPass, 2.5f);
        n.cutoff.set(1600, 0).exp(300, 0.22f);
        n.gain.set(0.4f, 0).exp(0.001f, 0.22f);
        v.push_back(n);
        break;
    }
    case Sfx::LuchaImpact: v.push_back(blip(W::Triangle, 140, 45, 0.35f, 0.5f)); break;
    case Sfx::SpikePierce: {
        Voice o = osc(W::Saw, 0, 0.16f);
        o.freq.set(1200, 0).lin(3200, 0.08f).exp(600, 0.16f);
        o.gain.set(0.3f, 0).exp(0.001f, 0.16f);
        v.push_back(o);
        break;
    }
    case Sfx::StaccatoViolin: {
        const float notes[] = {587.33f, 659.25f, 783.99f, 880.0f, 1046.5f};
        for (int i = 0; i < 5; ++i) v.push_back(blip(W::Saw, notes[i], notes[i] * 0.9f, 0.045f, 0.25f, true, i * 0.055f));
        break;
    }
    case Sfx::WinterViolin:
        v.push_back(blip(W::Saw, 1174.66f, 880, 0.3f, 0.3f));
        v.push_back(blip(W::Triangle, 1760, 1318.5f, 0.3f, 0.3f));
        break;
    case Sfx::Crescendo: {
        Voice o = osc(W::Saw, 0, 0.22f);
        o.freq.set(440, 0).lin(1320, 0.22f);
        o.gain.set(0.1f, 0).lin(0.4f, 0.18f).exp(0.001f, 0.22f);
        v.push_back(o);
        break;
    }
    case Sfx::MagmaExplosion: {
        Voice n = noiseVoice(0, 0.35f, Filter::LowPass);
        n.cutoff.set(250, 0).exp(80, 0.35f);
        n.gain.set(0.45f, 0).exp(0.001f, 0.35f);
        v.push_back(n);
        break;
    }
    case Sfx::VolcanoEruption: {
        Voice n = noiseVoice(0, 0.55f, Filter::BandPass, 2.0f);
        n.cutoff.set(180, 0).lin(650, 0.25f).lin(120, 0.55f);
        n.gain.set(0.5f, 0).exp(0.001f, 0.55f);
        v.push_back(n);
        break;
    }
    case Sfx::Crunch: {
        // Crack Brusco: impacto visceral de fratura/mandíbula em 3 camadas
        // 1. Estalo inicial agudo (snapping rápido de cartilagem / maxilar)
        v.push_back(blip(W::Saw, 2800.0f, 320.0f, 0.045f, 0.85f));
        // 2. Ruído ressonante de fratura (bandpass cortante de impacto ósseo)
        Voice n = noiseVoice(0.0f, 0.08f, Filter::BandPass, 3.2f);
        n.cutoff.set(3400.0f, 0.0f).exp(400.0f, 0.08f);
        n.gain.set(0.75f, 0.0f).exp(0.001f, 0.08f);
        v.push_back(n);
        // 3. Pancada sub-grave no corpo (peso físico e pressão no peito)
        Voice o = osc(W::Triangle, 0.0f, 0.22f);
        o.freq.set(160.0f, 0.0f).exp(35.0f, 0.22f);
        o.gain.set(0.90f, 0.0f).exp(0.001f, 0.22f);
        v.push_back(o);
        break;
    }
    case Sfx::TidalWave: {
        Voice n = noiseVoice(0, 0.5f, Filter::LowPass);
        n.cutoff.set(300, 0).lin(800, 0.25f).lin(150, 0.5f);
        n.gain.set(0.4f, 0).exp(0.001f, 0.5f);
        v.push_back(n);
        break;
    }
    case Sfx::Geyser: {
        Voice n = noiseVoice(0, 0.45f, Filter::BandPass, 2.0f);
        n.cutoff.set(450, 0).lin(1800, 0.35f);
        n.gain.set(0.4f, 0).exp(0.001f, 0.45f);
        v.push_back(n);
        break;
    }
    case Sfx::Select: v.push_back(blip(W::Triangle, 440, 880, 0.08f, 0.2f)); break; // src/engine/audio.js
    case Sfx::MenuNav: v.push_back(blip(W::Triangle, 660, 0, 0.04f, 0.12f)); break;
    case Sfx::Parry: {
        // Clang metálico agudo estilo Third Strike / Sekiro com ressonância de lâmina
        v.push_back(blip(W::Square, 1850.0f, 920.0f, 0.09f, 0.60f));
        v.push_back(blip(W::Sine, 2400.0f, 1300.0f, 0.26f, 0.45f));
        Voice n = noiseVoice(0.0f, 0.04f, Filter::HighPass, 2.0f);
        n.cutoff.set(3200.0f, 0.0f);
        n.gain.set(0.55f, 0.0f).exp(0.01f, 0.04f);
        v.push_back(n);
        break;
    }
    case Sfx::AnnouncerKorama: v.push_back(blip(W::Square, 220, 440, 0.4f, 0.7f)); break;
    case Sfx::CrowdVictory:    v.push_back(blip(W::Saw, 300, 500, 0.8f, 0.6f)); break;
    case Sfx::CrowdCount:      v.push_back(blip(W::Triangle, 180, 220, 0.3f, 0.5f)); break;
    case Sfx::CrowdImpact:     v.push_back(blip(W::Square, 120, 40, 0.25f, 0.7f)); break;
    case Sfx::CrowdWoah:       v.push_back(blip(W::Sine, 280, 180, 0.35f, 0.5f)); break;
    case Sfx::CameraFlash:     v.push_back(blip(W::Saw, 1200, 2400, 0.08f, 0.4f)); break;
    case Sfx::Rewind: {
        Voice m = osc(Wave::Saw, 0.0f, 0.75f);
        m.freq.set(180.0f, 0.0f).lin(720.0f, 0.50f).lin(600.0f, 0.75f);
        m.gain.set(0.35f, 0.0f).exp(0.01f, 0.75f);
        v.push_back(m);
        Voice n = noiseVoice(0.05f, 0.75f, Filter::HighPass, 2.0f);
        n.cutoff.set(2200.0f, 0.05f).lin(4800.0f, 0.60f);
        n.gain.set(0.28f, 0.05f).exp(0.01f, 0.75f);
        v.push_back(n);
        Voice c = osc(Wave::Square, 0.70f, 0.76f);
        c.freq.set(340.0f, 0.70f).exp(80.0f, 0.76f);
        c.gain.set(0.50f, 0.70f).exp(0.001f, 0.76f);
        v.push_back(c);
        break;
    }
    case Sfx::VersusWhoosh: {
        Voice n = noiseVoice(0.0f, 0.45f, Filter::BandPass, 1.8f);
        n.cutoff.set(300.0f, 0.0f).exp(2600.0f, 0.40f);
        n.gain.set(0.05f, 0.0f).lin(0.60f, 0.38f).exp(0.01f, 0.45f);
        v.push_back(n);
        Voice t = osc(Wave::Triangle, 0.0f, 0.42f);
        t.freq.set(120.0f, 0.0f).exp(480.0f, 0.40f);
        t.gain.set(0.1f, 0.0f).lin(0.40f, 0.38f).exp(0.01f, 0.42f);
        v.push_back(t);
        break;
    }
    case Sfx::VersusClash: {
        Voice sub = osc(Wave::Sine, 0.0f, 0.60f);
        sub.freq.set(130.0f, 0.0f).exp(42.0f, 0.35f);
        sub.gain.set(0.85f, 0.0f).exp(0.01f, 0.60f);
        v.push_back(sub);
        Voice m1 = osc(Wave::Square, 0.0f, 0.50f);
        m1.freq.set(820.0f, 0.0f).exp(410.0f, 0.45f);
        m1.gain.set(0.45f, 0.0f).exp(0.005f, 0.50f);
        v.push_back(m1);
        Voice m2 = osc(Wave::Triangle, 0.0f, 0.70f);
        m2.freq.set(1260.0f, 0.0f).exp(630.0f, 0.65f);
        m2.gain.set(0.35f, 0.0f).exp(0.005f, 0.70f);
        v.push_back(m2);
        Voice n = noiseVoice(0.0f, 0.12f, Filter::BandPass, 2.0f);
        n.cutoff.set(2400.0f, 0.0f);
        n.gain.set(0.70f, 0.0f).exp(0.01f, 0.12f);
        v.push_back(n);
        break;
    }
    case Sfx::VersusSlam: {
        Voice punch = osc(Wave::Sine, 0.0f, 0.60f);
        punch.freq.set(180.0f, 0.0f).exp(42.0f, 0.35f);
        punch.gain.set(0.95f, 0.0f).exp(0.01f, 0.60f);
        v.push_back(punch);
        Voice sub = osc(Wave::Sine, 0.0f, 2.50f);
        sub.freq.set(55.0f, 0.0f).exp(38.0f, 2.20f);
        sub.gain.set(0.85f, 0.0f).exp(0.01f, 2.50f);
        v.push_back(sub);
        Voice thud = osc(Wave::Triangle, 0.0f, 1.20f);
        thud.freq.set(110.0f, 0.0f).exp(60.0f, 1.0f);
        thud.gain.set(0.55f, 0.0f).exp(0.01f, 1.20f);
        v.push_back(thud);
        Voice snap = osc(Wave::Square, 0.0f, 0.18f);
        snap.freq.set(640.0f, 0.0f).exp(180.0f, 0.18f);
        snap.gain.set(0.35f, 0.0f).exp(0.01f, 0.18f);
        v.push_back(snap);
        break;
    }
    case Sfx::Count: break;
    }
    return v;
}

// Só esses sons variam com o parâmetro; os outros ficam num único buffer.
bool usesParam(Sfx id) {
    return id == Sfx::PunchLight || id == Sfx::PunchHeavy || id == Sfx::ChargeTick || id == Sfx::ComboHit || id == Sfx::RefereeCount;
}

// ---------------------------------------------------------------------------
// Trilha (BGMPlayer): bumbo, caixa, chimbal, baixo quadrado e melodia serra em 16 passos
// ---------------------------------------------------------------------------

std::vector<float> renderMusic(MusicTrack track) {
    const bool boss = track == MusicTrack::Boss || track == MusicTrack::BossPhase1 ||
                      track == MusicTrack::BossPhase2 || track == MusicTrack::BossPhase3;
    const float bpm = boss ? 155.0f : (track == MusicTrack::Menu ? 120.0f : (track == MusicTrack::Victory ? 132.0f : (track == MusicTrack::Defeat ? 84.0f : 140.0f)));
    const float step = 60.0f / bpm / 2.0f;
    static const float kBassFight[16] = {82.41f, 82.41f, 98, 98, 110, 110, 82.41f, 82.41f, 73.42f, 73.42f, 98, 98, 110, 110, 82.41f, 82.41f};
    static const float kBassBoss[16] = {55, 55, 65.41f, 65.41f, 73.42f, 73.42f, 61.74f, 61.74f, 55, 55, 82.41f, 82.41f, 73.42f, 73.42f, 61.74f, 61.74f};
    static const float kBassVictory[16] = {65.41f, 65.41f, 98.0f, 98.0f, 110.0f, 110.0f, 87.31f, 87.31f, 65.41f, 65.41f, 82.41f, 82.41f, 87.31f, 87.31f, 98.0f, 98.0f};
    static const float kBassDefeat[16] = {73.42f, 0, 110.0f, 0, 58.27f, 0, 87.31f, 0, 48.99f, 0, 73.42f, 0, 55.0f, 0, 82.41f, 0};
    static const float kLeadFight[16] = {329.63f, 0, 392, 440, 0, 523.25f, 493.88f, 0, 440, 0, 392, 349.23f, 0, 329.63f, 293.66f, 0};
    static const float kLeadBoss[16] = {220, 0, 261.63f, 293.66f, 0, 329.63f, 293.66f, 0, 349.23f, 0, 329.63f, 293.66f, 0, 261.63f, 246.94f, 0};
    static const float kLeadVictory[16] = {523.25f, 659.25f, 783.99f, 1046.5f, 987.77f, 880.0f, 783.99f, 0, 880.0f, 987.77f, 1046.5f, 1174.66f, 1046.5f, 783.99f, 1046.5f, 0};
    static const float kLeadDefeat[16] = {880.0f, 0, 698.46f, 659.25f, 587.33f, 0, 698.46f, 932.33f, 880.0f, 0, 783.99f, 698.46f, 659.25f, 0, 554.37f, 0};
    const float* bass = boss ? kBassBoss : (track == MusicTrack::Victory ? kBassVictory : (track == MusicTrack::Defeat ? kBassDefeat : kBassFight));
    const float* lead = boss ? kLeadBoss : (track == MusicTrack::Victory ? kLeadVictory : (track == MusicTrack::Defeat ? kLeadDefeat : kLeadFight));

    std::vector<Voice> v;
    auto note = [&](Wave w, float f, float dur, float t, float vol) {
        Voice o = osc(w, t, t + dur);
        o.freq.set(f, t);
        o.gain.set(vol, t).exp(0.001f, t + dur * 0.9f);
        v.push_back(o);
    };
    for (int s = 0; s < 16; ++s) {
        const float t = s * step;
        if (s % 4 == 0) { // bumbo
            Voice k = osc(Wave::Sine, t, t + 0.15f);
            k.freq.set(150, t).exp(30, t + 0.12f);
            k.gain.set(0.35f, t).exp(0.001f, t + 0.15f);
            v.push_back(k);
        }
        if (s % 4 == 2) { // caixa
            Voice n = noiseVoice(t, t + 0.08f, Filter::BandPass, 0.8f);
            n.cutoff.set(3000, t);
            n.gain.set(0.2f, t).exp(0.001f, t + 0.08f);
            v.push_back(n);
        }
        Voice h = noiseVoice(t, t + 0.04f, Filter::HighPass); // chimbal
        h.cutoff.set(7000, t);
        h.gain.set(0.08f, t).exp(0.001f, t + 0.04f);
        v.push_back(h);
        if (bass[s] > 0) note(Wave::Square, bass[s], step * 0.85f, t, 0.12f);
        if (lead[s] > 0) note(Wave::Saw, lead[s], step * 0.7f, t, 0.06f);
    }
    std::vector<float> out = render(v, 0.18f * 4.0f); // ganho master do BGM (0,18), compensado pelo volume do sf::Sound
    out.resize(static_cast<std::size_t>(16 * step * kRate)); // loop exato de 16 passos
    return out;
}

} // namespace

AudioSystem::AudioSystem() = default;

AudioSystem::AudioSystem(const std::filesystem::path& assetRoot) {
    init(assetRoot);
}

void AudioSystem::init(const std::filesystem::path& assetRoot) {
    assetRoot_ = assetRoot;
    loadSfxFiles();
}

std::filesystem::path AudioSystem::resolveMusicPath(MusicTrack track) const {
    if (assetRoot_.empty()) return {};
    const auto musicDir = assetRoot_ / "audio" / "music";
    switch (track) {
    case MusicTrack::Menu:
        return musicDir / "mus_menu_hora_do_show.mp3";
    case MusicTrack::Jack:
        return musicDir / "mus_jack_favos_e_socos.mp3";
    case MusicTrack::KingFrog:
        return musicDir / "mus_frog_pantano_real.mp3";
    case MusicTrack::Joana:
        return musicDir / "mus_joana_mare_alta.mp3";
    case MusicTrack::Cremoso:
        return musicDir / "mus_cremoso_circuito_doce.mp3";
    case MusicTrack::Valeria:
        return musicDir / "mus_valeria_fogo_do_frevo.mp3";
    case MusicTrack::Vivaldi:
        return musicDir / "mus_vivaldi_quatro_rounds.mp3";
    case MusicTrack::Espinha:
        return musicDir / "mus_espinha_baja_cacto.mp3";
    case MusicTrack::Marcov:
        return musicDir / "mus_marcov_orbita_vermelha.mp3";
    case MusicTrack::BossPhase1:
    case MusicTrack::Boss:
        return musicDir / "mus_boss_fase1_furia_mecanica.mp3";
    case MusicTrack::BossPhase2:
        return musicDir / "mus_boss_fase2_sobrecarga.mp3";
    case MusicTrack::BossPhase3:
        return musicDir / "mus_boss_fase3_desmonte.mp3";
    case MusicTrack::Fight:
        return musicDir / "mus_jack_favos_e_socos.mp3";
    case MusicTrack::Victory:
        return musicDir / "mus_vitoria_campeao.wav";
    case MusicTrack::Defeat:
        return musicDir / "mus_derrota_lona.wav";
    default:
        return {};
    }
}

void AudioSystem::loadSfxFiles() {
    if (assetRoot_.empty()) return;
    const auto sfxDir = assetRoot_ / "audio" / "sfx";
    std::error_code ec;

    auto tryLoad = [&](Sfx id, const std::string& filename, float peakBoost = 1.0f) {
        const auto path = sfxDir / filename;
        if (!std::filesystem::exists(path, ec)) return;
        sf::SoundBuffer buf;
        if (!buf.loadFromFile(path)) return;

        if (peakBoost > 1.01f) {
            const std::int16_t* src = buf.getSamples();
            const std::size_t count = buf.getSampleCount();
            int peak = 0;
            for (std::size_t i = 0; i < count; ++i) {
                int a = std::abs(static_cast<int>(src[i]));
                if (a > peak) peak = a;
            }
            if (peak > 0 && peak < 30000) {
                const float gain = std::min(32000.0f / static_cast<float>(peak), peakBoost);
                std::vector<std::int16_t> loud(count);
                for (std::size_t i = 0; i < count; ++i) {
                    float v = static_cast<float>(src[i]) * gain;
                    loud[i] = static_cast<std::int16_t>(std::clamp(v, -32767.0f, 32767.0f));
                }
                sf::SoundBuffer boosted;
                if (boosted.loadFromSamples(loud.data(), loud.size(), buf.getChannelCount(), buf.getSampleRate(), buf.getChannelMap())) {
                    fileSfxBuffers_[id] = std::move(boosted);
                    return;
                }
            }
        }
        fileSfxBuffers_[id] = std::move(buf);
    };

    // Narrador K.O-RAMA (voz épica do locutor com boost de presença)
    tryLoad(Sfx::AnnouncerKorama, "sfx_narrador_korama.wav", 1.5f);

    // Efeitos da plateia e imprensa sintetizados para a arena
    tryLoad(Sfx::CrowdVictory, "sfx_plateia_vitoria.wav");
    tryLoad(Sfx::CrowdCheer,   "sfx_plateia_vitoria.wav");
    tryLoad(Sfx::CrowdCount,   "sfx_plateia_contagem_kd.wav");
    tryLoad(Sfx::CrowdImpact,  "sfx_plateia_impacto_pesado.wav");
    tryLoad(Sfx::CrowdWoah,    "sfx_plateia_esquiva_woah.wav");
    tryLoad(Sfx::CrowdGasp,    "sfx_plateia_cordas_ooh.wav");
    tryLoad(Sfx::CameraFlash,  "sfx_flashes_imprensa.wav");

    // Efeito sonoro de fita rebobinando (Rewind pós-K.O.)
    tryLoad(Sfx::Rewind,       "sfx_rewind_tape.wav", 2.4f);

    // Efeitos da tela Versus (entrada rápida das metades, colisão titânica e slam do VS)
    tryLoad(Sfx::VersusWhoosh, "sfx_versus_whoosh.wav", 1.25f);
    tryLoad(Sfx::VersusClash,  "sfx_versus_clash.wav",  1.10f);
    tryLoad(Sfx::VersusSlam,   "sfx_versus_slam.wav",   1.45f);

    // Gongo de boxe autêntico de início/fim de round
    tryLoad(Sfx::Bell,         "sfx_gongo_round.wav");
}

const sf::SoundBuffer& AudioSystem::buffer(Sfx id, int param) {
    auto fIt = fileSfxBuffers_.find(id);
    if (fIt != fileSfxBuffers_.end()) return fIt->second;

    const std::pair<int, int> key{static_cast<int>(id), usesParam(id) ? param : 0};
    auto it = cache_.find(key);
    if (it != cache_.end()) return it->second;
    sf::SoundBuffer& b = cache_[key];
    (void)toBuffer(render(soundVoices(id, key.second)), b);
    return b;
}

void AudioSystem::setFightPaused(bool paused) {
    if (fightPaused_ == paused) return;
    fightPaused_ = paused;
    if (fightPaused_) {
        for (auto& v : voices_) {
            if (v.sound && v.sound->getStatus() == sf::SoundSource::Status::Playing) {
                if (v.isFightSound) {
                    v.sound->pause();
                    v.pausedByState = true;
                }
            }
        }
        if (musicStream_ && musicStream_->getStatus() == sf::SoundSource::Status::Playing) {
            musicStream_->pause();
            musicPausedByState_ = true;
        }
        if (synthMusic_ && synthMusic_->getStatus() == sf::SoundSource::Status::Playing) {
            synthMusic_->pause();
            synthMusicPausedByState_ = true;
        }
    } else {
        for (auto& v : voices_) {
            if (v.sound && v.pausedByState) {
                v.sound->play();
                v.pausedByState = false;
            }
        }
        if (musicPausedByState_) {
            if (musicStream_) musicStream_->play();
            musicPausedByState_ = false;
        }
        if (synthMusicPausedByState_) {
            if (synthMusic_) synthMusic_->play();
            synthMusicPausedByState_ = false;
        }
    }
}

void AudioSystem::stop(Sfx id) {
    for (auto& v : voices_) {
        if (v.sound && v.id == id) {
            v.sound->stop();
            v.sound.reset();
            v.id = Sfx::Count;
            v.pausedByState = false;
        }
    }
}

void AudioSystem::stopVersusSounds() {
    stop(Sfx::VersusWhoosh);
    stop(Sfx::VersusClash);
    stop(Sfx::VersusSlam);
}

void AudioSystem::stopFightSounds() {
    for (auto& v : voices_) {
        if (v.sound && v.isFightSound) {
            v.sound->stop();
            v.sound.reset();
            v.id = Sfx::Count;
            v.pausedByState = false;
        }
    }
}

void AudioSystem::stopAllSfx() {
    for (auto& v : voices_) {
        if (v.sound) {
            v.sound->stop();
            v.sound.reset();
            v.id = Sfx::Count;
            v.pausedByState = false;
        }
    }
}

void AudioSystem::play(Sfx id, int param) {
    if (muted_ || id == Sfx::Count) return;
    const sf::SoundBuffer& b = buffer(id, param);
    if (b.getSampleCount() == 0) return;

    // Procura primeira voz livre; não rouba vozes que estão pausadas pelo estado (ex: narrador congelado no pause)
    ActiveVoice* slot = nullptr;
    float oldest = -1.0f;
    for (auto& v : voices_) {
        if (!v.sound || v.sound->getStatus() != sf::SoundSource::Status::Playing) {
            if (!v.pausedByState) { slot = &v; break; }
        }
        float pos = v.sound ? v.sound->getPlayingOffset().asSeconds() : 0.0f;
        if (pos > oldest && !v.pausedByState) { oldest = pos; slot = &v; }
    }
    if (!slot) {
        for (auto& v : voices_) {
            if (!v.pausedByState) { slot = &v; break; }
        }
    }
    if (!slot) slot = &voices_[0];

    slot->sound.emplace(b);
    slot->id = id;
    slot->param = param;
    slot->isFightSound = (id != Sfx::Select && id != Sfx::MenuNav);
    slot->pausedByState = false;

    // O SFX toca com presença marcante e imersão total sobre a música.
    // Narrador K.O-RAMA ganha ganho dedicado elevado (100% master) para presença máxima.
    // Efeitos da plateia agora são encorpados e preenchedores (85% a 95%) vibrando junto com o ringue.
    float vol = muted_ ? 0.0f : sfxVolume_;
    if (id == Sfx::AnnouncerKorama) {
        vol = muted_ ? 0.0f : std::clamp(sfxVolume_ * 1.35f, 0.0f, 100.0f);
    } else if (id == Sfx::CrowdVictory || id == Sfx::CrowdCheer || id == Sfx::CrowdBoo) {
        vol = muted_ ? 0.0f : std::clamp(sfxVolume_ * 0.95f, 0.0f, 100.0f);
    } else if (id == Sfx::CrowdImpact) {
        vol = muted_ ? 0.0f : std::clamp(sfxVolume_ * 0.90f, 0.0f, 100.0f);
    } else if (id == Sfx::CrowdWoah || id == Sfx::CrowdGasp || id == Sfx::CrowdCount) {
        vol = muted_ ? 0.0f : std::clamp(sfxVolume_ * 0.85f, 0.0f, 100.0f);
    } else if (id == Sfx::Bell) {
        vol = muted_ ? 0.0f : std::clamp(sfxVolume_ * 0.95f, 0.0f, 100.0f);
    } else if (id == Sfx::CameraFlash) {
        vol = muted_ ? 0.0f : std::clamp(sfxVolume_ * 0.65f, 0.0f, 100.0f);
    } else if (id == Sfx::Rewind) {
        vol = muted_ ? 0.0f : std::clamp(sfxVolume_ * 1.15f, 0.0f, 100.0f);
    } else if (id == Sfx::VersusSlam) {
        vol = muted_ ? 0.0f : std::clamp(sfxVolume_ * 1.45f, 0.0f, 100.0f);
    } else if (id == Sfx::VersusClash) {
        vol = muted_ ? 0.0f : std::clamp(sfxVolume_ * 1.05f, 0.0f, 100.0f);
    } else if (id == Sfx::VersusWhoosh) {
        vol = muted_ ? 0.0f : std::clamp(sfxVolume_ * 1.20f, 0.0f, 100.0f);
    }
    slot->sound->setVolume(vol);
    if (id == Sfx::VersusSlam) {
        slot->sound->setLooping(true);
    }

    if (fightPaused_ && slot->isFightSound) {
        slot->sound->play();
        slot->sound->pause();
        slot->pausedByState = true;
    } else {
        slot->sound->play();
    }
}

void AudioSystem::play(const std::vector<SoundCue>& cues) {
    for (const SoundCue& c : cues) play(c.id, c.param);
}

void AudioSystem::playReplay(Sfx id, int param, float pitch) {
    if (muted_ || id == Sfx::Count) return;
    // Ignora sons de interface e bipes repetitivos que poluiriam o replay
    if (id == Sfx::Select || id == Sfx::MenuNav || id == Sfx::ChargeTick || id == Sfx::Bell) return;

    const sf::SoundBuffer& b = buffer(id, param);
    if (b.getSampleCount() == 0) return;

    ActiveVoice* slot = nullptr;
    float oldest = -1.0f;
    for (auto& v : voices_) {
        if (!v.sound || v.sound->getStatus() != sf::SoundSource::Status::Playing) {
            if (!v.pausedByState) { slot = &v; break; }
        }
        float pos = v.sound ? v.sound->getPlayingOffset().asSeconds() : 0.0f;
        if (pos > oldest && !v.pausedByState) { oldest = pos; slot = &v; }
    }
    if (!slot) {
        for (auto& v : voices_) {
            if (!v.pausedByState) { slot = &v; break; }
        }
    }
    if (!slot) slot = &voices_[0];

    slot->sound.emplace(b);
    slot->id = id;
    slot->param = param;
    slot->isFightSound = true;
    slot->pausedByState = false;
    float vol = muted_ ? 0.0f : (sfxVolume_ * 0.65f);
    if (id == Sfx::CrowdVictory || id == Sfx::CrowdCheer || id == Sfx::CrowdBoo) {
        vol = muted_ ? 0.0f : std::clamp(sfxVolume_ * 0.78f, 0.0f, 100.0f);
    } else if (id == Sfx::CrowdImpact) {
        vol = muted_ ? 0.0f : std::clamp(sfxVolume_ * 0.80f, 0.0f, 100.0f);
    } else if (id == Sfx::CrowdWoah || id == Sfx::CrowdGasp || id == Sfx::CrowdCount) {
        vol = muted_ ? 0.0f : std::clamp(sfxVolume_ * 0.72f, 0.0f, 100.0f);
    } else if (id == Sfx::PunchHeavy || id == Sfx::Crunch || id == Sfx::Impact) {
        vol = muted_ ? 0.0f : std::clamp(sfxVolume_ * 0.82f, 0.0f, 100.0f);
    } else if (id == Sfx::CameraFlash) {
        vol = muted_ ? 0.0f : std::clamp(sfxVolume_ * 0.50f, 0.0f, 100.0f);
    } else {
        vol = muted_ ? 0.0f : std::clamp(sfxVolume_ * 0.65f, 0.0f, 100.0f);
    }

    slot->sound->setPitch(pitch);
    slot->sound->setVolume(vol);

    if (fightPaused_) {
        slot->sound->play();
        slot->sound->pause();
        slot->pausedByState = true;
    } else {
        slot->sound->play();
    }
}

void AudioSystem::playReplay(const std::vector<SoundCue>& cues, float pitch) {
    for (const SoundCue& c : cues) playReplay(c.id, c.param, pitch);
}

const sf::SoundBuffer& AudioSystem::musicBuffer(MusicTrack track) {
    const std::size_t i = (track == MusicTrack::Boss || track == MusicTrack::BossPhase1 ||
                           track == MusicTrack::BossPhase2 || track == MusicTrack::BossPhase3) ? 1 :
                          (track == MusicTrack::Menu ? 0 : 2);
    if (!musicReady_[i]) {
        MusicTrack synthTrack = (i == 0) ? MusicTrack::Menu : (i == 1 ? MusicTrack::Boss : MusicTrack::Fight);
        musicReady_[i] = toBuffer(renderMusic(synthTrack), musicBuffers_[i]);
    }
    return musicBuffers_[i];
}

void AudioSystem::applyMusicVolume() {
    const float mult = replayMusic_ ? 0.50f : 0.65f;
    const float synthMult = replayMusic_ ? 0.20f : 0.25f;
    const float eff = (muted_ ? 0.0f : musicVolume_ * duckFactor_);
    if (musicStream_) musicStream_->setVolume(eff * mult);
    if (synthMusic_) synthMusic_->setVolume(eff * synthMult);
}

void AudioSystem::setMusic(MusicTrack track) {
    if (track == track_ && !replayMusic_ && musicPitch_ == 1.0f) return;
    track_ = track;
    replayMusic_ = false;
    musicPitch_ = 1.0f;
    duckFactor_ = 1.0f;
    if (musicStream_) musicStream_->stop();
    if (synthMusic_) synthMusic_->stop();
    if (track == MusicTrack::None) return;

    // 1. Tenta carregar o streaming de música real em MP3
    const auto path = resolveMusicPath(track);
    std::error_code ec;
    if (!path.empty() && std::filesystem::exists(path, ec)) {
        if (!musicStream_) musicStream_ = std::make_unique<sf::Music>();
        if (musicStream_->openFromFile(path)) {
            musicStream_->setLooping(true);
            musicStream_->setPitch(1.0f);
            applyMusicVolume();
            musicStream_->play();
            return;
        }
    }

    // 2. Fallback procedural caso os arquivos MP3 não estejam presentes
    synthMusic_.emplace(musicBuffer(track));
    synthMusic_->setLooping(true);
    synthMusic_->setPitch(1.0f);
    applyMusicVolume();
    synthMusic_->play();
}

sf::Time AudioSystem::musicPlayingOffset() const {
    if (musicStream_ && musicStream_->getStatus() == sf::SoundStream::Status::Playing) {
        return musicStream_->getPlayingOffset();
    }
    if (synthMusic_ && synthMusic_->getStatus() == sf::SoundSource::Status::Playing) {
        return synthMusic_->getPlayingOffset();
    }
    return sf::Time::Zero;
}

void AudioSystem::setReplayMusic(MusicTrack track, float pitch, sf::Time startOffset) {
    track_ = track;
    replayMusic_ = true;
    musicPitch_ = pitch;
    duckFactor_ = 1.0f;
    if (musicStream_) musicStream_->stop();
    if (synthMusic_) synthMusic_->stop();
    if (track == MusicTrack::None) return;

    // 1. Tenta carregar streaming com pitch desacelerado e som abafado cinematográfico
    const auto path = resolveMusicPath(track);
    std::error_code ec;
    if (!path.empty() && std::filesystem::exists(path, ec)) {
        if (!musicStream_) musicStream_ = std::make_unique<sf::Music>();
        if (musicStream_->openFromFile(path)) {
            musicStream_->setLooping(true);
            musicStream_->setPitch(pitch);
            applyMusicVolume();
            if (startOffset > sf::Time::Zero) {
                musicStream_->setPlayingOffset(startOffset);
            }
            musicStream_->play();
            return;
        }
    }

    // 2. Fallback procedural
    synthMusic_.emplace(musicBuffer(track));
    synthMusic_->setLooping(true);
    synthMusic_->setPitch(pitch);
    applyMusicVolume();
    if (startOffset > sf::Time::Zero) {
        synthMusic_->setPlayingOffset(startOffset);
    }
    synthMusic_->play();
}

void AudioSystem::setMusicPitch(float pitch) {
    musicPitch_ = pitch;
    if (musicStream_) musicStream_->setPitch(pitch);
    if (synthMusic_) synthMusic_->setPitch(pitch);
}

void AudioSystem::setMusicDuck(float factor) {
    duckFactor_ = std::clamp(factor, 0.0f, 1.0f);
    applyMusicVolume();
}

void AudioSystem::setMusicVolume(float percent) {
    musicVolume_ = percent;
    applyMusicVolume();
}

void AudioSystem::setSfxVolume(float percent) {
    sfxVolume_ = percent;
    for (auto& v : voices_) {
        if (v.sound && (v.sound->getStatus() == sf::SoundSource::Status::Playing || v.pausedByState)) {
            v.sound->setVolume(muted_ ? 0.0f : sfxVolume_);
        }
    }
}

void AudioSystem::toggleMute() {
    muted_ = !muted_;
    applyMusicVolume();
    for (auto& v : voices_) {
        if (v.sound && (v.sound->getStatus() == sf::SoundSource::Status::Playing || v.pausedByState)) {
            v.sound->setVolume(muted_ ? 0.0f : sfxVolume_);
        }
    }
}

} // namespace KoRama
