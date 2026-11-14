# Jack — tabela de tempos (o jogo manda, a arte obedece)

Fonte: `src/engine.cpp`, `src/ai.cpp`, `src/skills.cpp`. O jogo roda a 60 quadros por segundo.
"Acerto" é a janela em que o golpe pode conectar (fração do golpe). A animação precisa ter o
impacto visual dentro dessa janela.

## Golpes do jogador

| Ação | Duração | Acerto | Atordoa o rival | Observação |
|---|---|---|---|---|
| Jab (mão esquerda) | 28 q (0,47 s) | 40–70% (q 11–20) | 16 q | |
| Direto (mão direita) | 32 q (0,53 s) | 40–70% (q 13–22) | 20 q | |
| Gancho (lado + soco) | 38 q (0,63 s) | 40–70% (q 15–27) | 24 q | |
| No corpo (baixo + soco) | 34 q (0,57 s) | 40–70% (q 14–24) | 28 q | |
| Uppercut (cima + soco) | 48 q (0,80 s) | 40–70% (q 19–34) | 38 q | |
| Carga (segurar soco) | até 72 q (1,2 s), solta sozinho 24 q depois | — | — | golpe solto fica até 50% mais longo |

## Golpes do Jack como oponente (IA)

| Ação | Duração | Acerto |
|---|---|---|
| Aviso (telegraph) | 36 q (0,6 s); pesado 48 q (0,8 s) | — (pode ser contra-atacado) |
| Jab / direto | 28 q | 40–68% |
| Gancho / corpo / uppercut | 36 q | 40–68% |

## Defesa e reações

| Ação | Duração |
|---|---|
| Guarda | 14 q (jogador, segura enquanto apertar); 18 q (IA) |
| Esquiva lado | 22 q |
| Esquiva para trás | 24 q |
| Leva soco do rival (jogador) | 22 q |
| Leva soco na guarda | 8 q |
| Leva soco (rival) | 16–38 q conforme o golpe, até +80% com carga |

## Skills do Jack (duração do movimento de quem lança)

| Skill | Duração |
|---|---|
| Infestação de Abelhas | 44 q |
| Modo Melaço | 32 q |
| Favo Protetor | 30 q |
| Agulhada Rainha | 24 q |
| Explosão de Pólen | 28 q |
| Geleia Real | 30 q |
| Apicultor, Pele de Mel, Bênção da Rainha | passivas (sem movimento) |

## Comparação com a apresentação

| Apresentação | Duração lá | No jogo | Situação |
|---|---|---|---|
| Jab | 0,9 s, impacto em 40–55% | 0,47 s, acerto 40–70% | 2x mais lento lá; impacto bate em fração |
| Direto | 1,0 s | 0,53 s (direto) e 0,63 s (gancho) | idem; gancho não tem animação própria |
| Uppercut | 1,1 s | 0,80 s | idem |
| Leva direto / uppercut / corpo | 1,2–1,3 s | 0,27–0,63 s | muito mais longas lá |
| Parado, passada, tonto, cordas, nocaute, assinatura | ciclos | ciclos | servem como estão |

### Faltam animações para
- Gancho e soco no corpo (hoje reaproveitariam direto e jab)
- Aviso do golpe da IA (telegraph) e a carga do jogador
- Guarda, esquiva lado, esquiva para trás, abaixar
- Leva soco na guarda
- Movimento de cada uma das 6 skills ativas

## O que NÃO muda (vem do jogo, a arte só se encaixa)

- **Tamanho do lutador**: o corpo novo ocupa exatamente a caixa do sprite atual (largura/altura
  usadas em `RingRenderer::queueFighter`); pés no mesmo ponto do chão.
- **Luvas**: sprite, tamanho (58 + até 42% no auge do soco) e camada atrás/na frente do corpo são os
  de hoje.
- **Trajetória das luvas** (`Fighter::animateCommonPunch`, `animateChargingPose`, guarda, esquivas):
  preparação (a luva recua e desce), arco do soco em 3D (vai até 175–190 para frente e sobe 46–50),
  gancho atravessando de lado, a outra luva protegendo o rosto, tremor da carga.
- **Corpo inteiro**: inclinação (`bodySkew`), deslocamentos (`offsetX/Y/Z`), gingado do parado
  (`applyIdleBob`), esquivas e reações (`computeReaction`) continuam movendo o lutador como hoje.

A animação nova só acrescenta o que o sprite inteiro não tem: cabeça, chapéu, capa, pernas e rosto
se mexendo por dentro dessa caixa, no ritmo da tabela acima.
