#include "live_rig.hpp"
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace KoRama {

#pragma pack(push, 1)
struct RigbHeader {
    char magic[4]; // 'R', 'I', 'G', 'B'
    uint32_t version; // 1
    uint64_t trackFileSize;
    uint64_t trackModTime;
    int32_t fps;
    int32_t nSheets;
    uint32_t numParts;
    uint32_t numInsts;
    uint32_t numClips;
};

struct PartDisk {
    int32_t sheet;
    float x, y, w, h;
    float ox, oy, upp;
};

struct InstDisk {
    int32_t part;
    float a, b, c, d, e, f, op;
    int32_t glove;
    uint8_t slotKind;
};

struct FrameDisk {
    float opacity;
    uint32_t first;
    uint32_t count;
};
#pragma pack(pop)

bool LiveRig::saveBinary(const std::filesystem::path& rigbPath, uint64_t trackFileSize, uint64_t trackModTime, int nSheets) const {
    std::ofstream out(rigbPath, std::ios::binary);
    if (!out) return false;

    RigbHeader hdr;
    hdr.magic[0] = 'R'; hdr.magic[1] = 'I'; hdr.magic[2] = 'G'; hdr.magic[3] = 'B';
    hdr.version = 1;
    hdr.trackFileSize = trackFileSize;
    hdr.trackModTime = trackModTime;
    hdr.fps = fps;
    hdr.nSheets = nSheets;
    hdr.numParts = static_cast<uint32_t>(parts.size());
    hdr.numInsts = static_cast<uint32_t>(insts.size());
    hdr.numClips = static_cast<uint32_t>(clips.size());

    out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));

    std::vector<PartDisk> partDisks(parts.size());
    for (size_t i = 0; i < parts.size(); ++i) {
        partDisks[i].sheet = parts[i].sheet;
        partDisks[i].x = parts[i].rect.position.x;
        partDisks[i].y = parts[i].rect.position.y;
        partDisks[i].w = parts[i].rect.size.x;
        partDisks[i].h = parts[i].rect.size.y;
        partDisks[i].ox = parts[i].ox;
        partDisks[i].oy = parts[i].oy;
        partDisks[i].upp = parts[i].upp;
    }
    if (!partDisks.empty()) {
        out.write(reinterpret_cast<const char*>(partDisks.data()), partDisks.size() * sizeof(PartDisk));
    }

    std::vector<InstDisk> instDisks(insts.size());
    for (size_t i = 0; i < insts.size(); ++i) {
        instDisks[i].part = insts[i].part;
        instDisks[i].a = insts[i].a;
        instDisks[i].b = insts[i].b;
        instDisks[i].c = insts[i].c;
        instDisks[i].d = insts[i].d;
        instDisks[i].e = insts[i].e;
        instDisks[i].f = insts[i].f;
        instDisks[i].op = insts[i].op;
        instDisks[i].glove = insts[i].glove;
        instDisks[i].slotKind = insts[i].slotKind;
    }
    if (!instDisks.empty()) {
        out.write(reinterpret_cast<const char*>(instDisks.data()), instDisks.size() * sizeof(InstDisk));
    }

    for (const auto& [name, c] : clips) {
        uint16_t nameLen = static_cast<uint16_t>(name.size());
        out.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
        out.write(name.data(), nameLen);
        uint8_t loop = c.loop ? 1 : 0;
        out.write(reinterpret_cast<const char*>(&loop), sizeof(loop));
        out.write(reinterpret_cast<const char*>(&c.dur), sizeof(c.dur));
        out.write(reinterpret_cast<const char*>(c.box), sizeof(c.box));
        uint32_t numFrames = static_cast<uint32_t>(c.frames.size());
        out.write(reinterpret_cast<const char*>(&numFrames), sizeof(numFrames));
        std::vector<FrameDisk> frameDisks(c.frames.size());
        for (size_t i = 0; i < c.frames.size(); ++i) {
            frameDisks[i].opacity = c.frames[i].opacity;
            frameDisks[i].first = c.frames[i].first;
            frameDisks[i].count = c.frames[i].count;
        }
        if (!frameDisks.empty()) {
            out.write(reinterpret_cast<const char*>(frameDisks.data()), frameDisks.size() * sizeof(FrameDisk));
        }
    }
    return out.good();
}

bool LiveRig::loadBinary(const std::filesystem::path& rigbPath, uint64_t expectedTrackSize, uint64_t expectedTrackModTime, int& outNSheets) {
    std::ifstream in(rigbPath, std::ios::binary);
    if (!in) return false;

    RigbHeader hdr;
    if (!in.read(reinterpret_cast<char*>(&hdr), sizeof(hdr))) return false;
    if (hdr.magic[0] != 'R' || hdr.magic[1] != 'I' || hdr.magic[2] != 'G' || hdr.magic[3] != 'B') return false;
    if (hdr.version != 1) return false;
    if (hdr.trackFileSize != expectedTrackSize || hdr.trackModTime != expectedTrackModTime) return false;

    fps = hdr.fps;
    outNSheets = hdr.nSheets;

    parts.resize(hdr.numParts);
    if (hdr.numParts > 0) {
        std::vector<PartDisk> partDisks(hdr.numParts);
        if (!in.read(reinterpret_cast<char*>(partDisks.data()), hdr.numParts * sizeof(PartDisk))) return false;
        for (size_t i = 0; i < hdr.numParts; ++i) {
            parts[i].sheet = partDisks[i].sheet;
            parts[i].rect = sf::FloatRect({partDisks[i].x, partDisks[i].y}, {partDisks[i].w, partDisks[i].h});
            parts[i].ox = partDisks[i].ox;
            parts[i].oy = partDisks[i].oy;
            parts[i].upp = partDisks[i].upp;
        }
    }

    insts.resize(hdr.numInsts);
    if (hdr.numInsts > 0) {
        std::vector<InstDisk> instDisks(hdr.numInsts);
        if (!in.read(reinterpret_cast<char*>(instDisks.data()), hdr.numInsts * sizeof(InstDisk))) return false;
        for (size_t i = 0; i < hdr.numInsts; ++i) {
            insts[i].part = instDisks[i].part;
            insts[i].a = instDisks[i].a;
            insts[i].b = instDisks[i].b;
            insts[i].c = instDisks[i].c;
            insts[i].d = instDisks[i].d;
            insts[i].e = instDisks[i].e;
            insts[i].f = instDisks[i].f;
            insts[i].op = instDisks[i].op;
            insts[i].glove = instDisks[i].glove;
            insts[i].slotKind = static_cast<SlotKind>(instDisks[i].slotKind);
        }
    }

    clips.clear();
    clips.reserve(hdr.numClips);
    std::vector<char> nameBuf;
    for (uint32_t cIdx = 0; cIdx < hdr.numClips; ++cIdx) {
        uint16_t nameLen = 0;
        if (!in.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen))) return false;
        nameBuf.resize(nameLen);
        if (!in.read(nameBuf.data(), nameLen)) return false;
        std::string name(nameBuf.data(), nameLen);

        Clip c;
        uint8_t loop = 0;
        if (!in.read(reinterpret_cast<char*>(&loop), sizeof(loop))) return false;
        c.loop = (loop != 0);
        if (!in.read(reinterpret_cast<char*>(&c.dur), sizeof(c.dur))) return false;
        if (!in.read(reinterpret_cast<char*>(c.box), sizeof(c.box))) return false;

        uint32_t numFrames = 0;
        if (!in.read(reinterpret_cast<char*>(&numFrames), sizeof(numFrames))) return false;
        if (numFrames > 0) {
            std::vector<FrameDisk> frameDisks(numFrames);
            if (!in.read(reinterpret_cast<char*>(frameDisks.data()), numFrames * sizeof(FrameDisk))) return false;
            c.frames.resize(numFrames);
            for (size_t f = 0; f < numFrames; ++f) {
                c.frames[f].opacity = frameDisks[f].opacity;
                c.frames[f].first = frameDisks[f].first;
                c.frames[f].count = frameDisks[f].count;
            }
        }
        clips[std::move(name)] = std::move(c);
    }
    return !clips.empty();
}

bool LiveRig::parse(const std::filesystem::path& dir, const std::string& key) {
    const auto trackPath = dir / ("live_" + key + ".track");
    std::error_code ec;
    if (!std::filesystem::exists(trackPath, ec)) return false;
    const uint64_t trackSize = std::filesystem::file_size(trackPath, ec);
    const auto trackFtime = std::filesystem::last_write_time(trackPath, ec);
    const uint64_t trackModTime = static_cast<uint64_t>(trackFtime.time_since_epoch().count());

    const auto rigbPath = dir / ("live_" + key + ".rigb");
    int nSheets = 0;
    bool loadedRig = false;

    if (std::filesystem::exists(rigbPath, ec)) {
        loadedRig = loadBinary(rigbPath, trackSize, trackModTime, nSheets);
    }

    if (!loadedRig) {
        parts.clear();
        insts.clear();
        clips.clear();
        std::ifstream in(trackPath, std::ios::binary);
        if (!in) return false;
        std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        const char* s = text.c_str();
        const char* end = s + text.size();
        auto nextLine = [&](const char*& p) { while (p < end && *p != '\n') ++p; if (p < end) ++p; };
        auto word = [](const char*& p, char* out, std::size_t n) {
            while (*p == ' ') ++p;
            std::size_t i = 0;
            while (*p && *p != ' ' && *p != '\n' && *p != '\r' && i + 1 < n) out[i++] = *p++;
            out[i] = 0;
        };
        auto num = [](const char*& p) -> float {
            while (*p == ' ') ++p;
            bool neg = false;
            if (*p == '-') { neg = true; ++p; }
            else if (*p == '+') { ++p; }
            double v = 0.0;
            while (*p >= '0' && *p <= '9') {
                v = v * 10.0 + (*p - '0');
                ++p;
            }
            if (*p == '.') {
                ++p;
                double frac = 0.1;
                while (*p >= '0' && *p <= '9') {
                    v += (*p - '0') * frac;
                    frac *= 0.1;
                    ++p;
                }
            }
            if (*p == 'e' || *p == 'E') {
                ++p;
                bool expNeg = false;
                if (*p == '-') { expNeg = true; ++p; }
                else if (*p == '+') { ++p; }
                int exp = 0;
                while (*p >= '0' && *p <= '9') {
                    exp = exp * 10 + (*p - '0');
                    ++p;
                }
                double factor = 1.0;
                while (exp > 0) { factor *= 10.0; --exp; }
                if (expNeg) v /= factor;
                else v *= factor;
            }
            return static_cast<float>(neg ? -v : v);
        };
        Clip* cur = nullptr;
        const char* p = s;
        char w[64];
        while (p < end) {
            const char* line = p;
            if (*line == '#') { nextLine(p); continue; }
            word(p, w, sizeof w);
            if (!std::strcmp(w, "rig")) {
                fps = static_cast<int>(num(p));
                nSheets = static_cast<int>(num(p));
                parts.reserve(static_cast<std::size_t>(num(p)));
            } else if (!std::strcmp(w, "part")) {
                Part pt;
                pt.sheet = static_cast<int>(num(p));
                float x = num(p), y = num(p), ww = num(p), hh = num(p);
                pt.rect = sf::FloatRect({x, y}, {ww, hh});
                pt.ox = num(p); pt.oy = num(p); pt.upp = num(p);
                parts.push_back(pt);
            } else if (!std::strcmp(w, "clip")) {
                char form[64], view[8], name[64];
                word(p, form, sizeof form); word(p, view, sizeof view); word(p, name, sizeof name);
                Clip c;
                c.loop = num(p) != 0.0f;
                c.dur = num(p);
                c.frames.reserve(static_cast<std::size_t>(num(p)));
                for (float& b : c.box) { while (*p == ' ') ++p; if (*p == 10 || *p == 13 || !*p) break; b = num(p); }
                cur = &(clips[std::string(form) + "|" + view + "|" + name] = std::move(c));
            } else if (!std::strcmp(w, "f") && cur) {
                Frame fr;
                fr.opacity = num(p);
                num(p); // clarão do palco (não usado)
                fr.count = static_cast<std::uint32_t>(num(p));
                fr.first = static_cast<std::uint32_t>(insts.size());
                nextLine(p);
                for (std::uint32_t i = 0; i < fr.count && p < end; ++i) {
                    Inst it;
                    it.part = static_cast<int>(num(p));
                    it.a = num(p); it.b = num(p); it.c = num(p); it.d = num(p); it.e = num(p); it.f = num(p); it.op = num(p);
                    while (*p == ' ') ++p;
                    if (p[0] == 'g' && p[1] == 'L' && (p[2] == ' ' || p[2] == '\n' || p[2] == '\r')) {
                        it.glove = 1; it.slotKind = SlotGloveL;
                    } else if (p[0] == 'g' && p[1] == 'R' && (p[2] == ' ' || p[2] == '\n' || p[2] == '\r')) {
                        it.glove = 2; it.slotKind = SlotGloveR;
                    } else if (p[0] == 'l' && p[1] == 'e' && p[2] == 'g' && p[3] == 'L') {
                        it.glove = 0; it.slotKind = SlotLegL;
                    } else if (p[0] == 'l' && p[1] == 'e' && p[2] == 'g' && p[3] == 'R') {
                        it.glove = 0; it.slotKind = SlotLegR;
                    } else if (p[0] == 't' && p[1] == 'o' && p[2] == 'r' && p[3] == 's' && p[4] == 'o') {
                        it.glove = 0; it.slotKind = SlotTorso;
                    } else {
                        it.glove = 0; it.slotKind = SlotOther;
                    }
                    insts.push_back(it);
                    nextLine(p);
                }
                cur->frames.push_back(fr);
                continue;
            }
            nextLine(p);
        }
        text.clear(); text.shrink_to_fit();
        if (!clips.empty()) {
            saveBinary(rigbPath, trackSize, trackModTime, nSheets);
        }
    }

    if (clips.empty()) return false;

    const char* full = std::getenv("KORAMA_FULL_RIG_TEXTURES");
    const bool compact = nSheets >= 3 && key.find("_skin") != std::string::npos && !(full && full[0] == '1') &&
        std::all_of(parts.begin(), parts.end(), [](const Part& pt) { return pt.upp <= .401f; });

    images.clear();
    for (int i = 0; i < nSheets; ++i) {
        const auto pathOrig = dir / ("live_" + key + "_" + std::to_string(i) + ".png");
        const auto path2k = dir / ("live_" + key + "_" + std::to_string(i) + "_2k.png");

        sf::Image img;
        bool loaded2k = false;
        if (compact) {
            std::error_code ec2;
            if (std::filesystem::exists(path2k, ec2)) {
                auto t2k = std::filesystem::last_write_time(path2k, ec2);
                auto tOrig = std::filesystem::last_write_time(pathOrig, ec2);
                if (!ec2 && t2k >= tOrig) {
                    if (img.loadFromFile(path2k)) {
                        loaded2k = true;
                    }
                }
            }
        }

        if (!loaded2k) {
            if (!img.loadFromFile(pathOrig)) {
                std::cerr << "[K.O.-RAMA] Folha do esqueleto ausente: live_" << key << "_" << i << ".png\n";
                return false;
            }
            if (compact) {
                auto& image = img;
                const auto size = image.getSize();
                const unsigned w2 = (size.x + 1) / 2, h2 = (size.y + 1) / 2;
                std::vector<std::uint8_t> pixels(std::size_t(w2) * h2 * 4);
                const auto* source = image.getPixelsPtr();
                for (unsigned y = 0; y < h2; ++y) {
                    const unsigned y0 = y * 2;
                    const unsigned y1 = std::min(y0 + 1, size.y - 1);
                    const auto* row0 = source + std::size_t(y0) * size.x * 4;
                    const auto* row1 = source + std::size_t(y1) * size.x * 4;
                    auto* outRow = pixels.data() + std::size_t(y) * w2 * 4;
                    for (unsigned x = 0; x < w2; ++x) {
                        const unsigned x0 = x * 2;
                        const unsigned x1 = std::min(x0 + 1, size.x - 1);
                        const auto* p00 = row0 + x0 * 4;
                        const auto* p01 = row0 + x1 * 4;
                        const auto* p10 = row1 + x0 * 4;
                        const auto* p11 = row1 + x1 * 4;
                        auto* dst = outRow + x * 4;
                        dst[0] = static_cast<std::uint8_t>((p00[0] + p01[0] + p10[0] + p11[0] + 2) / 4);
                        dst[1] = static_cast<std::uint8_t>((p00[1] + p01[1] + p10[1] + p11[1] + 2) / 4);
                        dst[2] = static_cast<std::uint8_t>((p00[2] + p01[2] + p10[2] + p11[2] + 2) / 4);
                        dst[3] = static_cast<std::uint8_t>((p00[3] + p01[3] + p10[3] + p11[3] + 2) / 4);
                    }
                }
                image = sf::Image({w2, h2}, pixels.data());
                image.saveToFile(path2k);
            }
        }
        images.push_back(std::move(img));
    }
    if (compact) for (auto& pt : parts) { pt.rect.position *= .5f; pt.rect.size *= .5f; pt.upp *= 2.f; }
    return !clips.empty();
}

int LiveRig::uploadStep(std::size_t byteBudget) {
    if (clips.empty()) return -1;
    if (uploadPage >= images.size()) return 1;
    sf::Image& img = images[uploadPage];
    const auto size = img.getSize();
    if (!size.x || !size.y) return -1;
    if (uploadRow == 0) {
        auto t = std::make_unique<sf::Texture>();
        if (!t->resize(size)) return -1;
        t->setSmooth(true);
        sheets.push_back(std::move(t));
    }
    const unsigned rows = std::min(size.y - uploadRow, std::max(1u, unsigned(byteBudget / (std::size_t(size.x) * 4))));
    sheets.back()->update(img.getPixelsPtr() + std::size_t(uploadRow) * size.x * 4, {size.x, rows}, {0, uploadRow});
    uploadRow += rows;
    if (uploadRow == size.y) {
        sheets.back()->generateMipmap(); // retain trilinear/minification sampling
        img = sf::Image{};
        uploadRow = 0;
        ++uploadPage;
        if (uploadPage == images.size()) { images.clear(); images.shrink_to_fit(); return 1; }
    }
    return 0;
}

bool LiveRig::upload() {
    int status = 0;
    do { status = uploadStep(64u * 1024u * 1024u); } while (status == 0);
    return status == 1;
}

const LiveRig::Clip* LiveRig::clip(const std::string& form, bool back, const std::string& name) const {
    auto it = clips.find(form + (back ? "|B|" : "|F|") + name);
    return it == clips.end() ? nullptr : &it->second;
}

} // namespace KoRama
