# Padrões de luta por oponente (proposta)

Hoje a IA sorteia um golpe qualquer a cada 0,6–1,3 s, com o mesmo aviso (luva sobe e pisca 0,6 s ou
0,8 s) para todos os lutadores, e qualquer esquiva ou guarda resolve qualquer golpe. A proposta é que
cada oponente tenha um "jeito de lutar" que o jogador aprende e vence.

## Regras gerais (valem para todos)

1. **Cada golpe tem um aviso próprio** (o que o corpo faz + o som + a duração). Nunca menos de 0,35 s.
2. **Cada golpe tem uma resposta certa**:
   - gancho vindo da direita → esquivar para a esquerda (e vice-versa); esquivar para o lado errado = apanha;
   - uppercut → abaixar ou esquivar para trás;
   - golpe no corpo → guarda;
   - golpe "agarrão" (língua, gancho de marujo) → só esquiva; a guarda não segura.
3. **Contra-ataque**: socar durante o aviso continua dando ×1,6 de dano e agora também interrompe o
   golpe (o oponente fica atordoado 0,5 s).
4. **Brecha**: depois do golpe mais forte de cada um, ele fica 1–1,5 s vulnerável (cansado, preso,
   se recompondo). Acertos nessa janela dão +25% de dano. É o "prêmio" por ter lido o padrão.
5. **Armadilha**: cada um tem um truque que pune quem só decorou (aviso falso, atraso, segundo golpe).
6. **Fases pela vida**: acima de 60% joga o básico; de 60% a 30% adiciona um golpe; abaixo de 30%
   fica mais rápido e usa a armadilha mais vezes. O jogador sente a luta "subir de nível".
7. **Combos**: sequências fixas de 2–3 golpes com o mesmo aviso, para o jogador reconhecer o ritmo.

## Jack — o apicultor (oponente da 1ª luta: ensina o básico)

| Golpe | Aviso | Resposta | Depois |
|---|---|---|---|
| Ferroada (jab) | aba do chapéu baixa, 0,4 s | guarda ou qualquer esquiva | — |
| Colmeia Dupla (gancho E + gancho D) | bate as colmeias uma na outra ("BZZ"), 0,7 s | esquiva D, depois E | **brecha**: limpa o mel das luvas, 1,2 s |
| Enxame + uppercut (fase 2) | assobia, nuvem de abelhas 1 s | abaixar | — |

**Armadilha:** às vezes bate as colmeias duas vezes antes de soltar, para pegar quem esquiva cedo.
**Fase 3:** a Ferroada vira dupla (jab-jab).

## King Frog — o rei do brejo

| Golpe | Aviso | Resposta | Depois |
|---|---|---|---|
| Língua (agarrão de longe) | infla as bochechas, 0,6 s | só esquiva (a guarda não segura) | — |
| Tapa de Lama (no corpo) | pega lama do chão, 0,5 s | guarda | — |
| Salto Real (fase 2) | agacha 0,8 s, pula para fora da tela, a sombra aparece | esquiva para trás | **brecha**: atolado no chão, 1,5 s |

**Armadilha:** infla as bochechas e segura (finta), depois solta um jab rápido.
**Fase 3:** Pele Venenosa: socar quando a pele brilha custa stamina, então o jogador tem que esperar a
brecha do salto.

## Os outros (resumo, detalhar depois de aprovar os dois primeiros)

- **Joana** — Garra (aviso enorme, lenta, resposta: esquiva), Gancho de Marujo (puxa: só abaixar),
  Âncora (brecha longa depois). Armadilha: puxa com o gancho e emenda jab.
- **O Cremoso** — derrete e ataca de dentro da poça (não dá para socar enquanto derretido); brecha
  quando se remonta. Armadilha: finge derreter.
- **Valéria** — luta no ritmo do frevo: combos de 3 golpes no tempo da música; brecha no fim da
  dança. Armadilha (fase 3): um golpe fora do tempo.
- **Vivaldi** — a batuta conta quantos golpes vêm (2 batidas = 2 socos); fica intangível enquanto
  rege; brecha no "finale".
- **Lady Espinha** — espinhos brilham = socar dói em quem soca; voadora de lucha (esquiva lateral);
  brecha quando erra a voadora.
- **Dr. Marcov** — foguetes acendem (chama cresce) antes da investida rápida; superaquece depois de
  três investidas = brecha.
- **Batalhator** — cada estágio tem seu padrão (serras, cabeça-punho, nerd).

## Implementação

- Tabela de padrões por lutador (golpes, avisos, respostas, brechas, fases), num lugar só.
- O aviso de cada golpe vira uma animação própria no visual novo (o corpo faz o gesto) + som.
- Esquiva com direção certa passa a importar para ganchos.
- Primeiro Jack e King Frog; testar; depois os outros.
