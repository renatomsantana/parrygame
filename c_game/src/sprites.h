/*
 * sprites.h - os lutadores em pixel art, das pranchas que tools/personagens.c gera.
 * Cada pasta assets/sprites/<nome>/ traz um sprite.txt (célula, âncora dos pés,
 * guarda e, por animação, os quadros de preparação e de contato e o alcance) e
 * uma tira PNG por animação, virada para a direita.
 *
 * As tiras saem de packs pagos e ficam fora do git: sem elas (make sprites com
 * os packs em assets/sprites/), o jogo volta para os bonecos de rig.c.
 */
#ifndef APARA_SPRITES_H
#define APARA_SPRITES_H

#include <stdbool.h>

#include "raylib.h"

#define SPR_MAX_ANIMS 72

typedef struct {
    char name[40];
    Texture2D tex;
    int frames;
    int hold, contact, stop;     /* -1 = não tem */
    int reachX, reachY;          /* ponta da arma no contato, a partir dos pés */
    bool hasReach, loop;
    float frameTime;             /* segundos por quadro */
} SprAnim;

typedef struct {
    char id[24];
    int cw, ch, ax, ay;          /* célula e âncora dos pés dentro dela */
    int guardX, guardY;          /* onde a lâmina espera o golpe (prancha DEFEND) */
    bool hasGuard;
    int height;                  /* altura do corpo parado, em px */
    int count;
    SprAnim anims[SPR_MAX_ANIMS];
} SprSet;

void spr_init(void);             /* depois da janela */
void spr_shutdown(void);

/* Carrega na primeira vez. NULL quando a pasta não tem as tiras. */
const SprSet *spr_get(const char *id);
const SprAnim *spr_anim(const SprSet *s, const char *name);

typedef struct {
    bool faceLeft;
    int breath;                  /* px que o tronco desce (respiração) */
    bool flat;                   /* silhueta de uma cor só: contorno, apagão, clarão */
    Color color;                 /* tinta (ou a cor da silhueta) */
} SprDraw;

/* Desenha o quadro com os pés em `feet` (arredondado para o pixel). */
void spr_draw(const SprSet *s, const SprAnim *a, int frame, Vector2 feet, SprDraw o);

/* Tocador: um trecho de quadros [from, to] em `dur` segundos, e depois segura
 * o último quadro ou entra em loop na animação `after`. */
typedef struct {
    const SprAnim *anim;
    int from, to, frame;
    float t, dur;
    bool loop;
    const SprAnim *after;
} SprPlayer;

void spr_play(SprPlayer *p, const SprAnim *a, int from, int to, float dur); /* dur <= 0: o tempo de cada quadro */
void spr_loop(SprPlayer *p, const SprAnim *a);
void spr_update(SprPlayer *p, float dt);
bool spr_done(const SprPlayer *p);

/* Efeitos em folha (assets/sprites/_fx/<número do pack>.png): quadros de 64 x 64
 * lado a lado, uma cor por linha: 0 laranja, 1 roxo, 2 azul, 3 verde, 4 terra,
 * 5 branco, 6 malva, 7 vermelho, 8 anil. NULL quando o arquivo não está lá. */
typedef struct {
    char name[16];
    Texture2D tex;
    int frames, rows, cell;
} SprFx;

const SprFx *spr_fx(const char *name);
void spr_fx_draw(const SprFx *f, int row, int frame, Vector2 center, bool flip, Color tint);

/* Teclas e mouse de pixel (assets/sprites/_ui/): "A".."Z", "0".."9", "ESC",
 * "ENTER", "TAB", "SHIFT", "DEL", "CAPS", "SPACE"; mouse 0..3. `unit` é quantas
 * unidades da tela valem um pixel da folha. Devolvem a largura desenhada, 0 sem
 * a folha (quem chama escreve a tecla em texto). */
float spr_key(const char *key, float x, float y, float unit, bool pressed, Color tint);
float spr_mouse(int button, float x, float y, float unit, Color tint);

#endif
