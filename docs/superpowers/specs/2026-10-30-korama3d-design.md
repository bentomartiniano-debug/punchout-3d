# K.O.-RAMA 3D: jogo isolado em C++ nativo

## Objetivo

Testar se o K.O.-RAMA fica bom em 3D, jogando de verdade, com a mesma lógica do jogo atual e o visual dos bonecos "vinil". É um executável separado; o jogo 2D não muda.

## Decisões

- **Nativo em C++.** Sem navegador nem WebAssembly no jogo. O navegador só aparece na ferramenta de exportação dos modelos.
- **Mesma lógica.** O `korama3d` usa a biblioteca `korama_core` sem alterá-la. Toda regra, IA, dano e pose de golpe continua vindo dela.
- **Modelos exportados.** Os bonecos não são remodelados: saem dos arquivos do vinil (`tools3d/viewer/vinil/`) para `.glb`.
- **Primeira versão completa:** escolha de 2 entre os 9 lutadores, luta contra a IA, 3 rounds, quedas e contagem, especiais e HUD simples. Sem torneio, skins nem menus elaborados.

## Arquitetura

| Unidade | Faz | Depende de |
|---|---|---|
| `tools3d/export/` (exportador) | Roda um boneco do vinil no navegador e grava `assets3d/game/<lutador>.glb` com as peças nomeadas, a hierarquia, as cores e as texturas | Three.js, arquivos do vinil |
| `app3d/gl/` | Janela OpenGL 3.3 pela SFML, shaders, malhas, texturas e câmera | SFML 3.1, glad, glm |
| `app3d/model/` | Carrega o `.glb` numa árvore de peças | cgltf |
| `app3d/rig/` | Converte o estado do `Fighter` em transformações das peças e dobra as pernas | `korama_core`, `model` |
| `app3d/render/` | Desenha o ringue e os lutadores com toon e contorno por casca invertida | `gl`, `rig` |
| `app3d/game/` | Laço de 60 Hz, teclado, HUD e fluxo da partida | todos acima |

Alvo novo no `engine-cpp/CMakeLists.txt`: `korama3d`. O alvo `korama` e os testes atuais não mudam. glad, glm e cgltf entram pelo mesmo `FetchContent` já usado para a SFML.

## Fluxo de dados

1. O teclado vira a máscara `InputFlags`.
2. `GameEngine::update(mask, dt)` avança a luta.
3. `rig` lê de cada `Fighter`: `worldPos`, `facing`, `offsetX/Y/Z`, `bodySkew`, `gloveL`, `gloveR`, `state`, `hitKind`.
4. Isso move as peças: tronco, pescoço, quadril, luvas e pernas.
5. `render` desenha; o HUD lê vida, estamina, round, tempo e contagem do juiz do motor.

As luvas seguem as coordenadas do motor, então os golpes usam as poses já existentes em `fighter.cpp` e `skill_poses.cpp`, e não as poses de teste do viewer.

## Limites conhecidos

- O `.glb` não leva os shaders próprios de cada boneco (`onBeforeCompile`: padrões da pele, papo, magma). O exportador grava em textura o que for estático; o que é animado é refeito em GLSL.
- Molas de cabelo e rabo são escritas de novo em C++, por personagem, na etapa 2.
- Os efeitos dos especiais (`fxFrame` de cada arquivo do vinil) são portados à mão na etapa 3.
- A Fase 3 do Batalhator exporta o Nerd do commit, não as edições reprovadas que seguem no working copy.

## Etapas

1. **Teste de "está bacana?"**: Valéria (`charId` 4) contra Valéria, no ringue simples, jogável, com socos, bloqueio, esquiva, queda, contagem, 3 rounds, resultado e HUD. Rosto muda com a expressão. Sem especiais, som nem arena. Ao final, o usuário decide se continua.
2. **Elenco**: os outros 8 bonecos com as partes móveis deles e a tela de escolha.
3. **Especiais e formas**: porte dos efeitos, um personagem por vez.
4. **Acabamento**: sons, arenas e impacto.

## Erros

- `.glb` ausente ou inválido, ou shader que não compila: o jogo fecha mostrando o arquivo e o motivo.
- Peça obrigatória faltando no `.glb`: mesma saída, citando o nome da peça.

## Testes

- `korama_tests` continua passando, comprovando que a lógica não mudou.
- Exportador: verificação automática de que cada `.glb` tem todas as peças obrigatórias.
- `rig`: teste sem janela de que um `Fighter` em posições conhecidas gera as transformações esperadas.
- Visual: um modo do `korama3d` grava PNGs de quadros fixos. Conforme a preferência do usuário, a conferência não injeta teclas na máquina dele.

## Critério de pronto da etapa 1

O usuário joga uma luta inteira de Valéria contra Valéria no `korama3d.exe`, do gongo ao resultado, a 60 FPS nesta máquina, e consegue dizer se o 3D vale a continuação.
