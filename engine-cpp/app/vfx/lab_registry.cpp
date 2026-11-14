#include "lab_common.hpp"

namespace KoRama::vfx {

std::unique_ptr<LabFx> makeJackFx();
std::unique_ptr<LabFx> makeFrogFx();
std::unique_ptr<LabFx> makeJoanaFx();
std::unique_ptr<LabFx> makeCremosoFx();
std::unique_ptr<LabFx> makeValeriaFx();
std::unique_ptr<LabFx> makeVivaldiFx();
std::unique_ptr<LabFx> makeEspinhaFx();
std::unique_ptr<LabFx> makeMarcovFx();

std::unique_ptr<LabFx> makeLabFx(uint32_t charId) {
    switch (charId) {
    case 0: return makeJackFx();
    case 1: return makeFrogFx();
    case 2: return makeJoanaFx();
    case 3: return makeCremosoFx();
    case 4: return makeValeriaFx();
    case 5: return makeVivaldiFx();
    case 6: return makeEspinhaFx();
    case 7: return makeMarcovFx();
    default: return nullptr;
    }
}

} // namespace KoRama::vfx
