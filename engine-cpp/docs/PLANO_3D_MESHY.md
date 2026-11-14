# Plano de produção 3D do K.O.-RAMA (Meshy AI)

Complementa o *Dossiê Mestre de Transição 3D*. O dossiê define o estilo; este plano define **o que é gerado, como é
cortado e como é validado**, para que cada crédito do Meshy vire um asset utilizável na primeira ou segunda tentativa.

Referências: as 12 fichas em `modelsheets/` e a captura-alvo (Jack de costas × Dr. Marcov no ringue).

---

## 0. Decisões que travam o resto (responder antes da chave)

| # | Decisão | Recomendação | Por quê |
|---|---|---|---|
| D1 | **Braços** ✅ aprovado | Corpo **com braços** + IK de dois ossos; a luva/arma é um modelo separado preso na mão. | A captura-alvo mostra braços. O jogo continua mandando nas luvas (trajetórias do `korama_snapshot --track`): a luva vai para a posição do jogo e o braço a segue por IK. |
| D2 | **Rosto** | Olhos, sobrancelhas e boca em **planos 2.5D** com remendo de pele por baixo (mesma técnica do `reactions.js`). O Meshy pode esculpir nariz, focinho, queixo e orelhas. | Não precisa apagar o rosto da imagem de entrada: o plano cobre o olho pintado. Expressões (dor, raiva, K.O.) já estão desenhadas em 2D e reaproveitam. |
| D3 | **Imagem de entrada** | Para cada corpo, uma **folha limpa**: frente + costas, A-pose 45°, sem os props desta lista, fundo neutro liso. | Fichas atuais têm props colados ao corpo (luvas, chapéus, sombrinhas). Se entrarem assim, o Meshy funde tudo e o rig deforma o prop. |
| D4 | **Motor** | Piloto em **Three.js** (visualizador), decisão do motor final depois do King Frog. | Ver o visual rápido sem tocar no `korama.exe`. |
| D5 | **Escopo** | Só os **12 visuais base** agora. Skins depois, reaproveitando esqueleto, soquetes e o máximo de props. | 40 skins × corpo + props é o orçamento inteiro; os 12 base provam o pipeline. |
| D6 | **Formas/estados** ✅ | Resolver por **material, prop extra ou escala** antes de gerar outro corpo. Só ganham corpo próprio: Batalhator fase 2 (cockpit aberto, mesma silhueta da fase 1) e Joana monstruosa (fase 2 do projeto). | Cada corpo novo precisa de rig, validação e animação de novo. |

---

## 1. Regra de corte: o que vira modelo separado

Um elemento vira **prop separado (`.glb` preso num soquete)** quando cumpre pelo menos um destes:

1. **É a luva ou o que a mão segura.** As luvas são sempre separadas: o jogo controla a posição delas.
2. **Sai do corpo durante a luta:** voa no impacto ou no K.O., cai, é arremessado (chapéu, coroa, óculos, garrafa, osso).
3. **Troca por estado:** coroa/sem coroa, peruca das quatro estações, sombrinha aberta/fechada, viseira.
4. **Gira ou anima sozinho:** serras, sombrinha girando, olho no pedúnculo, garra que abre.
5. **É fino demais para o Meshy acertar colado:** antenas, batuta, pluma, cabos.

Vira **malha secundária com ossos próprios** (no Blender, presa ao mesmo esqueleto) quando precisa balançar:
cauda, dreads, cabelo de lava, capa/aba do casaco, saia, faixa, rabo de fantasma.

Vira **plano 2.5D**: olhos, sobrancelhas, boca, lágrimas/suor, estrelas de tontura, visor de LED.

Vira **shader/efeito, nunca modelo**: lava, ectoplasma translúcido, gelo, veneno, choques, chamas, fumaça,
faíscas, abelhas em enxame (instâncias de um único prop), notas musicais.

Fica **no corpo**: todo o resto (roupa, cinto, botas, orelhas, cabelo que não balança, cracas, espinhos grandes).

---

## 2. Padrões técnicos (valem para todo asset)

- **Unidade:** 1 = 1 metro. **Origem** entre os pés no chão. **Frente** para +Z. **Y** para cima.
- **Altura por porte** (alinhada às categorias do jogo): Pequenuxo 1,45 m · Normal 1,75 m · Grandalhão 2,15 m.
  Proporção chunky de 3 a 3,5 cabeças, mãos/luvas gigantes.
- **Pose:** A-pose 45°, palmas para baixo, pés na largura dos ombros, dedos separados do quadril.
- **Polígonos:** corpo 8–15 mil triângulos · luva/arma 1–3 mil · prop pequeno 300–1.500.
- **Textura:** 2048 px só cor base (*base color*). **Sem PBR, sem normal map**: o shader toon ignora e pesa.
  Pedir **cores chapadas, sem luz nem sombra pintada**; depois o script reduz para a paleta da ficha.
- **Esqueleto:** o do auto-rig do Meshy, renomeado no Blender para os nomes canônicos abaixo.
- **Soquetes (empties no Blender):** `head_top` (chapéu/coroa), `head_face` (planos 2.5D), `hand_L_grip`, `hand_R_grip`,
  `forearm_L`, `forearm_R`, `spine_back` (mochila/bateria/poste), `hips_front` (faixa/joystick), `tail_base`, `cockpit`.
- **Ossos extras** (Blender): `cape_*`, `tail_01..05`, `hair_*`, `skirt_ring_*`, `sash_01..03`, `ghost_tail_01..04`.
- **Nomes de arquivo:** `body_<personagem>.glb`, `prop_<personagem>_<peça>[_L|_R].glb`, `sec_<personagem>_<peça>.glb`.
  Personagens: `jack frog joana cremoso valeria vivaldi espinha marcov batalhator1 batalhator2 nerd espantalho`.
- **Pasta:** `assets3d/<personagem>/` com `source/` (o que veio do Meshy, intocado), `clean/` (pós-Blender) e `report.json`.

---

## 3. Ficha de corte por personagem

### Jack Mãos de Colmeia · Pequenuxo
- **Corpo:** casaco verde aberto, gola alta, cinto com fivela, calça e botas verdes, cabelo loiro espetado
  (fica no corpo: aparece quando o chapéu voa), rosto de queixo quadrado.
- **Props:** `prop_jack_hat` (chapéu amarelo + pluma, `head_top`, voa no K.O.) · `prop_jack_hive_L/R`
  (colmeias = luvas, `hand_*_grip`) · `prop_jack_bee` (1 abelha low-poly, instanciada no enxame).
- **Secundário:** `sec_jack_cape` (capa/aba traseira do casaco, 2×3 ossos).
- **2.5D:** olhos, sobrancelhas grossas, boca.
- **Estados:** Pele de Mel e Melaço = material âmbar translúcido; Rei da Colmeia = `prop_jack_honey_crown` + asas (fase 2).
- **Risco:** aba larga do chapéu e a pluma; por isso o chapéu é sempre prop.

### King Frog · Grandalhão  ← **piloto**
- **Corpo:** sapo musculoso, calção verde com debrum preto, cinto de campeão dourado, pés laranja palmados.
  Boca **fechada** no corpo (a ficha também mostra fechada).
- **Props:** `prop_frog_crown` (`head_top`, voa rodopiando no K.O.) · `prop_frog_glove_L/R` · `prop_frog_tongue`
  (cilindro com ossos, só no Golpe de Língua).
- **Papo (`sec_frog_papo`) — é o aviso dos dois golpes de longe:** infla durante o aviso do **Golpe de Língua** e da
  **Cuspida de Lama**, e também na finta (infla e segura). Malha própria presa no queixo, feita no Blender (não gasta
  crédito): membrana amarela clara com veias, 4 estágios de enchimento seguindo o tempo do aviso do motor
  (`tellKind`), mola de gelatina, e cor um pouco mais translúcida quando cheio. Precisa ser lido da câmera do jogador:
  de frente infla para a frente e para os lados; de costas (sapo do jogador) aparece nas laterais do pescoço.
  Esvazia de uma vez quando a língua sai ou quando cospe.
- **2.5D:** olhos (nas protuberâncias do topo), pálpebras de raiva, boca (abre para a língua e para a dor).
- **Estados:** Pele Venenosa = textura roxa com verrugas; Sem coroa = esconder prop; Inflado = escala do papo.

### Joana Cara-de-Craca · Normal
- **Corpo:** casaco azul-marinho com dourado, blusa listrada, calça listrada, botas, bochecha de carapaça
  e tapa-olho pintados (fixos), cracas no casaco.
- **Props:** `prop_joana_hat` (tricórnio, `head_top`, voa) · `prop_joana_eyestalk` (pedúnculo com 2 ossos, preso no
  chapéu; o olho dele é plano 2.5D com expressões) · `prop_joana_hook_L` · `prop_joana_claw_R` (garra em 2 partes:
  base + pinça com dobradiça para abrir em 3 etapas) · `prop_joana_rum` (assinatura) · `prop_joana_anchor` (habilidade).
- **Secundário:** `sec_joana_dreads` (4 mechas, 3 ossos cada).
- **2.5D:** boca com dente de ouro, sobrancelha; o olho humano fica sob o tapa-olho.
- **Estados:** Barreira de Cracas = prop de escudo; Monstruosa = corpo próprio (fase 2 do projeto).

### Valéria Vulcânica · Normal
- **Corpo:** pele morena, top cropped, abdômen definido, botas ciano com fivela dourada.
- **Props:** `prop_valeria_glove_L/R` (luvas laranja) · `prop_valeria_umbrella_open` (copa facetada + haste,
  presa na luva, gira) · `prop_valeria_umbrella_closed` (a fechada da assinatura).
- **Secundários:** `sec_valeria_hair` (cabeleira de lava com ossos, shader emissivo + lâmpada de lava) ·
  `sec_valeria_skirt_A/B/C`: **saia plissada em 3 camadas, uma por cor** (turquesa embaixo, amarela, laranja em cima),
  cada camada uma malha com o próprio anel de correntes de ossos, para abrirem com atraso uma da outra no giro.
- **2.5D:** olhos amarelos acesos, sobrancelhas, sorriso.
- **Estados:** Magma Frio = material pedra + rachaduras emissivas no cabelo, rosto, tronco e saia.

### O Cremoso · Grandalhão
- **Corpo:** golem de baunilha com escorridos, bigode de creme, avental com bolso de morango, caneleiras de casquinha.
  Órbitas escuras pintadas.
- **Props:** `prop_cremoso_chef_hat` (`head_top`, pula a cada jab e voa no K.O.) · `prop_cremoso_fist_L/R`
  (punhos de sorvete com morangos) · `prop_cremoso_puddle` (poça do Modo Derretido).
- **2.5D:** sobrancelhas, pupilas brilhando no fundo das órbitas, boca.
- **Estados:** sabores = troca de cor do material (baunilha, frozen, morango); Derretido = esmagar o corpo + poça + shader escorrendo.

### Maestra Vivaldi · Normal
- **Corpo:** fantasma de ectoplasma verde-menta, casaca roxa com dragonas, jabô, **sem pernas** (cauda fantasma).
- **Props:** `prop_vivaldi_wig` (peruca barroca com laço; 4 variantes das estações) · `prop_vivaldi_glasses`
  (pince-nez) · `prop_vivaldi_baton` (`hand_R_grip`) · `prop_vivaldi_hand_L` (mão fantasma livre).
- **Secundário:** `sec_vivaldi_ghost_tail` (4 ossos).
- **2.5D:** olhos dourados, sobrancelhas, sorriso.
- **Atenção:** o auto-rig do Meshy espera pernas. **Esqueleto feito no Blender** (coluna, braços, cabeça, cauda).
  Material translúcido + brilho de borda.

### Lady Espinha · Normal
- **Corpo:** cacto verde com espinhos dourados grandes, collant azul fechado nas costas com debrum laranja, botas azuis.
- **Props:** `prop_espinha_hair` (pétalas magenta + coroa dourada; vira a versão florida no Viva la Caatinga) ·
  `prop_espinha_glove_L/R` (punhos de cacto com munhequeira de espinhos) · `prop_espinha_flower` (assinatura/flor jogada).
- **Secundário:** `sec_espinha_sash` (medalhão + faixa, 3 ossos pendulares).
- **2.5D:** monocelha, olhos negros de pupila amarela, boca.
- **Risco:** espinhos pequenos viram ruído; manter só os grandes da ficha.

### Dr. Marcov · Normal
- **Corpo:** chimpanzé com tufo, orelhas redondas, traje preto, jaleco branco aberto com canetas, monitor no peito,
  botas tecnológicas.
- **Props:** `prop_marcov_gauntlet_L/R` (manoplas de plasma com a mão, em `hand_*_grip`; brilho e choques são efeito) ·
  `prop_marcov_banana` · `prop_marcov_peel` (assinatura).
- **Secundários:** `sec_marcov_tail` (5 ossos) · `sec_marcov_coat_tails` (abas do jaleco, 2×2 ossos).
- **2.5D:** olhos, sobrancelha com cicatriz, boca; monitor do peito = plano emissivo animado (batimento).
- **Estados:** Gravidade Zero = flutuar + chama azul nas botas (efeito).

### Batalhator 2000 · fases 1 e 2 · Grandalhão
As duas fases são **o mesmo robô**: mesma silhueta, mesmas proporções, mesmas **esteiras nos pés**. A fase 2 é a fase 1
avariada. Por isso os dois corpos saem da mesma folha limpa, e a fase 2 é gerada a partir da fase 1 (mesma imagem
editada), não de uma ficha nova.

- **Corpo fase 1 (`body_batalhator1`):** robô cinza rebitado, ombreiras listradas com espinhos, elmo de cavaleiro com
  visor, peito com escudo, esteiras.
- **Corpo fase 2 (`body_batalhator2`):** o mesmo, com o peito aberto mostrando a cabine, blindagem amassada,
  ombros com lançadores de mísseis e **sem a cabeça**: no pescoço fica uma **serra**.
- **Props fase 1:** `prop_batalhator_chestplate` (escudo do peito com dobradiça; abre na transição) ·
  `prop_batalhator_saw_L/R` (giram) · `prop_batalhator_battery` (`spine_back`) · `prop_batalhator_antenna_L/R`.
- **Props fase 2:** `prop_batalhator_saw_hot_L` (mão esquerda, incandescente) · **`prop_batalhator_head_fist_R`:
  a cabeça da fase 1 (elmo com visor) vira o punho da mão direita**, mesmo modelo do elmo · `prop_batalhator_neck_saw`
  (serra no lugar da cabeça, gira) · `prop_batalhator_battery_damaged`. O Nerd na cabine reaproveita `body_nerd`.
- **2.5D:** visor de LED com eletrocardiograma (fase 1 na cabeça; fase 2 no punho-cabeça, piscando em pane).
- **Efeitos fase 2:** fumaça, faíscas, chamas nos escapes.
- **Ficha a refazer:** a ficha atual da fase 2 tem rosto no lugar da serra; a folha limpa da fase 2 corrige isso.

### O Nerd Operador · fase 3 · Normal (franzino)
- **Corpo:** polo verde-petróleo com canetas, bermuda cáqui, meias listradas, tênis retrô, cabelo castanho.
- **Props:** `prop_nerd_headset` (cai no K.O.) · `prop_nerd_glasses` (entortam no cruzado) · `prop_nerd_joystick` (`hips_front`).
- **2.5D:** olhos de pânico atrás dos óculos, boca gritando; suor gigante é efeito.

### O Espantalho · Normal
- **Corpo:** cabeça de estopa, camisa xadrez com remendos, calça de estopa, palha saindo, botas de madeira,
  poste de madeira nas costas (fica no corpo).
- **Props:** `prop_espantalho_hat` (voa já no primeiro golpe) · `prop_espantalho_glove_L/R` (luvas de estopa).
- **2.5D:** olhos de botão, boca costurada em zigue-zague.

**Total para os 12 visuais base:** 12 corpos + ~45 props + ~20 malhas secundárias.

---

## 4. Preparação das imagens de entrada (antes da chave)

1. **Recorte** frente e costas de cada ficha (script com PIL, eu faço).
2. **Folha limpa do corpo** para os 12: você gera no seu gerador de imagem com o prompt-modelo abaixo, a partir da ficha.
   *"Same character, same proportions and colors, orthographic front and back view, A-pose with arms 45 degrees from the body,
   palms down, legs shoulder width, WITHOUT [lista de props do personagem], bare hands, flat cel colors, no shading,
   plain light grey background, no text, no arrows, no color swatches."*
3. **Imagem de cada prop** isolado (frente + lateral quando der), mesmo fundo liso. Para props simples
   (coroa, banana, garrafa) o texto do prompt basta.
4. Eu confiro cada imagem antes de ir para o Meshy: proporção, cores da paleta, nada colado.

## 5. Configuração do Meshy (conferir na documentação quando a chave chegar)

- Corpo: **multi-imagem** (frente + costas), modelo mais recente, triângulos, remesh ligado, alvo ~12 mil,
  textura ligada, **PBR desligado**, simetria automática, pose A forçada se a opção existir.
- Rig: auto-rig bípede com a altura do porte. Vivaldi fica de fora (esqueleto no Blender).
- Props: imagem → 3D com alvo de polígonos menor e sem rig.
- **A chave fica numa variável de ambiente (`MESHY_API_KEY`) que você define na sua máquina.** Ela não entra em
  arquivo do projeto nem no git, e não precisa ser colada no chat.
- Cada pedido salva `source/` + o JSON da resposta, para refazer sem gastar crédito.

## 6. Ferramentas que eu deixo prontas antes da chave

1. `tools/3d/meshy_client.py`: envia, acompanha e baixa; nunca reenvia um asset já baixado; registra créditos gastos.
2. `tools/3d/validate.py` (Blender sem janela): triângulos, altura, origem, frente, nomes de ossos, ângulo da A-pose,
   tamanho da textura, ausência de PBR, peças soltas. Gera `report.json` com aprovado/reprovado.
3. `tools/3d/postprocess.py` (Blender): renomeia ossos, cria soquetes, adiciona ossos secundários com pesos,
   reduz a textura para a paleta da ficha e exporta para `clean/`.
4. `tools/3d/viewer.html` (Three.js): toon de 3 tons, contorno por *inverted hull* com esqueleto, planos 2.5D,
   props nos soquetes, IK das luvas guiado pelas trilhas do jogo, câmera do ringue e comparação lado a lado com o 2D.
5. Um King Frog provisório de formas simples para testar o visualizador antes do primeiro modelo real.

## 6.1 Orçamento de créditos (plano Pro: 1.100)

Preços da API conferidos em docs.meshy.ai/en/api/pricing:

| Operação | Créditos |
|---|---|
| Multi-imagem → 3D com textura 2K (meshy-7.1) | 30 |
| Imagem → 3D com textura 2K (meshy-7.1) | 30 |
| Imagem → 3D com textura 2K (meshy-6-lite) | 15 |
| Auto-rig | 5 |
| Retextura | 10 |

**Regras para caber:** uma luva só por personagem (a outra é espelhada no Blender) · props simples no modelo leve ·
tudo que é forma simples feito no Blender sem crédito (papo, língua, caudas, saias, capas, dreads, faixa, cauda
fantasma, batuta, óculos, antenas, banana, osso, garrafa, âncora, abelha, poça, serra do pescoço reaproveita a
serra, punho-cabeça recortado do corpo da fase 1).

| Lote | Itens | Créditos |
|---|---|---|
| Piloto King Frog | corpo + rig (35), luva (30), coroa leve (15) | 80 |
| Outros 12 corpos | 11 × (30 + rig 5) + Vivaldi sem rig (30) | 415 |
| Props principais (30) | colmeia, chapéu do Jack, garra e chapéu da Joana, sombrinha aberta, punho do Cremoso, peruca da Vivaldi, cabeleira e luva da Espinha, manopla do Marcov, serra, escudo do peito | 360 |
| Props leves (15) | sombrinha fechada, luva da Valéria, gancho da Joana, chapéu do Cremoso, chapéu e luva do Espantalho, bateria, headset, joystick | 135 |
| **Reserva para refazer** | ~3 a 4 tentativas | **110** |
| **Total** | | **1.100** |

A reserva é pequena: por isso as folhas limpas passam pela minha conferência antes de qualquer envio, e o piloto
mostra a taxa de acerto real antes de gastar o resto. Se o piloto precisar de mais tentativas que o previsto,
rebaixo props principais para o modelo leve.

**Conferir antes de começar:** a página de preços diz que créditos de API são comprados nas configurações da
assinatura; confirme se os 1.100 do Pro valem para a API. Se não valerem, o pipeline funciona igual com você gerando
na interface web e salvando os arquivos em `assets3d/<personagem>/source/`. Confirme também que o Pro permite uso
comercial dos modelos.

## 6.2 Animação

O auto-rig do Meshy entrega o esqueleto e só caminhada/corrida; não há golpes de boxe. Então:
- **Luvas e braços:** posição das luvas pelas trilhas do jogo + IK (decisão D1).
- **Corpo:** poses por ação (guarda, passada, esquiva, abaixar, golpe levado, atordoado, aviso) feitas no Blender por
  script e misturadas por tempo de jogo, com os mesmos tempos do motor; inclinação e torção do tronco seguem a luva
  que está socando, como no visual novo 2D.
- **Assinaturas e K.O.:** animações próprias no Blender, uma por personagem.
- O esqueleto é confirmado no piloto; se for compatível com Mixamo, uso animações gratuitas de lá para quedas e
  comemorações como base.

## 6.3 Outros pontos já combinados

- **Ringue e público:** ringue 3D; o público continua 2D (cartazes) para não pesar; HUD continua 2D.
- **Tamanho dos arquivos:** malhas comprimidas (meshopt) e texturas KTX2 no pacote final; originais do Meshy guardados fora do pacote.
- **Desempenho:** 60 FPS com 2 lutadores, ~30 mil triângulos cada com props, ≤12 cadeias de mola cada.
- **Se um modelo vier ruim:** 1º ajustar a imagem e refazer; 2º retextura (10) se só a pintura falhou; 3º consertar no Blender.

## 7. Ordem de produção

1. **Piloto King Frog:** corpo + coroa + luvas + papo + língua → validação → visualizador → sua aprovação.
2. Humanos simples: Nerd, Jack, Marcov.
3. Com secundários: Joana, Valéria, Lady Espinha, Cremoso.
4. Especiais: Espantalho, Batalhator fases 1 e 2.
5. Vivaldi (esqueleto manual).
6. Formas extras (Joana monstruosa, Rei da Colmeia) e depois skins.

Critério para avançar de etapa: **todos os assets da etapa com `report.json` aprovado e visto no visualizador.**

## 8. Decisões de arte já tomadas

- **Valéria:** saia plissada em 3 camadas, uma de cada cor (turquesa, amarela, laranja).
- **Batalhator:** esteiras nas duas fases; fase 2 sem cabeça (serra no pescoço), mão esquerda serra, mão direita
  é a cabeça da fase 1; as duas fases fisicamente iguais.
- **Braços:** todos com braços.
- **Ainda em aberto:** o papo do King Frog (desenhar antes do piloto); Jack com braços nas mangas do casaco.

## 9. Tecido, cabelo e partes que balançam

Tudo que balança é **visual**: não muda alcance, acerto nem tempo de golpe. Roda depois do esqueleto animado,
a cada quadro, como ossos-mola (*spring bones*, o mesmo método do VRM).

**Como o corte de modelo muda por causa disso**
- Cada parte que balança é uma **malha separada** (`sec_*`) com **arestas suficientes para dobrar**: o Meshy reduz
  polígonos, então o pós-processamento no Blender refaz a malha dessas peças com mais anéis antes de criar os ossos.
- Cada uma ganha uma **cadeia de ossos** presa a um osso do corpo e **colisores** (esferas e cápsulas na cabeça,
  peito, quadril, coxas e braços) para não atravessar o corpo.
- Parâmetros por peça: rigidez, gravidade, arrasto, ângulo máximo. Ficam num arquivo de dados por personagem,
  ajustáveis no visualizador com controles deslizantes.

**Comportamento com o jogo**
- **Pausa de impacto:** as molas congelam junto com o motor e soltam com um tranco, o que vende o golpe.
- **Golpe levado / K.O.:** um impulso extra na direção do soco (cabelo e capa chicoteiam).
- **Giro do K.O. e assinaturas com giro:** força centrífuga (a saia e o cabelo abrem).
- **Teto de custo:** até ~12 cadeias por lutador, 2 lutadores em cena; simulação fixa a 60 Hz.

**O que balança em cada um**

| Personagem | Cadeias de ossos-mola |
|---|---|
| Jack | capa/aba do casaco, pluma do chapéu |
| King Frog | papo (gelatina), língua no Golpe de Língua |
| Joana | 4 dreads, abas do casaco, pedúnculo do olho |
| Valéria | cabelo de lava (6–8 mechas), 3 camadas da saia (anel de 8 correntes cada) |
| Cremoso | avental, gotas grandes escorrendo |
| Vivaldi | cauda fantasma, laço da peruca, abas da casaca |
| Lady Espinha | pétalas da cabeleira, faixa do medalhão |
| Dr. Marcov | cauda, abas do jaleco, tufo da cabeça |
| Batalhator | antenas, cabos soltos (fase 2) |
| Nerd | franja, fio do headset |
| Espantalho | palha (ombros, punhos, chapéu), barra da camisa |

**Riscos e como evito**
- Peça atravessando o corpo nos golpes rápidos → colisores + ângulo máximo + mais rigidez durante o golpe.
- Saia abrindo demais no giro → limite de abertura por camada, igual ao que fizemos no 2D.
- Custo de CPU → colisores simples, sem colisão entre cadeias, simulação só dos dois lutadores.

## 10. Piloto King Frog: o que aprendemos (vale para os 11 próximos)

Estado em 17/10/2026: o King Frog está fechado no visualizador (`tools3d/viewer/index.html`, servidor `viewer3d`
em `http://localhost:8766/tools3d/viewer/index.html`). Os modelos limpos ficam em `assets3d/frog/clean/`.

### 10.1 Linha de produção do corpo (Blender, sem janela)
Ordem que funcionou, do `source/rig_frog.glb` do Meshy até o `clean/body_frog.glb`:
1. `postprocess.py --tipo corpo --textura <chapada> --aplainar-olhos --arredondar-cabeca --decepar-maos --afinar-pulso 0.3 --normais-suaves 25`
   - **decepar as mãos** e afinar o pulso: a luva é peça própria e encaixa no toco (mão do Meshy + luva = braço esmagado).
   - **normais suaves**: a sombra dura de quadrinho segue a forma grande, não cada triângulo (sem manchas facetadas).
2. `make_frog_belt.py`: acessório com linhas retas (cinturão) é **modelado e pintado em código**, nunca tirado da textura do Meshy.
3. `apagar_cinto_meshy.py`: some com o acessório que o Meshy esculpiu no corpo (afunda até a pele, alisa com a malha
   soldada, repinta). Mexer só em vértices do tronco (pesos de quadril/coluna/coxa) e mover os **gêmeos de costura**
   juntos — senão abre furo e dá para ver o outro lado.
4. `pintar_bermuda.py`: roupa com **cor chapada por material** (sem textura). A textura do Meshy vem picotada em
   centenas de ilhas e o mipmap mistura as cores vizinhas; repintar a textura nunca resolve.
5. `validate.py ... --limpo` (≤ 16 mil triângulos, textura ≤ 2048, 24 ossos, A-pose).
Conferência visual sem abrir o Blender: `vistas.py modelo.glb pasta [--textura] [--sem-cinto] [--zoom-cabeca]`.

### 10.2 O que é shader/código no visualizador (reaproveitável)
- **Quadrinho**: luz em 2 tons duros + retícula a 45°, contorno por profundidade em tela, animação a 12 quadros/s.
- **Rosto 2.5D**: olhos/boca/narinas em canvas, **projetados na pele** (decalque), expressões por troca de textura
  (normal, raiva, dor, K.O., rangendo dentes ao carregar, olho serrado de raiva ao acumular).
- **Boca que abre de verdade**: portal com estêncil (abertura apaga a profundidade, interior 3D aparece por dentro).
- **Barriga clara e papo**: pintados/inflados **no shader do corpo** (elipse no espaço de repouso) — borda lisa,
  integrada à pele, sem peça separada.
- **IK**: braços até as luvas (com giro do punho controlado: palma para ele na guarda/uppercut, para baixo nos socos),
  pernas até os pés plantados (matrizes atualizadas antes de marcar os pés!).
- **Molas**: cinturão rígido no quadril com mola vertical, coroa, barra da bermuda (no shader), papo.
- **Golpes base** com quadros-chave e giro de quadril; **carga** igual ao jogo (72 quadros, solta sozinho em +24,
  luva para trás tremendo, soco até 50% mais lento).
- **Esquiva** em 3 direções, duas famílias: *inclina* (pés plantados, tronco sai da linha) e *salta* (sai do chão e volta).
- **Passada** no lugar: o ringue gira 90° (22 quadros), como no jogo.
- **Tempos do jogo** na constante `GAME` (aviso 38 quadros, língua 24 com acerto no 10º, lama 26 saindo em 35%).

### 10.3 King Frog: o que ainda falta
- Tapa de Lama (soco no corpo pegando lama do chão).
- Finta com jab (hoje a finta só enche o papo até o estágio 3).
- Salto Real Esmagador.
- Pele Venenosa (modo de 6 s: precisa de visual de pele tóxica).
- Captura de Moscas e Gêiser do Pântano (habilidades de loadout).
- Ligar o visualizador ao motor C++ (hoje os tempos foram copiados à mão para `GAME`).

### 10.4 Para o próximo personagem
- Mesma folha limpa (A-pose, sem rosto, sem acessórios) → Meshy multi-imagem → rig.
- Já separar na folha o que vai virar peça própria (luvas, cinturão, chapéu, armas): o Meshy esculpe tudo no corpo.
- Reaproveitar o visualizador: trocar só os assets, as cores (barriga, roupa) e as poses dos golpes assinatura.

## 11. Jack Mãos de Colmeia (segundo personagem): estado e lições

Estado em 17/10/2026: visual fechado no visualizador (`index.html?p=jack`). Créditos gastos: 95 (corpo 30, rig 5,
chapéu 30, colmeia 30 — a colmeia do Meshy acabou substituída). Modelos em `assets3d/jack/clean/`.

### 11.1 Linha de produção do Jack (em ordem)
1. `palette.py ... --cores 10 --extra "#c9a54a,#5a4532"` (cores pequenas como a fivela somem sem `--extra`).
2. `postprocess.py --tipo corpo --decepar-maos --afinar-pulso 0.2 --normais-suaves 25 --rosto-nitido --normais-cor "#c9a54a"`
   (sem `--aplainar-olhos`: alisaria os espetos do cabelo; `--rosto-nitido` mantém nariz/queixo).
3. `make_jack_buckle.py` (fivela modelada em código; a do Meshy saiu torta e vira cinto).
4. `pesos_balanco.py --cabelo ... --roupa ... --abertura 0.13` (atributos `_SWAY_HAIR`/`_SWAY_COAT`).
5. `marcar_regiao.py --nome _SHIRT ...` (camisa tem a mesma cor do casaco: inundação até a quina do casaco).
6. Peças: chapéu = `postprocess.py --tipo prop --cores-por-face ... --faixa 1 --normais-suaves 12`;
   colmeias = `make_jack_hive.py` (código: a do Meshy veio oca, parede dupla); abelha = `make_jack_bee.py`.

### 11.2 Lições novas (valem para os próximos)
- **Peça que o Meshy erra vira código no Blender**: colmeia (barril + furos em funil), fivela, abelha. Rápido e limpo.
- **Rosto do jogo direto do atlas** (`engine-cpp/assets/sprites/<p>.atlas`): olhos e bocas recortados no espaço
  360×360 mantêm a proporção original; olhos num sprite só viram um decalque único.
- **Rosto 2.5D com `depthWrite: false`**: senão o contorno e o vinco desenham borda preta em volta dele.
- **Chapéu prende o cabelo** no shader (o que passa da aba entra na copa; embaixo da aba fica cheio) e solta no K.O.;
  chapéu posicionado por altura no mundo + inclinação (o eixo do osso da cabeça é torto).
- **Vinco** (segunda derivada da profundidade) desenha as quinas vivas em preto: fivela, beira do casaco, cinto.
- **Texturas sem mipmap** (as ilhas picotadas do Meshy vazam cor nas costuras).
- **Regras fixas > textura** para recortes de roupa: decote em U, faixa central, sombra só nas laterais.
- **IK parte do repouso a cada quadro** (torção acumulava e afinava o antebraço); esfera de colisão na cabeça.
- **Config por personagem** em `CHARS` (index.html): arquivos, porte (`sc`), guarda, chapéu, rosto, roupa, colisão.

### 11.3 Jack: o que ainda falta (nada exige modelo novo nem créditos)
- Golpes de assinatura: Ferroada (aba do chapéu baixa + jab rápido), Colmeia Dupla (bate as colmeias, gancho dos
  dois lados), Enxame (abelhas instanciadas saindo das colmeias + uppercut).
- Estados: Pele de Mel e Melaço (material âmbar translúcido), Rei da Colmeia (coroa de mel + asas, fase 2).
- Expressões extras do atlas: feliz, piscando, olhando para cima, espiral (tontura).
- Capa/aba traseira do casaco como osso secundário (hoje o balanço é por shader, já suficiente).

## 12. Dr. Marcov (terceiro personagem): estado e lições

Estado em 17/10/2026: modelo fechado no visualizador (`index.html?p=marcov`). Créditos: 65 (corpo 30, rig 5,
manopla 30). Foco atual do projeto: **modelos primeiro**; golpes de assinatura de todos ficam para uma rodada depois.

### 12.1 Linha de produção do Marcov (em ordem)
1. `palette.py ... --cores 12 --extra "#2ee6e6,#3a7bd5"` (ciano das botas/monitor e a caneta).
2. `postprocess.py --tipo corpo --decepar-maos --afinar-pulso 0.2 --normais-suaves 25 --rosto-nitido`.
3. `pesos_balanco.py --cabelo "#5a413a" --cabelo-z 1.6 1.78 --roupa <brancos do jaleco> --roupa-z 0.95 0.62 --abertura 0.1`.
4. `pintar_pelo_pescoco.py` (só textura: o azul da camisa que o Meshy pintou embaixo da bochecha vira pelo).
5. `saia_no_quadril.py --perna 0.55` (barra do jaleco: 55% coxa / 45% quadril, forro e pano com o mesmo balanço).
6. Manopla: `postprocess.py --tipo prop --alinhar-eixo --virar --centro-base --altura 0.42 --textura <chapada> --normais-suaves 3`.
No visualizador (`CHARS.marcov`): `gloveTwist: π` + `gloveFlipZ` (manopla espelhada no Z), `palmsIn`, guarda alta padrão,
rabo em código (`tail`), roupa do terno recolorida por altura (`suit`), `coatSway: 0.35` (nunca para dentro das pernas).

### 12.2 Lições novas
- **Peça torta do Meshy** (punho na diagonal): `--alinhar-eixo` (PCA) + `--centro-base` (gira em volta da boca do pulso).
  Orientar luva = primeiro endireitar o modelo; girar no visualizador para compensar só confunde.
- **Olhos do jogo num sprite só**: recortar em duas metades (cada olho anda sem esticar); caixas do recorte centradas
  no centro do quadro do atlas (x = 240 no Marcov), senão o par fica torto. Narinas num sprite à parte, com controle próprio.
- **Casaco/jaleco comprido**: dividir o peso da barra entre quadril e coxas e dar o MESMO balanço ao forro e ao pano;
  balanço contido e nunca para a frente. Peso só no quadril -> pernas atravessam; balanço só no pano -> riscos pretos.
- **Rabo** em código (corrente de gomos com mola), igual à língua do sapo.
- **Não "limpar" o tronco com reclassificação agressiva + mexer na geometria** (achatar reator, recuar camisa): abriu
  furos e não convenceu. Correção de cor localizada direto na textura é segura; para tirar peças do modelo, refazer a folha.
- Servidor do visualizador agora sem cache (`tools3d/serve.py`).

### 12.3 Marcov: o que ainda falta
- Golpes de assinatura (Vostok com visor, osso nas costas, Aviador com óculos) — rodada de golpes depois dos modelos.
- Abas do jaleco como ossos secundários (balanço maior sem atravessar as pernas).
- Reator do peito e sujeira da camisa: se quiser tirar, refazer a folha limpa sem o reator.

## 13. Joana Cara-de-Craca (quarto personagem): estado e lições

Estado em 17/10/2026: modelo fechado no visualizador (`index.html?p=joana`). Créditos: 155 (corpo 30, rig 5, chapéu,
dreads, garra e gancho; o gancho do Meshy veio com um boneco junto e foi refeito em código). Golpes de assinatura
ficam para a rodada de golpes.

### 13.1 Linha de produção da Joana (em ordem)
Corpo:
1. `postprocess.py --tipo corpo --decepar-maos --recuo-pulso 0.14 --afinar-pulso 0.2 --normais-suaves 25 --rosto-nitido`
   (`--recuo-pulso`: corta a mão mais para dentro do antebraço — o toco claro do pulso some).
2. `engrossar_tronco.py --costas 1.3 --largura 1.25` (costas mais fundas e peito/costas mais largos; só move vértices).
3. `braco_cilindro.py --perto 0.14 --lado 0.8 --casca 1.0 --frente 50`: a manga do Meshy (fundida na lateral do casaco
   no A-pose) vira um cilindro sólido preso aos ossos do braço; o pano do casaco que tinha peso de braço passa a seguir
   o tronco; superfície lisa da cor do casaco cobre as laterais e as costas (a frente fica livre: camisa e barriga).
4. `mover_olho_siri.py --frente 0.2 --fora 0.07 --baixar 0.05` (haste do olho de siri para fora da aba do chapéu).
5. `pintar_olho_siri.py` (só textura: apaga a pupila do olho de siri; o visualizador desenha a pupila e o X do K.O.).
Peças:
6. Dreads em código: `make_joana_dreads.py --raiz 1.6 --volume 0.03 --rosto 60` (tons da peruca do Meshy; mechas nascem
   rente à cabeça sob a aba, sobem pelo couro cabeludo afundadas numa touca da mesma cor, nada na frente do rosto e do
   lado do siri só atrás do olho; atributo `_SWAY_HAIR`: só as pontas balançam).
7. Chapéu: `esticar_tras.py --fator 1.45` (aba de trás cobre a raiz dos dreads).
8. Garra: `separar_pinca.py --hy 0.075 --hz 0.244` (pinça em gancho separada com origem na dobradiça, corte tampado).
9. Gancho: `make_joana_hook.py` (copo + tubo em "?" feito em código).
No visualizador (`CHARS.joana`): `gloveTwist: [π, π]` (giro por lado), `wig.tips` (mola própria das pontas), `crabEye`
(pupila/X no globo, controles "Olho siri" no painel), `claw` (pinça fechada a +0,3 rad; abre e fecha com mola).

### 13.2 Lições novas
- **Manga fundida no casaco (A-pose)**: não dá para "soltar" por pesos nem apagando faces de ligação (abre buraco em
  todo canto). O que funcionou: manga -> cilindro sólido nos ossos; o resto do pano larga o braço; e uma superfície lisa
  cobrindo as laterais. Toda mudança de corpo: partir do corpo original e refazer a cadeia inteira (scripts em ordem).
- **Conferir sempre a câmera do ringue E de costas, corpo inteiro**, antes de mostrar (uma mudança global furou o
  casaco e só aparecia no corpo inteiro).
- **Cabelo em código > peruca do Meshy** quando o cabelo divide espaço com chapéu/olho: controle de onde nasce cada mecha,
  raiz parada sob a aba e só as pontas com mola. Atenção: o mesmo tom em touca e mecha some no sombreamento chapado.
- **Animação a ~12 quadros/s** (`animFps`): movimentos de peças (garra) precisam durar vários quadros (0,3-0,5 s),
  senão "teleportam". Mola: abrir macia, fechar rápida com pouco quique.
- **Balanço do chapéu sobre a inclinação inicial** só para quem tem peruca: no Jack mudou a altura do chapéu.
- `postprocess --cores-por-face` deixa a peça em faces soltas: soldar (`remove_doubles`) antes de tampar cortes.

### 13.3 Joana: o que ainda falta
- Golpes de assinatura (rodada de golpes).
- Tórax um pouco irregular nas laterais (aceitável por ora).

## 14. Valéria Vulcânica (quinto personagem): estado e lições

Estado em 18/10/2026: modelo fechado no visualizador (`index.html?p=valeria`). Créditos: 35 (corpo 30, rig 5). Só o
corpo veio do Meshy; cabelo, saia, guarda-chuvas, punhos e rosto são feitos em código.

### 14.1 Linha de produção da Valéria (em ordem)
Entrada: `tools3d/limpar_folha.py` recorta frente/costas da folha (tira textos, setas, amostras de cor, sombra do chão;
`--rosto` alisa os pontilhados do rosto). Folha com braços afastados e sem manga = corpo limpo no Meshy.
Textura: `palette.py --cores 10 --extra "#e2b23a,#2fc6cf"`, depois `limpar_borda_top.py` (top vermelho chapado com barra
reta, pele `--nova-pele "#c98a5e"`).
Corpo:
1. `postprocess.py --tipo corpo --decepar-maos --recuo-pulso 0.02 --afinar-pulso 0.15 --normais-suaves 25 --rosto-nitido`
2. `engrossar_tronco.py --costas 1.15 --largura 1.15`
3. `alargar_cintura.py --z 0.99 --fator 1.12 --altura 0.09`
Peças (todas em código, a partir do corpo pronto):
4. `make_valeria_hair.py --rosto 1.0 --franja 0.6` — cabelão de magma (metaballs em capuz, franja repartida em duas
   mechas, rabo de cavalo com prendedor; atributo `_LOCK` dá cor própria a franja/tufo/prendedor no shader de lava).
5. `make_valeria_skirt.py` — 3 camadas de babado, justa na cintura e sempre por fora do corpo.
6. `make_valeria_fist.py --pele "#c98a5e"` — punhos fechados com furo para o cabo.
7. `retarget_mixamo.py corpo.glb samba_dancing.fbx anim_valeria_samba.glb --nome samba --passo 1 --suave 2`.
No visualizador (`CHARS.valeria`): rosto em código (`eyeStyle: 'valeria'`: olhos, boca e nariz separados, expressões
próprias), guarda-chuvas `makeUmbrella()` (abrem como escudo na guarda, fecham no soco), shader de lava com fluxo e
contorno por casco invertido, saia com cápsula das coxas no shader, dança contida (botão Dança e na passada lateral).

### 14.2 Lições novas
- **Folha limpa = modelo limpo**: braços afastados, sem manga, careca e rosto liso. Cabelo, roupa solta e acessórios
  em código saem melhores que do Meshy e não custam crédito.
- **Rosto em código** quando o sprite não encaixa na cabeça do modelo: cada olho e a boca em peças separadas.
- **Saia/pano solto**: além de sair do corpo no Blender, empurrar o pano para fora das coxas no shader (cápsula
  quadril→joelho, cedendo aos poucos) resolve qualquer animação.
- **Animações do Mixamo** (`retarget_mixamo.py`): rotação de cada osso no mundo relativa ao descanso (T-pose → A-pose).
  Exportar SÓ a ação nova (o fbx e o rig trazem outras), sinal dos quaternions contínuo, loop com o 1º quadro repetido
  no fim. Para luta, misturar só quadril/tronco com a guarda e limitar o giro do quadril pelo lado curto do quaternion.
- **Metaball elipsoide no Blender**: `size_x/y/z` multiplica o raio; o limiar baixo (0,15) deixa a forma ~0,78 do raio.

### 14.3 Valéria: o que ainda falta
- Golpes de assinatura (rodada de golpes).
- Escultura do rosto (nariz, maçãs, queixo) e luz de lava no rosto — proposta, ainda não feita.
- Variante de magma escura/rachada do sprite (troca de cor no shader de lava).
