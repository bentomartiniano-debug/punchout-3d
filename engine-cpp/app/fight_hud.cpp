// HUD da luta em estilo gibi, desenhado no espaço de interface 1600x900 seguindo o css/hud.css do
// jogo original: etiqueta do nome com corte diagonal, barra de vida segmentada com hachura e texto
// "100 / 100 HP", stamina ciano, quedas com anel de nanquim, relógio preto com dígitos amarelos e
// os dois baralhos de habilidades no rodapé.
#include "ring_renderer.hpp"
#include "canvas2d.hpp"
#include "character_info.hpp"
#include "char_icon.hpp"
#include "ui_draw.hpp"
#include "sprite_atlas.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace KoRama {

namespace {

using cv::Canvas;

const std::vector<sf::Vertex>& hudVignetteMesh() {
    static const std::vector<sf::Vertex> mesh = [] {
        cv::Gradient gradient;
        gradient.radial = true;
        gradient.x0 = gradient.x1 = 800.0f;
        gradient.y0 = gradient.y1 = 520.0f;
        gradient.r0 = 260.0f;
        gradient.r1 = 980.0f;
        gradient.addColorStop(0.0f, "rgba(8,3,20,0)");
        gradient.addColorStop(0.55f, "rgba(8,3,20,0.18)");
        gradient.addColorStop(1.0f, "rgba(8,3,20,0.62)");
        constexpr int n = 28;
        std::vector<sf::Vertex> cells;
        cells.reserve(n * n * 6);
        std::vector<sf::Vector2f> positions((n + 1) * (n + 1));
        std::vector<sf::Color> colors((n + 1) * (n + 1));
        for (int y = 0; y <= n; ++y) for (int x = 0; x <= n; ++x) {
            const sf::Vector2f p{1600.0f * x / n, 900.0f * y / n};
            const int index = y * (n + 1) + x;
            positions[index] = p;
            colors[index] = cv::withAlpha(gradient.at(p), 1.0f);
        }
        for (int y = 0; y < n; ++y) for (int x = 0; x < n; ++x) {
            const int a = y * (n + 1) + x, b = a + 1, c = a + n + 1, d = c + 1;
            for (int index : {a, b, d, a, d, c}) cells.push_back({positions[index], colors[index]});
        }
        return cells;
    }();
    return mesh;
}

std::string upper(std::string s) {
    // Maiúsculas para ASCII e para as vogais acentuadas comuns (UTF-8 de 2 bytes, latin-1).
    std::string out;
    for (std::size_t i = 0; i < s.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(s[i]);
        if (ch == 0xC3 && i + 1 < s.size()) {
            unsigned char nx = static_cast<unsigned char>(s[i + 1]);
            if (nx >= 0xA0 && nx <= 0xBE && nx != 0xB7) nx -= 0x20;
            out += static_cast<char>(ch);
            out += static_cast<char>(nx);
            ++i;
        } else {
            out += static_cast<char>(std::toupper(ch));
        }
    }
    return out;
}

// Caixa com sombra dura (box-shadow sem desfoque) e borda de nanquim.
void inkBox(Canvas& ctx, float x, float y, float w, float h, const cv::Style& fill, float border, float shadow, float radius = 0) {
    ctx.fillStyle = "#000000";
    ctx.beginPath();
    if (radius > 0) ctx.roundRect(x + shadow, y + shadow, w, h, radius); else ctx.rect(x + shadow, y + shadow, w, h);
    ctx.fill();
    ctx.beginPath();
    if (radius > 0) ctx.roundRect(x, y, w, h, radius); else ctx.rect(x, y, w, h);
    ctx.fillStyle = fill;
    ctx.fill();
    if (border > 0) { ctx.strokeStyle = "#000000"; ctx.lineWidth = border; ctx.stroke(); }
}

// Paralelogramo com corte diagonal numa das pontas (clip-path do CSS).
void slantPath(Canvas& ctx, float x, float y, float w, float h, float cut, bool cutRight) {
    ctx.beginPath();
    if (cutRight) { ctx.moveTo(x, y); ctx.lineTo(x + w, y); ctx.lineTo(x + w - cut, y + h); ctx.lineTo(x, y + h); }
    else { ctx.moveTo(x + cut, y); ctx.lineTo(x + w, y); ctx.lineTo(x + w, y + h); ctx.lineTo(x, y + h); }
    ctx.closePath();
}

void hatch(Canvas& ctx, float x, float y, float w, float h, float spacing, const char* color, float lw, bool rising) {
    ctx.hatchRect(x, y, w, h, spacing, cv::Style(color), lw, rising);
}

void outlinedText(Canvas& ctx, const std::string& s, float x, float y, const cv::Style& fill, float stroke, float shadow) {
    if (shadow > 0) { ctx.fillStyle = "#000000"; ctx.fillText(s, x + shadow, y + shadow); }
    if (stroke > 0) { ctx.strokeStyle = "#000000"; ctx.lineWidth = stroke * 2; ctx.lineJoin = "round"; ctx.strokeText(s, x, y); }
    ctx.fillStyle = fill;
    ctx.fillText(s, x, y);
}


// Desenha as setas e teclas de um comando dentro do selo amarelo (◄ ► + X).
float commandBadge(Canvas& ctx, const SkillDef& def, float x, float cy, bool alignRight, bool passive,
                    const std::string& punchL = "Z", const std::string& punchR = "X") {
    enum Tok { L, R, U, D, PLUS, PUNCH_L, PUNCH_R, CHAIN2, CHAIN3 };   // mesma ordem do CmdTok (skills.hpp)
    std::vector<Tok> toks;
    for (CmdTok t : SkillSystem::commandTokens(def)) toks.push_back(static_cast<Tok>(t));
    ctx.font = "900 10px Montserrat";
    if (passive) {
        float w = ctx.measureText("PASSIVA") + 14.0f;
        const float h = 18.0f, bx = alignRight ? x - w : x, by = cy - h / 2.0f;
        inkBox(ctx, bx, by, w, h, cv::Style("#ffe600"), 2, 2, 4);
        ctx.fillStyle = "#000000";
        ctx.textBaseline = "middle";
        ctx.textAlign = "center";
        ctx.fillText("PASSIVA", bx + w / 2.0f, cy + 1.0f);
        return w;
    }

    float contentW = 0.0f;
    for (Tok t : toks) {
        if (t <= D) {
            contentW += 11.0f;
        } else if (t >= CHAIN2) {
            contentW += (t == CHAIN3 ? 3 : 2) * 7.0f + 10.0f;
        } else if (t == PLUS) {
            contentW += 9.0f;
        } else {
            const std::string& lbl = (t == PUNCH_L ? punchL : punchR);
            contentW += std::max(11.0f, ctx.measureText(lbl) + 4.0f);
        }
    }
    const float w = 10.0f + contentW;
    const float h = 18.0f, bx = alignRight ? x - w : x, by = cy - h / 2.0f;
    inkBox(ctx, bx, by, w, h, cv::Style("#ffe600"), 2, 2, 4);
    ctx.fillStyle = "#000000";
    ctx.textBaseline = "middle";

    float tx = bx + 5.0f;
    for (Tok t : toks) {
        float tokenW = 11.0f;
        if (t <= D) {
            tokenW = 11.0f;
            const float c = tx + tokenW * 0.5f, s = 4.0f;
            ctx.beginPath();
            if (t == L) { ctx.moveTo(c - s, cy); ctx.lineTo(c + s, cy - s); ctx.lineTo(c + s, cy + s); }
            else if (t == R) { ctx.moveTo(c + s, cy); ctx.lineTo(c - s, cy - s); ctx.lineTo(c - s, cy + s); }
            else if (t == U) { ctx.moveTo(c, cy - s); ctx.lineTo(c + s, cy + s); ctx.lineTo(c - s, cy + s); }
            else if (t == D) { ctx.moveTo(c, cy + s); ctx.lineTo(c + s, cy - s); ctx.lineTo(c - s, cy - s); }
            ctx.closePath();
            ctx.fill();
        } else if (t >= CHAIN2) {   // combo de entrada: uma bolinha por soco (jab, direto e, com três, gancho) e depois o finalizador
            const int n = t == CHAIN3 ? 3 : 2;
            tokenW = n * 7.0f + 10.0f;
            for (int i = 0; i < n; i++) { ctx.beginPath(); ctx.arc(tx + 3.5f + i * 7.0f, cy, 2.6f, 0.0f, 6.2832f); ctx.fill(); }
            ctx.textAlign = "center";
            ctx.fillText(">", tx + n * 7.0f + 4.5f, cy + 1.0f);
        } else if (t == PLUS) {
            tokenW = 9.0f;
            const float c = tx + tokenW * 0.5f;
            ctx.textAlign = "center";
            ctx.fillText("+", c, cy + 1.0f);
        } else {
            const std::string& lbl = (t == PUNCH_L ? punchL : punchR);
            tokenW = std::max(11.0f, ctx.measureText(lbl) + 4.0f);
            const float c = tx + tokenW * 0.5f;
            ctx.textAlign = "center";
            ctx.fillText(lbl, c, cy + 1.0f);
        }
        tx += tokenW;
    }
    return w;
}

} // namespace

void RingRenderer::drawHudVignette(sf::RenderTarget& target) {
    const auto& mesh = hudVignetteMesh();
    sf::RenderStates states;
    states.blendMode = sf::BlendAlpha;
    target.draw(mesh.data(), mesh.size(), sf::PrimitiveType::Triangles, states);
}

void RingRenderer::drawComicHud(sf::RenderTarget& target, const GameEngine& engine, bool paused) {
    const GameRenderState& state = engine.renderState;
    const sf::View fightView = target.getView();
    sf::View uiView(sf::FloatRect({0.0f, 0.0f}, {1600.0f, 900.0f}));
    uiView.setViewport(fightView.getViewport());
    target.setView(uiView);
    {
        Canvas ctx(target);
        const float time = static_cast<float>(state.frame_index) / 60.0f;
        const std::string punchLKey = keyBindings_.actionName(ActionKey::PunchL, true);
        const std::string punchRKey = keyBindings_.actionName(ActionKey::PunchR, true);
        if (!paused && (!combineHudVignette_ || !postFxHud_)) drawHudVignette(target);

        for (int side = 0; side < 2; ++side) {
            const bool left = side == 0;
            const Fighter& f = left ? engine.player : engine.enemy;
            const FighterRenderData& r = left ? state.player : state.enemy;
            const CharacterInfo& info = rosterInfo(f.charId);
            const float W = 560, x0 = left ? 18.0f : 1600.0f - 18.0f - W, xr = x0 + W;

            // ===== Placa do lutador: retrato na ponta, painel escuro com a faixa do nome, vida e stamina =====
            // (GDD: HQ retrô neon, contorno de nanquim, sombra dura sem blur, cor-assinatura do lutador)
            const sf::Color sig = info.color;
            char sigCss[32];
            std::snprintf(sigCss, sizeof sigCss, "rgb(%d,%d,%d)", sig.r, sig.g, sig.b);
            const float hpRatio = r.max_hp > 0 ? std::clamp(r.hp / r.max_hp, 0.0f, 1.0f) : 0.0f;
            const bool hit = f.state == FIGHTER_HIT;
            const bool danger = hpRatio > 0 && hpRatio <= 0.25f && engine.gameState == STATE_FIGHTING;
            const float shakeX = hit ? std::sin(time * 90) * 4 : 0;
            const float dir = left ? 1.0f : -1.0f;

            // Insígnia heráldica do lutador em silhueta branca (estilo fighting game de elite)
            const float P = 100, px = (left ? x0 + 10 : xr - 10 - P) + shakeX, py0 = 14;

            ctx.save();
            ctx.translate(px + P / 2, py0 + P / 2);
            ctx.rotate(dir * -2.0f * cv::PI / 180);

            // Sombra e moldura de nanquim limpa
            ctx.fillStyle = "#000000";
            ctx.beginPath(); ctx.roundRect(-P / 2 + 4.0f, -P / 2 + 4.0f, P, P, 10); ctx.fill();
            ctx.beginPath(); ctx.roundRect(-P / 2, -P / 2, P, P, 10);
            ctx.fillStyle = hit ? "#ffffff" : sigCss; ctx.fill();
            ctx.strokeStyle = "#000000"; ctx.lineWidth = 4.0f; ctx.stroke();

            // Insígnia vetorial centralizada com sombra e pulso dinâmico
            const float iconS = hit ? 34.0f : 30.0f;
            const float bob = std::sin(state.frame_index * 0.05f) * 1.0f;
            if (!hit) {
                // Sombra dura da insígnia
                ctx.fillStyle = "rgba(0, 0, 0, 0.40)";
                drawCharInsignia(ctx, static_cast<int>(f.charId), 2.5f, 2.5f + bob, iconS);
            }
            // Insígnia (branca no estado normal, inverte para preto no flash de hit, ou cinza no nocaute)
            ctx.fillStyle = hit ? "#000000" : (f.state == FIGHTER_KNOCKDOWN ? "#a0a5b5" : "#ffffff");
            drawCharInsignia(ctx, static_cast<int>(f.charId), 0.0f, bob, iconS);

            ctx.restore();

            // Geometria da barra de vida (fracionada em 4 blocos de 25% com retícula e rastro de dano)
            const float barW = 450.0f, barH = 28.0f;
            const float barX = left ? px + P + 14.0f : xr - P - 14.0f - barW;
            const float barY = 46.0f;

            // Nome do lutador diretamente acima da barra
            std::string name = f.bossStage == 3 ? "O NERD OPERADOR" : upper(info.name);
            ctx.font = "28px Bangers";
            const float nw = ctx.measureText(name);
            const float nameX = left ? barX : barX + barW;
            const float nameY = 28.0f;
            ctx.textAlign = left ? "left" : "right"; ctx.textBaseline = "middle";
            outlinedText(ctx, name, nameX, nameY, sigCss, 1.2f, 2.5f);

            // Subtítulo elegante e discreto sem moldura pesada
            const std::string tag = f.bossStage == 3 ? "PILOTO DO BATALHATOR" : upper(info.title);
            ctx.font = "900 11px Montserrat";
            ctx.fillStyle = "rgba(255, 255, 255, 0.65)";
            const float subX = left ? nameX + nw + 12.0f : nameX - nw - 12.0f;
            ctx.textAlign = left ? "left" : "right";
            ctx.fillText(tag, subX, nameY + 2.0f);

            // Barra de vida fracionada com retícula
            float& trail = hudTrail_[side];
            if (hpRatio >= trail || (engine.gameState != STATE_FIGHTING && engine.gameState != STATE_KNOCKDOWN)) trail = hpRatio;
            else if (!hit) trail = std::max(hpRatio, trail - 0.006f);

            // Sombra projetada
            slantPath(ctx, barX + 3.0f, barY + 3.0f, barW, barH, 11.0f, left);
            ctx.fillStyle = "rgba(0,0,0,0.6)"; ctx.fill();

            // Calha base da barra com retícula sutil
            slantPath(ctx, barX, barY, barW, barH, 11.0f, left);
            ctx.fillStyle = "#160c26"; ctx.fill();
            ctx.save();
            slantPath(ctx, barX, barY, barW, barH, 11.0f, left); ctx.clip();
            hatch(ctx, barX, barY, barW, barH, 8.0f, "rgba(255,255,255,0.035)", 1.5f, left);

            const char* hpCol = hpRatio > 0.5f ? "#facc15" : (hpRatio > 0.25f ? "#f59e0b" : "#ff1e42");
            const float tW = barW * trail, hW = barW * hpRatio;
            // Rastro fantasma de dano em laranja
            ctx.fillStyle = "#f97316";
            ctx.fillRect(left ? barX : barX + barW - tW, barY, tW, barH);
            if (hW > 0) {
                const float fx = left ? barX : barX + barW - hW;
                ctx.fillStyle = (danger && std::fmod(time, 0.4f) < 0.2f) ? "#ff1e42" : hpCol;
                ctx.fillRect(fx, barY, hW, barH);

                // Brilho superior
                auto g = ctx.createLinearGradient(0, barY, 0, barY + barH);
                g->addColorStop(0, "rgba(255,255,255,0.45)");
                g->addColorStop(0.35f, "rgba(255,255,255,0.08)");
                g->addColorStop(0.36f, "rgba(0,0,0,0)");
                g->addColorStop(1, "rgba(0,0,0,0.25)");
                ctx.fillStyle = g;
                ctx.fillRect(fx, barY, hW, barH);

                // Retícula de hachura no preenchimento ativo
                hatch(ctx, fx, barY, hW, barH, 6.5f, "rgba(0,0,0,0.18)", 1.5f, left);
            }
            ctx.restore();

            // Fracionamento da barra em 4 blocos de 25% (padrão Ko-Rama)
            for (int n = 1; n <= 3; ++n) {
                const float nxp = barX + barW * n / 4.0f - 1.5f;
                ctx.fillStyle = "#000000"; ctx.fillRect(nxp, barY, 3.5f, barH);
            }

            // Borda única de nanquim preto
            slantPath(ctx, barX, barY, barW, barH, 11.0f, left);
            ctx.strokeStyle = "#000000"; ctx.lineWidth = 3.5f; ctx.stroke();

            // Número da vida integrado no fim da barra
            char hpText[16];
            std::snprintf(hpText, sizeof(hpText), "%d", static_cast<int>(std::ceil(r.hp)));
            ctx.font = "24px Bangers";
            const float numX = left ? barX + barW + 12.0f : barX - 12.0f;
            ctx.textAlign = left ? "left" : "right"; ctx.textBaseline = "middle";
            outlinedText(ctx, hpText, numX, barY + barH / 2.0f + 1.0f, "#ffffff", 1.2f, 2.5f);

            // Stamina: 10 células ciano com retícula na perda de teto
            const float sw = barW * 0.76f, sh = 13.0f;
            const float sx = left ? barX : barX + barW - sw;
            const float sy = barY + barH + 7.0f;

            slantPath(ctx, sx + 2.0f, sy + 2.0f, sw, sh, 6.0f, left);
            ctx.fillStyle = "rgba(0,0,0,0.5)"; ctx.fill();
            slantPath(ctx, sx, sy, sw, sh, 6.0f, left);
            ctx.fillStyle = "#0c0618"; ctx.fill();
            ctx.save();
            slantPath(ctx, sx, sy, sw, sh, 6.0f, left); ctx.clip();
            const float st = std::clamp(r.stamina / 100.0f, 0.0f, 1.0f), cap = std::clamp(r.max_stamina / 100.0f, 0.0f, 1.0f);
            const float stw = sw * st;
            if (stw > 0) {
                const float stx = left ? sx : sx + sw - stw;
                ctx.fillStyle = r.is_exhausted ? (std::fmod(time, 0.3f) < 0.15f ? "#ff1e42" : "#7f1d1d") : "#00f0ff";
                ctx.fillRect(stx, sy, stw, sh);
                ctx.fillStyle = "rgba(255,255,255,0.45)"; ctx.fillRect(stx, sy + 1.5f, stw, 3.0f);
            }
            if (cap < 1.0f) {
                const float lw = sw * (1.0f - cap);
                ctx.fillStyle = "#5a1020"; ctx.fillRect(left ? sx + sw - lw : sx, sy, lw, sh);
                hatch(ctx, left ? sx + sw - lw : sx, sy, lw, sh, 5.0f, "rgba(0,0,0,0.5)", 1.5f, true);
            }
            // Fracionamento em 10 células
            for (int n = 1; n < 10; ++n) {
                ctx.fillStyle = "#000000"; ctx.fillRect(sx + sw * n / 10.0f - 1.0f, sy, 2.0f, sh);
            }
            ctx.restore();
            slantPath(ctx, sx, sy, sw, sh, 6.0f, left);
            ctx.strokeStyle = "#000000"; ctx.lineWidth = 2.0f; ctx.stroke();

            if (r.is_exhausted) {
                ctx.font = "14px Bangers";
                ctx.textAlign = left ? "left" : "right"; ctx.textBaseline = "middle";
                outlinedText(ctx, "EXAUSTO!", left ? sx + sw + 8.0f : sx - 8.0f, sy + sh / 2.0f, "#ff1e42", 0.8f, 1.5f);
            }

            // DANGER sutil no canto do retrato
            if (danger) {
                const float pulse = 1.0f + 0.08f * std::fabs(std::sin(time * 8.0f));
                ctx.save();
                ctx.translate(left ? px + P - 4 : px + 4, py0 + P - 4);
                ctx.rotate(dir * -10.0f * cv::PI / 180.0f); ctx.scale(pulse, pulse);
                ctx.font = "15px Bangers";
                inkBox(ctx, -28, -10, 56, 20, cv::Style("#ff1e42"), 1.8f, 2.5f, 3);
                ctx.fillStyle = "#ffffff"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
                ctx.fillText("DANGER!", 0, 1);
                ctx.restore();
            }

            // Quedas: 3 estrelas compactas e elegantes sob o retrato
            const int kds = left ? engine.playerKnockdowns : engine.enemyKnockdowns;
            auto star = [&](float ox, float oy, float R0, float r0) {
                ctx.beginPath();
                for (int k = 0; k < 10; ++k) {
                    const float a = -cv::PI / 2 + k * cv::PI / 5, rr = k % 2 ? r0 : R0;
                    if (k == 0) ctx.moveTo(ox + std::cos(a) * rr, oy + std::sin(a) * rr); else ctx.lineTo(ox + std::cos(a) * rr, oy + std::sin(a) * rr);
                }
                ctx.closePath();
            };
            for (int i = 0; i < GameEngine::kKnockdownsForTko; ++i) {
                const bool on = i < kds;
                const float cx = left ? px + 18.0f + i * 26.0f : px + P - 18.0f - i * 26.0f;
                const float cy = py0 + P + 14.0f;
                const float R0 = on ? 10.5f : 8.5f;
                star(cx + 1.5f, cy + 1.5f, R0, R0 * 0.48f); ctx.fillStyle = "#000000"; ctx.fill();
                star(cx, cy, R0, R0 * 0.48f); ctx.fillStyle = on ? "#ff1e42" : "#302048"; ctx.fill();
                ctx.strokeStyle = "#000000"; ctx.lineWidth = 1.8f; ctx.stroke();
                if (on) { star(cx - 0.8f, cy - 1.0f, R0 * 0.45f, R0 * 0.2f); ctx.fillStyle = "#ffe600"; ctx.fill(); }
            }
            float y = sy + sh + 12.0f;

            // Medidor do super soco
            if (left && engine.chargeActive) {
                const float c = engine.chargeRatio(), cw = sw, cy = sy + sh + 10;
                inkBox(ctx, sx, cy, cw, 12, cv::Style("#120a22"), 2.5f, 3, 3);
                auto g = ctx.createLinearGradient(sx, 0, sx + cw, 0);
                g->addColorStop(0, "#ffe600"); g->addColorStop(1, "#ff0055");
                ctx.fillStyle = g; ctx.fillRect(sx + 2, cy + 2, (cw - 4) * c, 8);
                if (c >= 1) {
                    ctx.font = "18px Bangers"; ctx.textAlign = "left"; ctx.textBaseline = "middle";
                    outlinedText(ctx, "SUPER!", sx + cw + 10, cy + 6, std::fmod(time, 0.2f) < 0.1f ? "#ffe600" : "#ff0055", 1, 2);
                }
                y = std::max(y, cy + 24);
            }

            // Estados ativos
            struct Chip { const char* label; const char* color; };
            std::vector<Chip> chips;
            const StatusEffects& s = f.status;
            if (s.infested > 0) chips.push_back({"ENXAME", "#fbbf24"});
            if (s.poisoned > 0) chips.push_back({"VENENO", "#a3e635"});
            if (s.slowed > 0) chips.push_back({"LENTO", "#93c5fd"});
            if (s.blind > 0) chips.push_back({"CEGO", "#facc15"});
            if (s.mud > 0) chips.push_back({"NA LAMA", "#b45309"});
            if (s.molasses > 0) chips.push_back({"MELAÇO", "#f59e0b"});
            if (s.poisonSkin > 0) chips.push_back({"PELE TÓXICA", "#10b981"});
            if (s.bleed > 0) chips.push_back({"SANGRANDO", "#f87171"});
            if (s.shell > 0) chips.push_back({"CRACAS", "#38bdf8"});
            if (f.monstrous) chips.push_back({"MONSTRUOSA", "#fb923c"});
            if (s.burn > 0) chips.push_back({"QUEIMANDO", "#fb923c"});
            if (s.melt > 0) chips.push_back({"DERRETIDO", "#fde68a"});
            if (s.heatwave > 0) chips.push_back({"CALOR", "#ff7a45"});
            if (s.seasons > 0) chips.push_back({"4 ESTAÇÕES", "#34d399"});
            if (s.thornVeil > 0) chips.push_back({"ESPINHOS", "#10b981"});
            if (s.plasma > 0) chips.push_back({"PLASMA", "#22d3ee"});
            if (f.coldMagma) chips.push_back({"MAGMA FRIO", "#94a3b8"});
            if (f.bossStage == 1) chips.push_back({"BLINDADO", "#94a3b8"});
            if (f.bossStage == 2) chips.push_back({"FÚRIA", "#00f5d4"});
            if (f.bossStage == 3) chips.push_back({"QUÂNTICO", "#818cf8"});
            if (s.honeyShield > 0) chips.push_back({s.honeyShield >= 3 ? "FAVO x3" : (s.honeyShield == 2 ? "FAVO x2" : "FAVO x1"), "#38bdf8"});
            float cxp = left ? x0 + 140 : xr - 140;
            ctx.font = "900 10px Montserrat";
            for (const Chip& c : chips) {
                const float w = ctx.measureText(c.label) + 16;
                const float bxp = left ? cxp : cxp - w;
                inkBox(ctx, bxp, y, w, 18, cv::Style(c.color), 1.5f, 2.5f, 3);
                ctx.fillStyle = "#000000"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
                ctx.fillText(c.label, bxp + w / 2, y + 10);
                cxp += left ? w + 8 : -(w + 8);
            }

            // Baralho de habilidades no rodapé
            if (SkillSystem::hasSkills(f.charId) || f.bossStage > 0) {
                const std::array<SkillId, 3> slots = {f.loadout.spec1, f.loadout.spec2, f.loadout.passive};
                float py = 900 - 22 - 3 * 34 - 2 * 8;
                ctx.font = "900 9.6px Montserrat";
                const std::string header = left ? "SUAS HABILIDADES" : "HABILIDADES DO RIVAL";
                const float hw = ctx.measureText(header) + 16;
                const float hx = left ? 22 : 1600 - 22 - hw;
                // Tela limpa: o rival só mostra a carta que está usando; as suas ficam discretas até usar.
                const bool anyCasting = f.state == FIGHTER_SKILL && !f.signatureOnly;
                if (left) {
                    ctx.save(); ctx.globalAlpha = anyCasting ? 1.0f : 0.45f;
                    inkBox(ctx, hx, py - 26, hw, 17, cv::Style("#000000"), 1.5f, 2, 3);
                    ctx.fillStyle = "#ffe600"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
                    ctx.fillText(header, hx + hw / 2, py - 17);
                    ctx.restore();
                }
                int idx = 0;
                for (SkillId id : slots) {
                    const SkillDef* def = SkillSystem::get(id);
                    if (!def) { py += 42; continue; }
                    const bool casting = f.state == FIGHTER_SKILL && f.activeSkill == id && !f.signatureOnly;
                    const bool passive = def->type == SkillType::Passive;
                    if (!left && !casting) { py += 34 + 8; ++idx; continue; }
                    const char* badgeCol = idx == 0 ? "#ff0055" : (idx == 1 ? "#00f5d4" : "#c084fc");
                    ctx.font = "900 9.3px Montserrat";
                    const std::string badge = passive ? "PASSIVA" : upper(def->tag);
                    const float bw = ctx.measureText(badge) + 12;
                    ctx.font = "18.4px Bangers";
                    const std::string nm = upper(def->name);
                    const float nmw = ctx.measureText(nm);
                    ctx.font = "900 10px Montserrat";
                    const float kwid = !left ? -8.0f : (passive ? ctx.measureText("PASSIVA") + 14 : 54);
                    const float pw = 20 + bw + 8 + nmw + 8 + kwid, ph = 34;
                    const float pxp = left ? 22 : 1600 - 22 - pw;
                    const float tilt = (left ? -0.6f : 0.6f) * (idx % 2 ? -0.66f : 1.0f) * cv::PI / 180;
                    const bool isCombo = left && !passive && engine.comboCount >= 3;
                    const bool canAfford = passive || (!f.isExhausted && f.stamina >= (isCombo ? 12.0f : 15.0f));
                    float cardAlpha = 0.45f;
                    if (casting || !left) cardAlpha = 1.0f;
                    else if (isCombo && canAfford) cardAlpha = 0.95f;
                    else if (!canAfford) cardAlpha = 0.22f;

                    ctx.save();
                    ctx.globalAlpha = cardAlpha;
                    ctx.translate(pxp + pw / 2, py + ph / 2);
                    ctx.rotate(tilt);
                    const float pop = casting ? 1.06f + 0.03f * std::sin(time * 20) : (isCombo && canAfford ? 1.03f + 0.02f * std::sin(time * 10) : 1.0f);
                    ctx.scale(pop, pop);
                    ctx.translate(-pw / 2, -ph / 2);
                    inkBox(ctx, 0, 0, pw, ph, cv::Style(casting ? "#2a1650" : (isCombo && canAfford ? "#1e1335" : "#120b20")), 3, 4, 6);
                    ctx.save(); ctx.beginPath(); ctx.roundRect(0, 0, pw, ph, 6); ctx.clip();
                    hatch(ctx, 0, 0, pw, ph, 6, "rgba(255,255,255,0.04)", 2, false);
                    // Recarga das curas
                    if (s.healCooldown > 0 && (def->fx.heal > 0 || id == SkillId::JackRoyalJelly || id == SkillId::FrogFlyCatcher)) {
                        ctx.fillStyle = "rgba(0,0,0,0.55)";
                        ctx.fillRect(0, 0, pw * s.healCooldown / (8.0f * 60.0f), ph);
                    }
                    ctx.restore();
                    if (casting) {
                        ctx.beginPath(); ctx.roundRect(0, 0, pw, ph, 6);
                        ctx.strokeStyle = badgeCol; ctx.lineWidth = 3; ctx.stroke();
                    } else if (isCombo && canAfford) {
                        ctx.beginPath(); ctx.roundRect(0, 0, pw, ph, 6);
                        ctx.strokeStyle = "#ffe600"; ctx.lineWidth = 2.5f; ctx.stroke();
                    }
                    // Ordem: etiqueta, nome, comando (invertida do lado do rival)
                    float cx = left ? 10 : pw - 10;
                    ctx.font = "900 9.3px Montserrat";
                    const float bxp = left ? cx : cx - bw;
                    inkBox(ctx, bxp, ph / 2 - 8, bw, 16, cv::Style(badgeCol), 1.5f, 2, 3);
                    ctx.fillStyle = idx == 0 ? "#ffffff" : "#000000"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
                    ctx.fillText(badge, bxp + bw / 2, ph / 2 + 1);
                    cx += left ? bw + 8 : -(bw + 8);
                    ctx.font = "18.4px Bangers";
                    ctx.textAlign = left ? "left" : "right";
                    outlinedText(ctx, nm, cx, ph / 2 + 1, "#ffffff", 0.5f, 1.5f);
                    cx += left ? nmw + 8 : -(nmw + 8);
                    if (left) commandBadge(ctx, *def, cx, ph / 2, false, passive, punchLKey, punchRKey); // o rival não revela os comandos

                    // Selo de status de recurso no canto superior da carta
                    if (left && !passive) {
                        ctx.save();
                        ctx.globalAlpha = 1.0f; // Totalmente visível e nítido
                        if (isCombo && canAfford) {
                            ctx.font = "900 8.5px Montserrat";
                            const float cw = ctx.measureText("COMBO 12") + 8;
                            inkBox(ctx, pw - cw - 4, -6, cw, 14, cv::Style("#ffe600"), 1.5f, 2.0f, 3);
                            ctx.fillStyle = "#000000"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
                            ctx.fillText("COMBO 12", pw - cw / 2 - 4, 1);
                        } else if (!canAfford) {
                            ctx.font = "900 8.5px Montserrat";
                            const float cw = ctx.measureText("EXAUSTO") + 8;
                            inkBox(ctx, pw - cw - 4, -6, cw, 14, cv::Style("#ff1e42"), 1.5f, 2.0f, 3);
                            ctx.fillStyle = "#ffffff"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
                            ctx.fillText("EXAUSTO", pw - cw / 2 - 4, 1);
                        } else {
                            ctx.font = "900 8px Montserrat";
                            const float cw = ctx.measureText("20 STAM") + 6;
                            inkBox(ctx, pw - cw - 4, -6, cw, 13, cv::Style("#1a0f2e"), 1.0f, 1.5f, 3);
                            ctx.fillStyle = "#00f0ff"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
                            ctx.fillText("20 STAM", pw - cw / 2 - 4, 0);
                        }
                        ctx.restore();
                    }
                    ctx.restore();
                    py += ph + 8;
                    ++idx;
                }
            }
        }

        // Round e relógio no centro (Padrão Ko-Rama HQ)
        {
            ctx.font = "26px Bangers";
            const std::string round = engine.trainingMode ? "MODO TREINO" : ("ROUND " + std::to_string(engine.round));
            const float rw = ctx.measureText(round) + 36.0f + 16.0f, rh = 34.0f, rx = 800.0f - rw / 2.0f, ry = 12.0f;
            ctx.fillStyle = "#000000";
            ctx.beginPath();
            ctx.moveTo(rx + 8.0f + 4.5f, ry + 4.5f);
            ctx.lineTo(rx + rw + 4.5f, ry + 4.5f);
            ctx.lineTo(rx + rw - 8.0f + 4.5f, ry + rh + 4.5f);
            ctx.lineTo(rx + 4.5f, ry + rh + 4.5f);
            ctx.closePath();
            ctx.fill();

            ctx.beginPath();
            ctx.moveTo(rx + 8.0f, ry);
            ctx.lineTo(rx + rw, ry);
            ctx.lineTo(rx + rw - 8.0f, ry + rh);
            ctx.lineTo(rx, ry + rh);
            ctx.closePath();
            ctx.fillStyle = "#ffffff";
            ctx.fill();
            ctx.strokeStyle = "#000000";
            ctx.lineWidth = 3.5f;
            ctx.stroke();

            ctx.fillStyle = "#000000";
            ctx.textAlign = "center";
            ctx.textBaseline = "middle";
            ctx.fillText(round, 800.0f, ry + rh / 2.0f + 1.0f);

            const int remaining = (engine.roundFrames + 59) / 60;
            char clock[16];
            if (engine.trainingMode) {
                std::snprintf(clock, sizeof(clock), "INF");
            } else {
                std::snprintf(clock, sizeof(clock), "%02d:%02d", remaining / 60, remaining % 60);
            }
            const bool danger = !engine.trainingMode && remaining <= 10 && remaining > 0 && state.game_state == STATE_FIGHTING;
            const float cw = 172.0f, ch = 64.0f, cxp = 800.0f - cw / 2.0f, cyp = ry + rh + 8.0f;
            inkBox(ctx, cxp, cyp, cw, ch, cv::Style("#000000"), 3.5f, 5.5f, 6);
            ctx.save();
            ctx.beginPath();
            ctx.roundRect(cxp, cyp, cw, ch, 6);
            ctx.clip();
            hatch(ctx, cxp, cyp, cw, ch, 6.0f, "rgba(255,255,255,0.06)", 2.0f, false);
            ctx.restore();
            for (auto [dx, dy] : {std::pair{cxp + 6.0f, cyp + 6.0f}, std::pair{cxp + cw - 6.0f, cyp + ch - 6.0f}}) {
                ctx.beginPath();
                ctx.arc(dx, dy, 2.5f, 0, cv::PI * 2);
                ctx.fillStyle = "#ffffff";
                ctx.fill();
                ctx.strokeStyle = "#000000";
                ctx.lineWidth = 1.2f;
                ctx.stroke();
            }
            ctx.save();
            ctx.translate(800.0f, cyp + ch / 2.0f + 2.0f);
            if (danger) {
                const float k = 1.0f + 0.12f * std::fabs(std::sin(time * 6.3f));
                ctx.scale(k, k);
            }
            ctx.font = "52px Bangers";
            ctx.textAlign = "center";
            ctx.textBaseline = "middle";
            outlinedText(ctx, clock, 0, 0, danger ? "#ff1e42" : "#ffe600", 1.4f, 3.5f);
            ctx.restore();

            float cyNext = cyp + ch + 10.0f;
            if (state.game_state == STATE_KNOCKDOWN) {
                ctx.save();
                ctx.translate(800.0f, cyNext + 14.0f);
                const float k = 1.0f + 0.08f * std::fabs(std::sin(time * 10.0f));
                ctx.scale(k, k);
                ctx.font = "28px Bangers";
                outlinedText(ctx, "CONTAGEM: " + std::to_string(engine.refereeCount), 0, 0, "#ff3355", 1.4f, 3.5f);
                ctx.restore();

                // HUD Arcade de Levantar (Wake-Up Mash): reposicionado para a parte inferior
                // da tela (y = 690), deixando o centro do ringue e o oponente 100% visíveis!
                if (!engine.enemyIsDown) {
                    ctx.save();
                    ctx.translate(800, 740);

                    // 1. Título com pulso dinâmico de urgência
                    const float pulse = 1.0f + 0.06f * std::sin(time * 14.0f);
                    ctx.save();
                    ctx.scale(pulse, pulse);
                    ctx.font = "28px Bangers";
                    ctx.textAlign = "center"; ctx.textBaseline = "middle";
                    outlinedText(ctx, "LEVANTE-SE! APERTE RÁPIDO!", 0, -52, "#ffe600", 2.0f, 4);
                    ctx.restore();

                    // 2. Teclas de Arcade simulando mashing alternado com as teclas reais do jogador
                    const float mashPhase = std::sin(time * 20.0f);
                    const float zPress = mashPhase > 0 ? mashPhase * 4.0f : 0.0f;
                    const float xPress = mashPhase < 0 ? -mashPhase * 4.0f : 0.0f;

                    auto drawArcadeKey = [&](float kx, float ky, float press, const std::string& label, const char* bgCol) {
                        ctx.save();
                        ctx.translate(kx, ky + press);
                        ctx.font = "26px Bangers";
                        const float textW = ctx.measureText(label);
                        const float keyW = std::max(48.0f, textW + 18.0f);
                        const float halfW = keyW / 2.0f;
                        // Sombra da tecla
                        ctx.fillStyle = "#000000";
                        ctx.beginPath(); ctx.roundRect(-halfW, -20.0f + (4.0f - press), keyW, 40.0f, 6.0f); ctx.fill();
                        // Corpo da tecla
                        ctx.beginPath(); ctx.roundRect(-halfW, -20.0f, keyW, 40.0f, 6.0f);
                        ctx.fillStyle = bgCol; ctx.fill();
                        ctx.strokeStyle = "#0f172a"; ctx.lineWidth = 3.0f; ctx.stroke();
                        // Highlight
                        ctx.fillStyle = "rgba(255,255,255,0.45)";
                        ctx.fillRect(-halfW + 6.0f, -17.0f, keyW - 12.0f, 6.0f);
                        // Texto
                        ctx.textAlign = "center"; ctx.textBaseline = "middle";
                        outlinedText(ctx, label, 0.0f, 2.0f, "#000000", 0.0f, 0.0f);
                        ctx.restore();
                    };

                    drawArcadeKey(-68, -14, zPress, punchLKey, "#00f5d4");

                    // Raio estilizado de quadrinho vetorial
                    ctx.save();
                    ctx.translate(0, -14);
                    const float boltPulse = 1.0f + 0.15f * std::fabs(mashPhase);
                    ctx.scale(boltPulse, boltPulse);
                    ctx.beginPath();
                    ctx.moveTo(3, -15);
                    ctx.lineTo(-8, 1);
                    ctx.lineTo(0, 1);
                    ctx.lineTo(-4, 15);
                    ctx.lineTo(8, -1);
                    ctx.lineTo(1, -1);
                    ctx.closePath();
                    ctx.fillStyle = "#ffe600"; ctx.fill();
                    ctx.strokeStyle = "#000000"; ctx.lineWidth = 2.0f; ctx.stroke();
                    ctx.restore();

                    drawArcadeKey(68, -14, xPress, punchRKey, "#fbbf24");

                    // 3. Barra de Energia Segmentada Arcade
                    const float progress = engine.mashTarget > 0 ? std::clamp(static_cast<float>(engine.mashProgress) / engine.mashTarget, 0.0f, 1.0f) : 0.0f;
                    const float bw = 320.0f, bh = 20.0f, bx = -bw / 2.0f, by = 22.0f;

                    inkBox(ctx, bx - 4, by - 4, bw + 8, bh + 8, cv::Style("#120d20"), 3.5f, 4.0f, 4);
                    ctx.fillStyle = "rgba(10, 8, 20, 0.9)";
                    ctx.fillRect(bx, by, bw, bh);

                    const int kSegs = 12;
                    const float segPad = 3.0f;
                    const float segW = (bw - segPad * (kSegs + 1)) / kSegs;
                    const int activeSegs = static_cast<int>(std::round(progress * kSegs));

                    for (int s = 0; s < kSegs; ++s) {
                        const float sx = bx + segPad + s * (segW + segPad);
                        if (s < activeSegs) {
                            const char* segCol = (s < 4) ? "#ef4444" : (s < 8 ? "#f59e0b" : "#00f5d4");
                            ctx.fillStyle = segCol;
                            ctx.fillRect(sx, by + segPad, segW, bh - segPad * 2.0f);
                            ctx.fillStyle = "rgba(255,255,255,0.6)";
                            ctx.fillRect(sx, by + segPad, segW, 3.0f);
                        } else {
                            ctx.fillStyle = "rgba(45, 35, 65, 0.4)";
                            ctx.fillRect(sx, by + segPad, segW, bh - segPad * 2.0f);
                        }
                    }

                    const int pct = static_cast<int>(progress * 100.0f);
                    ctx.font = "16px Bangers";
                    ctx.textAlign = "center"; ctx.textBaseline = "middle";
                    outlinedText(ctx, std::to_string(pct) + "%", 0, by + bh / 2.0f, "#ffffff", 1.2f, 2);

                    ctx.restore();
                }
            }
        }

        // Borda vermelha quando o jogador apanha
        if (engine.hurtTimer > 0 && !paused) {
            const float a = std::min(1.0f, engine.hurtTimer / 18.0f) * 0.55f;
            auto rg = ctx.createRadialGradient(800, 450, 380, 800, 450, 900);
            char c0[48], c1[48];
            std::snprintf(c0, sizeof c0, "rgba(255,30,66,0)");
            std::snprintf(c1, sizeof c1, "rgba(255,30,66,%.2f)", a);
            rg->addColorStop(0, c0); rg->addColorStop(1, c1);
            ctx.fillStyle = rg; ctx.fillRect(0, 0, 1600, 900);
        }

        // Combo do jogador (canto direito, abaixo dos estados do rival)
        if (engine.comboCount >= 2) {
            const int n = engine.comboCount;
            const char* tier = n < 5 ? "COMBO!" : n < 8 ? "GREAT COMBO!" : n < 12 ? "SUPER COMBO!" : n < 16 ? "HYPER COMBO!" : "ULTRA GODLIKE!";
            const char* col = n < 5 ? "#ffe600" : n < 8 ? "#ff6600" : n < 12 ? "#ff0055" : n < 16 ? "#00f5d4" : "#c084fc";
            ctx.save();
            ctx.translate(1600 - 40, 250);
            ctx.rotate(-4 * cv::PI / 180);
            const float pulse = n >= 16 ? 1 + 0.12f * std::fabs(std::sin(time * 11)) : 1.0f;
            ctx.scale(pulse, pulse);
            ctx.textAlign = "right"; ctx.textBaseline = "middle";
            ctx.font = "64px Bangers";
            ctx.shadowBlur = 20; ctx.shadowColor = col;
            outlinedText(ctx, std::to_string(n), -70, 0, col, 1.5f, 3);
            ctx.shadowBlur = 0;
            ctx.font = "28px Bangers";
            outlinedText(ctx, "HITS", 0, 6, "#ffffff", 1.2f, 3);
            ctx.font = "22px Bangers";
            outlinedText(ctx, tier, 0, 42, col, 1.2f, 3);
            ctx.restore();
            const float ratio = static_cast<float>(engine.comboTimer) / GameEngine::kComboFrames;
            inkBox(ctx, 1600 - 40 - 160, 310, 160, 7, cv::Style("rgba(0,0,0,0.8)"), 2, 2, 3);
            ctx.fillStyle = col; ctx.fillRect(1600 - 40 - 160 * ratio, 311, 160 * ratio - 1, 5);
        }

        // Faixa de status discreta e compacta (triggerStatusBanner)
        if (state.status_banner_timer > 0.0f && state.status_banner[0] != '\0') {
            const float a = std::min(1.0f, state.status_banner_timer * 3.0f);
            ctx.save();
            ctx.globalAlpha = a;
            const float bannerY = 118.0f;
            ctx.translate(800, bannerY);
            ctx.font = "18px Bangers";
            const std::string banner = state.status_banner;
            const float bw = ctx.measureText(banner) + 24.0f;
            const float bh = 20.0f;
            // Cápsula sutil e translúcida: não cobre lutadores nem polui a tela
            ctx.beginPath();
            ctx.roundRect(-bw / 2.0f, -bh / 2.0f, bw, bh, 10.0f);
            ctx.fillStyle = "rgba(10, 6, 18, 0.72)";
            ctx.fill();
            ctx.strokeStyle = "rgba(255, 230, 0, 0.8)";
            ctx.lineWidth = 1.4f;
            ctx.stroke();
            ctx.textAlign = "center"; ctx.textBaseline = "middle";
            outlinedText(ctx, banner, 0, 1, "#ffe600", 0.6f, 1.2f);
            ctx.restore();
        }

        if (state.game_state == STATE_ROUND_OVER) {
            ctx.save();
            ctx.translate(800, 430);
            ctx.rotate(-3 * cv::PI / 180);
            ctx.font = "84px Bangers";
            ctx.textAlign = "center"; ctx.textBaseline = "middle";
            outlinedText(ctx, "FIM DO ASSALTO!", 0, 0, "#ffe600", 3, 6);
            ctx.font = "26px Bangers";
            outlinedText(ctx, "PRÓXIMO ROUND EM INSTANTES...", 0, 64, "#ffffff", 1.5f, 3);
            ctx.restore();
        }

        if (state.game_state == STATE_VICTORY || state.game_state == STATE_GAME_OVER) {
            const bool isVictory = state.game_state == STATE_VICTORY;
            const bool isPerfect = isVictory && engine.isPerfectVictory();
            const uint32_t elapsed = (state.frame_index >= engine.matchEndFrame) ? (state.frame_index - engine.matchEndFrame) : 0;
            const float sec = static_cast<float>(elapsed) / 60.0f;
            const float pop = 1.0f + 0.65f * std::exp(-sec * 7.0f) + 0.03f * std::sin(sec * 12.0f);

            ctx.save();
            ctx.translate(800.0f, 410.0f);
            ctx.scale(pop, pop);
            ctx.rotate(-4.0f * cv::PI / 180.0f);

            if (isPerfect) {
                // Raios cósmicos e dourados girando atrás do PERFEITO!
                const int rayCount = 20;
                for (int r = 0; r < rayCount; ++r) {
                    const float ang = r * (2.0f * cv::PI / rayCount) + sec * 0.8f;
                    const float r1 = 150.0f;
                    const float r2 = r1 + 140.0f * (1.0f + 0.15f * std::sin(sec * 10.0f + r));
                    ctx.strokeStyle = (r % 2 == 0) ? cv::rgba(1.0f, 0.90f, 0.0f, 0.55f) : cv::rgba(0.0f, 0.96f, 0.83f, 0.45f);
                    ctx.lineWidth = 4.0f;
                    ctx.beginPath();
                    ctx.moveTo(std::cos(ang) * r1, std::sin(ang) * r1);
                    ctx.lineTo(std::cos(ang) * r2, std::sin(ang) * r2);
                    ctx.stroke();
                }

                // Faixa de corte mangá diagonal estilizada
                const float slashL = 880.0f;
                ctx.fillStyle = cv::rgba(0.04f, 0.02f, 0.08f, 0.82f);
                ctx.beginPath();
                ctx.moveTo(-slashL * 0.5f - 40.0f, -60.0f);
                ctx.lineTo(slashL * 0.5f + 40.0f, -40.0f);
                ctx.lineTo(slashL * 0.5f, 75.0f);
                ctx.lineTo(-slashL * 0.5f - 80.0f, 55.0f);
                ctx.closePath();
                ctx.fill();
                ctx.strokeStyle = cv::rgba(1.0f, 0.90f, 0.0f, 0.95f);
                ctx.lineWidth = 4.0f;
                ctx.stroke();

                // Aberração cromática estilo arcade retrô
                const float chroma = 4.0f + 2.0f * std::sin(sec * 14.0f);
                ctx.font = "140px 'Titan One'";
                ctx.textAlign = "center";
                ctx.textBaseline = "middle";

                // Camadas ciano e magenta deslocadas
                outlinedText(ctx, "PERFEITO!", -chroma, -12.0f, cv::rgba(0.0f, 0.96f, 0.83f, 0.85f), 3.0f, 0);
                outlinedText(ctx, "PERFEITO!", chroma, -12.0f, cv::rgba(1.0f, 0.0f, 0.50f, 0.85f), 3.0f, 0);

                // Texto principal dourado com contorno pesado de nanquim
                outlinedText(ctx, "PERFEITO!", 0.0f, -12.0f, "#ffe600", 6.0f, 12.0f);

                // Subtítulo: FLAWLESS / 100% VIDA
                ctx.font = "38px Bangers";
                outlinedText(ctx, "VITÓRIA IMPECÁVEL! (100% DE VIDA)", 0.0f, 85.0f, "#00f5d4", 2.2f, 4.0f);

                // Selo bônus de XP
                ctx.font = "26px Bangers";
                outlinedText(ctx, "+1500 XP DE COMBATE (BÔNUS PERFEITO!)", 0.0f, 126.0f, "#ffe600", 1.8f, 3.0f);
            } else if (isVictory) {
                // Vitória normal (K.O.!, T.K.O.! ou VITÓRIA!)
                const std::string title = (engine.result == MatchResult::WinKO) ? "K.O.!" :
                                          ((engine.result == MatchResult::WinTKO) ? "T.K.O.!" : "VITÓRIA!");
                ctx.font = "140px 'Titan One'";
                ctx.textAlign = "center";
                ctx.textBaseline = "middle";
                outlinedText(ctx, title, 0.0f, -10.0f, "#ffe600", 5.5f, 10.0f);

                const std::string sub = (engine.result == MatchResult::WinDecision) ? "VITÓRIA POR PONTOS!" : "VITÓRIA NO ROUND!";
                ctx.font = "36px Bangers";
                outlinedText(ctx, sub, 0.0f, 80.0f, "#00f5d4", 2.0f, 4.0f);

                ctx.font = "24px Bangers";
                outlinedText(ctx, "+500 XP DE COMBATE", 0.0f, 120.0f, "#ffffff", 1.5f, 3.0f);
            } else {
                // Derrota
                const std::string title = (engine.result == MatchResult::LossKO) ? "K.O.!" :
                                          ((engine.result == MatchResult::LossTKO) ? "T.K.O.!" : "DERROTA");
                ctx.font = "135px 'Titan One'";
                ctx.textAlign = "center";
                ctx.textBaseline = "middle";
                outlinedText(ctx, title, 0.0f, -10.0f, "#ff1e42", 5.5f, 10.0f);

                const std::string sub = (engine.result == MatchResult::LossDecision) ? "DERROTA POR PONTOS" : "NOCAUTEADO NA LONA";
                ctx.font = "36px Bangers";
                outlinedText(ctx, sub, 0.0f, 80.0f, "#ffffff", 2.0f, 4.0f);
            }

            ctx.restore();
        }

        if (state.game_state == STATE_ROUND_INTRO) {
            const int tRem = engine.roundIntroTimer;
            const int tDuration = engine.roundIntroDuration;
            const float elapsedSec = static_cast<float>(std::max(0, tDuration - tRem)) / 60.0f;
            std::string text;
            std::string color;
            std::string subtitle;
            float stepT = 0.0f;

            // Contagem Regressiva de Início de Round sincronizada com a voz do narrador e a vinheta:
            // 0.2s -> "K." (3)
            // 1.4s -> "O." (2)
            // 2.4s -> "RAMA!" (1)
            // 3.6s até 5.4s -> "LUTA!" (acompanhando toda a vinhetinha musical antes do gongo)
            if (elapsedSec < 0.20f) {
                text = "";
            } else if (elapsedSec < 1.40f) {
                text = "3";
                color = "#ffe600";
                subtitle = "K. • PREPARE-SE!";
                stepT = (elapsedSec - 0.20f) / (1.40f - 0.20f);
            } else if (elapsedSec < 2.40f) {
                text = "2";
                color = "#ff8800";
                subtitle = "O. • EM POSIÇÃO!";
                stepT = (elapsedSec - 1.40f) / (2.40f - 1.40f);
            } else if (elapsedSec < 3.60f) {
                text = "1";
                color = "#ff007f";
                subtitle = "RAMA! • GUARDAS!";
                stepT = (elapsedSec - 2.40f) / (3.60f - 2.40f);
            } else {
                text = "LUTA!";
                color = "#00f5d4";
                subtitle = "VALENDO!";
                const float totalSec = static_cast<float>(tDuration) / 60.0f;
                stepT = (elapsedSec - 3.60f) / std::max(0.1f, totalSec - 3.60f);
            }

            if (!text.empty()) {
                stepT = std::clamp(stepT, 0.0f, 1.0f);
                const bool isFight = (text == "LUTA!");
                // Efeito elástico de slam/drop cartoon e pulsação rítmica da vinhetinha
                const float pop = isFight
                    ? (1.0f + 0.55f * std::exp(-stepT * 8.0f) + 0.04f * std::sin((elapsedSec - 3.60f) * 18.0f))
                    : (1.0f + 0.65f * std::exp(-stepT * 7.0f));

                // Fase de desaparecimento VHS de "LUTA! VALENDO!" no final da vinheta (de 4.70s a 5.40s):
                const float exitT = isFight ? std::clamp((elapsedSec - 4.70f) / (5.40f - 4.70f), 0.0f, 1.0f) : 0.0f;
                const int frame = state.frame_index;

                // Colapso do feixe CRT: no terço final (exitT > 0.60f), achata drasticamente na vertical
                float crtScaleX = 1.0f;
                float crtScaleY = 1.0f;
                if (exitT > 0.60f) {
                    const float collapseP = (exitT - 0.60f) / 0.40f;
                    crtScaleY = std::max(0.015f, 1.0f - collapseP * 0.985f);
                    crtScaleX = 1.0f + collapseP * 0.35f;
                }

                // Jitter horizontal e vertical de tracking analógico VHS
                float vhsJitterX = 0.0f;
                float vhsJitterY = 0.0f;
                if (exitT > 0.0f) {
                    const float amp = 2.0f + exitT * 28.0f;
                    if ((frame % 4) != 0) {
                        vhsJitterX = std::sin(frame * 47.3f) * amp;
                    }
                    if ((frame % 5) == 0) {
                        vhsJitterY = ((frame % 3) - 1) * (2.0f + exitT * 4.0f);
                    }
                }

                // Alpha VHS com quedas de sinal estroboscópicas
                float vhsAlpha = 1.0f;
                if (exitT > 0.0f) {
                    vhsAlpha = 1.0f - exitT * 0.70f;
                    if ((frame % 5 == 0) || (frame % 9 == 2)) {
                        vhsAlpha *= 0.25f; // queda rápida de frame/tracking
                    } else if (frame % 7 == 0) {
                        vhsAlpha *= 0.60f;
                    }
                    if (exitT > 0.85f && (frame % 2 == 0)) {
                        vhsAlpha *= 0.12f;
                    }
                }

                ctx.save();
                ctx.translate(800.0f + vhsJitterX, 410.0f + vhsJitterY);
                ctx.scale(pop * crtScaleX, pop * crtScaleY);
                ctx.globalAlpha = std::clamp(vhsAlpha, 0.0f, 1.0f);

                // Linhas de impacto cartoon atrás do número / palavra
                if (exitT < 0.85f) {
                    const float rayAlpha = (1.0f - exitT * 1.15f) * 0.32f;
                    ctx.strokeStyle = cv::rgba(1.0f, 1.0f, 1.0f, std::max(0.0f, rayAlpha));
                    ctx.lineWidth = 4.0f;
                    const int rayCount = isFight ? 20 : 10;
                    for (int r = 0; r < rayCount; ++r) {
                        const float ang = r * (2.0f * cv::PI / rayCount) + stepT * 0.4f;
                        const float r1 = isFight ? 140.0f : 110.0f;
                        const float r2 = r1 + (isFight ? 120.0f : 70.0f) * (1.0f - stepT * 0.5f);
                        ctx.beginPath();
                        ctx.moveTo(std::cos(ang) * r1, std::sin(ang) * r1);
                        ctx.lineTo(std::cos(ang) * r2, std::sin(ang) * r2);
                        ctx.stroke();
                    }
                }

                if (isFight) {
                    // Tipografia explosiva de ação estilo mangá/quadrinhos (sem caixa fechada)
                    ctx.rotate(-5.5f * cv::PI / 180.0f);

                    // Faixa de corte e velocidade diagonal estilo mangá cortando o centro
                    if (exitT < 0.80f) {
                        const float slashL = 840.0f * (1.0f - std::exp(-stepT * 10.0f)) * (1.0f - exitT * 0.70f);
                        const float slashAlpha = 0.76f * (1.0f - exitT * 0.85f);
                        ctx.fillStyle = cv::rgba(0.04f, 0.02f, 0.08f, std::max(0.0f, slashAlpha));
                        ctx.beginPath();
                        ctx.moveTo(-slashL * 0.5f - 50.0f, -55.0f);
                        ctx.lineTo(slashL * 0.5f + 50.0f, -35.0f);
                        ctx.lineTo(slashL * 0.5f + 10.0f, 60.0f);
                        ctx.lineTo(-slashL * 0.5f - 90.0f, 40.0f);
                        ctx.closePath();
                        ctx.fill();

                        // Lâminas de energia ciano e ouro nas bordas do corte
                        ctx.strokeStyle = cv::rgba(0.0f, 0.96f, 0.83f, std::max(0.0f, 1.0f - exitT * 1.1f));
                        ctx.lineWidth = 4.0f;
                        ctx.beginPath();
                        ctx.moveTo(-slashL * 0.52f, -45.0f); ctx.lineTo(slashL * 0.52f, -25.0f);
                        ctx.moveTo(-slashL * 0.48f, 50.0f); ctx.lineTo(slashL * 0.48f, 70.0f);
                        ctx.stroke();

                        ctx.strokeStyle = cv::rgba(1.0f, 0.90f, 0.0f, std::max(0.0f, 1.0f - exitT * 1.1f));
                        ctx.lineWidth = 2.0f;
                        ctx.beginPath();
                        ctx.moveTo(-slashL * 0.35f, -15.0f); ctx.lineTo(slashL * 0.45f, -2.0f);
                        ctx.stroke();
                    }

                    // Aberração cromática VHS (split ciano para esquerda, magenta para direita)
                    const float chromaSplit = exitT > 0.0f ? (4.0f + exitT * 36.0f) : 0.0f;

                    // "LUTA!" gigante e estilizado em Titan One com sombra pesada de tinta
                    ctx.font = "165px 'Titan One'";
                    ctx.textAlign = "center";
                    ctx.textBaseline = "middle";

                    if (chromaSplit > 1.0f) {
                        // Canal Ciano VHS (deslocado)
                        outlinedText(ctx, text, -chromaSplit, -12, cv::rgba(0.0f, 0.96f, 0.83f, 0.85f), 3.0f, 8);
                        // Canal Magenta VHS (deslocado)
                        outlinedText(ctx, text, chromaSplit, -12, cv::rgba(1.0f, 0.0f, 0.50f, 0.85f), 3.0f, 8);
                    }
                    // Camada principal amarelo ouro com contorno nanquim
                    outlinedText(ctx, text, 0, -12, "#ffe600", 5.5f, 14);

                    // Subtítulo cinético "VALENDO!"
                    ctx.font = "46px Bangers";
                    if (chromaSplit > 1.0f) {
                        outlinedText(ctx, "VALENDO!", -chromaSplit * 0.7f, 95, cv::rgba(1.0f, 0.0f, 0.50f, 0.85f), 1.8f, 4);
                        outlinedText(ctx, "VALENDO!", chromaSplit * 0.7f, 95, cv::rgba(0.0f, 0.96f, 0.83f, 0.85f), 1.8f, 4);
                    }
                    outlinedText(ctx, "VALENDO!", 0, 95, "#00f5d4", 2.2f, 5);

                    // Barras horizontais de ruído de tracking estático da fita VHS cortando o texto
                    if (exitT > 0.08f && exitT < 0.95f) {
                        const int barCount = static_cast<int>(3 + exitT * 7);
                        for (int b = 0; b < barCount; ++b) {
                            const float barY = -90.0f + std::fmod((frame * 27.3f + b * 53.0f), 220.0f);
                            const float barH = 2.0f + (b % 4) * 3.0f;
                            const float barW = 340.0f + (b % 5) * 120.0f;
                            const float barOffX = std::sin(frame * 17.0f + b * 2.1f) * (60.0f + exitT * 100.0f);
                            ctx.fillStyle = (b % 2 == 0)
                                ? cv::rgba(1.0f, 1.0f, 1.0f, 0.65f * (1.0f - exitT * 0.5f))
                                : cv::rgba(0.0f, 0.96f, 0.83f, 0.55f * (1.0f - exitT * 0.5f));
                            ctx.fillRect(-barW * 0.5f + barOffX, barY, barW, barH);
                        }
                    }

                    // Clarão de fósforo CRT na linha central durante o colapso final
                    if (exitT > 0.68f) {
                        const float flareP = (exitT - 0.68f) / 0.32f; // 0.0 -> 1.0
                        const float flareW = std::max(0.0f, (1.0f - flareP) * 980.0f);
                        const float flareH = std::max(1.5f, (1.0f - flareP) * 7.0f);
                        const float flareAlpha = std::clamp(1.0f - flareP, 0.0f, 1.0f);
                        ctx.fillStyle = cv::rgba(1.0f, 1.0f, 1.0f, flareAlpha * 0.95f);
                        ctx.fillRect(-flareW * 0.5f, -flareH * 0.5f, flareW, flareH);
                        ctx.fillStyle = cv::rgba(0.0f, 0.96f, 0.83f, flareAlpha * 0.75f);
                        ctx.fillRect(-flareW * 0.45f, -flareH * 0.25f, flareW * 0.9f, flareH * 0.5f);
                    }
                } else {
                    // Números gigantes comic
                    ctx.font = "175px 'Titan One'";
                    ctx.textAlign = "center";
                    ctx.textBaseline = "middle";
                    outlinedText(ctx, text, 0, -10, color.c_str(), 4.0f, 10);

                    // Subtítulo estilizado abaixo do número
                    ctx.font = "36px Bangers";
                    outlinedText(ctx, subtitle, 0, 95, "#ffffff", 1.5f, 4);
                }

                ctx.restore();
            }
        }

        if (!paused && !engine.isMatchOver()) {
            if (engine.trainingMode) {
                // Dica no canto inferior direito, sem colidir com o botão de pausa central
                ctx.save();
                ctx.translate(1390, 857);
                ctx.font = "900 11px Montserrat";
                ctx.textAlign = "center";
                ctx.textBaseline = "middle";
                const std::string hint = "[TAB] TROCAR LUTADOR   ·   [B] MONTAR BUILD";
                const float hw = ctx.measureText(hint) + 26;
                inkBox(ctx, -hw / 2, -13, hw, 26, cv::Style("#120d20"), 1.8f, 2, 4);
                ctx.fillStyle = "#ffe600";
                ctx.fillText(hint, 0, 0);
                ctx.restore();
            }
            // Botão de pausa para o mouse
            const sf::FloatRect pb = pauseButton();
            const bool hover = pb.contains(uiMouse);
            ctx.save();
            ctx.globalAlpha = hover ? 1.0f : 0.7f;
            inkBox(ctx, pb.position.x, pb.position.y, pb.size.x, pb.size.y, cv::Style(hover ? "#ffe600" : "#120d20"), 3, 4, 6);
            ctx.fillStyle = hover ? "#000000" : "#ffffff";
            ctx.fillRect(pb.position.x + 20, pb.position.y + 13, 8, 24);
            ctx.fillRect(pb.position.x + 36, pb.position.y + 13, 8, 24);
            ctx.restore();
        }
        if (paused) {
            // Quadro congelado: escurece, faixa diagonal magenta com "PAUSA" carimbado e as
            // opções em placas inclinadas com a tecla em destaque.
            ctx.fillStyle = "rgba(5, 0, 13, 0.74)";
            ctx.fillRect(0, 0, 1600, 900);
            hatch(ctx, 0, 0, 1600, 900, 10, "rgba(255,255,255,0.03)", 3, true);
            ctx.beginPath(); ctx.moveTo(0, 196); ctx.lineTo(1600, 136); ctx.lineTo(1600, 332); ctx.lineTo(0, 392); ctx.closePath();
            ctx.fillStyle = "#000000"; ctx.fill();
            ctx.beginPath(); ctx.moveTo(0, 210); ctx.lineTo(1600, 150); ctx.lineTo(1600, 318); ctx.lineTo(0, 378); ctx.closePath();
            ctx.fillStyle = "#ff007f"; ctx.fill();
            hatch(ctx, 0, 150, 1600, 228, 12, "rgba(0,0,0,0.12)", 4, false);
            ctx.save();
            ctx.translate(800, 262); ctx.rotate(-2.2f * cv::PI / 180);
            ctx.font = "150px 'Titan One'"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
            for (int i = 8; i >= 1; --i) { ctx.fillStyle = "#000000"; ctx.fillText("PAUSA", i * 1.2f, i * 1.2f); }
            outlinedText(ctx, "PAUSA", 0, 0, "#ffe600", 4, 0);
            // Duas barras de "pause" ao lado
            for (int k = 0; k < 2; ++k) {
                inkBox(ctx, -330 + k * 34, -46, 22, 92, cv::Style("#ffffff"), 4, 5);
                inkBox(ctx, 286 + k * 34, -46, 22, 92, cv::Style("#ffffff"), 4, 5);
            }
            ctx.restore();

            const std::pair<const char*, const char*> rows[] = {{"ESC", "CONTINUAR"}, {"R", "REINICIAR LUTA"}, {"Q", "SAIR PARA O MENU"}, {"M", "LIGAR / DESLIGAR SOM"}};
            static const char* kFill[] = {"#00f5d4", "#ffe600", "#ff007f", "#9d4edd"};
            static const char* kInk[] = {"#000000", "#000000", "#ffffff", "#ffffff"};

            const std::pair<const char*, const char*> rowsTrain[] = {
                {"ESC", "CONTINUAR"},
                {"TAB", "TROCAR LUTADOR"},
                {"B", "MONTAR BUILD"},
                {"R", "REINICIAR TREINO"},
                {"Q", "SAIR PARA O MENU"},
                {"M", "LIGAR / DESLIGAR SOM"}
            };
            static const char* kFillTrain[] = {"#00f5d4", "#ff007f", "#ffe600", "#38bdf8", "#f43f5e", "#9d4edd"};
            static const char* kInkTrain[] = {"#000000", "#ffffff", "#000000", "#000000", "#ffffff", "#ffffff"};

            const int count = engine.trainingMode ? 6 : 4;
            for (int i = 0; i < count; ++i) {
                const sf::FloatRect pr = pausePlate(i, engine.trainingMode);
                const bool hover = pr.contains(uiMouse);
                const float w = pr.size.x, h = pr.size.y, x = pr.position.x - (hover ? 4.0f : 0.0f), y = pr.position.y - (hover ? 4.0f : 0.0f);
                ctx.fillStyle = "#000000"; slantPath(ctx, x + 8, y + 8, w, h, 22, true); ctx.fill();
                ctx.fillStyle = hover ? "#2a1f45" : "#120d20"; slantPath(ctx, x, y, w, h, 22, true); ctx.fill();
                ctx.strokeStyle = hover ? "#ffffff" : "#000000"; ctx.lineWidth = 4; ctx.stroke();
                const char* fill = engine.trainingMode ? kFillTrain[i] : kFill[i];
                const char* ink = engine.trainingMode ? kInkTrain[i] : kInk[i];
                ctx.fillStyle = fill; slantPath(ctx, x, y, 130, h, 22, true); ctx.fill(); ctx.stroke();
                ctx.font = "18px 'Press Start 2P'"; ctx.textAlign = "center"; ctx.textBaseline = "middle";
                ctx.fillStyle = ink; ctx.fillText(engine.trainingMode ? rowsTrain[i].first : rows[i].first, x + 56, y + h / 2 + 2);
                ctx.font = engine.trainingMode ? "24px 'Titan One'" : "28px 'Titan One'"; ctx.textAlign = "left";
                outlinedText(ctx, engine.trainingMode ? rowsTrain[i].second : rows[i].second, x + 150, y + h / 2, "#ffffff", 0, 3);
            }
        }
    }
    target.setView(fightView);
}

// Onomatopeia do impacto (POW!, BAM!, KA-POW!): desenhada no ringue, atrás dos lutadores e
// deslocada para o lado, para não esconder a reação de quem apanha.
void RingRenderer::queueImpactWord(const GameEngine& engine) {
    if (engine.impactTimer <= 0) return;
    auto pr = camera_.project(engine.impactAt);
    if (!pr) return;
    const int kind = engine.impactKind, seed = engine.impactSeed, timer = engine.impactTimer;
    const float side = (seed % 2) ? 1.0f : -1.0f;
    const sf::Vector2f at{pr->pos.x + side * 120.0f * std::max(0.6f, pr->scale), pr->pos.y - 60.0f * std::max(0.6f, pr->scale)};
    queue_.push_back({3.2f, pr->depth + 40.0f, [at, kind, seed, timer](sf::RenderTarget& t) {
        Canvas ctx(t);
        const int dur = kind == 4 ? 40 : (kind == 0 ? 16 : 26);
        const float age = 1.0f - static_cast<float>(timer) / dur;
        static const char* kLight[] = {"PÁ!", "TUM!", "PLAFT!"};
        static const char* kHeavy[] = {"POW!", "BAM!", "WHAM!", "SOCK!"};
        const char* word = kind == 0 ? kLight[seed % 3] : kind == 1 ? kHeavy[seed % 4] : kind == 2 ? "KA-POW!" : kind == 3 ? "CRACK!" : "K.O.!";
        const char* fill = kind == 2 ? "#00f5d4" : kind == 3 ? "#ff0055" : kind == 4 ? "#ff1e42" : "#ffe600";
        const char* ink = kind == 0 || kind == 1 ? "#ff1e42" : "#ffffff";
        const float base = (kind == 0 ? 0.3f : (kind == 4 ? 0.8f : 0.5f));
        const float pop = age < 0.18f ? 0.5f + age / 0.18f * 0.7f : 1.2f - std::min(0.2f, (age - 0.18f) * 0.6f);
        const float k = base * pop;
        ctx.save();
        ctx.globalAlpha = age > 0.75f ? std::max(0.0f, 1.0f - (age - 0.75f) / 0.25f) : 1.0f;
        ctx.translate(at.x, at.y - age * 12.0f);
        ctx.rotate(((seed % 5) - 2) * 6.0f * cv::PI / 180);
        ctx.scale(k, k);
        ctx.beginPath();
        const int spikes = 14;
        for (int i = 0; i < spikes * 2; ++i) {
            const float a = i * cv::PI / spikes, r0 = i % 2 ? 62.0f : 100.0f + ((seed * 7 + i * 13) % 5) * 9.0f;
            if (i == 0) ctx.moveTo(std::cos(a) * r0, std::sin(a) * r0 * 0.7f); else ctx.lineTo(std::cos(a) * r0, std::sin(a) * r0 * 0.7f);
        }
        ctx.closePath();
        ctx.save(); ctx.translate(7, 7); ctx.fillStyle = "#000000"; ctx.fill(); ctx.restore();
        ctx.fillStyle = fill; ctx.fill();
        ctx.strokeStyle = "#000000"; ctx.lineWidth = 6; ctx.stroke();
        ctx.font = kind == 2 ? "54px Bangers" : "64px Bangers";
        ctx.textAlign = "center"; ctx.textBaseline = "middle";
        outlinedText(ctx, word, 0, 4, ink, 3.5f, 5);
        ctx.restore();
    }});
}

} // namespace KoRama
