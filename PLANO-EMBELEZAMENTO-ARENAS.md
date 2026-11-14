# K.O-RAMA — plano de embelezamento das arenas para o Antigravity

Data: 14/10/2026. Este documento é um plano de execução, não autorização para redesenhar personagens, alterar combate ou trocar cenários aprovados. Trabalhar **uma arena por vez**, mostrar evidência real e instalar só a etapa aprovada nos testes. A referência jogável é `engine-cpp`; o pacote usado pelo jogador é `dist/KO-RAMA`.

## Norte visual

O jogo é um quadrinho 3D low-poly: formas claras, contorno grosso, cor chapada com variação seletiva e animação expressiva. O lutador e seus golpes são o foco; fundo, torcida e efeitos devem dar profundidade sem competir com rosto, punhos ou leitura do ringue. Uma arena bonita em captura, mas confusa durante uma luta, está errada.

Manter decisões já fechadas:

- Nove arenas atuais, paletas e elementos principais. Joana: plataforma de tábuas, água, barcos e ilhas; Valéria: Pernambuco festivo; Marcov: observatório; Frog: brejo; Vivaldi: teatro; Cremoso: sorveteria; Espinha: pátio; Jack: jardim e colmeias; Batalhator: arena central.
- Céu aberto com posterização leve e retícula discreta **só no céu**. Não ligar shader global de retícula sobre lutadores e golpes.
- Sombras atuais dos lutadores (máscara alfa única, incluindo punhos) e sombras discretas de postes/cordas; preservar seus vínculos com a pose e a deformação.
- **Não reintroduzir os dois fachos translúcidos fixos atravessando o centro da tela.** Luzes devem pertencer a fontes do cenário, não a triângulos presos à câmera.
- Não alterar física, hitboxes, regras de combate, saves, desbloqueios, rigs ou assets de skin neste arco.

## Ordem de trabalho

### 0. Travar uma linha de base antes de editar

1. Ler `outputs/CURADORIA-ARENAS-KORAMA-2026-10-14.md`, `outputs/ACABAMENTO-QUADRINHOS-ARENAS-2026-10-14.md`, `outputs/SOMBRAS-DO-RINGUE-KORAMA-2026-10-14.md` e `outputs/SEM-FACHOS-CENTRAIS-KORAMA-2026-10-14.md` na pasta de trabalho do Codex (`C:/Users/bento/Documents/Codex/2026-10-09/referenced-chatgpt-conversation-this-is-an`). São histórico e critérios, não instruções para sobrescrever mudanças posteriores.
2. Conferir hashes do código, executáveis e assets atuais; verificar edições externas antes de tocar neles. Guardar cópia recuperável dos executáveis antes de cada instalação. Nunca modificar `korama_save.txt`.
3. Capturar as nove arenas em frente, costas e giro, com personagens de contraste difícil e um golpe de efeito grande. Registrar 1–2 lutas completas em janela visível na máquina MX110, inclusive com compartilhamento de tela se possível. Medir tempo por quadro, picos, transição versus→luta e memória; não usar somente a média de um benchmark curto como prova.
4. Registrar as 46 falhas preexistentes da suíte de regras como limitação histórica, não como aprovação ou motivo para reformular combate.

### 1. Acabamento de alto impacto e baixo risco

Em cada arena, fazer no máximo **uma ou duas mudanças por versão** para poder atribuir ganhos e regressões. Priorizar:

- **Hierarquia de valor:** cenário mais calmo atrás do adversário; objetos importantes mais saturados nas bordas. Ajustes devem existir no mundo e continuar corretos no giro da câmera, sem painel escuro fixo.
- **Contato e profundidade:** sombras pequenas e estáveis sob objetos próximos, apoio visual de postes, barcos/água ou plantas/solo conforme a arena. Não desenhar sobre ringue, água ou personagens por engano. Não somar opacidade em peças sobrepostas; uma silhueta deve ter uma intensidade coerente.
- **Materiais de quadrinhos:** luz curta de borda e planos de cor em madeira, metal, vidro, tecido, lama e água. Preferir geometria/cor local ou shader de área limitada a um pós-processamento de tela cheia.
- **Movimento ambiente com intenção:** poucas animações assíncronas e lentas (folhas, fitas, reflexos, água, instrumentos), sem ruído contínuo na área de combate. Objetos devem ficar ancorados no mundo em qualquer câmera.
- **Vitória e nocaute:** um acento visual específico da arena pode responder ao evento, mas não deve encobrir lutador, HUD ou o impacto principal. Preservar splash da Joana e outros eventos já aprovados.

### 2. Direção específica por arena

| Arena | Próxima experiência visual a prototipar | O que evitar |
| --- | --- | --- |
| Joana | Reflexos curtos junto aos cascos e ondulação ao redor da plataforma; sombras de contato dos barcos sobre a água, acompanhando sua posição 3D. | Cortes quadrados no mar, ilhas presas à tela, mais ruído atrás do adversário. |
| Valéria | Luz quente localizada em superfícies e bandeiras/elementos de festa, com pequenas variações rítmicas fora do ringue. | Torcida mais luminosa que a lutadora, fachos atravessando o centro. |
| Marcov | Brilhos controlados em bordas de metal, vidro e monitores; um pulso lento de instrumentação. | Pós-processamento azul global, reflexo branco sobre o rosto ou punhos. |
| King Frog | Reflexos de umidade nas margens, sombras de taboas do brejo e juncos/plantas bem ancorados na terra. | Voltar a deixar piso e pele com o mesmo verde, plantas erradas ou água chamativa. |
| Vivaldi | Mancha de luz de vitral **em superfícies do palco**, localizada no mundo; veludo com dois tons e movimento mínimo. | Retícula forte no fantasma ou luz cruzando o rosto. |
| Cremoso | Brilho de vitrine e sombra de contato de balcões/decoração em planos claros, preservando os tons creme/rosa. | Piso lavado, reflexos grandes ou elementos que pareçam golpes do Cremoso. |
| Lady Espinha | Contraste quente/frio entre terracota e turquesa; sombras recortadas de plantas/arquitetura nas bordas. | Outra arena bege uniforme; ornamentação ocupando o centro. |
| Jack | Variedade já aprovada de flores e roseiras; manchas pequenas de luz/sombra de folhagem ao redor, abelhas pontuais no cenário. | Parede verde atrás da torcida, flores coplanares ou enxame que confunda ataque. |
| Central/Batalhator | Luz industrial sobre arquitetura e monitores, com mudanças discretas entre fases do chefe. | Variação de cor que dificulte ler a forma/fase ou efeitos contínuos caros. |

Essas linhas são protótipos de curadoria, não uma lista obrigatória de props novos. Se algo já existir e funcionar, preservar. Para um objeto ainda não modelado, fazer estudo isolado e mostrar frente/costas/giro antes de pôr na arena (como foi feito com os barcos e flores).

## Orçamento de desempenho e implementação

- Alvo: conforto de jogo real na NVIDIA MX110, inclusive com tela compartilhada. Não assumir que “60 FPS no capturador” basta.
- Comparar sempre a **mesma cena, mesmo personagem/skin, mesma câmera, mesma resolução e mesmo trecho de luta** antes/depois. Se a mediana subir mais de ~0,5 ms ou p95/picos piorarem de forma recorrente, investigar e otimizar ou reverter o efeito. Esses são gatilhos de investigação, não licença para aceitar travamentos.
- Evitar novos passes de tela cheia, uploads dinâmicos de textura, alocações por quadro e centenas de draw calls por detalhe. Reutilizar buffers, agrupar geometria e descartar fora da câmera. Atualizar movimentos ambientes em cadência própria quando 60 Hz não for perceptível.
- Sombras de props estáticos podem ser pré-computadas/cacheadas por arena/câmera; as de corda e personagem precisam acompanhar a pose. Não usar uma sombra genérica circular como substituta de forma importante.
- Respeitar ordem de camadas: chão → sombra de cenário → sombra de lutador → personagens/efeitos → cordas/HUD. Conferir água, plataforma, aproximação às cordas e câmera de nocaute.

## Aceitação de cada arena

Uma arena só passa à próxima quando houver:

1. Comparação visual antes/depois em frente, costas, giro e momento de golpe/KO. Inspecionar tamanho real de jogo, não só miniatura ou screenshot isolada.
2. Checagem de luz e sombras: nada flutua, atravessa borda, aparece dentro de casco/parede, escurece por sobreposição indevida ou fica preso à tela quando a câmera gira.
3. Checagem de combate: rosto, punhos, pés, telégrafos, efeitos e cordas continuam legíveis; HUD e personagem não são cobertos pelo cenário.
4. Benchmark pareado e pelo menos uma luta longa visível nas arenas de maior risco (Joana com água/barcos, Vivaldi com transparência, Central com Batalhator). Registrar limitações honestamente.
5. Build Release de jogo e capturador, smoke do pacote `dist/KO-RAMA`, hashes antes/depois e backup recuperável; save intacto. Se executável estiver aberto, não encerrar à força nem sobrescrever.

## Entrega esperada do Antigravity

Começar por **uma arena-piloto clara (Jack)** e **uma escura (Marcov)** para calibrar contraste e custo. Mostrar capturas comparativas e medidas; ajustar o método; só então seguir uma arena por vez. Registrar para cada etapa: mudança feita, imagens, tempo de quadro, bugs encontrados/corrigidos, hash do pacote e o que ainda não foi verificado. Não declarar “todas embelezadas” enquanto faltar revisão visual ou desempenho real de qualquer arena.
