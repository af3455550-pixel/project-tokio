<div align="center">

# 🎬 INKBOUND: THE LAST REEL

### *"Quando a última bobina queimar, o mundo desenhado deixa de existir."*

**Um jogo 2D de ação, plataformas e boss-rush desenhado à mão, inspirado na animação
americana dos anos 1930 — grão de película, tinta viva e jazz de fita rota.**

[![Estado](https://img.shields.io/badge/estado-em%20desenvolvimento-c69e50?style=for-the-badge)]()
[![Linguagem](https://img.shields.io/badge/C%2B%2B-17-8a6a4a?style=for-the-badge&logo=cplusplus&logoColor=white)]()
[![Plataformas](https://img.shields.io/badge/plataformas-Windows%20%7C%20Linux%20%7C%20macOS-221a17?style=for-the-badge)]()
[![Licença](https://img.shields.io/badge/licença-proprietária-9a4232?style=for-the-badge)]()

</div>

---

## 📖 Sobre o jogo

**INKBOUND: THE LAST REEL** passa-se em **Celluloid City**, uma metrópole de tinta e papel
projetada 24 vezes por segundo. Quando uma entidade conhecida apenas como **O Apagador**
começa a raspar personagens da película, a cidade começa a desaparecer quadro a quadro.

Controlas o **Nib**, um pequeno cartoon com um chapéu em forma de aparo de caneta e um
blaster de tinta, que atravessa seis regiões para recuperar as bobinas roubadas antes que
a última se queime.

Combate rápido e preciso. Bosses longos e implacáveis. Dificuldade elevada, mas **sempre
justa**: cada padrão é aprendível, cada morte é culpa tua, cada vitória é tua.

> Todos os personagens, bosses, cenários, nomes, músicas e efeitos são **originais**.
> A inspiração é a era dourada da animação (rotoscopia, *rubber hose*, *squash & stretch*),
> não qualquer jogo específico.

---

## ✨ Características

- 🖌️ **Arte 100% desenhada à mão** — contornos expressivos, texturas de papel, grão de
  película, *weave* de projetor e riscos autênticos de fita gasta.
- 🎞️ **Animação frame-by-frame** com antecipação, *squash & stretch*, *follow-through* e
  expressões faciais exageradas.
- ⚔️ **Combate de precisão** — correr, saltar, dash aéreo, esquiva com *i-frames*,
  disparo omnidirecional, ataque especial e **parry por timing** numa janela de 8 frames.
- 👑 **12 bosses multifase**, cada um com introdução cinematográfica, personalidade,
  tema musical exclusivo e derrota animada.
- 🗺️ **6 mundos** com mapa interativo, níveis de plataforma, caminhos alternativos,
  áreas escondidas, NPCs e colecionáveis.
- 🎷 **Banda sonora original** de jazz e swing gravada com *big band*, com música
  **dinâmica** que muda de arranjo conforme a fase do boss.
- 📊 **Classificação por combate** (D a S+) com base em tempo, dano sofrido, parries
  perfeitos e uso do especial.
- 🕹️ Suporte total a **teclado e comando**, resolução adaptável e reinício instantâneo.

---

## 👹 Galeria de Bosses

| # | Boss | Região | Conceito |
|---|------|--------|----------|
| 1 | **Rainha Zumbidora** | Prado de Mel-Ferrugem | Rainha-abelha mecânica de latão e cera |
| 2 | **Mestre Borrão** | Circo Sem Nome | Mágico de circo feito de tinta derramada |
| 3 | **Locomotiva Bufa-Fuligem** | Ramal Enferrujado | Comboio vivo que cospe carvão em brasa |
| 4 | **Capitão Oito-Braços** | Enseada da Espuma | Polvo pirata com oito armas diferentes |
| 5 | **Sr. Cordas** | Teatro dos Fios | Marioneta gigante que se manipula a si mesma |
| 6 | **Vinca, o Dragão de Papel** | Templo Dobrado | Dragão de origami que se redobra ao levar dano |
| 7 | **Tiquetaque Rachado** | Torre dos Ponteiros | Relógio monstruoso que rouba tempo ao jogador |
| 8 | **A Mancha** | Poço de Tinta | Criatura amorfa nascida de erros apagados |
| 9 | **Duo Fita & Cola** | Sala de Montagem | Dupla de editores que cortam o cenário ao vivo |
| 10 | **Diva Fonógrafo** | Cabaré da Estática | Cantora de rádio cujas notas são projéteis |
| 11 | **Projeccionista** | Cabine de Projeção | Servo do Apagador, muda o cenário à vontade |
| 12 | **O APAGADOR** | A Última Bobina | Boss final em cinco fases, sem checkpoints |

---

## 🎮 Controlos

| Ação | Teclado | Comando |
|------|---------|---------|
| Mover | `A` `D` / setas | Stick esquerdo |
| Apontar para cima | `W` / ↑ | Stick esquerdo ↑ |
| Saltar | `Espaço` | A / Cross |
| Disparar | `J` | X / Square |
| Dash aéreo | `K` | RB / R1 |
| Esquiva | `L` | LB / L1 |
| Parry | `I` | Y / Triangle |
| Ataque especial | `U` | B / Circle |
| Pausa | `Esc` | Start |
| Reinício rápido | `R` | — |

---

## 🛠️ Compilar

**Requisitos:** compilador com C++17, [raylib](https://www.raylib.com/) 5.x, CMake 3.16+.

```bash
git clone https://github.com/<o-teu-utilizador>/inkbound-the-last-reel.git
cd inkbound-the-last-reel
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/inkbound
```

**Compilação direta (Linux):**
```bash
g++ -std=c++17 -O2 -Wall src/inkbound.cpp -o inkbound -lraylib -lm -ldl -lpthread -lGL -lX11
```

**Windows (MinGW-w64):**
```bash
g++ -std=c++17 -O2 -Wall src/inkbound.cpp -o inkbound.exe -lraylib -lopengl32 -lgdi32 -lwinmm
```

---

## 🗂️ Arquitetura

Sistemas independentes, cada um editável sem tocar nos restantes:

```
PlayerController   movimento, dash, esquiva, parry, estados do herói
CombatSystem       hitboxes, dano, i-frames, hitstop, knockback
EnemySystem        FSM: IDLE → PATROL → ALERT → ATTACK → RECOVER → DEFEATED
BossSystem         máquinas de estados por fase, transições e cinemáticas
AnimationSystem    clips frame-by-frame, squash & stretch, easing
AudioSystem        mixer, música dinâmica por fase, SFX cartoon
SaveSystem         perfis, progresso, classificações, colecionáveis
UIManager          menus, HUD, mapa, inventário, resultados
DialogueSystem     diálogos typewriter, retratos, cenas
LevelManager       tilemaps, checkpoints, parallax, segredos
CameraController   follow com lookahead, shake, clamp, enquadramento de boss
```

---

## 🗺️ Roadmap

- [x] Fase 1 — Núcleo: movimento, combate, câmara, render de película
- [x] Fase 2 — EnemySystem e primeiro boss
- [ ] Fase 3 — Os 12 bosses completos
- [ ] Fase 4 — Os 6 mundos e mapa interativo
- [ ] Fase 5 — Narrativa, cinemáticas e final secreto
- [ ] Fase 6 — Polimento, acessibilidade e localização

---

<div align="center">

# 🎩 CRÉDITOS

### *INKBOUND: THE LAST REEL*
**Produzido por FIRE STATE STUDIOS**

</div>

---

### DIREÇÃO

| | |
|---|---|
| **Game Director** | Aurélio "Rell" Vasconcelos |
| **Creative Director** | Marisol Duquesne |
| **Technical Director** | Ivan Kolarov |

### ENGENHARIA

| | |
|---|---|
| **Lead C++ Programmer** | Tobias Reinhardt-Vale |
| **Gameplay Programmer** | Emiko Tanabe |
| **Engine Programmer** | Casimir Andrzejak |
| **AI Programmer** | Nadia Fontaine-Aro |
| **2D Rendering Engineer** | Rustam Belaïd |
| **Physics Engineer** | Gwen Halloran |
| **Tools Programmer** | Dmitri Sallowbrook |
| **Performance Engineer** | Priya Ramanathan |
| **Build Engineer** | Oskar Lindqvist |

### DESIGN

| | |
|---|---|
| **Level Designer** | Hugo Marcanti |
| **Boss Designer** | Selma Okonkwo |
| **Character Designer** | Léo Batiste-Marchand |
| **UI/UX Designer** | Yara Solheim |
| **Narrative Designer** | Constance "Connie" Ferrow |

### ARTE E ANIMAÇÃO

| | |
|---|---|
| **2D Animator** | Rosalie Vantongeren |
| **2D Animator** | Kenji Amagawa |
| **VFX Artist** | Bruno Falqueiro |
| **Technical Artist** | Anka Petrescu |

### ÁUDIO

| | |
|---|---|
| **Sound Designer** | Théo Bramwell |
| **Composer** | Vivienne "Viv" Ashcombe |
| **Orquestração de metais** | The Copperline Eight |

### QUALIDADE

| | |
|---|---|
| **QA Engineer** | Samir Oyelaran |
| **QA Engineer** | Iolanda Crest |

---

<div align="center">

### AGRADECIMENTOS ESPECIAIS

Às famílias e amigos de toda a equipa, que aguentaram três anos de conversas
sobre a curvatura correta de uma linha de tinta.

Aos animadores anónimos dos anos 1930, que desenharam um quadro de cada vez
sem saber que estavam a inventar uma linguagem.

E a ti, que estás a ler os créditos.

---

**© 2026 Fire State Studios. Todos os direitos reservados.**

*Inkbound: The Last Reel, Celluloid City, Nib, O Apagador e todos os personagens,
nomes e composições são marcas e obras originais da Fire State Studios.*

*Nenhuma bobina foi realmente queimada durante a produção.*

**🎬 FIM DA BOBINA 🎬**

</div>

---

## 🛠️ Desenvolvimento (build, controlos, QA)

Instruções completas de build (Windows MSVC/MinGW e Linux), tabela de controlos,
arquitetura, testes e o harness de QA headless estão em
[`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md).
