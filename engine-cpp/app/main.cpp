#include "game_app.hpp"
#include <cstdlib>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>

extern "C" {
    // Força laptops com placas híbridas (NVIDIA Optimus) a usarem a GPU dedicada NVIDIA (GeForce MX110)
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    // Força laptops com AMD a usarem a GPU dedicada
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
    timeBeginPeriod(1);
#endif
    std::srand(static_cast<unsigned>(std::time(nullptr))); // IA e partículas variam a cada luta
    if (argc == 9 && std::string(argv[1]) == "--bench-versus") {
        KoRama::GameApp app(argv[2]);
        return app.benchmarkVersus(argv[3], static_cast<uint32_t>(std::atoi(argv[4])),
                                   static_cast<uint32_t>(std::atoi(argv[5])), static_cast<uint32_t>(std::atoi(argv[6])),
                                   static_cast<uint32_t>(std::atoi(argv[7])), std::atoi(argv[8]));
    }
    KoRama::GameApp app(KoRama::GameApp::findAssetRoot(argc > 0 ? argv[0] : nullptr));
    if (argc > 1 && std::string(argv[1]) == "--test-boot") {
        app.dumpBootFrames();
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--test-frame") {
        app.dumpTestFrame("test_frame.png");
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--test-joana") {
        app.dumpTestFrame("snap_stage_joana_boat.png", false, 8);
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--test-training") {
        app.dumpMenuFrame("snap_mode_select.png", 5);
        app.dumpTestFrame("snap_training_idle.png", true, 0);
        app.dumpTestFrame("snap_training_hit.png", true, 1);
        app.dumpTestFrame("snap_training_upper.png", true, 2);
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--test-bees") {
        app.dumpTestFrame("snap_bees_swarm.png", true, 3);
        app.dumpTestFrame("snap_bees_infest.png", true, 4);
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--test-hud") {
        app.dumpTestFrame("snap_hud_blind_banner.png", true, 5);
        app.dumpTestFrame("snap_hud_knockdown_mash.png", true, 6);
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "--test-build") {
        const uint32_t c = argc > 2 ? static_cast<uint32_t>(std::atoi(argv[2])) : 3u;
        const uint32_t s = argc > 3 ? static_cast<uint32_t>(std::atoi(argv[3])) : 0u;
        app.dumpBuildFrame("snap_build_cremoso.png", c, s);
        return 0;
    }
    const int ret = app.run();
#ifdef _WIN32
    timeEndPeriod(1);
#endif
    return ret;
}
