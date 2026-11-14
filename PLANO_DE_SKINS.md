# 🥊 K.O.-RAMA — Plano Diretor Oficial de Skins e Paletas do Elenco

> **Documento Oficial de Especificação de Arte, Conceito, Estados Visuais e Prompts de Geração**  
> *Versão: 4.1 (Refinamento do Jogador: Valéria Mary Poppins com Cel Animation/Outlines Finas/Mais Frames, Joana Pré-Maldição com Despertar Anglerfish, Cremoso Hippie Vegano, Marcov Vostok Yuri Gagarin e Vitória Primal Direto no Chão)*  
> *Data: 08 de Outubro de 2026*  
> *Destino: Uso pelo jogador e cópia de prompts para geração de assets com Claude.*

---

## 📌 1. Princípios Inegociáveis do Sistema de Skins

1. **Jogabilidade 100% Inalterada (Cosmética Pura):**
   - Hitboxes, alcances de golpe, velocidades de animação, janelas de esquiva e atributos numéricos são rigorosamente **idênticos**.
   - As skins não concedem vantagens nem desvantagens competitivas.
2. **Estrutura de 4 Visuais por Personagem (Base + 3 Novas):**
   - **Skin Base (Original do Jogo Atual):** A aparência canônica já implementada nos spritesheets e rigs do motor C++.
   - **Skin 1 (Nova Skin 1):** Primeiro novo conceito temático forte e consagrado (ex.: o clássico Traje de Apicultor completo para Jack, Vivaldi Humana Viva, Mary Poppins para Valéria, Marcov das Cavernas).
   - **Skin 2 (Nova Skin 2):** Segundo conceito temático com alto contraste visual e estilístico.
   - **Skin 3 (Nova Skin 3):** Terceiro conceito temático de alta criatividade, lendário ou comemorativo.
3. **Legibilidade Obrigatória de Estados Visuais (Gameplay Readability):**
   - Cada skin deve preservar **marcos visuais inconfundíveis** para que o jogador adversário sempre saiba instantaneamente qual habilidade ou estado está ativo:
     - **Jack:** Modo Melaço (chapéu inclinado com sombra dramática e olhos ciano, mel escorrendo), Pele de Mel (brilho âmbar dourado translúcido) e Nocaute (olhos XX).
     - **King Frog:** Papo inflável de 4 estágios, Pele Venenosa (pele escura borbulhante e toxina fosforescente), Salto Esmagador (squash/achatamento elástico no chão) e estado Sem Coroa (careca lisa).
     - **Joana:** Braços assimétricos (arpão/gancho e garra titânica), Barreira de Cracas e no Modo Monstruoso (< 40 HP na Skin 1) a capa rasga com patas de caranguejo irrompendo, olhos cegos acesos de peixe-pescador (*anglerfish*) e dentes afiados.
     - **O Cremoso (Legibilidade Rigorosa):**
       - *Modo Derretido (Melt):* O corpo SEMPRE amolece e liquefaz, escorrendo poça contínua viscosa no chão.
       - *Estamina Fria (Frozen Stamina):* O corpo SEMPRE congela em bloco duro com geada cristalina e **OS DOIS TUBETES/CANUDINHOS DE WAFER NA NUCA** (marco visual obrigatório e inconfundível!).
       - *Vingança Doce (Sweet Revenge):* SEMPRE calda escura fervendo borbulhante e olhos brilhando de raiva com vapor de açúcar.
     - **Valéria:** Dançarina acrobática com sombrinha e saia; Skin Mary Poppins com **outlines mais finas de cel animation e alta densidade de quadros**; Modo Magma Frio (fuligem londrina de chaminé *Step in Time* na Mary Poppins e basalto negro nas demais).
     - **Maestra Vivaldi:** Modo Quatro Estações (transição cromática elemental entre Primavera, Verão, Outono e Inverno).
     - **Lady Espinha:** O visual original **já é** a Luchadora de Mandacaru inspirada em Frida Kahlo; as 3 novas skins trazem conceitos inéditos (El Manto de Oro com Capa Longa de Campeã, La Catrina do Día de los Muertos e Guerreira Jaguar Asteca).
     - **Dr. Marcov:** Cauda funcional que se move fora do traje; Escudo Orbital (Cápsula Vostok 1 soviética em órbita na Skin Gagarin, meteoro com marcas rupestres na Skin Primitiva, Sputnik na base), Gravidade Zero e Raio Orbital.
     - **Batalhator 2000:** Fases 1 e 2 no mecha industrial bruto (serras circulares, bateria 12V com fita silver tape, fios soltando faíscas); Fase 3 com o Nerd Piloto em pessoa ejetado do robô (camisa polo, óculos com esparadrapo, fones gamer e suando em bicas).
4. **Sistema de Assinaturas Temáticas por Skin (Victory & Face-Off):**
   - Cada skin ganha **uma pose de vitória única (Assinatura de Vitória)**, **um corte de olhos dramático exclusivo no Face-Off do início do round**, e **uma frase de vitória comemorativa temática**!

---

## 🐝 2. JACK — MÃOS DE COLMEIA

### 🔍 Skin Base (Original Atual do Jogo)
- **Descrição Canônica:** Jovem combatente florestal das colmeias. Túnica de algodão verde oliva (`#769a55`), capa/manto verde floresta escura (`#2b4528`), mangas e pernas verdes claras (`#dcedc1`), botas/caneleiras de trabalho com presilhas verdes escuras (`#41643c`). Chapéu de palha com aba larga (`#e5cc5b`) e véu leve caindo atrás. Luvas de boxe amarelas com favos de mel dourados (`#ffd700`).

---

### Skin 1 (Nova) — Traje de Apicultor Profissional (Master Hivekeeper)
- **Conceito:** O autêntico macacão integral de apicultor profissional de alta resistência, desenhado para lidar com os enxames mais agressivos do mundo.
- **Cores Principais:** Branco linho imaculado (`#f8fafc`), amarelo pólen vivo (`#facc15`), cinza tela de aço (`#94a3b8`) e preto borracha (`#18181b`).
- **Traje & Cabeça:**
  - Macacão integral branco espesso com zíper frontal reforçado no pescoço e elásticos nas barras das calças e mangas.
  - **Máscara esférica fechada de apicultor profissional com tela metálica protetora cinzenta** cobrindo 360° da cabeça, permitindo ver a silhueta do rosto e os olhos de Jack sob a malha.
  - Luvas grossas de borracha vulcanizada amarela de apicultura subindo até os cotovelos.
  - Fumigador clássico de fumaça de apicultor de fole e latão preso ao cinto de lona.
- **Adaptação de Estados:**
  - *Modo Melaço:* O macacão branco fica **completamente ensopado e manchado de mel âmbar escuro escorrendo em grossas poças**, e os olhos brilham em ciano intenso (`#00f5d4`) através da malha de aço da máscara!
  - *Pele de Mel:* A lona branca do macacão absorve o néctar e passa a **brilhar em âmbar dourado translúcido reluzente** com gotas viscosas.
  - *Enxame de Abelhas:* Abelhas operárias zunindo em nuvem densa com fumaça aromática de apicultura saindo do fumigador.
- **Assinatura Temática:**
  - *Pose de Vitória:* Aciona o fumigador no ar com baforada aromática e abre os braços enquanto o enxame cobre a tela em reverência.
  - *Quote:* *"Até a abelha mais doce sabe a hora exata de ferroar!"*

---

### Skin 2 (Nova) — Sir Jack, O Cavaleiro do Favo (Knight of the Comb)
*⭐ Conceito Especial do Jogador*
- **Conceito:** Nobre paladino medieval jurado à Rainha das Abelhas, forjado em aço e cera sagrada.
- **Cores Principais:** Aço polido escuro (`#475569`), ferro prateado (`#cbd5e1`), ouro heráldico (`#eab308`) e veludo carmesim (`#991b1b`).
- **Traje & Cabeça:**
  - Elmo de cavaleiro medieval fechado com **viseira de grade de ferro forjado entrelaçada** (releitura metálica da rede de apicultor).
  - Peitoral de aço com relevo de favos hexagonais e brasão heráldico da Abelha Rainha.
  - Ombreiras de placas articuladas e manoplas de aço com rebites dourados.
  - Capa de veludo carmesim com bordas desgastadas presa por broches de favo.
- **Adaptação de Estados:**
  - *Modo Melaço:* A fresta do elmo brilha em ciano místico; a armadura fica banhada por **cera de vela de catedral e breu fervente** cor de âmbar escuro com gotas incandescentes no chão.
  - *Pele de Mel:* A armadura de aço transmuta em **ouro maciço líquido cintilante**, brilhando como uma relíquia sagrada.
  - *Enxame de Abelhas:* Abelhinhas medievais usando **elmos minúsculos com lanças pontiagudas em riste** e estandartes microscópicos!
- **Assinatura Temática:**
  - *Pose de Vitória:* Crava a manopla de aço no tablado e ajoelha-se como cruzado em oração à Abelha Rainha.
  - *Quote:* *"Pela Rainha, pelo Favo e pela Honra do Enxame!"*

---

### Skin 3 (Nova) — Jack Bio-Hazard / Mech-Hornet (Cyberpunk 2099)
- **Conceito:** Engenheiro bio-cibernético de contenção de vespas geneticamente modificadas em laboratório de alta tecnologia.
- **Cores Principais:** Preto fosco tático (`#09090b`), amarelo de perigo radioativo (`#facc15`) e ciano neon (`#00f5d4`).
- **Traje & Cabeça:**
  - Traje hazmat pressurizado de borracha vulcanizada preta com faixas amarelas de advertência "BIOHAZARD".
  - Máscara de gás cibernética com visor hexagonal ciano neon pulsante e respiradores laterais duplos.
  - Mochila traseira com tanques cilíndricos de feromônio líquido âmbar iluminados internamente por LEDs.
  - Manoplas com pistões hidráulicos e agulhas retráteis de injeção.
- **Adaptação de Estados:**
  - *Modo Melaço:* O visor escurece e os olhos do visor piscam em vermelho de perigo; os tanques traseiros vazam **resina química luminescente** que goteja no chão.
  - *Pele de Mel:* O exoesqueleto sobrecarrega com **escudo iônico de plasma hexagonal amarelo-ouro**.
  - *Enxame de Abelhas:* Micro-drones voadores mecânicos (*Hornet-Drones*) com hélices giratórias e lasers de mira.
- **Assinatura Temática:**
  - *Pose de Vitória:* Digita comandos no braço mecânico e uma revoada de drones forma um holograma hexagonal gigante piscando "QUARANTINE CLEARED".
  - *Quote:* *"Amostra contida. Nível de ameaça reduzido a zero."*

---

## 🐸 3. KING FROG — O REI DA LAMA

### 🔍 Skin Base (Original Atual do Jogo)
- **Descrição Canônica:** Sapo antropomórfico verde vivo (`#769a55`), olhos esbugalhados com expressão furiosa, papo verde menta claro expansível em 4 estágios (`#dcedc1`). Coroa real de ouro pontiaguda com veludo roxo imperial (`#6b2c85`) e rubi lapidado central (`#ac3232`) *(destacável, que voa revelando a carequinha verde fofinha!)*. Shorts de boxe verde escuro floresta (`#2b4528`) com fivela dourada (`#e5cc5b`). Pés de sapo palmados e alaranjados com membranas (`#d17b24`).

---

### Skin 1 (Nova) — Dom Sapone, O Mafioso da Lama (The Frogfather)
- **Conceito:** Chefão da máfia do submundo que controla as docas clandestinas e o contrabando do porto lamacento.
- **Cores Principais:** Terno risca de giz preto carvão (`#18181b` / `#475569`), branco linho (`#f8fafc`) e vermelho sangue (`#dc2626`).
- **Traje & Cabeça:**
  - No lugar da coroa tradicional, um elegante **chapéu fedora gangster inclinado** na cabeça com fita roxa *(que também voa deixando-o carequinha quando golpeado!)*.
  - Paletó risca de giz com lapela pontuda, camisa social branca, gravata borboleta preta e cravo vermelho na lapela.
  - Charuto cubano aceso no canto da bocarra soltando anéis de fumaça a cada respiração.
- **Adaptação de Estados:**
  - *Pele Venenosa:* O sapo solta uma **névoa densa de fumaça de charuto e pólvora** com notas de dólar flutuando ao redor do corpo.
  - *Golpe de Língua:* A língua estica como uma **fita zebrada amarela e preta policial** ("CRIME SCENE") ou tira de couro de chicote.
  - *Gêiser de Lama:* O solo racha como uma **tampa de bueiro estourando**, jorrando vapor d'água de cidade e água suja de asfalto.
- **Assinatura Temática:**
  - *Pose de Vitória:* Ajusta a aba do fedora com o polegar, solta uma baforada densa de fumaça de charuto e atira uma moeda de ouro no peito do oponente caído.
  - *Quote:* *"Você veio ao meu ringue no dia do casamento da minha filha..."*

---

### Skin 2 (Nova) — Imperador Sapo do Coliseu (Gladiador Romano)
- **Conceito:** Campeão invicto da arena de gladiadores romanos, idolatrado pela plebe do império anfíbio.
- **Cores Principais:** Bronze polido (`#b45309`), púrpura imperial romano (`#581c87`), marfim (`#fef3c7`) e ouro nobre (`#eab308`).
- **Traje & Cabeça:**
  - No lugar da coroa, uma **coroa de folhas de louro de ouro maciço** com fitas púrpuras caídas na nuca.
  - Peitoral de gladiador de bronze trabalhado em relevo anatômico com ombreira única de gladiador (*galerus*).
  - Toga curta romana púrpura debruada a ouro sobre o abdômen e sandálias romanas de tiras de couro amarradas nas patas palmadas.
- **Adaptação de Estados:**
  - *Pele Venenosa:* **Somente a pele do sapo escurece e muda de cor**, soltando miasmas tóxicos. Peitoral, ombreira, caneleiras, placas das luvas, louros e tecidos mantêm suas cores normais; não há oxidação da armadura.
  - *Salto Real Esmagador:* Desaba como uma coluna de mármore do Coliseu, quebrando o solo em mosaicos romanos rachados.
- **Assinatura Temática:**
  - *Pose de Vitória:* Faz o gesto clássico romano de polegar para baixo (*Pollice Verso*) e ergue o braço sob chuva de louros e aclamação da multidão.
  - *Quote:* *"Veni, Vidi, Vici... e o pântano agora é meu império!"*

---

### Skin 3 (Nova) — Sapo Dardo Xamânico (Toxic Dartfrog & Cocar Asteca)
- **Conceito:** Inspirado no sapo dardo mais letal da floresta tropical (*Dendrobates*), venerado como divindade xamânica da selva.
- **Cores Principais:** Azul cobalto profundo (`#1d4ed8`), manchas pretas (`#09090b`), amarelo fluorescente (`#facc15`) e verde jade sagrado (`#059669`).
- **Traje & Cabeça:**
  - Pele com a coloração de aviso aposemática do sapo dardo: dorso cobalto com manchas pretas e barriga amarela fluorescente.
  - No lugar da coroa, um majestoso **cocar cerimonial asteca de penas de quetzal verdes e azuis com adorno central de pedra jade**.
  - Pinturas faciais geométricas de guerra brancas e vermelhas; calção tribal em tecido com motivos de serpente emplumada.
- **Adaptação de Estados:**
  - *Pele Venenosa:* Runas místicas bioluminescentes astecas e pólen tóxico verde flutuam ao redor da pele.
  - *Golpe de Língua:* Língua bicolor azul e amarela reluzente com secreção paralisante.
  - *Gêiser de Lama:* Coluna de água sagrada turquesa cristalina com poeira de ouro cerimonial e incenso copal.
- **Assinatura Temática:**
  - *Pose de Vitória:* Dá um salto acrobático estufando o papo azul e pousa em cócoras com os olhos reluzindo em verde-jade sagrado.
  - *Quote:* *"Um único toque da minha pele adormece o coração de um titã."*

---

## ⚓ 4. JOANA — CARA DE CRACA

### 🔍 Skin Base (Original Atual do Jogo)
- **Descrição Canônica:** Pirata mutante meio humana, meio crustáceo! Lado humano: pele morena bronzeada (`#c8815f`), dreadlocks pretos com anéis de ouro, tapa-olho pirata preto, colete de capitão preto aberto com detalhes em ouro (`#22252a`) sobre ataduras de marinheiro no peito (`#f3efe0`), shorts náutico desgastado listrado em vermelho e bege (`#8d2b2b` / `#d3c89d`), perna de pau de madeira nobre talhada com anéis dourados (`#7a472d` / `#f4be41`). Lado crustáceo: carapaça alaranjada de caranguejo (`#e65c00`), pedúnculo com olho de siri espetado para cima (`circle cx="75" cy="8"`). Braço esquerdo com gancho de ferro pesado (`#cbd2d0`); braço direito com **GARRA/PINÇA COLOSSAL DE CARANGUEJO** com dentes afiados de coral calcificado e cracas.

---

### Skin 1 (Nova) — Pescadora do Mar do Norte / Joana Humana Pré-Maldição (The Harpooner Before the Curse)
*⭐ Refinamento do Jogador (Versão Humana Pré-Maldição & Despertar Abissal)*
- **Conceito:** A lenda viva antes do abismo! Joana em seus dias de capitã e arpoadora humana destemida do Ártico, antes de ser tocada e deformada pela maldição dos monstros do oceano profundo.
- **Cores Principais:** Amarelo impermeável encerado (`#eab308`), azul marinho tempestade (`#0f172a`), vermelho caranguejo-real (`#991b1b`), couro cru envelhecido (`#78350f`) e aço forjado (`#475569`).
- **Traje & Cabeça (Forma Humana Normal):**
  - **Rosto 100% humano e saudável!** Pele morena bronzeada e curtida pelo sol e salitre, traços firmes e expressivos, **sem dente de ouro podre, sem deformações faciais e sem o pedúnculo ocular de siri**.
  - **Possui os dois olhos humanos normais e SEM tapa-olho**, mas a **aba frontal baixa do chapéu oculta completamente os olhos** na forma humana, mantendo a boca visível e criando uma silhueta dramática e imponente.
  - Grande **capa de chuva impermeável amarela de alta resistência, FECHADA NA PARTE SUPERIOR COBRINDO O PEITO com botões pretos firmes, e com os BOTÕES INFERIORES ABERTOS**, transformando a peça em uma capa náutica longa que esvoaça dramaticamente para trás a cada esquiva e golpe!
  - Chapéu clássico de pescador sou'wester amarelo impermeável com aba protetora estendida na nuca.
  - Empunha um pesado **arpão de pesca baleeira de ferro forjado com farpa dupla** na mão esquerda e um **anzol de pesca grande na mão direita humana**, sem manopla nem prótese. A manga junto ao arpão é amarela, sem trecho azul de braço.
  - De costas, os dreadlocks ficam **sobre a cabeça e sob o chapéu**. A calça fica sempre **por baixo da capa**, visível apenas pela abertura inferior e abaixo da barra.
- **Adaptação de Estados:**
  - *Barreira de Cracas:* Finas crostas de salitre, placas de gelo ártico e pequenas cracas brancas se cristalizam discretamente sobre a lona amarela da capa.
  - *Modo Monstruoso (< 40 HP — O Despertar Violento da Maldição):*
    - A dor do combate desperta a terrível maldição adormecida nas profundezas!
    - **A capa de chuva amarela se abre mostrando o físico ultra definido e musculoso**, mas **também se rasga violentamente em diferentes pontos**, por onde **irrompem patas pontiagudas de caranguejo, crostas duras de cracas calcificadas e anêmonas marinhas pulsantes**!
    - Por trás da sombra do chapéu amarelo, **os olhos se acendem redondos, brancos e cegos como os de um peixe-pescador abissal (*anglerfish*)**!
    - A boca se escancara em uma fúria bestial com **dentes sobressalentes e afiados pontiagudos como presas de coral e tubarão**!
    - O braço direito se transfigura na carapaça rubi titânica de King Crab colossal com espigões de gelo.
  - *Impacto Coralino:* Erupção violenta de espigões de gelo salgado e dentes de coral calcificado.
- **Assinatura Temática:**
  - *Pose de Vitória:* Limpa a água salgada do rosto humano com o antebraço, drena a chuva da aba do chapéu amarelo e crava o arpão baleeiro no tablado com o vento uivando na capa esvoaçante.
  - *Quote:* *"O mar bravio do norte nunca me engoliu... e não vai ser você quem vai me arrastar pro fundo!"*

---

### Skin 2 (Nova) — Almirante do Abismo / Fantasma do Davy Jones
- **Conceito:** Joana amaldiçoada pelas fossas oceânicas mais profundas, tornando-se uma entidade espectral feita de coral negro e bioluminescência abissal.
- **Cores Principais:** Azul marinho abissal (`#020617`), verde espectral ectoplásmico (`#2dd4bf` / `#0d9488`), carapaça de lagosta azul real (`#1d4ed8`) e madrepérola escura (`#475569`).
- **Traje & Cabeça:**
  - Casaca de almirante naval desgastada em azul-noite com galões fantasmagóricos que brilham em ciano.
  - O lado caranguejo possui a textura vítrea e iridescente de uma **lagosta azul profunda rara**; o olho em pedúnculo pulsa como uma lanterna de peixe-pescador.
  - Dreadlocks transformados em filamentos de algas marinhas brilhantes e tentáculos minúsculos de anêmona.
  - Gancho de pirata esculpido em **coral negro afiadíssimo** com ponta de arbalete.
- **Adaptação de Estados:**
  - *Barreira de Cracas:* Anêmonas do mar bioluminescentes ciano e pequenas águas-vivas brilhantes cobrem o peito e as costas.
  - *Modo Monstruoso:* A pinça vira uma **massa titânica de mandíbulas de abismo e tentáculos com ventosas brilhantes**.
  - *Impacto Coralino:* Erupção de corais negros pontiagudos brilhando com fosforescência azul-marinha.
- **Assinatura Temática:**
  - *Pose de Vitória:* Bate o gancho de coral negro contra a pinça monstruosa com eco oco e afunda parcialmente numa poça de água negra bioluminescente.
  - *Quote:* *"Você agora pertence ao armário do Davy Jones..."*

---

### Skin 3 (Nova) — Joana Mecânica / Steampunk Dredger
- **Conceito:** Corsária industrial com próteses mecânicas a vapor construídas a partir de peças de escavadeiras submarinas e caldeiras de navio.
- **Cores Principais:** Latão polido (`#f59e0b`), bronze oxidado com azinhavre (`#0f766e`), couro marrom graxa (`#78350f`) e ferro fundido rebitado (`#334155`).
- **Traje & Cabeça:**
  - O pedúnculo ocular é um **periscópio de latão com lentes ópticas múltiplas e manômetro de pressão**.
  - O lado crustáceo é uma **carcaça de blindagem de cobre rebitada** com tubos de vapor e engrenagens de transmissão expostas.
  - A pinça monstruosa vira uma **garra mecânica hidráulica de dragagem industrial** com dentes de aço temperado e pistões pneumáticos a vapor!
  - Perna de pau substituída por um **amortecedor pneumático de alta pressão com mola de aço espiral funcional**.
  - Colete de couro reforçado de mergulhador escafandrista com fivelas de bronze.
- **Adaptação de Estados:**
  - *Barreira de Cracas:* Placas de blindagem de caldeira rebitadas se fecham sobre o torso, soltando jatos de vapor sob pressão.
  - *Modo Monstruoso:* O motor a vapor da garra entra em sobrecarga, girando engrenagens em brasa e soltando fumaça preta de carvão!
- **Assinatura Temática:**
  - *Pose de Vitória:* Puxa a alavanca da garra mecânica soltando um apito ensurdecedor de vapor náutico com engrenagens girando em alta rotação.
  - *Quote:* *"Potência máxima de caldeira. O fundo do mar é nosso depósito de sucata!"*

---

## 🍨 5. O CREMOSO — SORVETEIRO DOCE

### 🔍 Skin Base (Original Atual do Jogo)
- **Descrição Canônica:** Gigante esculpido em massa cremosa ondulada de sorvete de baunilha suave (`#fef08a`), com camadas arredondadas e dobras macias de creme. Ombreiras feitas com fatias grossas de morango fresco suculento (`#shoulder-strawberry`). Avental branco clássico de confeiteiro com laço amarrado na nuca e nas costas (`#f8fafc`). Chapéu tradicional de chef patissier (*toque blanche*) no topo da cabeça, encimado por uma **cereja vermelha brilhante com cabinho verde**.

---

### Skin 1 (Nova) — Cremoso Matcha Zen / Hippie Vegano & Chás Orgânicos (The Vegan Zen Hippie)
*⭐ Refinamento do Jogador (Estética Hippie Vegano & Filosofia Orgânica)*
- **Conceito:** O mestre zen da confeitaria botânica e da cultura hippie vegana! 100% plant-based, cruelty-free, sustentável e em profunda sintonia cósmica com a natureza.
- **Cores Principais:** Verde matcha suave (`#84cc16` / `#65a30d`), creme de leite vegetal de aveia (`#fef3c7`), terra crua e cânhamo (`#78350f`) e vermelho framboesa orgânica (`#9f1239`).
- **Traje & Cabeça (O Visual Hippie Vegano):**
  - Corpo modelado em sorbet aveludado de chá verde matcha orgânico cerimonial e leite de amêndoas sustentável.
  - Usa **óculos redondinhos retrô estilo John Lennon / Mahatma Gandhi** com armação fina de arame dourado sobre os olhos calmos de creme.
  - **Faixa de cabeça rústica tecida em macramê e fibra de cânhamo cru**, adornada com contas de madeira de reflorestamento, prendendo um coque zen relaxado no alto da cabeça.
  - **Colar comprido de sementes naturais e contas de madeira aromática de sândalo** caindo sobre o peitoral largo.
  - Avental ecológico feito de **linho cru rústico e algodão reciclado bege (`#fef3c7`)**, com um delicado **bordado de folhinha verde "VEGAN FRIENDLY"** no peito.
  - Ombreiras feitas com **ramos frescos de hortelã orgânica, fatias suculentas de babosa (aloe vera) e framboesas silvestres de colheita consciente**.
  - No topo da cabeça, um raminho perfumado de alecrim silvestre fresco no lugar da cereja tradicional.
- **Legibilidade Rigorosa de Estados:**
  - *Modo Derretido (Melt):* O sorbet vegetal amolece claramente em uma **calda semilíquida verde esmeralda de matcha com leite de coco**, escorrendo em poça luminosa pelo chão com aroma calmante de chá verde.
  - *Estamina Fria (Frozen Stamina):* O corpo cristaliza imediatamente em bloco rígido de raspadinha de gelo verde com névoa fria aromática e **OS DOIS TUBETES/CANUDINHOS DE WAFER DE CANELA OU BAMBU NA NUCA** (marco visual 100% legível e obrigatório!).
  - *Vingança Doce:* O matcha ferve intensamente, borbulhando calda quente de feijão azuki orgânico com fumaça aromática de incenso natural de sândalo e olhos faiscando de revolta cósmica vegana!
  - *Estocada de Casquinha:* A luva vira uma casquinha artesanal de waffle integral crocante feita com farinha de grão-de-bico e carvão ativado.
- **Assinatura Temática:**
  - *Pose de Vitória:* Senta-se em posição de lótus serena no tablado, bate o batedor de bambu (*chasen*), respira fundo e junta as mãos em reverência cósmica *Namastê* soltando uma baforada de incenso sobre o adversário estirado.
  - *Quote:* *"Respire fundo, sinta a energia da Terra... seu nocaute foi 100% natural, orgânico e sem crueldade."*

---

### Skin 2 (Nova) — Cremoso Napolitano / Três Sabores (Gourmet Neapolitan)
- **Conceito:** O clássico e inconfundível sorvete napolitano em camadas nobres: Chocolate Trufado, Morango Silvestre e Baunilha de Madagascar.
- **Cores Principais:** Chocolate meio amargo trufado (`#3b1a08`), morango rosa vibrante (`#f43f5e`) e baunilha dourada (`#fef08a`).
- **Traje & Cabeça:**
  - Mantém a silhueta clássica do sorveteiro com divisões nítidas: base de chocolate escuro, tronco de morango rosa e cabeça de baunilha pura!
  - Ombreiras feitas de **biscoitos de chocolate recheados crocantes (estilo Oreo)**.
  - Avental marrom de barista gourmet com costuras douradas.
  - No topo, uma trufa de chocolate belga redonda com folha de ouro.
- **Legibilidade Rigorosa de Estados:**
  - *Modo Derretido (Melt):* As três camadas amolecem e misturam-se em um **redemoinho marmorizado tricolor líquido escorrendo em poça quente**.
  - *Estamina Fria (Frozen Stamina):* Congela instantaneamente com geada azulada e **DOIS TUBETES BICOLORES DE CHOCOLATE BRANCO E PRETO ESPETADOS NA NUCA**.
  - *Vingança Doce:* Calda vermelha de morango escuro ferve com vapor doce e olhos de fúria.
- **Assinatura Temática:**
  - *Pose de Vitória:* Retira a trufa de chocolate do topo, joga-a para o alto com efeito circense e a apanha na colher dourada de confeiteiro.
  - *Quote:* *"Três sabores, três rounds, um nocaute doce e perfeito!"*

---

### Skin 3 (Nova) — Cremoso Sundae de Festa / Confeito de Circo (Carnaval de Algodão Doce)
- **Conceito:** Sorvete festivo infantil de algodão doce e tutti-frutti com confeitos crocantes coloridos.
- **Cores Principais:** Azul algodão doce suave (`#38bdf8`), rosa tutti-frutti (`#f472b6`), branco glacê (`#f8fafc`) e confeitos multicoloridos (`#facc15`, `#4ade80`, `#a855f7`).
- **Traje & Cabeça:**
  - Corpo de creme fofo em espiral bicolore azul e rosa pastel salpicado por granulados de arco-íris.
  - Ombreiras de pirulitos circulares espiralados coloridos.
  - Avental listrado de vendedor de parque de diversões vintage em branco e vermelho carrossel.
  - No topo, um suspiro gigante espiralado dourado no lugar da cereja.
- **Legibilidade Rigorosa de Estados:**
  - *Modo Derretido (Melt):* O sorvete liquefaz claramente em **calda líquida azul-celeste brilhante escorrendo no chão**.
  - *Estamina Fria (Frozen Stamina):* O corpo vira picolé duro congelado com geada fosca e **DOIS TUBETES DE WAFER LISTRADOS DE FESTA ESPETADOS NA NUCA**.
  - *Vingança Doce:* Calda de framboesa avermelhada ferve borbulhando com fumaça de açúcar quente e olhos de raiva.
- **Assinatura Temática:**
  - *Pose de Vitória:* Gira o corpo criando uma explosão festiva de confetes de açúcar coloridos com corneta de festa soprando.
  - *Quote:* *"A festa acabou e sobrou só você derretido na lona!"*

---

## 🌋 6. VALÉRIA VULCÂNICA — A TITÃ DE LAVA

### 🔍 Skin Base (Original Atual do Jogo)
- **Descrição Canônica:** A autêntica dançarina de frevo vulcânica de Pernambuco! Top cropped de frevo laranja com debrum amarelo (`#ff4500` / `#facc15`). **Saia de frevo em camadas com babados triangulares em zigue-zague** típicos do Carnaval: camada de cima laranja, camada do meio amarela, camada inferior azul-turquesa carnavalesco vivo (`#2dd4bf` / `#0f766e`). Sapatilhas de passista baixas turquesa com fivelas amarelas. Pele morena bronzeada quente (`#a35d3b`) com **tatuagens tribais rituais de cinzas vulcânicas escuras** (`--ash-tattoo: #3d3634`) descendo pela barriga, pernas e costas. Cabelo volumoso esculpido em **labaredas de fogo vulcânico vivo** em 4 frames animados com franja de chamas na testa. Sombrinha de frevo de ferro fundido incandescente e luvas de magma. Modo Magma Frio em rocha de basalto negro e pedra-pomes com fissuras de brasa laranja e saia cinza-ardósia.

---

### Skin 1 (Nova) — Rainha do Maracatu & Frevo de Olinda (Galo da Madrugada)
- **Conceito:** A celebração máxima da realeza do Carnaval nordestino! Mistura a nobreza dos estandartes de Maracatu Nação com a energia acrobática do Frevo tradicional de Olinda.
- **Cores Principais:** Ouro real cintilante (`#facc15`), azul royal profundo (`#1e40af`), vermelho veludo (`#dc2626`) e verde esmeralda carnavalesco (`#16a34a`).
- **Traje & Cabeça:**
  - Cabelo de fogo com labaredas estilizadas coroadas por uma **tiara de pedrarias douradas e espelhos de Maracatu**.
  - Top de veludo azul royal com uma **gola ricamente bordada de lantejoulas douradas brilhantes** que refletem a iluminação dos refletores.
  - Saia de frevo multicolorida em quatro camadas de cetim (azul, amarelo, vermelho e verde) com **fitas longas de cetim coloridas esvoaçantes penduradas**, que dançam em arcos vibrantes a cada esquiva e golpe!
  - Sombrinha de frevo tradicional de quatro cores com ponteira e cabo de metal dourado reluzente.
- **Adaptação de Estados:**
  - *Modo Magma Frio:* As lantejoulas e espelhos viram **pedras de obsidiana negra polida** e as fitas de cetim transformam-se em **tiras de cinzas fumegantes e brasas vivas** que flutuam ao redor da lutadora!
  - *Dança das Brasas:* As fitas de cetim deixam rastros luminosos dourados e faíscas no chão da arena.
  - *Estocada de Lava:* A sombrinha dourada abre como uma roda de fogo de artifício de festa popular!
- **Assinatura Temática:**
  - *Pose de Vitória:* Dá um salto acrobático tesoura de frevo, pousa na ponta dos pés e gira a sombrinha dourada no ar com serpentinas e confetes caindo em festa de Olinda.
  - *Quote:* *"No passo do frevo e no calor do Galo, ninguém fica de pé na minha frente!"*

---

### Skin 2 (Nova) — Mary Poppins / A Governanta Mágica do Guarda-Chuva
*⭐ Ideia do Jogador (Estilo Cel Animation Clássico, Bordas Finas & Mais Quadros)*
- **Conceito:** Homenagem irreverente inspirada em Mary Poppins e suas aventuras com o guarda-chuva voador! Uma refinada governanta vitoriana que esconde uma acrobata formidável.
- **🎨 Estilo de Animação & Traço Exclusivos (Quadro a Quadro dos Anos 60):**
  - **Mudança de Paradigma Gráfico:** Nesta skin, até o estilo de animação e traço do jogo se transforma! Em vez do traço padrão, **as bordas e linhas de contorno (outlines) ficam visivelmente mais finas e elegantes**, com textura de desenho animado clássico em celulóide (*cel animation*).
  - **Densidade Alta de Frames (Mais Quadros Desenhados):** A personagem conta com **uma média maior de quadros desenhados à mão** (em vez da movimentação procedural do rig), reproduzindo a fluidez orgânica e saltitante dos filmes clássicos de animação tradicional — exatamente como a Mary Poppins real quando entra e interage com o universo dos desenhos animados no clássico *Jolly Holiday*!
- **Cores Principais:** Azul marinho eduardiano (`#1e3a8a`), branco linho engomado (`#f8fafc`), vermelho carmim (`#dc2626`) e preto polido (`#0f172a`).
- **Traje & Cabeça:**
  - Chapéu preto clássico de aba curva adornado com pequenas margaridas brancas e cerejas vermelhas.
  - Sobretudo azul marinho elegante e acinturado com gola alta branca de camisa engomada e gravata borboleta vermelha de fita de cetim.
  - Saia longa eduardiana plissada e botinas pretas engraxadas de amarrar com salto baixo.
  - Na mão, o icônico **guarda-chuva preto clássico com cabo de papagaio entalhado em madeira falante**, que ela abre para planar nas esquivas e girar nos socos com rotação graciosa quadro a quadro!
- **Adaptação de Estados:**
  - *Modo Magma Frio:* O sobretudo, a pele e o rosto ganham **manchas escuras de fuligem de chaminé londrina** (tributo à clássica dança dos limpadores de chaminé *Step in Time*!), e o guarda-chuva fica chamuscado soltando brasas e fumaça de lareira!
  - *Dança das Brasas:* Flutua a alguns centímetros do chão segurando o guarda-chuva aberto enquanto o vento mágico sopra sob os pés com poeira cintilante mágica.
  - *Estocada de Lava:* Abre o guarda-chuva de repente em rotação veloz soltando fagulhas mágicas douradas e fumaça de chaminé!
- **Assinatura Temática:**
  - *Pose de Vitória:* Abre o guarda-chuva de papagaio, ajeita a gravata borboleta vermelha e flutua graciosamente para fora da tela segurando a saia enquanto o vento sopra suavemente.
  - *Quote:* *"Praticamente perfeita em todos os sentidos... especialmente no direto de direita!"*

---

### Skin 3 (Nova) — Bailarina de Flamenco das Chamas (Sevilha Caliente)
- **Conceito:** A paixão e o ritmo feroz do Flamenco espanhol andaluz, trocando a sombrinha carnavalesca por uma sombrinha de renda espanhola (*parasol*) e leques de fogo!
- **Cores Principais:** Vermelho rubi espanhol (`#b91c1c`), preto renda de Sevilha (`#09090b`), dourado castanhola (`#d97706`) e rosa fresca (`#e11d48`).
- **Traje & Cabeça:**
  - Cabelo preto azeviche preso em coque polido baixo com uma grande rosa vermelha fresca e uma **penteadeira alta tradicional espanhola de tartaruga e ouro (*peineta*)**.
  - Vestido de flamenco justo com decote ombro a ombro e **saia longa volumosa em babados recortados em camadas vermelhas e pretas (*bata de cola*)**, e xale de Manila bordado com franjas longas de seda nos ombros.
  - Na mão, empunha uma elegante **sombrinha de renda espanhola negra e vermelha com ponteira de ferro forjado**; nos punhos, castanholas de ferro que estalam faíscas a cada golpe e esquiva!
- **Adaptação de Estados:**
  - *Modo Magma Frio:* As rosas murcham em cinzas cinzentas e o vestido vermelho escurece para carvão queimado com brasas vivas nos babados.
  - *Dança das Brasas:* O sapateado flamenco (*taconeo*) estala chamas no chão da arena.
  - *Estocada de Lava:* A sombrinha de renda se abre e gira como um leque de fogo espanhol.
- **Assinatura Temática:**
  - *Pose de Vitória:* Dá um sapateado flamenco estrondoso no chão, estala as castanholas e estende a sombrinha de renda sobre o ombro com olhar altivo e rosa entre os dentes.
  - *Quote:* *"¡Olé! O ritmo do meu sangue arde mais forte que a sua guarda!"*

---

## 🎻 7. MAESTRA VIVALDI — A MUSICISTA ESPECTRAL

### 🔍 Skin Base (Original Atual do Jogo)
- **Descrição Canônica:** Fantasma aristocrática virtuosa do século XVIII flutuante (sem pernas humanas, cauda espectral ondulante em gradiente verde-ectoplasma translúcido `#52b788` / `#74c69d`). Pele esverdeada espectral translúcida. Monumental peruca empoada barroca com cachos brancos e verdes, com **notas musicais 8-bit (`♪ ♫ ♩`) em pixel art flutuando e orbitando a peruca a cada frame**. Casaca / Frac clássico violeta imperial profundo (`#7b2cbf` / `#9d4edd`), gola alta com debrum de ouro (`#ffb703`) e babado de renda branca no pescoço (`#f8fafc`). Batuta de cristal ectoplásmico. Modo Quatro Estações dinâmico.

---

### Skin 1 (Nova) — Vivaldi Humana / A Virtuosa de Veneza (The Living Virtuoso)
*⭐ Ideia do Jogador (Vivaldi Viva em Carne e Osso)*
- **Conceito:** Vivaldi em carne e osso no ápice de sua juventude e carreira no século XVIII em Veneza! Uma concertista viva, elegante e apaixonada.
- **Cores Principais:** Vermelho carmesim de gala (`#991b1b`), ouro veneziano polido (`#ca8a04`), marfim linho (`#fefce8`) e madeira nobre de violino (`#78350f`).
- **Corpo & Traje:**
  - **Pernas humanas reais** calçando meias de seda branca e sapatos aristocráticos de salto baixo com fivela de ouro (sem cauda de fantasma: ela pisa no chão com passos elegantes de dança de salão!).
  - Pele humana viva e saudável em tom mediterrâneo claro com bochechas coradas.
  - Cabelos naturais castanho-avermelhados presos em um refinado coque veneziano com laço de veludo carmesim (ou peruca aristocrática humana de época com cachos dourados).
  - Corpete e casaca aristocrática em veludo carmesim bordada a fios de ouro com mangas de renda pura;
  - Na mão esquerda, ela segura um autêntico **violino de madeira envernizada de mestre luthier italiano** e na direita o arco de crina de cavalo!
- **Adaptação de Estados:**
  - *Notas Musicais:* No lugar das notas 8-bit, **partituras douradas brilhantes e ondas sonoras acústicas refinadas** flutuam no ar geradas pela paixão de sua música viva!
  - *Modo Quatro Estações:* A iluminação de concerto foca nela e a orquestra muda de clima (pétalas de primavera, folhas douradas de outono, vento frio de inverno).
  - *Inverno Allegro:* O golpe com o arco do violino dispara um floreio dramático de palco com estalactites gélidas!
- **Assinatura Temática:**
  - *Pose de Vitória:* Coloca o violino sob o queixo, toca um arpejo virtuosístico triunfal de finalização e faz uma profunda reverência de palco enquanto buquês de rosas são atirados no ringue sob aplausos de teatro.
  - *Quote:* *"A verdadeira música não morre... ela pulsa viva em cada nota e em cada soco!"*

---

### Skin 2 (Nova) — Vivaldi Ópera Fantasma / La Scala Gótica (Diva da Noite)
- **Conceito:** Inspirada na grandiosidade dramática das grandes óperas italianas e no Fantasma da Ópera: diva espectral trágica sob a luz da lua de Milão.
- **Cores Principais:** Preto ébano da meia-noite (`#09090b`), carmesim sangue profundo (`#881337`), marfim de porcelana (`#f8fafc`) e ouro antigo (`#ca8a04`).
- **Traje & Cabeça:**
  - Meia-máscara veneziana gótica de porcelana polida branca e ouro cobrindo a metade direita do rosto.
  - A monumental peruca barroca é tingida em preto ônix acetinado, decorada com **rosas vermelhas e partituras antigas queimadas que flutuam ao redor**.
  - Casaca de veludo negro da meia-noite com forro de cetim carmesim vivo e brocados de renda preta gótica.
  - A cauda espectral se dissolve em névoa negra e carmesim com pequenas pétalas de rosa flutuando.
- **Adaptação de Estados:**
  - *Modo Quatro Estações:* A aura transiciona entre 4 temas de tragédias operísticas com iluminação de ribalta teatral dramática.
  - *Inverno Allegro:* O gelo torna-se **cristal de quartzo escuro fumê / vidro quebrado de candelabro de teatro**.
  - *Cadência Staccato:* Notas musicais carmesim e partituras esvoaçantes cortam o ar como navalhas.
- **Assinatura Temática:**
  - *Pose de Vitória:* Solta uma risada melodramática de ópera enquanto as velas do teatro se apagam e ela desaparece na névoa carmesim deixando apenas a máscara sobre o ringue.
  - *Quote:* *"O pano caiu para você. Bravo! Bravíssimo!"*

---

### Skin 3 (Nova) — Vivaldi Rainha do Gelo / Aurora Boreal Glacial
- **Conceito:** A personificação espectral ártica do movimento do *Inverno*, unindo o frio polar ao espetáculo luminoso da Aurora Boreal.
- **Cores Principais:** Azul glacial translúcido (`#38bdf8`), verde aurora boreal (`#34d399`), violeta etéreo (`#c084fc`) e branco geada pura (`#f8fafc`).
- **Traje & Cabeça:**
  - Casaca esculpida em tecido cristalino gélido que refrata ondas móveis de luz verde e violeta de Aurora Boreal por dentro do tecido.
  - Peruca de geada pura empoada com estalactites cristalinas agudas e **flocos de neve fractais geométricos iluminados flutuando ao redor da cabeça**.
  - A cauda espectral é uma névoa densa de vento polar cortante que congela o tablado por onde passa.
  - Batuta esculpida em uma agulha pontiaguda de gelo eterno de geleira milenar.
- **Adaptação de Estados:**
  - *Modo Quatro Estações:* O corpo cicla através de 4 fenômenos polares (Sol da Meia-Noite, Vento Solar, Neve Perpétua e Aurora Boreal Total).
  - *Inverno Allegro:* O golpe levanta um iceberg pontiagudo transparente cintilante.
- **Assinatura Temática:**
  - *Pose de Vitória:* Ergue a batuta de gelo para o céu e as cortinas de Aurora Boreal dançam no teto da arena congelando as cordas do ringue em cristais azuis.
  - *Quote:* *"O silêncio do gelo é a mais sublime das sinfonias."*

---

## 🌵 8. LADY ESPINHA — CACTUS LUCHADORA

### 🔍 Skin Base (Original Atual do Jogo)
- **Descrição Canônica:** A Luchadora de Mandacaru inspirada no folclore mexicano e na pintora Frida Kahlo! Pele de cacto verde esmeralda com textura de pequenos sulcos (`--cactus-skin: #00a86b`) e espinhos dourados brotando do corpo (`--gold-bud: #ffd600`). **Monocelha contínua marcante estilizada de Frida Kahlo** (`frida-w-brow`), olhos amarelos penetrantes com esclera preta. No alto da cabeça, uma **exuberante coroa de pétalas cor-de-rosa/flores de dália vivas** (`rearranged-petals-front`) com botão dourado central. Collant de lucha livre azul cerúleo/turquesa (`#00b4d8`) com vivos alaranjados e flores bordadas no peito (`frida-chest-flowers-rich`); avental frontal em azul e rosa mexicano com franjas de borlas douradas oscilantes (`gold-tassel-fringes`). Botas azuis de luchadora com cadarços dourados cruzados.

---

### Skin 1 (Nova) — El Manto de Oro / La Campeona Enmascarada
*⭐ Conceito Especial do Jogador*
- **Conceito:** A campeã absoluta dos ringues da Arena México! Homenagem aos maiores ícones da história da Lucha Libre clássica (como El Santo e Blue Demon), usando a lendária máscara completa fechada e a triunfal capa longa de campeã!
- **Cores Principais:** Ouro polido espelhado (`#ffd700`), marfim reluzente (`#fefce8`), preto couro de arena (`#0f172a`) e vermelho escarlate triunfal (`#dc2626`).
- **Traje & Cabeça:**
  - **Máscara completa clássica fechada de Lucha Libre** em ouro metálico e marfim cobrindo toda a face (sem monocelha exposta; apenas a boca firme à mostra e os olhos brancos iluminados através dos recortes clássicos em forma de chama da máscara!).
  - No alto da cabeça, os espinhos de cacto são estilizados como uma **crista nobre de águia asteca dourada**.
  - **UMA LONGA E MAJESTOSA CAPA DE CETIM CARMESIM COM FORRO DOURADO** presa aos ombros por broches de ouro maciço, que tremula e dança de forma dramática a cada esquiva, soco e no *Gran Salto Luchador*!
  - Collant de luta em couro preto polido com costuras douradas e o **cinturão de ouro maciço de campeã peso-pesado** afivelado na cintura.
  - Botas de couro preto amarradas com cadarços dourados até o joelho.
- **Adaptação de Estados:**
  - *Velo de las Espinas:* A capa carmesim e a máscara dourada resplandecem com **auréola de ouro puro espelhado**, refletindo flashes de fotógrafos da arena lotada!
  - *La Garra del Cacto:* Garras de águia imperial dourada rasgando o ar com brilho metálico.
  - *Gran Salto Luchador:* A capa abre-se como asas imensas de águia no ar antes do impacto devastador!
- **Assinatura Temática:**
  - *Pose de Vitória:* Desprende o cinturão de ouro e o ergue bem alto com as duas mãos enquanto a longa capa carmesim dança ao vento sob chuva de confetes e flashes de imprensa esportiva.
  - *Quote:* *"¡Por el oro, por la gloria y por el honor de la máscara!"*

---

### Skin 2 (Nova) — La Catrina Mortífera / Día de los Muertos (La Calavera de Espinas)
- **Conceito:** A personificação mística da festa do Día de los Muertos! Elegância fúnebre barroca, calaveras tradicionais e flores de cempasúchil sagradas.
- **Cores Principais:** Branco osso polido (`#f8fafc`), preto charro fúnebre (`#09090b`), laranja cempasúchil ardente (`#ea580c`), violeta fúnebre (`#7e22ce`) e prata filigrana (`#cbd5e1`).
- **Traje & Cabeça:**
  - A pele de cacto ganha o tom de **osso polido branco**, com pintura facial tradicional de **caveira mexicana decorada (Calavera)** com ornamentos violeta e pretos ao redor dos olhos e boca costurada.
  - A coroa da cabeça é uma guirlanda abundante de **cravos-de-defunto (flores de Cempasúchil) laranjas vivas**, com uma pequena caveirinha de açúcar decorativa no centro.
  - Traje elegante de mariachi fúnebre / *charra* negra de gala com **bordados prateados em filigrana de ossos e costelas** nas mangas e pernas.
  - Avental frontal de veludo roxo com bordados de teias de prata e borlas pretas.
- **Adaptação de Estados:**
  - *Velo de las Espinas:* Os espinhos de ossos estalam para fora e o corpo é rodeado por **dezenas de borboletas monarcas alaranjadas de fogo e pétalas de cempasúchil flutuantes**!
  - *Lágrimas de Frida:* **Velas votivas em miniatura com chamas alaranjadas** acendem-se flutuando no ar para curar a lutadora.
  - *Gran Salto Luchador:* Cai com uma explosão de pétalas laranjas de cempasúchil e papel picado mexicano multicolorido.
- **Assinatura Temática:**
  - *Pose de Vitória:* Dança uma pose de Calavera clássica com as mãos estalando os dedos esqueléticos e assopra uma chuva de pétalas laranjas de cempasúchil em direção à câmera.
  - *Quote:* *"Na morte como no ringue... ninguém escapa dos meus espinhos."*

---

### Skin 3 (Nova) — Lady Espinha Guerreira Jaguar / Deusa Asteca (Ocelotl Lucha)
- **Conceito:** Inspirada na casta sagrada dos temidos Guerreiros Jaguar do império Asteca (*Ocelotl*).
- **Cores Principais:** Dourado jaguar com manchas pretas (`#eab308` / `#09090b`), turquesa maia (`#06b6d4`), vermelho sangue solar (`#dc2626`) e obsidiana negra (`#1c1917`).
- **Traje & Cabeça:**
  - Máscara de lucha livre estilizada com o crânio e a pele de um jaguar feroz, com dentes afiados de felino emoldurando o rosto.
  - Manto de pele de jaguar nos ombros e collant com texturas de couro de agave pintado com manchas pretas.
  - Espinhos do corpo substituídos por **lâminas pontiagudas de obsidiana negra vítrea afiada**, que brilham com reflexos escuros.
  - Saiote cerimonial com plumas de águia e glifos solares astecas.
- **Adaptação de Estados:**
  - *Velo de las Espinas:* As lâminas de obsidiana negra estalam e vibram com energia solar asteca avermelhada.
  - *La Garra del Cacto:* Garras de jaguar cortando o ar em arcos afiados duplos.
  - *Gran Salto Luchador:* Rugido de jaguar místico e impacto de cratera de templo solar.
- **Assinatura Temática:**
  - *Pose de Vitória:* Solta um rugido feroz agachada no chão como uma fera predadora e crava as garras de obsidiana na lona do ringue.
  - *Quote:* *"O sol exige sacrifício... e você foi a oferenda de hoje!"*

---

## 🚀 8. DR. MARCOV — O ASTRONAUTA QUÂNTICO

### 🔍 Skin Base (Original Atual do Jogo)
- **Descrição Canônica:** Chimpanzé cosmonauta soviético genial e furioso! Rosto peludo de macaco com orelhas grandes proeminentes (`#d4a373`), rugas expressivas na testa e focinho simiesco. **Cauda longa peluda marrom (`#5c3d2e`) que sai por trás do traje espacial e se move ativamente**. Punhos peludos de primata. Traje espacial soviético pressurizado branco com faixas carmesim e douradas, **painel de ECG ciano (`#00f5d4`) no peito** e estrela soviética dourada. Botas lunares grossas com caneleiras de titânio. Escudo Sputnik com 4 antenas metálicas e raio orbital laser.

---

### Skin 1 (Nova) — Marcov Primitivo / Primata das Cavernas (Stone Age Chimp)
*⭐ Ideia do Jogador (Marcov da Idade da Pedra)*
- **Conceito:** O primata em seu estado mais ancestral e brutal! Um chimpanzé da Idade da Pedra com força bruta pré-histórica.
- **Cores Principais:** Marrom pelo primata (`#5c3d2e`), pele de leopardo das cavernas (`#d97706` com manchas pretas), osso fóssil (`#fef3c7`) e cinzas de fogueira (`#27272a`).
- **Traje & Cabeça:**
  - Tanga rústica de pele de felino dente-de-sabre e ombreira de couro amarrada com cipó vegetal.
  - Grande **colar com dente fóssil gigante de mamute** e pinturas tribais de barro branco no peito e na testa enrugada.
  - Ataduras de couro cru nos punhos e na cauda peluda, com lascas pontiagudas de sílex amarradas.
- **Adaptação de Estados:**
  - *Escudo Sputnik:* No lugar da sonda moderna, uma **grande pedra esférica pré-histórica / meteoro em brasa com marcas rupestres esculpidas orbitando ao redor do primata soltando faíscas de fogueira**!
  - *Raio Orbital:* Um **relâmpago colossal de tempestade primordial ou meteoro flamejante** despenca das nuvens rachando o solo!
  - *Gravidade Zero:* Cinzas de fogueira, folhas secas e pequenas pedrinhas flutuam ao redor dele.
  - *Propulsão:* Arrancada terrestre violenta levantando nuvens de terra batida e poeira!
- **Assinatura Temática:**
  - *Pose de Vitória:* Pula direto no tablado do ringue (sem elementos artificiais como rochas surgindo do nada!), desfecha socos pesados na madeira, ergue-se sobre duas patas e bate furiosamente no próprio peito com as duas mãos soltando um rugido primal ensurdecedor de chimpanzé alfa que faz a câmera tremer, e morde uma fruta silvestre.
  - *Quote:* *"Ooga-booga! Macaco bate forte, você dorme no chão!"*

---

### Skin 2 (Nova) — Marcov Cosmonauta Vostok / Herói Soviético (Tributo a Yuri Gagarin & Programa Vostok)
*⭐ O Ídolo Inegociável de Marcov (Gagarin & CCCP)*
- **Conceito:** O herói cósmico supremo e inegociável de Marcov é **Yuri Gagarin**! Ele jamais vestiria uma insígnia ou o nome do projeto americano da Apollo. Esta skin é a homenagem definitiva aos pioneiros da corrida espacial soviética: o autêntico traje espacial extraveicular soviético integral (baseado nos trajes históricos *SK-1 de Gagarin* e *Orlan/Sokol*)!
- **Cores Principais:** Branco térmico soviético (`#ffffff`), vermelho bandeira CCCP (`#dc2626`), ouro da Estrela de Herói (`#facc15`), cinza isolamento (`#94a3b8`) e azul celeste (`#0284c7`).
- **Traje & Cabeça:**
  - Macacão espacial pressurizado acolchoado espesso com tubos umbilicais frontais e as lendárias letras **"СССР" em vermelho vivo e tipografia cirílica estampadas com orgulho na testa do capacete** e no peitoral de titânio!
  - **Capacete esférico pressurizado soviético completo com viseira dourada espelhada fechada**, refletindo a curvatura azul da Terra e o brilho do cosmos!
  - A cauda peluda é totalmente blindada por uma **manga pressurizada sanfonada flexível cinza-clara hermética com ponteira de borracha reforçada**.
  - Grande mochila dorsal de suporte à vida com medidores analógicos de ponteiro soviéticos e válvulas manuais de latão.
- **Adaptação de Estados:**
  - *Escudo Sputnik:* A autêntica **Cápsula Espacial Vostok 1 esférica com módulo de serviço e antenas soviéticas em miniatura orbitando** com folha térmica e luzes vermelhas!
  - *Raio Orbital:* Feixe concentrado disparado da estação orbital soviética Mir / satélite Polyus.
  - *Gravidade Zero:* Flutua suavemente com microjatos de gás pressurizado e partículas de poeira cósmica.
- **Assinatura Temática:**
  - *Pose de Vitória:* Dá um salto em gravidade zero, bate uma rígida continência militar soviética com a luva pressurizada e aponta o indicador para o cosmos enquanto a cápsula Vostok dá uma volta orbital sobre sua cabeça.
  - *Quote:* *"Поехали! (Poyekhali!) A Terra é azul... mas a sua cara vai ficar roxa!"*

---

### Skin 3 (Nova) — Marcov Ás da Aviação 1918 (Piloto de Biplano / Barão Vermelho Primata)
- **Conceito:** O lendário piloto aviador primata da Primeira Guerra Mundial, pilotando seu biplano através de combates aéreos lendários.
- **Cores Principais:** Couro marrom envelhecido (`#78350f`), cáqui militar (`#a3a375`), lã de carneiro branca (`#f8fafc`) e seda esvoaçante (`#ffffff`).
- **Traje & Cabeça:**
  - Jaqueta de aviador clássica de couro marrom pesado com gola grossa forrada de lã de carneiro.
  - Capacete de couro justo com **óculos de voo (*goggles*) com lentes de vidro redondas puxadas na testa**.
  - **Um longo cachecol de seda branca no pescoço que tremula dramaticamente ao vento a cada golpe e esquiva!**
  - Luvas grossas de couro de aviador e botas de cano alto.
- **Adaptação de Estados:**
  - *Escudo Sputnik:* Um **mini-aeroplano biplano de caça com hélice dianteira giratória zunindo** orbitando o chimpanzé com som de motor radial a pistão!
  - *Raio Orbital:* Sinalizador luminoso aéreo vermelho disparado para o céu seguido por rajada aérea de artilharia!
  - *Propulsão:* Nuvens brancas de fumaça de escapamento de manobra acrobática aérea (*looping*).
- **Assinatura Temática:**
  - *Pose de Vitória:* Ajeita os óculos de aviador nos olhos, enrola o cachecol branco com elegância no pescoço e aciona a hélice do mini-biplano que decola da sua mão.
  - *Quote:* *"Nenhum caça me derruba nos céus... e nenhum boxeador me derruba no ringue!"*

---

## 🤖 9. BATALHATOR 2000 — O CAMPEÃO & O OPERADOR

### 🔍 Skin Base (Original Atual do Jogo)
- **Descrição Canônica:** Fases 1 e 2: mecha de combate industrial brutal montado com chapas de titânio aparafusadas (`#334155`), **duas serras circulares industriais giratórias gigantes** nos punhos, cabeçote com visor de LED carmesim pulsante (`#ff0044`), **bateria de carro 12V presa nas costas com fita adesiva prateada (silver tape)** e cabos de aço desencapados soltando faíscas elétricas. Fase 3: o robô explode e é ejetado **o Nerd Piloto em pessoa**: camisa polo verde-marinho (`#2a9d8f`) com "BOSS 01" e "1UP", protetor de bolso com canetas, óculos grossos com esparadrapo branco no meio, headset gamer vermelho, **suando desesperadamente em bicas** enquanto soca em pânico.

---

### Skin 1 (Nova) — Gabinete de Fliperama 1984 (Retro Arcade 8-Bit)
- **Conceito:** O chefe foi construído a partir de gabinetes de fliperama clássicos de madeira dos anos 1980 e o piloto é um legítimo mestre de fliperama da época!
- **Cores Principais:** Preto laca de gabinete (`#09090b`), ciano neon (`#06b6d4`), magenta CRT (`#d946ef`) e amarelo joystick (`#ffbe0b`).
- **Traje & Personagens:**
  - **Fases 1 e 2 (O Robô):**
    - A carcaça do robô é feita de **compensado preto de gabinete arcade clássico com artes laterais em pixel art retrô**;
    - As serras circulares dos punhos são substituídas por **discos gigantes de vinil retrô ou cartuchos de fliperama gigantes com dentes de acrílico iluminados**!
    - No cabeçote, uma autêntica **tela CRT abaulada exibindo scanlines e o rosto do robô em pixel art 8-bit com bocarras animadas**!
    - Nas costas, em vez da bateria 12V comum, um compartimento moedeiro iluminado dizendo "INSERT COIN / 25¢".
  - **Fase 3 (O Operador Ejetado):**
    - O piloto é ejetado como o legítimo **Nerd Campeão dos Fliperamas dos Anos 80**: colete de lã xadrez sobre camisa social com gravata torta, relógio calculadora Casio prateado no pulso, óculos gigantes de armação tartaruga, boné retrô de aba reta e meias brancas puxadas até a canela!
    - Ao atacar, grita desesperado segurando dois joysticks vermelhos de arcade com botões coloridos!
- **Adaptação de Estados:**
  - *Raio Apocalíptico:* Dispara um feixe holográfico em **grade tridimensional de wireframe Synthwave rosa e azul** com texto flutuante "INSERT COIN"!
  - *Fase 3 Pânico:* As gotas de suor são pixelizadas em quadradinhos azuis 8-bit e o operador chora em cascata de pixels!
- **Assinatura Temática:**
  - *Pose de Vitória (Fase 3):* O nerd cai de joelhos no chão abraçando um cartucho de videogame dourado enquanto a tela CRT exibe "HIGH SCORE #1: NERD - 9999990".
  - *Quote:* *"GG EZ! Ninguém quebra meu recorde na minha máquina!"*

---

### Skin 2 (Nova) — Autômato Vitoriano a Vapor (Steampunk Brass Golem)
- **Conceito:** Monstro mecânico da Revolução Industrial do século XIX, movido a carvão e engrenagens de relógio, pilotado por um cientista excêntrico.
- **Cores Principais:** Bronze polido (`#b45309`), ferro fundido oxidado (`#292524`), cobre rebitado (`#ca8a04`) e couro marrom oficina (`#78350f`).
- **Traje & Personagens:**
  - **Fases 1 e 2 (O Robô):**
    - Carcaça feita de **placas de cobre marteladas e caldeiras de ferro fundido rebitadas**;
    - No cabeçote, um visor com grelha de fornalha de carvão ardendo em brasa viva no lugar do LED carmesim;
    - Nas costas, uma **chaminé de ferro de locomotiva a vapor antiga cuspindo fumaça negra densa e fagulhas** no lugar da bateria;
    - As serras são substituídas por **lâminas de serrote circular de carpintaria vitoriana com dentes de aço oxidado**;
    - Engrenagens douradas e pêndulos de relógio giram à mostra nas articulações dos braços.
  - **Fase 3 (O Operador Ejetado):**
    - O piloto ejetado é um **relojoeiro / inventor vitoriano excêntrico em pânico**: avental pesado de couro de oficina cheio de chaves de fenda e alicates nos bolsos, camisa branca arregaçada com suspensórios de couro, **monóculo de precisão de múltiplas lentes de latão articuladas sobre o olho direito**, e um bigode farto enrolado eriçado de pânico enquanto sua em bicas!
- **Adaptação de Estados:**
  - *Raio Apocalíptico:* Erupção monumental de vapor d'água superaquecido a 500°C com fuligem de carvão e brasas de fornalha.
  - *Fase 3 Pânico:* O inventor tenta acionar cordas de relógio e chaves de fenda tremendo de medo enquanto desfecha socos estabanados!
- **Assinatura Temática:**
  - *Pose de Vitória (Fase 3):* O inventor puxa um relógio de bolso de ouro tremendo, ajusta o monóculo de latão e sopra a fumaça de uma chave inglesa fumegante.
  - *Quote:* *"Pontual como a mecânica inglesa... e letal como uma caldeira pressurizada!"*

---

### Skin 3 (Nova) — Mecha Neon Sentai / Tokusatsu Robot (Megazord)
- **Conceito:** Robô gigante defensor da Terra estilo Megazord japonês de seriados Tokusatsu dos anos 1990!
- **Cores Principais:** Branco pérola (`#f8fafc`), vermelho herói (`#ef4444`), dourado polido (`#eab308`) e azul cobalto (`#1d4ed8`).
- **Traje & Personagens:**
  - **Fases 1 e 2 (O Robô):**
    - Mecha de alta tecnologia com pintura automotiva reluzente, peitoral estilizado em formato de crista de falcão dourado e asas dorsais retráteis;
    - As serras são substituídas por **lâminas gigantes de shuriken de plasma energético dourado e azul** que giram nos pulsos;
    - Cabeçote com visor de viseira dourada espelhada com olhos de LED azul heroico;
  - **Fase 3 (O Operador Ejetado):**
    - O operador é um **jovem cosplayer otaku / fã de Tokusatsu**: usando um capacete de piloto sentai desproporcionalmente grande que fica caindo sobre os olhos, lenço vermelho longo no pescoço esvoaçante e camiseta estampada, tremendo e desferindo socos com poses dramáticas exageradas de anime!
- **Adaptação de Estados:**
  - *Raio Apocalíptico:* Feixe triplo de canhão peitoral multicolorido estilo raio final de vitória de Sentai!
- **Assinatura Temática:**
  - *Pose de Vitória (Fase 3):* Faz a pose triunfal clássica exagerada de esquadrão Tokusatsu de costas com faíscas coloridas explodindo no fundo do ringue.
  - *Quote:* *"A justiça triunfou! O monstro foi derrotado mais uma vez!"*

---

## 🌾 10. O ESPANTALHO (DUMMY DE TREINO)

### 🔍 Skin Base (Original Atual do Jogo)
- **Descrição Canônica:** Espantalho de feno e palha seca (`#d97706`), camisa de retalhos xadrez esfarrapada com costuras grossas de barbante, chapéu cônico de palha desfiada, olhos de botão preto cosidos em cruz com linha grossa, luvas de pano estofadas amarradas com cordas nos punhos. Nunca cai nem desiste: absorve combos infinitos com sorriso costurado.

---

### Skin 1 (Nova) — Crash Test Dummy (Manequim de Colisão Automotiva)
- **Conceito:** O famoso boneco de testes de segurança automotiva amarelo e preto feito para bater de frente com caminhões!
- **Cores Principais:** Amarelo segurança automotiva (`#facc15`), preto fosco (`#09090b`) e branco reflexivo (`#f8fafc`).
- **Traje & Cabeça:**
  - Corpo emborrachado amarelo brilhante com os clássicos **alvos circulares pretos e amarelos de calibragem de impacto** pintados na cabeça, peito e ombros.
  - Sensores adesivos com fios conectados aos punhos e pés medindo a força em Joules de cada soco desferido pelo jogador!
- **Assinatura Temática:**
  - *Pose de Vitória:* Pisca uma lâmpada vermelha de teste na cabeça e projeta um painel digital com a nota de segurança "5 ESTRELAS / IMPACT APPROVED".
  - *Quote:* *"Impacto absorvido com sucesso. Airbags não foram necessários."*

---

### Skin 2 (Nova) — Holo-Dummy / Manequim Cibernético de Simulação
- **Conceito:** Manequim holográfico translúcido de sala de treinamento de realidade virtual com matriz poligonal.
- **Cores Principais:** Verde fósforo holográfico (`#22c55e`), ciano wireframe (`#06b6d4`) e preto matrix (`#020617`).
- **Traje & Cabeça:**
  - Corpo translúcido composto por uma **malha poligonal 3D de arames digitais (wireframe)** que oscila com interferências e estática quando golpeado.
  - No peito, um alvo holográfico concêntrico giratório que mede a precisão do jogador (*CRITICAL HIT!*).
- **Assinatura Temática:**
  - *Pose de Vitória:* Dissolve-se em cubos digitais verdes e se remonta instantaneamente com som de boot de computador.
  - *Quote:* *"Reiniciando simulação de combate. Pronto para o próximo round."*

---

### Skin 3 (Nova) — Espantalho Espooky de Halloween / Pumpkin Scarecrow
- **Conceito:** O clássico espantalho sinistro de plantação de milho na noite de Dia das Bruxas com cabeça de abóbora flamejante!
- **Cores Principais:** Laranja abóbora ardente (`#ea580c`), preto bruxa (`#09090b`), roxo místico (`#7e22ce`) e verde musgo podre (`#14532d`).
- **Traje & Cabeça:**
  - Cabeça esculpida em uma **grande abóbora de Halloween (Jack-o'-lantern)** com chamas amarelas e velas acesas brilhando por dentro dos olhos e da bocarra dentada entalhada!
  - Sobretudo rasgado feito de folhas secas de outono e teias de aranha; dedos e luvas feitas de galhos retorcidos de árvore morta.
- **Assinatura Temática:**
  - *Pose de Vitória:* As chamas da abóbora sobem alto como um fogaréu sinistro com risada ecoada de bruxa e morcegos microscópicos voando.
  - *Quote:* *"Doces ou travessuras? Você escolheu apanhar na noite de Halloween!"*

---

## 📋 11. Prompts Prontos para Copiar e Colar para o Claude

Abaixo estão os prompts atualizados e estruturados prontos para você copiar e colar na conversa com o Claude amanhã para gerar os códigos SVG / spritesheets de cada personagem sem qualquer ambiguidade de lore:

### Prompt A — Gerar Skins da Valéria (Frevo Vulcânico & Mary Poppins)
```text
Você é o artista e animador vetorial oficial do jogo de boxe arcade "KO-RAMA".
Preciso gerar 3 NOVAS SKINS para a personagem VALÉRIA VULCÂNICA.

ATENÇÃO AO CANON DO JOGO:
Valéria é uma PASSISTA DE FREVO VULCÂNICA DE PERNAMBUCO!
- Skin Base Atual: Top cropped de frevo laranja e amarelo, saia de frevo em camadas com babados triangulares em zigue-zague (laranja, amarelo e turquesa vivo), sapatilhas de passista turquesa, tatuagens de cinzas no abdômen, cabelo de labaredas de fogo vivo vulcânico e sombrinha de frevo de ferro e chamas.
- ESTADO OBRIGATÓRIO (MODO MAGMA FRIO): Quando ativado na 2ª queda, o cabelo de chamas resfria e se transforma em rocha de BASALTO ESCURO com fissuras alaranjadas incandescentes, e a pele e saia ganham tons de cinza carvão/ardósia.

Gere o código SVG completo e estilizado para as 3 NOVAS SKINS:
1. Nova Skin 1: "Rainha do Maracatu & Frevo de Olinda"
   - Cores: ouro #facc15, azul royal #1e40af, vermelho #dc2626 com gola de veludo ricamente bordada de lantejoulas douradas, saia multicolorida com fitas longas de cetim esvoaçantes que dançam nas esquivas e tiara de pedrarias e espelhos de Maracatu no cabelo de fogo.
2. Nova Skin 2: "Mary Poppins / A Governanta Mágica do Guarda-Chuva"
   - Cores: azul marinho eduardiano #1e3a8a, branco engomado, gravata borboleta vermelha e sobretudo elegante acinturado com chapéu preto com margaridas; empunha o guarda-chuva preto com cabo de papagaio esculpido! No Modo Magma Frio, o traje e o rosto ganham fuligem de limpador de chaminé de Londres (Step in Time!) e o guarda-chuva solta brasas de lareira!
   - 🎨 ESTILO DE ANIMAÇÃO & TRAÇO: Bordas/outlines visivelmente MAIS FINAS e delicadas (estilo cel animation clássico dos anos 1960). Média maior de frames desenhados à mão (quadro a quadro) com física elástica saltitante, exatamente como Mary Poppins interagindo com o mundo dos desenhos em Jolly Holiday!
3. Nova Skin 3: "Bailarina de Flamenco das Chamas (Sevilha Caliente)"
   - Cores: vermelho rubi espanhol #b91c1c e preto renda; vestido justo com babados volumosos em camadas (bata de cola), xale de Manila, rosa no cabelo com peineta de ouro, e sombrinha de renda espanhola negra e vermelha com castanholas de ferro nos punhos!
Lembre-se de incluir a versão do Modo Magma Frio para cada uma!
```

---

### Prompt B — Gerar Skins da Maestra Vivaldi (Com Vivaldi Humana Viva)
```text
Você é o artista e animador vetorial oficial do jogo de boxe arcade "KO-RAMA".
Preciso gerar 3 NOVAS SKINS para a personagem MAESTRA VIVALDI.

ATENÇÃO AO CANON DO JOGO:
- Skin Base Atual: Fantasma flutuante com cauda espectral de ectoplasma verde-água, casaca violeta imperial, peruca barroca branca e verde com notas 8-bit flutuantes e batuta de cristal ectoplásmico. Modo Quatro Estações dinâmico.

Gere o código SVG completo para as 3 NOVAS SKINS:
1. Nova Skin 1: "Vivaldi Humana / A Virtuosa Viva de Veneza (The Living Virtuoso)"
   - Vivaldi viva em carne e osso no século XVIII! PERNAS HUMANAS REAIS calçando meias de seda branca e sapatos aristocráticos com fivela de ouro (sem cauda fantasma: pisa no chão!); pele humana mediterrânea saudável; cabelos naturais castanho-avermelhados em coque veneziano elegante; casaca e corpete carmesim com ouro e rendas finas; segura um autêntico VIOLINO DE MADEIRA NOBRE ITALIANA com arco de crina nos ataques; ondas sonoras acústicas e partituras douradas no lugar das notas 8-bit!
2. Nova Skin 2: "Vivaldi Ópera Fantasma / La Scala Gótica (Diva da Noite)"
   - Meia-máscara veneziana gótica de porcelana polida branca e ouro sobre o rosto; peruca monumental em preto ônix com rosas vermelhas e partituras queimadas flutuando; casaca de veludo negro da meia-noite e cauda que se dissolve em névoa negra e pétalas de rosa!
3. Nova Skin 3: "Vivaldi Rainha do Gelo / Aurora Boreal Glacial"
   - Casaca gélida translúcida com luzes móveis de Aurora Boreal verde e magenta; peruca de geada pura com estalactites e flocos de neve fractais flutuando; cauda de vento polar cortante e batuta de agulha de gelo eterno!
```

---

### Prompt C — Gerar Skins do Dr. Marcov (Cavernas & Vostok Soviético & Aviador)
```text
Você é o artista e animador vetorial oficial do jogo de boxe arcade "KO-RAMA".
Preciso gerar 3 NOVAS SKINS para o personagem DR. MARCOV.

ATENÇÃO AO CANON DO JOGO:
Dr. Marcov é um CHIMPANZÉ PRIMATA GENIAL com orelhas grandes, rugas na testa e CAUDA LONGA PELUDA MARROM (#5c3d2e) que se move ativamente!
- ESTADOS OBRIGATÓRIOS: Escudo orbital, Raio vindo do céu, Gravidade Zero com flutuação leve.

Gere o código SVG completo para as 3 NOVAS SKINS (variadas e criativas):
1. Nova Skin 1: "Marcov Primitivo / Primata das Cavernas (Stone Age Chimp)"
   - Chimpanzé pré-histórico da Idade da Pedra: tanga rústica de pele de tigre dente-de-sabre com cipó, colar com dente fóssil de mamute, ataduras de couro cru com pedras lascadas nos punhos; no lugar do Sputnik: UMA GRANDE PEDRA PRÉ-HISTÓRICA / METEORO EM BRASA COM MARCAS RUPESTRES orbitando o primata com faíscas de fogueira; no Raio Orbital: um relâmpago de tempestade primordial ou meteoro flamejante despencando do céu! Na pose de vitória, pula direto no chão do ringue (sem rochas surgindo da arena!), soca o tablado e estufa o peito com rugido primal alfa!
2. Nova Skin 2: "Marcov Cosmonauta Vostok / Herói Soviético (Tributo a Yuri Gagarin & CCCP)"
   - O ídolo inegociável de Marcov é Yuri Gagarin! Traje espacial extraveicular soviético integral acolchoado (Orlan/Sokol) branco com cinza; capacete esférico fechado com as letras "СССР" em vermelho vivo na testa e viseira dourada espelhada reflexiva; cauda peluda encapada por manga pressurizada sanfonada hermética; no lugar do Sputnik: A CÁPSULA ESPACIAL VOSTOK 1 soviética em miniatura orbitando com antenas e módulo de serviço! Frase de vitória: "Поехали! (Poyekhali!) A Terra é azul... mas a sua cara vai ficar roxa!"
3. Nova Skin 3: "Marcov Ás da Aviação 1918 (Piloto de Biplano / Barão Vermelho Primata)"
   - Jaqueta marrom de couro pesado com gola de lã de carneiro branca, capacete de piloto com óculos de voo (goggles) na testa, LONGO CACHECOCOL DE SEDA BRANCA ESVOAÇANTE no pescoço; no lugar do Sputnik: um mini-aeroplano biplano de caça com hélice giratória zunindo em órbita; propulsão solta fumaça de manobra aérea!
```

---

### Prompt D — Gerar Skins de O Cremoso (Com Legibilidade Rigorosa & Hippie Vegano)
```text
Você é o artista e animador vetorial oficial do jogo de boxe arcade "KO-RAMA".
Preciso gerar 3 NOVAS SKINS para o personagem O CREMOSO com RIGOROSA LEGIBILIDADE DE HABILIDADES.

ATENÇÃO AO CANON DO JOGO:
O adversário precisa saber instantaneamente qual habilidade está ativa em combate:
- Modo Derretido (Melt): O corpo SEMPRE liquefaz em calda quente semilíquida escorrendo em poça no chão.
- Estamina Fria (Frozen Stamina): O corpo SEMPRE congela em bloco duro com geada cristalina e OS DOIS TUBETES/CANUDINHOS DE WAFER NA NUCA (marco obrigatório de passiva ativa!).
- Vingança Doce: SEMPRE calda vermelha escura fervendo borbulhando com vapor de açúcar e olhos furiosos.

Gere o código SVG completo para as 3 NOVAS SKINS:
1. Nova Skin 1: "Cremoso Matcha Zen / Hippie Vegano & Chás Orgânicos (The Vegan Zen Hippie)"
   - Visual de autêntico Hippie Vegano Zen: óculos redondinhos retrô estilo John Lennon com armação dourada fina, faixa de cabeça em macramê/cânhamo cru prendendo coque zen relaxado, colar longo de sementes e sândalo aromático, avental ecológico de linho cru com bordado de folhinha verde "VEGAN FRIENDLY", ombreiras com ramos frescos de hortelã orgânica, fatias de babosa (aloe vera) e framboesas silvestres. No topo, raminho de alecrim. No Melt vira calda líquida esmeralda de matcha e leite de amêndoas; na Estamina Fria vira bloco congelado duro com OS DOIS TUBETES DE CANELA/WAFER NA NUCA; na Vingança Doce ferve calda de feijão azuki com incenso de sândalo!
2. Nova Skin 2: "Cremoso Napolitano / Três Sabores"
   - Corpo clássico em 3 camadas (base chocolate trufado #3b1a08, meio morango rosa #f43f5e, topo baunilha #fef08a); ombreiras de biscoito Oreo; avental marrom de barista; no Melt vira redemoinho marmorizado tricolor; na Estamina Fria ganha tubetes bicolores de chocolate e cobertura crocante de avelã!
3. Nova Skin 3: "Cremoso Sundae de Festa / Confeito de Circo (Algodão Doce)"
   - Sorvete festivo espiral azul e rosa pastel salpicado de granulados arco-íris; avental listrado de parque vintage; no Melt vira calda líquida azul brilhante; na Estamina Fria vira picolé duro congelado com os dois tubetes listrados na nuca; na Vingança Doce ferve calda de framboesa!
```

---

### Prompt E — Gerar Skins da Joana (Humana Pré-Maldição & Despertar Abissal)
```text
Você é o artista e animador vetorial oficial do jogo de boxe arcade "KO-RAMA".
Preciso gerar 3 NOVAS SKINS para a personagem JOANA CARA DE CRACA.

ATENÇÃO AO CANON DO JOGO:
- Skin Base Atual: Pirata mutante meio humana, meio crustáceo com garra colossal, pedúnculo de siri, cracas calcificadas e perna de pau.

Gere o código SVG completo para as 3 NOVAS SKINS:
1. Nova Skin 1: "Pescadora do Mar do Norte / Joana Humana Pré-Maldição (The Harpooner Before the Curse)"
   - FORMA NORMAL (ANTES DA MALDIÇÃO): Rosto 100% HUMANO! Dois olhos normais e saudáveis, sem pedúnculo de siri, sem tapa-olho (chapéu sombreia os olhos), sem dente de ouro podre. Grande capa de chuva impermeável amarela FECHADA NA PARTE SUPERIOR COBRINDO O PEITO COM BOTÕES PRETOS E ABERTA NOS BOTÕES INFERIORES, funcionando como uma capa longa de capitã esvoaçante; chapéu sou'wester amarelo com aba protetora na nuca; empunha pesado arpão baleeiro de ferro forjado de farpa dupla!
   - O DESPERTAR DA MALDIÇÃO (MODO MONSTRUOSO < 40 HP): A capa amarela se abre mostrando o físico definido/musculoso e SE RASGA VIOLENTAMENTE em múltiplos pontos de onde saltam PATAS PONTIEAGUDAS DE CARANGUEJO, CRACAS E ANÊMONAS! Sob a sombra do chapéu, os olhos acendem REDONDOS, BRANCOS E CEGOS COMO UM PEIXE-PESCADOR ABISSAL (ANGLERFISH) e a boca escancara com DENTES SOBRESSALENTES AFIADOS DE TUBARÃO/CORAL! A manopla direita incha na garra gigante de King Crab rubi!
2. Nova Skin 2: "Almirante do Abismo / Fantasma do Davy Jones"
   - Casaca desgastada em azul-noite com galões ciano fantasma; lado lagosta azul profunda vítrea e gancho esculpido em coral negro pontiagudo afiado.
3. Nova Skin 3: "Joana Mecânica / Steampunk Dredger"
   - Corsária a vapor: olho em periscópio de latão com manômetro, blindagem de cobre rebitada, garra mecânica hidráulica de dragagem industrial com pistões e amortecedor pneumático de alta pressão na perna!
```

---

## 💾 12. Arquitetura de Dados no Motor C++

Para quando os arquivos estiverem prontos para compilação no motor C++ (`engine-cpp`):

```cpp
// Definição em include/fighter.hpp
struct FighterSkin {
    int skinId = 0;              // 0 = Base, 1 = Skin 1, 2 = Skin 2, 3 = Skin 3
    std::string skinName;        // Ex: "Vivaldi Humana", "Mary Poppins", "Marcov das Cavernas"
    std::string prefixAtlas;     // Ex: "vivaldi_human_", "valeria_poppins_", "marcov_stone_"
    sf::Color primaryColor;
    sf::Color secondaryColor;
    std::string victoryQuote;    // Frase temática da vitória
};
```

1. **Gravação no Savefile (`korama_save.txt`):**
   ```text
   skin 4 2   # Valéria com Skin 2 (Mary Poppins)
   skin 5 1   # Vivaldi com Skin 1 (Vivaldi Humana Viva)
   skin 7 1   # Marcov com Skin 1 (Primata das Cavernas)
   ```
2. **Troca Rápida na Tela de Seleção (`CharSelect`):**
   - Tecla `[TAB]` ou `[ESPAÇO]` cicla entre `Base -> Skin 1 -> Skin 2 -> Skin 3`.
   - Atualiza o card, o sprite e a assinatura sonora imediatamente!

---
*K.O.-RAMA — Documento Oficial de Skins e Paletas atualizado com 100% dos conceitos ajustados.*
