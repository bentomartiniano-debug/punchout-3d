# DOSSIÊ DE COMBATE, HABILIDADES E BALANCEAMENTO: KO-RAMA (PUNCH-OUT 3D)

> **Documento de Referência Técnica & Game Design**  
> Criado para consulta e trabalho colaborativo em dispositivos móveis com LLMs / IAs de design de jogos.

---

## 1. VISÃO GERAL DO PROJETO & FILOSOFIA DE JOGO

**KO-RAMA** é um jogo de boxe de ação arcade em perspectiva de terceira pessoa (câmera nas costas do lutador, olhando de frente para o adversário no ringue), combinando o timing rigoroso e leitura de padrões de **Punch-Out!!** com a profundidade tática e personalização de estilos de **jogos de luta contemporâneos (Street Fighter, Guilty Gear)** através de um **Sistema de Builds (Loadouts de Habilidades)**.

### Pilares Fundamentais:
1. **Timing & Reação**: Reconhecer avisos de golpe inimigo (ponto de exclamação / flashes de telegrafia), esquivar para o lado correto e punir nas brechas (*punish windows*).
2. **Gerenciamento de Fôlego (Estamina)**: Atacar, esquivar e apanhar consomem estamina. Ficar a zero estamina entra em estado de **Exaustão (*Sluggish*)**, deixando o lutador vulnerável e com socos fracos.
3. **Builds Estratégicas**: Cada lutador possui um catálogo de **9 habilidades** (6 ativas/comandos e 3 passivas). O jogador equipa um **Loadout de 3 habilidades** antes da luta, definindo seu arquétipo (burst ofensivo, controle de ringue por status, contra-golpe defensivo, sustain de cura, etc.).
4. **Espacialidade de Ringue**: As cordas elásticas do ringue deformam sob impacto. Golpes que empurram podem prender o oponente nas cordas (*ropes*), ampliando dano e limitando mobilidade.

---

## 2. NÚMEROS E REGRAS DO MOTOR DE COMBATE

### Atributos Base
- **Vida (HP)**: 100 HP para todos os lutadores jogáveis (Batalhator / Boss final possui 200 HP distribuídos em 3 fases).
- **Estamina Máxima**: 100 pontos padrão (alterável por passivas).
- **Regeneração de Estamina**: Ocorre passivamente quando o lutador não está atacando nem em delay de recuperação (após delay de 30-35 frames).
- **Rounds**: Rounds de até 3 minutos.
- **Sistema de Quedas (Knockdowns)**:
  - 3 Knockdowns no mesmo round = **TKO (Technical Knockout)** imediato.
  - Knockdown tradicional = Contagem do árbitro até 10. O jogador levanta martelando os botões de soco (`Z`/`X`).
  - Se a luta atingir o fim dos rounds = Decisão por pontos (quem causou mais dano e quedas).

---

### Socos Básicos (Arsenal Padrão)

Todos os lutadores têm acesso ao mesmo conjunto básico de 5 socos (com modificadores individuais de tamanho/alcance):

| Golpe | Comando | Dano Base | Custo Stamina | Duração (Frames) | Alcance (Unidades) | Hitstun / Vantagem | Efeito Especial / Propriedade |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Jab** | `Z` (neutro) | **6 HP** | 18 | 28f | 215u | 16f | Golpe rápido para iniciar combos e quebrar ritmo. |
| **Cross (Direto)** | `X` (neutro) | **8 HP** | 22 | 32f | 200u | 20f | Golpe mais pesado da mão de trás. |
| **Hook (Cruzado)** | `←` ou `→` + Soco | **10 HP** | 30 | 38f | 205u | 24f | Arco lateral potente. |
| **Body Blow (Rim)** | `↓` + Soco | **10 HP** | 28 | 34f | 200u | 28f | Drena **20 de estamina** do adversário (*se não estiver em exaustão*). |
| **Uppercut** | `↑` + Soco | **14 HP** | 40 | 48f | 185u | 38f | Golpe vertical de alto impacto. Alcance curto, teto de 19 HP em Counter. |

#### Multiplicadores Universais & Ritmo de Punch-Out!!:
- **Soco Carregado (Charge)**: Segurar o botão de soco carrega o golpe (até 45 frames). Multiplica o dano proporcionalmente à carga.
- **Contra-Ataque no Aviso (Counter Hit)**: Acertar o oponente exatamente durante a telegrafia ou início do golpe dele causa **1,6× de dano** (Uppercut tem teto balanceado de **1,35× / 19 HP** para não estourar a barra).
- **Defesa no Neutro & Whiff Punish**:
  - A IA mantém guarda e defende **80% dos socos secos desferidos no neutro** (bloqueio ou esquiva).
  - Esquivar com sucesso de um golpe do oponente abre imediatamente uma **janela de BRECHA (Whiff Punish) de 20 a 28 frames**, onde a guarda da IA é desativada e socos entram limpos com bônus de `PUNISH!`.
- **Golpes do Oponente (IA)**:
  - Golpes pesados (cross, gancho, uppercut) causam **14 HP** base (cortam 8 de estamina máxima).
  - Golpes leves (jab) causam **8 HP** base. Acabou o pico de 27-35 HP em socos comuns.

---

### Defesa, Esquiva e Bloqueio

1. **Bloqueio Comum (`Espaço` ou segurar `↓`)**:
   - Socos normais causam apenas **2 HP de dano chip**.
   - **Habilidades Especiais**: Sofrem **55% de mitigação**, causando apenas **45% do dano base** (com faíscas de guarda, áudio de bloqueio e texto `BLOQUEADO (-X)`), e anulam atordoamentos e quedas diretas.
   - **Exceções à Guarda**:
     - **Quebra de Guarda (`BREAK`)**: Causa 100% de dano total, destrói a guarda e aplica 25 frames de atordoamento (ex: *Tesoura* da Joana, *La Garra* da Espinha).
     - **Fura-Guarda (`STING`)**: Golpes perfurantes que ignoram totalmente o bloqueio causando 100% de dano limpo (ex: *Agulhada Rainha* do Jack, *Estocada de Lava* da Valéria).
2. **Parry (Bloqueio Perfeito / Aparada)**:
   - **Ativação**: Apertar `Espaço` (ou `↓`) com timing preciso (janela ativa de **10 frames = 166ms** ao erguer a guarda).
   - **Abrangência**: Funciona tanto contra **socos normais** quanto contra **habilidades especiais** (exceto golpes com tag `BREAK`).
   - **Recompensas & Game Feel**:
     - **Anula 100% do dano** (0 dano sofrido).
     - **Hitstop Dramático (Freeze Frame)**: Congela o combate por **12 a 14 frames**.
     - **VFX Shockwave Ripple**: Dispara efeito de onda de distorção de impacto e aberração cromática no shader GLSL.
     - **Estamina**: Restaura **+20 a +25 de estamina** na hora e cancela qualquer exaustão ativa.
     - **Desestabilização do Oponente**: O agressor entra em stagger com **30 a 35 frames de brecha aberta** (`PARRY!` flutuante em ciano), abrindo punição garantida.
3. **Esquivas Dedicadas (`C`)**:
   - **`C` neutro (sem direcionais)**: **Abaixar (*Duck*)**. Passa por baixo de ganchos altos e socos na cabeça. Duração: 22f.
   - **`C` + Trás (`S` / `↓`)**: **Saltar para trás (*Back Dodge*)**. Ganha distância imediata e recuo seguro. Duração: 24f. Custo: 14 stamina.
   - **`C` + Esquerda (`A` / `←`)**: **Esquiva Lateral Esquerda**. O lutador inclina para a esquerda e esquiva lateralmente. Duração: 22f. Custo: 14 stamina.
   - **`C` + Direita (`D` / `→`)**: **Esquiva Lateral Direita**. O lutador inclina para a direita e esquiva lateralmente. Duração: 22f. Custo: 14 stamina.
   - **Cancelamento de Guarda**: É possível esquivar ou abaixar instantaneamente no mesmo frame enquanto segura bloqueio.
   - **Buffer de Esquiva**: Apertar esquiva durante o final de um soco agenda a evasão para o 1º frame livre.

---

### Mecânica Anti-Spam e Exaustão (*Sluggish*)

Para impedir que jogadores vençam apenas martelando socos sem pensar:
1. **Quando a Estamina zera**: O lutador fica em estado **Exausto (*Sluggish*)** até recuperar 50 de estamina.
2. **Penalidades de Exaustão**:
   - Socos saem **85% mais lentos** e causam apenas **30% do dano**.
   - O *Body Blow* (soco no rim) **não drena mais estamina** do inimigo enquanto exausto.
   - Bater na guarda do inimigo em exaustão causa **0 de dano**, deflete o atacante para trás, abre uma janela de punição de **25 frames de vulnerabilidade** e congela a regeneração de estamina por 75 frames.
   - Errar um soco exausto no ar (*whiff*) abre **28 frames de vulnerabilidade** para punição total.

---

## 3. STATUS EFFECTS (DEBUFFS E BUFFS)

| Status | Efeito no Jogo | Como é Aplicado | Duração / DPS |
| :--- | :--- | :--- | :--- |
| **Queimadura (*Burn*)** | Dano contínuo que drena vida a cada frame. | Habilidades de fogo/calda quente (Valéria, Cremoso). | 3.0 a 4.0 HP/s por 5s (total 15 a 20 HP). |
| **Veneno (*Poison*)** | Dano contínuo de toxina. Se aplicado por King Frog, cura o sapo simultaneamente em 1.5 HP/s. | Pele Venenosa do King Frog. | 3.5 HP/s por 6s (total 21 HP). |
| **Sangramento (*Bleed*)** | Dano contínuo por laceração. | Impacto Coralino da Joana, Serras do Batalhator. | 3.0 HP/s por 5s (total 15 HP). |
| **Lentidão (*Slow*)** | Velocidade de socos e locomoção reduzida em 35%. | Melaço do Jack, Língua do Frog, Requiem do Vivaldi. | 4 a 6 segundos. |
| **Cegueira (*Blind*)** | O lutador afetado tem 40% de chance de errar qualquer ataque desferido (aparece "ERROU!"). | Nuvem de Pólen (Jack), Cinzas (Valéria). | 4 a 5 segundos. |
| **Lama (*Mud*)** | O lutador tem os pés presos à lona. **Não pode esquivar nem saltar** (apenas bloquear). | Cuspidela de Lama (Frog), Poça do Cremoso. | 4.5 a 5 segundos. |
| **Atordoamento (*Stun*)** | O lutador fica imóvel e indefeso por frames, aberto a combos limpos. | Erupções, Tesoura da Joana, Inverno Allegro. | 25 a 96 frames (0.4s a 1.6s). |
| **Fura-Guarda (*Sting*)** | Golpe perfurante que atravessa 100% da defesa do oponente. | Estocadas da Queen, Lava, Casquinha, Batuta. | Instantâneo (16 a 17 HP limpos). |
| **Quebra de Guarda (*Break*)** | Destrói a postura do oponente se ele estiver defendendo, causando hitstun pesado. | Tesoura (Joana), Tapa de Mandacaru (Espinha). | Instantâneo (21 a 25 HP + 25f stun). |

---

## 4. CATÁLOGO COMPLETO DE LUTADORES E HABILIDADES

Cada personagem possui 9 opções de habilidades. No jogo, ele equipa **3 habilidades** (Loadout).

---

### LUTADOR 1: JACK MÃOS DE COLMEIA
- **Arquétipo**: Apicultor / Boxeador Técnico de Controle e Desgaste (*Damage-over-Time* & Barreira de Mel).
- **Porte**: Médio (Raio 115u).

| ID | Nome | Tipo | Comando | Tag | Dano / Efeito | Descrição Técnica & Avaliação de Força |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `JackInfestation` | **Infestação de Abelhas** | Comando | `←→ + Z` | `SWARM` | 3.2 HP/s por 6s (19.2 HP) | Enxame de abelhas orbita o rival causando dano contínuo. Animação de cast longa (44f). |
| `JackMolasses` | **Modo Melaço** | Modo | `Z + X` | `HONEY` | -50% dano próprio, slow 6s | **FRACO / PROBLEMA**: Reduz o próprio dano pela metade por 6s em troca de dar slow no oponente. Auto-punitivo. |
| `JackHoneyWall` | **Favo Protetor** | Comando | `↓↓← + X` | `SHIELD` | -60% dano em 3 hits, +10 stamina, slow 2s | **MUITO FORTE (RECÉM-BUFFADO)**: Escudo absorve 3 golpes, anula perda de estamina, devolve +10 stamina e dá slow de 2s em quem bate. |
| `JackStingJab` | **Agulhada Rainha** | Comando | `↑↑→ + X` | `STING` | 16 HP limpos | Fura o bloqueio completamente. Excelente contra quem abusa de guarda. |
| `JackCombBurst` | **Explosão de Pólen** | Comando | `↓↑ + X` | `POLLEN` | **0 HP**, cegueira 40% por 4s | **MUITO FRACO**: Causa 0 de dano. Gastar uma brecha para só ter 40% de chance de miss do rival é uma aposta ruim. |
| `JackRoyalJelly` | **Geleia Real** | Comando | `←←← + X` | `HEAL` | Regenera 20 HP | Recarga de 8s. Boa sustentação. |
| `JackBeekeeper` | **Apicultor** | Passiva | Passiva | `PASSIVA` | Esquiva -15% stamina | **MUITO FRACO**: Economiza apenas 2 de estamina na esquiva (14 $\to$ 12). Quase imperceptível. |
| `JackHoneySkin` | **Pele de Mel** | Passiva | Passiva | `PASSIVA` | HP < 45%: -35% dano, +stamina | **FORTE**: Reduz dano recebido em 35% quando ferido e recupera estamina em impactos/bloqueios. |
| `JackQueenBlessing` | **Bênção da Rainha** | Passiva | Passiva | `PASSIVA` | KD cura +25 HP | Regenera 25 HP toda vez que derruba o adversário na lona. |

---

### LUTADOR 2: KING FROG
- **Arquétipo**: Sapo Nobre / Veneno Vampírico, Controle de Espaço e Saltos Evasivos.
- **Porte**: Pequeno / Rebaixado (Raio 138u).

| ID | Nome | Tipo | Comando | Tag | Dano / Efeito | Descrição Técnica & Avaliação de Força |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `FrogTongue` | **Golpe de Língua** | Comando | `←→ + Z` | `REACH` | 18 HP + empurrão + slow 35% por 5s | Alcance enorme de língua. Empurra contra as cordas e desacelera o oponente. Meta central. |
| `FrogPoisonSkin` | **Pele Venenosa** | Modo | `Z + X` | `VENOM` | 3.5 HP/s por 6s (21 HP) + cura 1.5 HP/s | **MUITO FORTE**: Drena vida do rival para o sapo (vampirismo de 9 HP enquanto envenenado). |
| `FrogRoyalLeap` | **Salto Real Esmagador** | Comando | `↓↑ + X` | `IMPACT` | 22 HP + atordoamento | Salta fora da tela (invulnerável no ar) e cai com soco duplo. Excelente contra golpes lentos. |
| `FrogMudSlime` | **Cuspidela de Lama** | Comando | `↓↓← + X` | `SLOW` | **0 HP**, poça impede esquiva por 5s | **FRACO**: Causa 0 de dano. O oponente simplesmente segura guarda e não toma pressão real de vida. |
| `FrogFlyCatcher` | **Captura de Moscas** | Comando | `←←← + X` | `HEAL` | Cura 22 HP + purifica debuffs | Recarga de 8s. Remove queimadura, sangramento, veneno e lentidão instantaneamente. |
| `FrogSwampSpout` | **Gêiser do Pântano** | Comando | `↑↑→ + X` | `BURST` | 12 HP + desestabilização firme no chão | Erupção de água dos pés ao peito: causa 12 HP e abre brecha de 35f de vulnerabilidade no chão (sem juggle aéreo). |
| `FrogJumper` | **Saltador Anfíbio** | Passiva | Passiva | `PASSIVA` | Esquiva custa 7 stamina (-50%) | Muito eficiente para mobilidade constante. |
| `FrogSlipperySkin` | **Pele Mucosa Deslizante** | Passiva | Passiva | `PASSIVA` | 30% chance de absorver 50% dano + 12 stamina | Deflexão passiva excelente contra sequências rápidas. |
| `FrogAmphibianRage` | **Fúria Anfíbia** | Passiva | Passiva | `PASSIVA` | Stamina < 30%: socos +25% rápidos e +4 dano | Efeito de virada excelente para sair da exaustão machucando. |

---

### LUTADOR 3: BARONESA JOANA ESTRELA
- **Arquétipo**: Caranguejo Monstruoso das Profundezas / Dano Bruto Devastador e Carapaça Blindada.
- **Porte**: Grande (*Big Char*, Raio 125u; ativa *Modo Monstruoso* gigante abaixo de 40 HP).

| ID | Nome | Tipo | Comando | Tag | Dano / Efeito | Descrição Técnica & Avaliação de Força |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `JoanaClawCrush` | **Tesoura Esmagadora** | Comando | `→→→ + X` | `BREAK` | **25 HP limpos**, quebra guarda + 25f stun | **DEVASTADORA**: Quebra bloqueios, dano altíssimo e abre o rival para punição imediata. |
| `JoanaHookPull` | **Arrastão do Harpão** | Comando | `←→← + X` | `REACH` | 10 HP + puxão para curta distância | **FRACO**: Comando longo para causar míseros 10 de dano. |
| `JoanaCrustaceanShell` | **Barreira de Cracas** | Modo | `Z + X` | `SHIELD` | 5s: -70% dano sofrido, devolve 35% ao agressor | Placa defensiva impenetrável que machuca o atacante. |
| `JoanaCoralSpout` | **Impacto Coralino** | Comando | `↓↑ + X` | `BURST` | 18 HP + sangramento 3 HP/s por 5s (total 33 HP) | Uppercut de corais com excelente dano combinado. |
| `JoanaAnchorSlam` | **Onda da Âncora** | Comando | `↓↓← + X` | `STUN` | **0 HP**, slow por 4s (cast de 34f) | **MUITO FRACO**: Mais de meio segundo travada para dar 0 dano enquanto a Tesoura no mesmo tempo dá 25 dano. |
| `JoanaOceanBounty` | **Bálsamo de Anêmona** | Comando | `←←← + X` | `HEAL` | Regenera 20 HP | Recarga de 8s. Sustentação básica. |
| `JoanaAsymmetricArms` | **Armamento Assimétrico** | Passiva | Passiva | `PASSIVA` | Garra +30% dano (ganchos/uppers), gancho +40% alcance | Passiva agressiva fortíssima no curto alcance. |
| `JoanaCalcifiedArmor` | **Carapaça Calcária** | Passiva | Passiva | `PASSIVA` | -20% em todo dano frontal | Redução passiva passiva sólida e consistente o combate inteiro. |
| `JoanaDeepSeaTide` | **Maré das Profundezas** | Passiva | Passiva | `PASSIVA` | HP < 40%: stamina regenera +50% | **MEDIANA / SITUACIONAL**: Só ativa à beira da morte, ofuscada pela Carapaça Calcária. |

---

### LUTADOR 4: O CREMOSO
- **Arquétipo**: Sorveteiro Gigante de Baunilha / Absorção Maleável, Queimaduras de Calda Quente e Retaliação.
- **Porte**: Grande (*Big Char*, Raio 125u).

| ID | Nome | Tipo | Comando | Tag | Dano / Efeito | Descrição Técnica & Avaliação de Força |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `CremosoSyrupBlast` | **Explosão de Calda Quente** | Comando | `→→→ + X` | `BURN` | 20 HP + queima 3 HP/s por 5s (total 35 HP) | Dano altíssimo com calda fervente. |
| `CremosoVanillaWhip` | **Tornado de Baunilha** | Comando | `↓↑ + X` | `BURST` | 18 HP (3 hits) + slow 4s | Espiral de socos giratórios. Bom controle. |
| `CremosoMeltMode` | **Modo Derretido** | Modo | `Z + X` | `ABSORB` | 6s: -50% dano, 20% do dano vira cura | Transforma porradas sofridas em regeneração de vida. |
| `CremosoConePunch` | **Estocada de Casquinha** | Comando | `↑↑→ + X` | `STING` | 16 HP limpos | Fura a defesa do rival com a ponta da casquinha. |
| `CremosoPuddleTrap` | **Poça de Calda Grudenta** | Comando | `↓↓← + X` | `SLOW` | **0 HP**, poça slow e impede esquiva por 4.5s | **FRACO**: 0 de dano. Dá brecha sem ameaçar a vida do rival. |
| `CremosoCherryTopping` | **Cereja do Bolo** | Comando | `←←← + X` | `HEAL` | Regenera 22 HP | Engole a cereja gigante do topo. Recarga de 8s. |
| `CremosoCreamCushion` | **Amortecedor de Creme** | Passiva | Passiva | `PASSIVA` | Jabs recebidos causam -25% dano | **MUITO FRACO**: Só protege contra jabs (6 dano vira 4.5). Inútil contra golpes pesados e especiais. |
| `CremosoFrozenStamina` | **Stamina Gelada** | Passiva | Passiva | `PASSIVA` | Stamina regenera +35%, máx não cai com pesados | Estamina fria que não se esgota facilmente. |
| `CremosoSweetRevenge` | **Vingança Doce** | Passiva | Passiva | `PASSIVA` | Esquiva perfeita cura 3 HP e prox soco bate +15% | Recompensa excelente para esquivas precisas. |

---

### LUTADORA 5: VALÉRIA VULCÃO
- **Arquétipo**: Boxeadora de Magma / Pressão Implacável de Fogo, Esquivas Sem Custo e Burst Explosivo.
- **Porte**: Médio (Raio 115u).

| ID | Nome | Tipo | Comando | Tag | Dano / Efeito | Descrição Técnica & Avaliação de Força |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `ValeriaMagmaPunch` | **Soco Magmático** | Comando | `→→→ + X` | `BURN` | **22 HP + queima 4 HP/s por 5s (total 42 HP)** | **DESTRUIDOR**: A habilidade de maior dano bruto do jogo. Derrete barras de vida inteiras. |
| `ValeriaVolcanicErupt` | **Erupção Solavanco** | Comando | `↓↑ + X` | `KNOCKDOWN` | 24 HP + 1.6s de atordoamento | Pilar vulcânico de baixo para cima. |
| `ValeriaHeatwaveGuard` | **Escudo de Calor** | Modo | `Z + X` | `SHIELD` | 5s: atacante erra 20%, toma 3 dano e perde 10 stamina | **FORTE (RECÉM-BUFFADO)**: Punição severa contra quem tenta pressionar na guarda. |
| `ValeriaLavaKickJab` | **Estocada de Lava** | Comando | `↑↑→ + X` | `STING` | 17 HP limpos | Fura o bloqueio com rocha vulcânica pontiaguda. |
| `ValeriaAshCloud` | **Cortina de Cinzas** | Comando | `↓↓← + X` | `SLOW` | **0 HP**, cegueira por 4.5s | **FRACO**: 0 de dano. Totalmente ofuscada pelo Soco Magmático e pela Erupção. |
| `ValeriaPhoenixEmber` | **Reagrupamento das Brasas** | Comando | `←←← + X` | `HEAL` | Regenera 20 HP | Recarga de 8s. Sustentação de calor. |
| `ValeriaEmberDance` | **Dança das Brasas** | Passiva | Passiva | `PASSIVA` | **Esquivas custam 0 stamina** e resetam delay | **MUITO FORTE**: Mobilidade infinita para abusar da esquiva sem cansar. |
| `ValeriaColdMagma` | **Modo Magma Frio** | Passiva | Passiva | `PASSIVA` | Cabelo de basalto (-50% dano) **ao levantar da 2ª queda** | **MUITO FRACO / PLACEBO**: Só ativa na segunda queda. Se você jogar bem e não cair duas vezes, tem utilidade ZERO. |
| `ValeriaVolcanicRage` | **Fúria Vulcânica** | Passiva | Passiva | `PASSIVA` | HP $\le$ 35: socos causam +40% de dano | Efeito de virada assustador em momentos de perigo. |

---

### LUTADOR 6: MAESTRA VIVALDI
- **Arquétipo**: Maestrina Espectral / Boxe Musical Rítmico, Aceleração de Golpes e Cadência de Combo.
- **Porte**: Médio (Raio 115u).

| ID | Nome | Tipo | Comando | Tag | Dano / Efeito | Descrição Técnica & Avaliação de Força |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `VivaldiCadenceStaccato` | **Cadência Staccato** | Comando | `→→→ + X` | `RHYTHM` | 20 HP total (5 socos rápidos) | Metralhadora de socos musicais. Excelente para subir combo. |
| `VivaldiWinterAllegro` | **Inverno Allegro** | Comando | `↓↑ + X` | `STUN` | 19 HP + congela (1.5s stun) | Uppercut de gelo com atordoamento prolongado. |
| `VivaldiFourSeasons` | **Modo Quatro Estações** | Modo | `Z + X` | `BOOST` | 6s: socos +35% rápidos e -30% custo stamina | Transforma a Vivaldi em uma máquina de socos velozes. |
| `VivaldiCrescendoJab` | **Estocada em Crescendo** | Comando | `↑↑→ + X` | `STING` | 16 HP limpos | Batuta ectoplásmica que fura a guarda. |
| `VivaldiAutumnRequiem` | **Réquiem de Outono** | Comando | `↓↓← + X` | `DRAIN` | **0 HP**, drena 30 stamina e slow 5s | Drena 30 de stamina e desacelera, mas peca pela falta de dano à vida. |
| `VivaldiSpringSonata` | **Sonata de Primavera** | Comando | `←←← + X` | `HEAL` | Regenera 20 HP | Recarga de 8s. |
| `VivaldiPerfectRhythm` | **Ritmo Perfeito** | Passiva | Passiva | `PASSIVA` | Acertos seguidos somam +8% dano cada (até +40%) | Recompensa brutal para quem joga no ritmo sem apanhar. |
| `VivaldiPhantomCloak` | **Manto Fantasma** | Passiva | Passiva | `PASSIVA` | 15% chance de golpe inimigo atravessar sem dano | **MEDIANO / INSTÁVEL**: 15% de chance de esquiva etérea é baixo demais para se confiar. |
| `VivaldiSymphonicResonance`| **Ressonância Sinfônica** | Passiva | Passiva | `PASSIVA` | Derrubar ou combo de 5 hits restaura 25 stamina | Mantém a máquina de socos sempre abastecida. |

---

### LUTADORA 7: LADY ESPINHA
- **Arquétipo**: Luchadora de Mandacaru / Quebra de Guarda, Punição Aérea e Espinhos de Retaliação.
- **Porte**: Médio (Raio 115u).

| ID | Nome | Tipo | Comando | Tag | Dano / Efeito | Descrição Técnica & Avaliação de Força |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `EspinhaNopalSlap` | **La Garra del Cacto** | Comando | `→→→ + X` | `BREAK` | **21 HP (2 hits)**, quebra guarda + 1.6s stun | **MUITO FORTE**: Quebra guarda brutal com tapa duplo de mandacaru. |
| `EspinhaLuchaFlyer` | **Gran Salto Luchador** | Comando | `↓↑ + X` | `IMPACT` | 23 HP + invulnerável no ar | Salto plástico de lucha libre que esmaga o oponente de cima. |
| `EspinhaThornVeil` | **Velo de las Espinas** | Modo | `Z + X` | `SHIELD` | 6s: -50% dano sofrido, espinhos refletem o dobro | Dobra a retaliação passiva para 30% do dano de volta. |
| `EspinhaNeedleJab` | **Estocada de Mandacaru** | Comando | `↑↑→ + X` | `STING` | 17 HP limpos | Fura o bloqueio com espinho afiado. |
| `EspinhaDesertWind` | **Viento del Desierto** | Comando | `↓↓← + X` | `SLOW` | **0 HP**, slow 5s + cegueira 2s | **FRACO**: Rodopio no chão que não causa dano. |
| `EspinhaFridaTears` | **Lágrimas de Frida** | Comando | `←←← + X` | `HEAL` | Regenera 20 HP | Flores de dália da resiliência. Recarga 8s. |
| `EspinhaThornRecoil` | **Espinas de Lucha** | Passiva | Passiva | `PASSIVA` | Quem ataca Espinha toma 15% do dano de volta | Pune atacantes agressivos continuamente. |
| `EspinhaRebelSpirit` | **Espíritu Rebelde** | Passiva | Passiva | `PASSIVA` | Contra-ataques causam +25% de dano | Bom bônus cumulativo ao multiplicador 1.6x de counter. |
| `EspinhaVivaCaatinga` | **Viva La Caatinga** | Passiva | Passiva | `PASSIVA` | Menor a vida, maior a regeneração de stamina e socos mais baratos | Permite viradas intensas quando com pouca vida. |

---

### LUTADOR 8: DR. MARCOV
- **Arquétipo**: Cosmonauta Soviético Pesado / Propulsão Cinética, Laser Orbital e Resistência de Titânio.
- **Porte**: Grande (*Big Char*, Raio 125u).

| ID | Nome | Tipo | Comando | Tag | Dano / Efeito | Descrição Técnica & Avaliação de Força |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `MarcovRocketThrust` | **Propulsão Foguete** | Comando | `→→→ + X` | `ROCKET` | 23 HP + empurrão de 60u nas cordas | Arrancada cinética com propulsor a jato. |
| `MarcovOrbitalLaser` | **Raio Orbital Sputnik** | Comando | `↓↑ + X` | `BURST` | 24 HP + 1.6s atordoamento | Raio disparado do satélite direto na cabeça do rival. |
| `MarcovPlasmaBarrier` | **Escudo Sputnik** | Modo | `Z + X` | `BARRIER` | 6s: -55% dano sofrido, cada golpe recarrega 15 stamina | Converte pancadas recebidas em combustível de estamina. |
| `MarcovKineticOverdrive`| **Impulso de Reação** | Comando | `↑↑→ + X` | `STING` | 17 HP limpos | Fura o bloqueio com turbinas ativadas. |
| `MarcovExhaustCloud` | **Fumaça de Exaustão** | Comando | `↓↓← + X` | `SLOW` | **0 HP**, drena 15 stamina e slow 5s | **FRACO**: Drena míseros 15 de stamina e causa 0 dano. |
| `MarcovOxygenRecharge` | **Célula de Oxigênio** | Comando | `←←← + X` | `HEAL` | Regenera 20 HP | Recarga de 8s. |
| `MarcovZeroGravity` | **Gravidade Zero** | Passiva | Passiva | `PASSIVA` | **Imune a empurrões e escapa da 1ª queda de cada round com 1 HP** | **UMA DAS MELHORES PASSIVAS DO JOGO**: Salva o lutador de nocautes e anula pressão de cordas. |
| `MarcovThrusterCharge` | **Módulo Sputnik** | Passiva | Passiva | `PASSIVA` | Socos carregados e especiais recuperam 20 stamina | Reabastece o tanque a cada especial desferido. |
| `MarcovTitaniumSuit` | **Traje de Titânio** | Passiva | Passiva | `PASSIVA` | Golpes pesados e críticos causam -25% dano | Reduz drasticamente a letalidade de uppercuts e counters. |

---

### CHEFE FINAL: BATALHATOR 2000 (IA EXCLUSIVA)
- **Vida Total**: 200 HP | 3 Fases com troca dinâmica de build a cada queda sofrida.
- **Fase 1 (Blindagem de Titânio)**: Passiva `BossTitanium` (-35% de dano sofrido o tempo todo), usa *Vórtice de Serras* (`BossDualSaw`: 22 HP + sangramento) e *Sobrecarga de Reator* (`BossOverclock`: 25 HP + drena 35 stamina).
- **Fase 2 (Dínamo Enfurecido)**: Passiva `BossDynamo` (devolve 15% do dano sofrido como choque elétrico), usa *Descarga PEM* (`BossEmp`: 24 HP + paralisia) e *Barragem de Mísseis* (`BossMissiles`: 24 HP teleguiado em 4 projéteis).
- **Fase 3 (Matriz Quântica)**: Passiva `BossQuantum` (35% de chance de esquiva quântica com glitch digital e cura +6 HP em cada acerto), usa *Combo Holográfico* (`BossHolo`: 26 HP + cegueira) e *Raio Apocalíptico* (`BossApocalypse`: 28 HP + drena 45 stamina).

---

## 5. DIAGNÓSTICO DOS 4 GRANDES PROBLEMAS DE BALANCEAMENTO

Se você for trabalhar com outra IA no celular para criar propostas de rebalanceamento, estes são os **4 gargalos principais** que ela precisa conhecer:

### Gargalo 1: O "Buraco Negro" do comando `↓↓← + X` (Comandos de Zero Dano)
- Quase todos os personagens possuem uma habilidade com o comando `↓↓← + X` que causa **0 HP de dano** e só aplica um status leve (pólen, lama, cinzas, fumaça, âncora, vento).
- **O dilema de design**: Em um jogo de boxe de janelas estreitas, gastar a janela de contra-ataque para dar 0 de dano é um suicídio de ritmo. O jogador sente que "desperdiçou" a esquiva.
- **Diretriz de correção**: Toda habilidade de controle precisa causar um **dano de impacto base (entre 12 e 16 HP)** além do seu efeito de status.

### Gargalo 2: Habilidades Ofensivas Obsoletas / Redundantes
- Algumas habilidades ofensivas são simplesmente versões piores de outras do mesmo personagem (exemplo clássico: *Gêiser do Pântano* do Frog que dá 16 HP sem nada, enquanto a *Língua* dá 18 HP + slow + empurrão; e o *Harpão* da Joana com 10 HP comparado aos 25 HP da *Tesoura*).
- **Diretriz de correção**: Dar identidade mecânica única. O Gêiser deve lançar o oponente para o ar (*Airborne juggle* para combo); o Harpão deve garantir atordoamento total de 25 frames.

### Gargalo 3: Passivas Desiguais (Algumas Quebradas, Outras Placebo)
- Enquanto *Gravidade Zero* (Marcov) salva de uma queda com 1 HP e *Dança das Brasas* (Valéria) dá esquiva com custo ZERO, passivas como *Apicultor* (Jack) economizam míseros 2 de estamina e *Amortecedor de Creme* (Cremoso) só defende contra jabs leves.
- **Diretriz de correção**: Elevar o patamar de valor das passivas fracas para que cada uma altere visivelmente o estilo de jogo do personagem.

### Gargalo 4: Modos com Auto-Penalização Excessiva
- O *Modo Melaço* do Jack reduz o próprio dano do lutador em 50% por 6 segundos. O jogador se sente mais fraco ativando o poder do que sem ele.
- **Diretriz de correção**: Habilidades de modo nunca devem reduzir o dano do próprio jogador pela metade sem oferecer uma compensação devastadora (ou remover a penalidade de dano e fazê-la roubar estamina/velocidade).

---

## 6. INSTRUÇÕES PARA A OUTRA IA (PROMPT DE TRABALHO)

Quando você for conversar com outra IA pelo celular, pode colar a seguinte instrução:

```text
Você é um Game Designer especialista em balanceamento e combate de jogos de luta/boxe arcade (referências: Punch-Out!!, Street Fighter 6, Guilty Gear Strive).
Com base no dossiê de combate do KO-RAMA:
1. Revise as 54 habilidades e passivas dos 8 lutadores.
2. Foque em corrigir as habilidades de "Zero Dano" (↓↓← + X), tornando-as recompensadoras com dano base (12-16 HP) + utilidade.
3. Repagine as habilidades matematicamente superadas e as passivas fracas/placebo (como Apicultor, Amortecedor de Creme, Magma Frio).
4. Crie sinergias reais para que builds alternativas (ex: Jack Tanque/Melaço, King Frog Veneno/Sustain, Valéria Controle de Cinzas) sejam tão competitivas e divertidas quanto as builds de burst padrão.
Apresente os novos valores numéricos (Dano, Frames, Status, Stamina) de forma organizada.
```

---

## 7. SISTEMA DE IMPACTO VISCERAL ("CRACK BRUSCO" & FEEDBACK AUDIOVISUAL)

Para solucionar a falta de peso em golpes impactantes e o incômodo auditivo do enxame de abelhas, foi implementado o **Sistema de Impacto Visceral**:

### 7.1. Remoção do Som Repetitivo das Abelhas
- **Problema anterior**: A cada 20 frames (3x por segundo, até 18 vezes), a infestação disparava `cue(Sfx::SpikePierce)`, gerando um ruído estridente e cansativo de perfuração contínua.
- **Ajuste aplicado**: O som repetitivo de ferroada foi **100% removido** do loop de dano de status (`tickStatus`).
- **Feedback preservado**: As abelhas mantêm a faísca dourada de ferrão (`0xffd700ff`), o número flutuante pulsado `-1` e um leve tremor de câmera (1.8f), garantindo leitura clara de perda de HP sem poluição sonora.

### 7.2. O Novo Som de "Crack Brusco" (`Sfx::Crunch`)
O antigo efeito sonoro de `Crunch` era apenas um ruído agudo fraco de 140ms. Ele foi totalmente reformulado para uma síntese em **3 camadas físicas simultâneas**:
1. **Snapping Cortante Inicial (Transiente)**: Onda Sawtooth rápida descendo de 2800 Hz para 320 Hz em 45ms (Ganho 0.85) — simula o estalo seco e imediato do impacto em cartilagem / maxilar.
2. **Fratura Ressonante (Textura Óssea)**: Filtro BandPass cortante com Q=3.2 centrado em 3400 Hz caindo para 400 Hz em 80ms (Ganho 0.75) — adiciona a granulação de rachadura e quebra física.
3. **Pancada Sub-Grave (Peso Corpóreo)**: Onda Triangular com decaimento exponencial de 160 Hz até 35 Hz por 220ms (Ganho 0.90) — transmite o golpe visceral no peito e o deslocamento de ar.
O limitador analógico por tangente hiperbólica (`tanh`) do motor de áudio amortece a soma, criando saturação arcade sem clipagem estourada.

### 7.3. Feedback Visual de Impacto (GLSL Shader + Shockwave)
- **Clarão Focal no Ponto de Impacto (*Contact Flash*)**: Injetado diretamente no `kPostFxFragShader`. Nos primeiros 4 frames (`progress < 0.18`), a área exata do impacto recebe uma explosão de brilho branco-quente com queda de intensidade radial (`diff < 0.28`).
- **Propagação Imediata via `lastImpactSeed_`**: O renderer agora rastreia sementes de impacto consecutivas, permitindo que múltiplos golpes pesados em sequência disparem ripples independentes sem aguardar a onda anterior expirar.

### 7.4. Auditoria de Todos os Personagens: Golpes que Disparam o "Crack Brusco"

| Lutador | Golpes / Habilidades Contempladas com o "Crack Brusco" | Motivo / Sensação de Jogo |
| :--- | :--- | :--- |
| **Universal (Qualquer Lutador)** | **Contra-Ataques (*Counter Hit*)**, **Uppercuts (`FIGHTER_UPPER`)**, **Socos Carregados (> 60%)** e **Punições em Brecha com Golpe Pesado**. | Recompensa máxima para precisão tática e timing de esquiva/leitura. |
| **Universal (Defesa)** | **Quebra de Guarda (*Guard Break*)** e **Golpes Pesados Sofridos na Cara**. | O jogador ouve a quebra violenta da postura e o estalo da pancada. |
| **Jack Mãos de Colmeia** | *Agulhada Rainha* (`JackStingJab` - 15 HP) e *Explosão de Pólen* (`JackCombBurst`). | A agulhada perfura a guarda como um estalo limpo; a explosão detona à queima-roupa. |
| **King Frog** | *Gêiser do Pântano* (`FrogSwampSpout` - 12 HP), *Salto Real Esmagador* (`FrogRoyalLeap` - 16 HP) e *Língua Chicote* (`FrogTongue` - 14 HP). | O gêiser ganha o estrondo da pressão da água; o salto real possui 15f de freeze frame e crack esmagador de queda. |
| **Baronesa Joana Estrela** | *Tesoura Esmagadora* (`JoanaClawCrush` - 16 HP), *Onda da Âncora* (`JoanaAnchorSlam` - 14 HP) e *Arrastão do Harpão* (`JoanaHookPull` - 15 HP). | O fechamento da tenaz gigante arrebenta qualquer guarda; a âncora esmaga o solo com ferro maciço. |
| **O Cremoso** | *Soco de Casquinha* (`CremosoConePunch` - 15 HP), *Explosão de Calda Quente* (`CremosoSyrupBlast` - 14 HP) e *Poça de Calda Grudenta* (`CremosoPuddleTrap` - 14 HP). | A casquinha quebra como vidro endurecido na cara do oponente; as pancadas no solo quebram a lona. |
| **Valéria Vulcão** | *Soco Magmático* (`ValeriaMagmaPunch` - 14 HP), *Erupção Solavanco* (`ValeriaVolcanicErupt` - 16 HP) e *Estocada de Lava* (`ValeriaLavaKickJab` - 15 HP). | O soco magmático combina 14f de hitstop com a detonação da rocha incandescente. |
| **Maestra Vivaldi** | *Inverno Allegro* (`VivaldiWinterAllegro` - 14 HP) e *Estocada em Crescendo* (`VivaldiCrescendoJab` - 15 HP). | O uppercut de gelo quebra placas de ectoplasma congelado; a batuta perfura com força seca. |
| **Lady Espinha** | *Gran Salto Luchador* (`EspinhaLuchaFlyer` - 16 HP), *La Garra del Cacto* (`EspinhaNopalSlap` - 15 HP) e *Estocada de Mandacaru* (`EspinhaNeedleJab` - 15 HP). | O salto da terceira corda causa impacto colossal (15f hitstop); a Garra quebra a guarda com duplo estalo de espinhos. |
| **Dr. Marcov** | *Propulsão Foguete* (`MarcovRocketThrust` - 15 HP), *Raio Orbital Sputnik* (`MarcovOrbitalLaser` - 16 HP) e *Impulso de Reação* (`MarcovKineticOverdrive` - 15 HP). | O punho a jato descarrega energia cinética instantânea que ecoa como um trovão mecânico. |
| **Batalhator 2000 (Boss)** | *Vórtice de Serras* (`BossDualSaw` - 15 HP) e *Sobrecarga de Reator* (`BossOverclock` - 16 HP). | O impacto das serras e a descarga do núcleo transmitem o peso industrial de máquina mortífera. |

