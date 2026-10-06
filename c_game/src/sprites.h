/*
 * sprites.h - os lutadores em pixel art, das pranchas que tools/personagens.c gera.
 * Cada pasta assets/sprites/<nome>/ traz um sprite.txt (célula, âncora dos pés,
 * guarda e, por animação, os quadros de preparação e de contato e o alcance) e
 * uma tira PNG por animação, virada para a direita.
 *
 * As tiras saem de packs pagos e ficam fora do git: sem elas (make sprites com
 * os packs em assets/sprites/), os personagens não são desenhados.
 */
#ifndef APARA_SPRITES_H
#define APARA_SPRITES_H

#include <stdbool.h>

#include "raylib.h"

#define SPR_MAX_ANIMS 72
#define SPR_MAX_FRAMES 64

typedef struct {
    char name[40];
    Texture2D tex;
    Texture2D cleanTex;           /* mesma tira, sem rastro; corpo e arma intactos */
    int frames;
    int hold, contact, stop;     /* -1 = não tem */
    int reachX, reachY;          /* ponta da arma no contato, a partir dos pés */
    bool hasReach, loop;
    float frameTime;             /* segundos por quadro */
    float times[16];             /* `tempos`: a duração de cada quadro (s), quando varia */
    int ntimes;
    Vector2 weapon[SPR_MAX_FRAMES]; /* centro da lâmina neste quadro, relativo aos pés */
    bool hasWeapon[SPR_MAX_FRAMES];
    Vector2 offhand[SPR_MAX_FRAMES];
    bool hasOffhand[SPR_MAX_FRAMES];
    float body[SPR_MAX_FRAMES];     /* o meio do corpo (da cintura para baixo) neste quadro, em x, relativo aos pés: de onde se mede o avanço de um quadro para o outro */
    bool hasBody[SPR_MAX_FRAMES];
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
    bool withoutTrail;           /* substituição: não desenhar o arco antigo junto */
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
    float limit;                 /* laço: acaba depois deste tempo (0 = nunca) */
    const SprAnim *after;
} SprPlayer;

void spr_play(SprPlayer *p, const SprAnim *a, int from, int to, float dur); /* dur <= 0: o tempo de cada quadro */
void spr_loop(SprPlayer *p, const SprAnim *a);
void spr_cycle(SprPlayer *p, const SprAnim *a, float time);  /* o laço por `time` segundos */
void spr_update(SprPlayer *p, float dt);
bool spr_done(const SprPlayer *p);
/* A tira só com a lâmina, para o apagão do yoru: do corpo ficam só as adagas, nas cores delas. É aço o pixel frio (o azul igual ou maior que o vermelho, o que deixa
 * a pele de fora) que está abaixo da altura dos olhos e é bem claro (branco e lilás) em qualquer lugar, ou é violeta mas perto de um ponto de lâmina
 * (`arma` e `arma2` do sprite.txt: o violeta também é do cabelo e das botas, e longe da lâmina fica apagado). Cada tira é feita na primeira vez que se pede,
 * e fica guardada. NULL se a tira não está lá. */
const SprAnim *spr_lamina(const SprSet *s, const SprAnim *a);
bool spr_pixel_de_lamina(Color c, int acimaDosPes, bool pertoDoPonto);   /* a regra de cor e altura, aberta para o teste */
/* Quanto o corpo avança (px, + para onde o lutador olha) do quadro `de` para o quadro `para`; 0 se algum dos dois não tem corpo. */
float spr_salto_do_corpo(const SprAnim *a, int de, int para);
bool spr_weapon_point(const SprPlayer *p, Vector2 feet, bool faceLeft, int breath, Vector2 *point);
bool spr_offhand_point(const SprPlayer *p, Vector2 feet, bool faceLeft, int breath, Vector2 *point);

/* Efeitos em folha (assets/sprites/_fx/<número do pack>.png): quadros de 64 x 64
 * lado a lado, uma cor por linha: 0 laranja, 1 roxo, 2 azul, 3 verde, 4 terra,
 * 5 branco, 6 malva, 7 vermelho, 8 anil. NULL quando o arquivo não está lá. */
typedef struct {
    char name[16];
    Texture2D tex;
    int frames, rows, cell;
    Vector2 pivot[12];            /* centro dos pixels no quadro principal do slash */
} SprFx;

const SprFx *spr_fx(const char *name);
/* O pack slash usa células de 96 px e linhas com durações diferentes; as
 * colunas transparentes de preenchimento não pertencem à animação. */
int spr_fx_row_frames(const SprFx *f, int row);
int spr_fx_cache_count(void);    /* diagnóstico: quantas folhas já foram consultadas */
void spr_ui_preload(void);       /* ícones carregados antes do primeiro quadro */
void spr_fx_draw(const SprFx *f, int row, int frame, Vector2 center, bool flip, Color tint);
/* O mesmo, em outra escala (a poeira menor que a folha do pack). */
void spr_fx_draw_scaled(const SprFx *f, int row, int frame, Vector2 center, bool flip, Color tint, float scale);
void spr_fx_draw_rotated(const SprFx *f, int row, int frame, Vector2 center, bool flip,
                         Color tint, float scale, float rotation, bool flatTint);

void spr_fx_draw_weapon(const SprFx *f, int row, int frame, Vector2 at, bool flip, Color tint, float scale);

/* Teclas e mouse de pixel (assets/sprites/_ui/): "A".."Z", "0".."9", "ESC",
 * "ENTER", "TAB", "SHIFT", "DEL", "CAPS", "SPACE"; mouse 0..3. `unit` é quantas
 * unidades da tela valem um pixel da folha. Devolvem a largura desenhada, 0 sem
 * a folha (quem chama escreve a tecla em texto). */
float spr_key(const char *key, float x, float y, float unit, bool pressed, Color tint);

#endif
