cmake_minimum_required(VERSION 3.24)
project(KoRama LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Simulação da luta (lutadores, combate, IA, partículas) — sem dependência gráfica.
set(KORAMA_CORE_SOURCES
    src/fighter.cpp
    src/skills.cpp
    src/particles.cpp
    src/combat.cpp
    src/ai.cpp
    src/engine.cpp
    src/skill_poses.cpp
    src/tournament.cpp
)

add_library(korama_core STATIC ${KORAMA_CORE_SOURCES})
target_include_directories(korama_core PUBLIC include)

# Testes das regras de luta, sem janela.
enable_testing()
add_executable(korama_tests tests/engine_tests.cpp)
target_link_libraries(korama_tests PRIVATE korama_core)
if (MINGW)
    target_link_options(korama_tests PRIVATE -static)
endif()
add_test(NAME engine_rules COMMAND korama_tests)

# --- SFML (baixada e compilada estaticamente na primeira configuração) ---
include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO ON CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)
FetchContent_MakeAvailable(SFML)

# --- Jogo 3D isolado (etapa 1 do docs/superpowers/specs/2026-10-30-korama3d-design.md) ---
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1 GIT_SHALLOW ON EXCLUDE_FROM_ALL SYSTEM)
FetchContent_Declare(cgltf GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git GIT_TAG v1.14 GIT_SHALLOW ON SOURCE_SUBDIR none)
set(GLAD_PROFILE "core" CACHE STRING "" FORCE)
set(GLAD_API "gl=3.3" CACHE STRING "" FORCE)
FetchContent_Declare(glad GIT_REPOSITORY https://github.com/Dav1dde/glad.git GIT_TAG v0.1.36 GIT_SHALLOW ON EXCLUDE_FROM_ALL SYSTEM)
FetchContent_MakeAvailable(glm cgltf)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)   # o CMakeLists do glad 0.1.36 declara uma versão antiga demais
FetchContent_MakeAvailable(glad)
unset(CMAKE_POLICY_VERSION_MINIMUM)

add_library(korama3d_rig STATIC app3d/rig/rig.cpp)
target_include_directories(korama3d_rig PUBLIC app3d)
target_link_libraries(korama3d_rig PUBLIC korama_core glm::glm)

add_executable(korama3d_rig_tests tests/rig_tests.cpp)
target_link_libraries(korama3d_rig_tests PRIVATE korama3d_rig)
if (MINGW)
    target_link_options(korama3d_rig_tests PRIVATE -static)
endif()
add_test(NAME rig3d COMMAND korama3d_rig_tests)

# o menu é o do jogo original (título, seleção de lutador, modo, build, VS): o mesmo código do app/
add_executable(korama3d app3d/game/main.cpp app3d/gl/shader.cpp app3d/model/model.cpp app3d/render/renderer.cpp
    app/canvas2d.cpp app/ui_fonts.cpp app/ui_draw.cpp app/sprite_atlas.cpp app/live_rig.cpp app/preview_video.cpp app/menu_screens.cpp app/menu_draw.cpp app/audio_system.cpp
    # a HUD da luta é a do jogo original (RingRenderer::drawComicHud), que puxa o desenho 2D inteiro
    app/ring_renderer.cpp app/fight_hud.cpp app/celebration.cpp app/fighter_pose.cpp app/jack_rig.cpp app/fighter_rig.cpp app/live_fighter.cpp
    app/vfx/lab_registry.cpp app/vfx/lab_jack.cpp app/vfx/lab_frog.cpp app/vfx/lab_joana.cpp app/vfx/lab_cremoso.cpp app/vfx/lab_valeria.cpp app/vfx/lab_vivaldi.cpp app/vfx/lab_espinha.cpp app/vfx/lab_marcov.cpp)
target_include_directories(korama3d PRIVATE app)
target_include_directories(korama3d PRIVATE ${cgltf_SOURCE_DIR})
target_link_libraries(korama3d PRIVATE korama3d_rig SFML::Graphics SFML::Window SFML::System SFML::Audio glad)
target_compile_definitions(korama3d PRIVATE KORAMA_ROOT="${CMAKE_SOURCE_DIR}/..")
if (MINGW)
    target_link_options(korama3d PRIVATE -static)
endif()

# --- Jogo nativo ---
add_executable(korama
    app/main.cpp
    app/game_app.cpp
    app/audio_system.cpp
    app/canvas2d.cpp
    app/ui_fonts.cpp
    app/celebration.cpp
    app/fight_hud.cpp
    app/fighter_pose.cpp
    app/jack_rig.cpp
    app/fighter_rig.cpp
    app/live_rig.cpp
    app/live_fighter.cpp
    app/vfx/lab_registry.cpp
    app/vfx/lab_jack.cpp
    app/vfx/lab_frog.cpp
    app/vfx/lab_joana.cpp
    app/vfx/lab_cremoso.cpp
    app/vfx/lab_valeria.cpp
    app/vfx/lab_vivaldi.cpp
    app/vfx/lab_espinha.cpp
    app/vfx/lab_marcov.cpp
    app/ring_renderer.cpp
    app/sprite_atlas.cpp
    app/menu_screens.cpp
    app/menu_draw.cpp
    app/preview_video.cpp
    app/ui_draw.cpp
)
target_include_directories(korama PRIVATE app)
target_link_libraries(korama PRIVATE korama_core SFML::Graphics SFML::Window SFML::System SFML::Audio)

# assets/ viaja junto com o executável para a build rodar de qualquer pasta.
add_custom_command(TARGET korama POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:korama>/assets
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(korama PRIVATE -Wall -Wextra)
endif()

# Ferramenta de desenvolvimento: fotografa cenas da luta em PNG sem abrir janela (tools/snapshot).
add_executable(korama_snapshot EXCLUDE_FROM_ALL
    tools/snapshot/snapshot.cpp
    app/canvas2d.cpp
    app/ui_fonts.cpp
    app/celebration.cpp
    app/fight_hud.cpp
    app/fighter_pose.cpp
    app/jack_rig.cpp
    app/fighter_rig.cpp
    app/live_rig.cpp
    app/live_fighter.cpp
    app/vfx/lab_registry.cpp
    app/vfx/lab_jack.cpp
    app/vfx/lab_frog.cpp
    app/vfx/lab_joana.cpp
    app/vfx/lab_cremoso.cpp
    app/vfx/lab_valeria.cpp
    app/vfx/lab_vivaldi.cpp
    app/vfx/lab_espinha.cpp
    app/vfx/lab_marcov.cpp
    app/menu_screens.cpp
    app/menu_draw.cpp
    app/preview_video.cpp
    app/ring_renderer.cpp
    app/sprite_atlas.cpp
    app/ui_draw.cpp
    tools/snapshot/preview_capture.cpp
)
target_include_directories(korama_snapshot PRIVATE app)

# Ferramenta de desenvolvimento: grava todos os sons sintetizados em WAV e mede pico e duração.
add_executable(korama_sound_dump EXCLUDE_FROM_ALL tools/sound_dump/sound_dump.cpp app/audio_system.cpp)
target_include_directories(korama_sound_dump PRIVATE app)
target_link_libraries(korama_sound_dump PRIVATE korama_core SFML::Audio SFML::System)
if (MINGW)
    target_link_options(korama_sound_dump PRIVATE -static)
endif()

# Player interativo de SFX via terminal
add_executable(korama_sound_player EXCLUDE_FROM_ALL tools/sound_player/sound_player.cpp app/audio_system.cpp)
target_include_directories(korama_sound_player PRIVATE app)
target_link_libraries(korama_sound_player PRIVATE korama_core SFML::Audio SFML::System)
if (MINGW)
    target_link_options(korama_sound_player PRIVATE -static)
endif()
target_link_libraries(korama_snapshot PRIVATE korama_core SFML::Graphics SFML::Window SFML::System)
if (MINGW)
    target_link_options(korama_snapshot PRIVATE -static)
endif()

# Visualizador 3D interativo das arenas com câmera livre / noclip (tools/arena_viewer).
add_executable(korama_arena_viewer
    tools/arena_viewer/arena_viewer.cpp
    app/canvas2d.cpp
    app/ui_fonts.cpp
    app/celebration.cpp
    app/fight_hud.cpp
    app/fighter_pose.cpp
    app/jack_rig.cpp
    app/fighter_rig.cpp
    app/live_rig.cpp
    app/live_fighter.cpp
    app/vfx/lab_registry.cpp
    app/vfx/lab_jack.cpp
    app/vfx/lab_frog.cpp
    app/vfx/lab_joana.cpp
    app/vfx/lab_cremoso.cpp
    app/vfx/lab_valeria.cpp
    app/vfx/lab_vivaldi.cpp
    app/vfx/lab_espinha.cpp
    app/vfx/lab_marcov.cpp
    app/menu_screens.cpp
    app/menu_draw.cpp
    app/preview_video.cpp
    app/ring_renderer.cpp
    app/sprite_atlas.cpp
    app/ui_draw.cpp
)
target_include_directories(korama_arena_viewer PRIVATE app)
target_link_libraries(korama_arena_viewer PRIVATE korama_core SFML::Graphics SFML::Window SFML::System SFML::Audio)
if (MINGW)
    target_link_options(korama_arena_viewer PRIVATE -static)
endif()

if (WIN32)
    # Release abre sem console; Debug mantém o console para logs.
    set_target_properties(korama PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
    target_link_libraries(korama PRIVATE SFML::Main)
    if (MINGW)
        # Executável autossuficiente: sem DLLs do runtime do MinGW/libc++.
        target_link_options(korama PRIVATE -static)
    endif()
endif()
