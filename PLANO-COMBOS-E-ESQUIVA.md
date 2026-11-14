# Plano: especiais por combo, golpes avisados e esquiva com peso

Objetivo: nenhum golpe forte sai do nada. Quem vai lançar um especial precisa montar um combo, e isso avisa o outro lado. Quem esquiva consegue fugir de verdade e ganha algo em troca. Vale igual para o jogador e para a IA.

Hoje o jogo funciona assim:
- **Especiais:** saem por comandos curtos de setas mais soco (`SkillCommand`, em `include/skills.hpp`), a qualquer momento.
- **IA:** lança especiais com um tempo de recarga sorteado, de 5 a 8 s (`src/ai.cpp:231`).
- **Dano dos especiais:** é checado no quadro de impacto da animação (`updateSkillMove`, em `src/engine.cpp`), de 10 a 15 quadros depois do lançamento. Antes da etapa 1 não havia aviso nenhum antes do lançamento.
- **Esquiva:** é uma pose com imunidade. Os lutadores ficam sempre de frente um para o outro (`placeFighters`, `src/engine.cpp:2221`), e quem gira é o ringue.

---

## Etapa 1: especiais do oponente com aviso (FEITA, sem commit)

O que a medição mostrou: o dano já era checado no impacto, mas o impacto vinha 10 a 15 quadros (0,17 a 0,25 s) depois do lançamento, sem nada antes. Um soco pesado comum avisa por 36 a 48 quadros. O que faltava era o aviso, não mudar a checagem.

O que foi feito:
- **Aviso de 30 quadros antes do especial.** Quando a IA decide lançar um especial que ataca o jogador, o oponente entra no mesmo estado de aviso dos socos pesados (`FIGHTER_TELEGRAPH`), com o especial na fila (`Fighter::queuedSkill`) e uma pose própria (luvas recuadas e erguidas, agachado, tremendo). Só depois lança. Do aviso ao impacto passam cerca de 42 quadros (0,7 s).
- **Sem aviso (saem direto):** curas (`HEAL`), modos (`Mode`), escudos (`SHIELD`) e o enxame (`SWARM`).
- **Sinais para o jogador:** aro vermelho no chão, nos pés do jogador, com o miolo enchendo até a hora do golpe; um "!" vermelho sobre o oponente; e um tique de carga subindo no som. A marca e o "!" também aparecem no replay.
- **Soco durante o aviso cancela o especial** e conta como contra-golpe (a regra de contra-golpe do estado de aviso já existia).
- **Golpe-assinatura** (32% dos socos dos oponentes sem padrão de luta): saía sem aviso; agora arma por 36 quadros como um soco pesado comum, sem a marca no chão.

Conferido com lutas de teste (King Frog contra Valéria):
- sem esquivar, o especial acerta;
- esquivando cedo demais (começo do aviso), ainda acerta;
- esquivando do meio do aviso até perto do impacto, erra;
- um soco no começo do aviso cancela o especial e tira 11 de vida do oponente.

Não conferido: o golpe-assinatura em jogo (os oponentes testados usam padrões de luta e não passam por esse caminho) e o som do tique.

Fica para depois:
- Os especiais do **jogador** continuam saindo na hora. Eles mudam na etapa 3, com o combo.
- A marca no chão é a mesma para todos os especiais. Marcas próprias (linha para o raio, área para o gêiser) podem vir depois, se fizer falta.

**Arquivos:** `src/ai.cpp`, `src/engine.cpp`, `include/fighter.hpp`, `app3d/game/main.cpp`.

---

## Etapa 2: soco reto, e o oponente não gira junto com a esquiva (FEITA, versão barata, sem commit)

Como era: a esquiva lateral gira o ringue 90° (`ringAngle`), e os dois lutadores e a câmera giravam juntos. O oponente ficava sempre de frente para o jogador, mesmo no meio do soco.

O que foi feito (só no desenho, em `app3d/game/main.cpp`, rotina `tickSmooth`):
- Enquanto o oponente está no soco ou no especial, ele fica no lugar e na direção em que começou. O jogador e a câmera continuam girando com a esquiva, então o golpe passa reto e o oponente aparece de lado.
- Quando o golpe termina, ele gira e volta a ficar de frente (cerca de 15 quadros).
- Durante o aviso, antes do soco, ele ainda acompanha o jogador.
- Cada lutador olha para onde o outro está de fato: o jogador vira para o oponente que ficou de lado, então o contra-golpe aponta para ele.
- As regras de acerto não mudaram (esquivou = não toma).

Conferido com King Frog contra Valéria, esquivando logo depois do Soco Magmático: de cima, ela fica parada socando para onde ele estava e só depois se reposiciona; na câmera do jogo, ela aparece de lado durante a esquiva e volta sem saltos.

Fica para depois:
- **O caso inverso** (o jogador soca e o oponente esquiva) continua como era. Para o soco do jogador passar reto, a câmera teria de ficar parada junto com ele.
- **Versão completa** (posição lateral real, com acerto decidido pela posição): só se a barata não bastar jogando.

---

## Etapa 3: especiais em três níveis de entrada (FEITA, sem commit)

Os especiais deixaram de sair a qualquer hora. Cada um tem uma forma de entrada, conforme o risco que oferece ao rival:

| Nível | Quem entra | Como lança |
|---|---|---|
| **Gatilho rápido** | Curas (`HEAL`), modos (`Mode`) e escudos (`SHIELD`, o Favo Protetor) | O comando simples de sempre (`Z + X`, `← ← + Z`, `↓ ↓ + Z`), a qualquer momento |
| **Fluxo** | Debuffs e dano passivo: `SLOW`, Explosão de Pólen, Réquiem de Outono (`DRAIN`), Infestação de Abelhas e Pele Venenosa | Jab e direto (`Z`, `X`) e então o finalizador; ou só o finalizador logo depois de uma esquiva bem feita (1 s) |
| **Combo completo** | Todos os outros: golpes fortes e os do chefe | Jab, direto e gancho (`Z`, `X`, seta para o lado + soco) e então o finalizador |

**Finalizador:** uma seta segurada mais um soco, tirado da última tecla do comando antigo de cada especial:

| Comando antigo | Finalizador |
|---|---|
| `← →` + soco (Língua, Infestação) | `→` + `Z` ou `X` |
| `→ →` + `X` | `→` + `X` |
| `← →` + `Z` (Arrastão) | `→` + `Z` |
| `↓ ↑` + soco | `↑` + `Z` |
| `↑ ↑` + `X` | `↑` + `X` |
| `↓ ↓` + `Z` | `↓` + `Z` |
| `Z + X` (Pele Venenosa) | `Z + X` |

**Regras do combo do jogador:**
- Não há ordem fixa: cada soco diferente dos que já estão no combo carrega um espaço. O mesmo golpe com a outra mão já conta como diferente (jab de esquerda e jab de direita são dois). Os socos valem mesmo sem acertar e mesmo se o rival defender.
- Repetir um soco que já está no combo não carrega nem zera; só mantém o combo quente.
- O combo esfria 40 quadros depois do fim do último soco.
- Levar um golpe zera o combo.
- O finalizador tem prioridade sobre o soco: com o combo pronto, a seta + soco do especial lança o especial em vez de dar gancho ou uppercut.
- O oponente não tem aparo, então a regra "aparo quebra o combo" vale só para o combo dele.

**IA:**
- Todo especial que não é de gatilho rápido passa pelo combo de entrada: jab (aviso de 22 quadros), direto (16) e, nos fortes, gancho (20). Depois vem o aviso de 30 quadros da etapa 1 (quando o especial pede) e o especial.
- Do primeiro jab ao especial passam cerca de 3,8 s (medido contra a Valéria).
- Enquanto monta o combo, o oponente não para para defender.
- Apanhar ou levar um aparo esvazia o combo, e o especial não sai.
- Os golpes-assinatura continuam sendo socos comuns e não entram nessa regra.

**Interface:**
- Bolinhas sobre a cabeça de quem está montando o combo: vermelhas no oponente (vem especial), amarelas no jogador, azuis quando a esquiva abriu o fluxo.
- No painel "Suas habilidades" e nas telas de build, o rótulo do especial mostra as bolinhas do combo (duas ou três) e o finalizador.

**Conferido:**
- Jogador (King Frog contra o espantalho): combo completo lança o Golpe de Língua; jab e direto e `Z + X` lançam a Pele Venenosa; o comando antigo sozinho, o `Z + X` sem combo e o combo incompleto não lançam; o combo esfria se demorar.
- Contra a Valéria: esquivar o jab dela abre o fluxo (`Z + X` lança a Pele Venenosa sem combo); o aparo no jab zera o combo dela e o especial não sai.
- IA: jab, direto, gancho e então o especial, com as bolinhas indo de 1/3 a 3/3.
- Testes automáticos (`korama_tests`): 26 grupos falham, os mesmos 26 que já falhavam no último commit (valores de dano antigos). Três testes que lançavam especial pelo comando antigo foram atualizados para o combo.

**Não conferido:** as telas de build e de seleção (os rótulos usam a mesma rotina do HUD, mas não abri essas telas) e o chefe.

**Ponto de equilíbrio para avaliar jogando:** os socos do combo do oponente causam dano de verdade. Um jogador parado, que antes levava 14 de um especial, agora leva 50 na sequência inteira (8 + 14 + 14 + 14). Em troca, o especial demora 3,8 s para sair e pode ser defendido, esquivado ou quebrado. Se ficar pesado, dá para reduzir o dano dos socos de entrada ou espaçar os especiais.

**Arquivos:** `include/skills.hpp`, `src/skills.cpp`, `include/engine.hpp`, `src/engine.cpp`, `include/ai.hpp`, `src/ai.cpp`, `include/fighter.hpp`, `app/menu_draw.cpp`, `app/fight_hud.cpp`, `app3d/game/main.cpp`, `tests/engine_tests.cpp`.

---

## Ajuste de ritmo: especiais 1,7× mais longos (FEITO, sem commit)

Todo especial (menos os do Batalhator) dura 1,7 vezes o tempo da ficha (`kSkillStretch` em `include/fighter.hpp`): antes iam de 0,4 a 0,7 s, agora de 0,7 a 1,25 s. O acerto continua na mesma fração do golpe, então a preparação também ficou mais longa (mais tempo para esquivar). Os tempos escritos em quadros no motor e nos efeitos do app3d continuam na escala da ficha, lidos por `skillElapsed`.

---

## IA: defesa, esquiva e contra-ataque (FEITO, sem commit)

Antes a defesa era sorteada uma vez por "sequência" de socos e só com o oponente parado, então martelar passava direto (96% de acertos limpos). Agora:

- **Um sorteio por soco**, mesmo emendado no anterior.
- **Tempo de reação:** a guarda sobe de 2 a 5 quadros depois de ler o soco; a esquiva sai na hora de o golpe cair no meio dela.
- **Leitura do golpe:** soco pesado ou carregado pede mais esquiva; soco leve, mais guarda. Depois de uma esquiva, a próxima defesa prefere a guarda (duas esquivas seguidas giram o ringue demais).
- **Apanhando em sequência:** depois de dois golpes limpos seguidos, o oponente fecha a guarda quase sempre (95%).
- **Guarda mantida:** se o jogador emenda outro soco, a guarda continua fechada (85%).
- **Larga o próprio golpe:** no começo do aviso de um soco comum, o oponente pode desistir e defender (50%). Na metade final do aviso ele está comprometido, e o soco do jogador entra como contra-ataque. Especial armado e combo de entrada vão até o fim.
- **Especiais do jogador:** o oponente pode esquivar perto do acerto (50%, só parado). A guarda não segura especial.
- **Contra-ataque:** depois de segurar um soco, responde com jab ou direto (45%, mais 25% se o jogador insiste em bater na guarda); depois de esquivar, com gancho (55%). O aviso é curto (10 e 14 quadros).

Medição (`korama_tests --ai`, 90 s contra cada um dos 8 rivais, vida e fôlego repostos):

| Situação | Antes | Agora |
|---|---|---|
| Jab isolado: acertos limpos | 71% | 44% (metade deles no aviso do rival) |
| Jab martelado: acertos limpos | 96% | 27% |
| Especial do jogador: esquivado | quase nunca | 20% |

Os ajustes ficam em `include/ai.hpp`: `defenseChance`, `tellCancelChance`, `skillDodgeChance`, `counterChance`.

---

## Fôlego do combo e golpe em linha reta (FEITO, sem commit)

**Fôlego.** Com o combo de entrada, um especial forte custava 95 de 100 (jab 18 + direto 22 + gancho 30 + especial 25) e um de fluxo 65; antes do combo o especial custava 25. Agora:

- O soco que carrega um espaço do combo (diferente dos que já estão nele) custa 40% menos (`kChainPunchCost` em `src/engine.cpp`). Repetir o mesmo soco paga o preço cheio.
- O especial lançado pelo combo ou pelo fluxo não cobra fôlego de novo nem falha por falta de fôlego. Os de gatilho rápido (curas, modos, escudos) continuam custando 20 a 25.
- Resultado: especial forte 42 no total (jab 10,8 + direto 13,2 + gancho 18), especial de fluxo 24.

**Golpe em linha reta.** Quando o oponente esquiva, o jogador no meio de um soco ou especial não gira mais com o ringue: termina o golpe para onde estava mirando, e a câmera fica com ele. Depois que o golpe bate, ele e a câmera se viram para o rival. É o mesmo tratamento que o oponente já tinha (etapa 2).

---

## Nocaute final: arremesso e estrelinha (FEITO, sem commit)

Vale para a queda que encerra a luta na hora (a terceira queda, T.K.O.). Quedas anteriores e o nocaute por contagem continuam como eram.

- **Golpe comum:** quem perdeu é arremessado. Vai pelo ar deitado para trás, com os pés para a frente e para cima, e bate nas cordas (chega mais rápido: 4 unidades por quadro em vez de 2,5).
- **Golpe especial ou assinatura:** sai girando para o céu com rastro de fumaça, encolhe com a distância e vira uma estrelinha de quatro pontas que pisca e some (com um tilintar). A câmera da luta acompanha o voo olhando para cima e volta para o ringue; a estrela é desenhada por cima do cenário, então aparece mesmo em arena com teto. Ele não bate nas cordas nem fica pendurado nelas.
- O motor marca `koBySkill` (quem ficou de pé estava num especial na hora da queda). O efeito é o `KoFx` em `app3d/render/char_fx.hpp`.
- O replay mostra o arremesso e a subida; a estrela pode ficar fora do quadro, porque as câmeras do replay são fixas.

Conferência por imagem: `KORAMA3D_NODEF` (oponente não defende), `KORAMA3D_NOHUD` (sem a HUD), `KORAMA3D_LAST_KD_P` (a próxima queda do jogador encerra a luta).

---

## Etapa 4: recompensa da esquiva

- Logo depois de uma esquiva que evita um golpe, abre uma janela curta (cerca de 20 quadros) de contra-ataque. O primeiro soco nessa janela causa mais dano e não pode ser defendido, porque você está de lado.
- Esquivar dentro dessa janela também conta como abertura para os especiais do nível "fluxo", como na proposta.
- Hoje só o Cremoso e o Jack (com Apicultor) ganham algo ao esquivar (`src/engine.cpp:1930`). Esses bônus continuam por cima da regra geral.

**Arquivos:** `src/engine.cpp` (janela de contra-ataque) e o HUD (sinal da janela aberta).

---

## Decisões tomadas por padrão (mude se quiser)

1. **Pele Venenosa** ficou no nível "fluxo"; os outros modos ficaram no gatilho rápido.
2. **Estocadas** (`STING`) ficaram no combo completo.
3. **O chefe** segue a mesma regra dos outros.
4. **Janela entre socos do combo:** 40 quadros.
5. **Etapa 2:** feita pela versão barata (trava visual).

## Ordem e tamanho

1. **Etapa 1:** feita.
2. **Etapa 2 (versão barata):** feita.
3. **Etapa 3:** feita.
4. **Etapa 4:** pequena.

Cada etapa dá para jogar e conferir sozinha, e cada uma vira um commit próprio.
