#!/usr/bin/env python3
"""Gera os lutadores do aparar a partir das pranchas do Musashi (pack A).

Cada personagem sai do mesmo corpo, mas o script não faz só troca de paleta:
  - tira o chapéu (só dois personagens ficam com um) e desenha a cabeça:
    coque, rabo de cavalo, capuz, careca, cabelo espetado...
  - troca a arma: odachi, florete, adagas, espada pesada, lança, cajado,
    garras, espada curta, duas espadas;
  - pinta o rastro do golpe com o elemento de cada um (fogo, água, raio...);
  - acrescenta acessórios atrás do corpo: cachecol, casco, rabo de cavalo.

Tudo é troca e acréscimo de pixel inteiro, sem escala nem rotação. O mesmo
comando sempre gera os mesmos pixels, então dá para rodar em qualquer máquina.

Uso (de dentro de c_game/):
    pip install pillow numpy
    python3 tools/personagens.py                    # todas as pranchas, todos os personagens
    python3 tools/personagens.py --so enjin kage    # só alguns
    python3 tools/personagens.py --folhas           # também as folhas de conferência

Entrada: assets/sprites/_original/*.png (tiras horizontais de quadros 106 x 84,
         viradas para a direita). Na primeira vez é copiada de
         assets/sprites/musashi/, que passa a receber o Musashi sem chapéu.
Saída:   assets/sprites/<personagem>/<ANIM>.png, um sprite.txt com o alcance
         da arma no contato, e assets/sprites/_folhas/ com as folhas.
Detalhes e o elenco em docs/PERSONAGENS.md.
"""
import argparse
import math
import os
import shutil
import sys
from collections import deque

import numpy as np
from PIL import Image

CELL_W, CELL_H = 106, 84

# --------------------------------------------------------------------------
# Paleta de origem: cada cor do Samurai #3 vira uma letra.
# --------------------------------------------------------------------------
SRC = {
    (255, 255, 255): 'W',  # luz da camisa, lâmina, rastro
    (199, 207, 221): 'L',  # camisa, chapéu, borda da lâmina e do rastro
    (146, 161, 185): 'M',  # camisa e chapéu, meio-tom
    (101, 115, 146): 'D',  # camisa e chapéu, sombra
    (180, 180, 180): 'g',  # lâmina borrada no movimento
    (93, 93, 93): '9',     # hakama, do claro...
    (61, 61, 61): '6',
    (39, 39, 39): '3',
    (27, 27, 27): '2',
    (19, 19, 19): '1',     # ...ao escuro; também cabelo e pomo
    (28, 18, 28): 'h',     # bainha (saya)
    (12, 46, 68): 'b',     # cabo (tsuka)
    (230, 156, 105): 'S',  # pele
    (191, 111, 74): 's',
    (138, 72, 54): 'k',
    (87, 28, 39): 'r',
}
# Os quase-brancos da camisa; todos os outros quase-brancos são da lâmina.
SHIRT_NEAR_WHITE = {(244, 252, 254), (246, 251, 255)}

# O chapéu é sempre o mesmo carimbo de 17 x 8, só muda de lugar.
HAT = [
    ".........MM......",
    ".......MMLMD.....",
    "....MMMMLLMDD....",
    "..MMMMMLLMMDDD...",
    "DMMMMMMMMMMDDDD..",
    ".DDMMMMMMMMDDDDD.",
    "...DDDMMMMMDDDDDD",
    "......DDDDDDDDD..",
]
HAT_PIX = [(x, y, k) for y, row in enumerate(HAT) for x, k in enumerate(row) if k != '.']

# Rótulos das partes.
NONE, HATL, HATFX, HAIR, FACE, SKIN, SHIRT, DARK, SAYA, HANDLE, BLADE, SMEAR, OTHER = range(13)


def rgb(h):
    h = h.lstrip('#')
    return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16))


def hsh(*v):
    """Número pseudoaleatório estável (0..1) a partir de inteiros e textos."""
    x = 2166136261
    for a in v:
        for ch in str(a):
            x = ((x ^ ord(ch)) * 16777619) & 0xffffffff
        x = ((x ^ 0x9e) * 16777619) & 0xffffffff
    return (x & 0xffffff) / float(0x1000000)


# --------------------------------------------------------------------------
# Segmentação
# --------------------------------------------------------------------------
def classify(fr):
    H, W = fr.shape[:2]
    c = np.full((H, W), '.', dtype='<U1')
    packed = (fr[:, :, 0].astype(np.int64) << 16) | (fr[:, :, 1].astype(np.int64) << 8) | fr[:, :, 2]
    vis = fr[:, :, 3] > 0
    unknown = set()
    for p in np.unique(packed[vis]):
        t = (int(p) >> 16, (int(p) >> 8) & 255, int(p) & 255)
        if t in SRC:
            k = SRC[t]
        elif min(t) >= 236:
            k = 'v' if t in SHIRT_NEAR_WHITE else 'w'
        else:
            k = '?'
            unknown.add(t)
        c[(packed == p) & vis] = k
    return c, unknown


def find_hat(c):
    """Acha o carimbo do chapéu. Aceita braços e cabo passando na frente."""
    H, W = c.shape
    pad = 20
    P = np.full((H + 2 * pad, W + 2 * pad), '.', dtype='<U1')
    P[pad:pad + H, pad:pad + W] = c
    hatcol = np.isin(P, ['M', 'L', 'D'])
    transp = P == '.'
    white = np.isin(P, ['W', 'w', 'v', 'g'])
    oy0, oy1, ox0, ox1 = -2, H - 6, -6, W - 10
    S = np.zeros((oy1 - oy0, ox1 - ox0))
    for dx, dy, k in HAT_PIX:
        ys, xs = pad + oy0 + dy, pad + ox0 + dx
        sl = (slice(ys, ys + oy1 - oy0), slice(xs, xs + ox1 - ox0))
        ex = P[sl] == k
        S += 2 * ex + 1 * (hatcol[sl] & ~ex) - 2 * transp[sl] - 1 * white[sl]
    i = np.unravel_index(np.argmax(S), S.shape)
    return float(S[i]), int(i[1] + ox0), int(i[0] + oy0)


def components(mask, conn=8):
    H, W = mask.shape
    lab = -np.ones((H, W), int)
    comps = []
    nb = [(1, 0), (-1, 0), (0, 1), (0, -1)]
    if conn == 8:
        nb += [(1, 1), (1, -1), (-1, 1), (-1, -1)]
    for y, x in zip(*np.nonzero(mask)):
        if lab[y, x] >= 0:
            continue
        q = deque([(y, x)])
        lab[y, x] = len(comps)
        pts = []
        while q:
            cy, cx = q.popleft()
            pts.append((cy, cx))
            for dy, dx in nb:
                ny, nx = cy + dy, cx + dx
                if 0 <= ny < H and 0 <= nx < W and mask[ny, nx] and lab[ny, nx] < 0:
                    lab[ny, nx] = len(comps)
                    q.append((ny, nx))
        comps.append(pts)
    return lab, comps


def shift(m, dy, dx):
    """Desloca uma máscara sem dar a volta nas bordas."""
    H, W = m.shape
    out = np.zeros_like(m)
    ys, yd = (slice(0, H - dy), slice(dy, H)) if dy >= 0 else (slice(-dy, H), slice(0, H + dy))
    xs, xd = (slice(0, W - dx), slice(dx, W)) if dx >= 0 else (slice(-dx, W), slice(0, W + dx))
    out[yd, xd] = m[ys, xs]
    return out


def dilate(m, r=1, cross=False):
    out = m.copy()
    for dy in range(-r, r + 1):
        for dx in range(-r, r + 1):
            if cross and abs(dy) + abs(dx) > r:
                continue
            out |= shift(m, dy, dx)
    return out


class Seg:
    pass


def segment(fr, fix=None):
    """Separa o quadro em partes. `fix` pode forçar a origem do chapéu."""
    c, unknown = classify(fr)
    H, W = c.shape
    s = Seg()
    s.c, s.unknown = c, unknown
    s.lab = np.zeros((H, W), np.int8)
    score, ox, oy = find_hat(c)
    if fix and 'hat' in fix:
        ox, oy = fix['hat']
        score = 999
    s.ox, s.oy, s.score = ox, oy, score
    s.erase = (fix or {}).get('apaga', [])
    s.has_hat = score >= 60 and not (fix and fix.get('nohat'))

    hat = np.zeros((H, W), bool)
    if s.has_hat:
        clean = score >= 150
        for dx, dy, k in HAT_PIX:
            x, y = ox + dx, oy + dy
            if 0 <= x < W and 0 <= y < H:
                if c[y, x] == k or (clean and c[y, x] in 'MLD'):
                    hat[y, x] = True
    # Rastro de movimento do chapéu (no DASH): cor de chapéu na altura do
    # chapéu, fora do carimbo e antes da aba de trás.
    # Só conta o que encosta no chapéu pela esquerda, linha a linha.
    hatfx = np.zeros((H, W), bool)
    if s.has_hat:
        for y in range(max(0, oy), min(H, oy + 8)):
            row = [x for x in range(W) if hat[y, x]]
            if not row:
                continue
            x, gap = row[0] - 1, 0
            while x >= 0 and gap <= 1:
                if c[y, x] in 'MLDWv':
                    hatfx[y, x] = True
                    gap = 0
                else:
                    gap += 1
                x -= 1

    white = np.isin(c, ['W', 'L', 'w', 'v', 'g'])
    block = np.isin(c, ['M', 'D', 'v', 'S', 's', 'k', 'b', 'r']) & ~hat & ~hatfx
    # Rastro: mancha grande de branco e cinza claro, sem sombra de camisa
    # nem pele por perto, e que sai para fora do corpo.
    near_block = dilate(block, 1)
    wsum = np.zeros((H, W), int)
    bsum = np.zeros((H, W), bool)
    for dy in range(-2, 3):
        for dx in range(-2, 3):
            wsum += shift(white, dy, dx)
            bsum |= shift(block, dy, dx)
    seed = white & ~bsum & (wsum >= 16) & ~hat & ~hatfx
    grow = white & ~hat & ~hatfx & (c != 'v') & ~near_block
    _, comps = components(grow, 4)
    smear = np.zeros((H, W), bool)
    for pts in comps:
        if len(pts) < 20 or not any(seed[p] for p in pts):
            continue
        outside = sum(1 for (py, px) in pts if not (ox - 3 <= px <= ox + 20 and oy - 1 <= py <= oy + 34))
        if outside >= 8:
            for p in pts:
                smear[p] = True
    # A borda cinza do rastro que encosta na hakama também é rastro.
    for _ in range(2):
        ring = dilate(smear, 1, cross=True) & np.isin(c, ['L', 'W', 'w', 'g']) & ~hat & ~near_block & ~smear
        smear |= ring

    # Camisa x lâmina: a camisa é grossa (blocos 2 x 2), a lâmina é um fio.
    shirtcol = np.isin(c, ['W', 'L', 'M', 'D', 'v', 'w']) & ~hat & ~smear & ~hatfx
    full = shirtcol[:-1, :-1] & shirtcol[1:, :-1] & shirtcol[:-1, 1:] & shirtcol[1:, 1:]
    thick = np.zeros((H, W), bool)
    thick[:-1, :-1] |= full
    thick[1:, :-1] |= full
    thick[:-1, 1:] |= full
    thick[1:, 1:] |= full
    nthick = sum(shift(thick, dy, dx).astype(int) for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)))
    shirt = thick | (shirtcol & ((nthick >= 2) | np.isin(c, ['M', 'D', 'v'])))
    blade = (white | (c == 'w')) & ~hat & ~hatfx & ~smear & ~shirt

    # Cabelo e rosto ficam debaixo do chapéu, sempre no mesmo lugar dele.
    rel_x = np.arange(W)[None, :] - ox
    rel_y = np.arange(H)[:, None] - oy
    hair_zone = (rel_x >= 0) & (rel_x <= 8) & (rel_y >= 5) & (rel_y <= 9)
    face_zone = (rel_x >= 6) & (rel_x <= 13) & (rel_y >= 7) & (rel_y <= 11)
    darkc = np.isin(c, ['1', '2', '3', '6', '9'])
    skinc = np.isin(c, ['S', 's', 'k'])
    handle = (c == 'b') | ((c == '1') & dilate(c == 'b', 1) & ~hair_zone)

    L = s.lab
    L[c != '.'] = OTHER
    L[darkc] = DARK
    L[skinc] = SKIN
    L[c == 'h'] = SAYA
    L[handle] = HANDLE
    L[shirt] = SHIRT
    L[blade] = BLADE
    L[smear] = SMEAR
    if s.has_hat:
        L[darkc & hair_zone] = HAIR
        L[skinc & face_zone] = FACE
    L[hatfx] = HATFX
    L[hat] = HATL
    L[c == '.'] = NONE

    s.blades = find_blades(s)
    return s


def find_blades(s):
    """Cada fio de lâmina vira uma reta: empunhadura, direção e ponta."""
    L = s.lab
    H, W = L.shape
    _, comps = components(L == BLADE, 8)
    handles = [(x, y) for y, x in zip(*np.nonzero(L == HANDLE))]
    hands = [(x, y) for y, x in zip(*np.nonzero(L == SKIN))]
    out = []
    for pts in comps:
        if len(pts) < 4:
            continue
        P = np.array([(x, y) for y, x in pts], float)
        m = P.mean(0)
        cov = np.cov((P - m).T)
        w, v = np.linalg.eigh(cov)
        u = v[:, 1]
        t = (P - m) @ u
        tmin, tmax = t.min(), t.max()
        if tmax - tmin < 4:
            continue
        e0, e1 = m + u * tmin, m + u * tmax

        def score(q):
            q = np.array(q, float)
            d = q - m
            perp = abs(d[0] * u[1] - d[1] * u[0])
            along = d @ u
            beyond = max(0.0, tmin - along, along - tmax)
            inside = 1.0 if tmin < along < tmax else 0.0
            return perp * 2 + beyond + inside * 30

        best = None
        for q in handles:
            sc = score(q)
            if best is None or sc < best[0]:
                best = (sc, q)
        if best is None or best[0] > 14:
            for q in hands:
                sc = score(q) + 3
                if best is None or sc < best[0]:
                    best = (sc, q)
        if best is not None and best[0] <= 20:
            q = np.array(best[1], float)
            th = (q - m) @ u
        else:
            # sem mão por perto: a ponta mais perto do meio do corpo é o cabo
            body = np.array([s.ox + 8, s.oy + 18], float)
            th = tmin if np.linalg.norm(e0 - body) < np.linalg.norm(e1 - body) else tmax
        if abs(th - tmin) > abs(th - tmax):
            u, t, tmin, tmax, th = -u, -t, -tmax, -tmin, -th
        hilt = m + u * th
        b = Seg()
        b.pts = [(int(x), int(y)) for x, y in P]
        b.dist = {(int(x), int(y)): float(tt - th) for (x, y), tt in zip(P, t)}
        b.u = u
        b.hilt = hilt
        b.near = tmin - th     # distância do cabo até o começo visível
        b.far = tmax - th      # distância do cabo até a ponta
        out.append(b)
    return out


# --------------------------------------------------------------------------
# Desenho
# --------------------------------------------------------------------------
class Canvas:
    def __init__(self, fr, seg):
        self.a = fr.copy()
        self.seg = seg
        self.H, self.W = fr.shape[:2]
        # onde dá para desenhar "atrás" do corpo: só no vazio original
        self.empty = seg.lab == NONE
        self.drawn = np.zeros((self.H, self.W), bool)

    def ok(self, x, y):
        return 0 <= x < self.W and 0 <= y < self.H

    def put(self, x, y, col, a=255):
        if self.ok(x, y):
            self.a[y, x] = (col[0], col[1], col[2], a)
            self.drawn[y, x] = True

    def clear(self, x, y):
        if self.ok(x, y):
            self.a[y, x] = (0, 0, 0, 0)

    def behind(self, x, y, col):
        """Pinta só onde não havia nada (fica atrás do corpo)."""
        if self.ok(x, y) and self.empty[y, x] and self.a[y, x, 3] == 0:
            self.put(x, y, col)
            return True
        return False

    def is_empty(self, x, y):
        return self.ok(x, y) and self.a[y, x, 3] == 0


def line_pts(x0, y0, x1, y1):
    """Pontos inteiros de uma reta, um por passo no eixo maior."""
    n = int(max(abs(x1 - x0), abs(y1 - y0)))
    if n == 0:
        return [(int(round(x0)), int(round(y0)))]
    out = []
    for i in range(n + 1):
        t = i / n
        p = (int(math.floor(x0 + (x1 - x0) * t + 0.5)), int(math.floor(y0 + (y1 - y0) * t + 0.5)))
        if not out or out[-1] != p:
            out.append(p)
    return out


def ramp_map(src_keys, ramp):
    return {k: ramp[min(i, len(ramp) - 1)] for i, k in enumerate(src_keys)}


def recolor(cv, ch):
    s = cv.seg
    c, L = s.c, s.lab
    shirt = ramp_map('WLMD', ch['camisa'])
    shirt['v'] = shirt['W']
    shirt['w'] = shirt['W']
    shirt['g'] = shirt['L']
    hak = ramp_map('96321', ch['hakama'])
    skin = ramp_map('Ssk', ch['pele'])
    hair = ch['cabelo']
    blade = ch['lamina']
    hatp = ch.get('chapeu_cor')
    H, W = L.shape
    for y in range(H):
        for x in range(W):
            lb, k = L[y, x], c[y, x]
            if lb == NONE:
                continue
            col = None
            if lb == SHIRT:
                col = shirt.get(k)
            elif lb == DARK:
                col = hak.get(k)
            elif lb in (SKIN, FACE):
                col = skin.get(k)
            elif lb == HAIR:
                col = hair[0] if k in '12' else hair[1]
            elif lb == SAYA:
                col = ch['saya']
            elif lb == HANDLE:
                col = ch['cabo'] if k == 'b' else hak['1']
            elif lb == BLADE:
                col = blade[0] if k in 'Wwv' else blade[1]
            elif lb in (HATL, HATFX):
                if hatp:
                    col = {'L': hatp[0], 'M': hatp[1], 'D': hatp[2]}.get(k, hatp[0])
            if col is not None:
                cv.a[y, x, :3] = col
    # Faixa (obi): a última linha da camisa em cima da hakama.
    if ch.get('obi'):
        shirt_m = L == SHIRT
        below = np.zeros_like(shirt_m)
        below[:-1] = L[1:] == DARK
        for y, x in zip(*np.nonzero(shirt_m & below)):
            cv.a[y, x, :3] = ch['obi']
    # Bainha: quem não usa espada comprida não carrega a saya.
    if ch.get('saya') is None:
        for y, x in zip(*np.nonzero(L == SAYA)):
            erase_px(cv, x, y)


def erase_px(cv, x, y):
    """Apaga um pixel; se ele estava no meio do corpo, fecha com a cor vizinha."""
    L = cv.seg.lab
    H, W = L.shape
    body = []
    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        nx, ny = x + dx, y + dy
        if 0 <= nx < W and 0 <= ny < H and L[ny, nx] in (SHIRT, DARK, SKIN) and cv.a[ny, nx, 3]:
            body.append(tuple(int(v) for v in cv.a[ny, nx, :3]))
    if len(body) >= 3:
        best = max(set(body), key=body.count)
        cv.a[y, x] = (*best, 255)
    else:
        cv.a[y, x] = (0, 0, 0, 0)


# ----- rastro ---------------------------------------------------------------
def paint_smear(cv, ch, anim, idx):
    """Rastro em três tons (miolo, borda, cauda) e partículas do elemento."""
    L, c = cv.seg.lab, cv.seg.c
    H, W = L.shape
    sm = L == SMEAR
    if not sm.any():
        return
    inner_c, edge_c, tail_c = ch['rastro'][0], ch['rastro'][1], ch['rastro'][2]
    inner = sm & shift(sm, 1, 0) & shift(sm, -1, 0) & shift(sm, 0, 1) & shift(sm, 0, -1)
    for y, x in zip(*np.nonzero(sm)):
        if c[y, x] in 'Lg':
            col = tail_c
        elif inner[y, x]:
            col = inner_c
        else:
            col = edge_c
        cv.a[y, x, :3] = col
    el = ch.get('elemento')
    if not el:
        return
    # Partículas soltas perto da borda de fora do rastro.
    ring = dilate(sm, 2) & (L == NONE)
    for y, x in zip(*np.nonzero(ring)):
        r = hsh('p', anim, idx, x, y)
        if el == 'fogo' and r < 0.07:
            cv.put(x, y, (255, 120, 30) if r < 0.035 else (255, 200, 70))
        elif el == 'agua' and r < 0.05:
            cv.put(x, y, (150, 230, 255))
        elif el == 'raio' and r < 0.05:
            # faísca em zigue-zague saindo do rastro
            cv.put(x, y, (240, 250, 255))
            nx, ny = x + (1 if r < 0.025 else -1), y - 1
            if cv.ok(nx, ny) and L[ny, nx] == NONE and cv.is_empty(nx, ny):
                cv.put(nx, ny, (90, 170, 255))
        elif el == 'roxo' and r < 0.04:
            cv.put(x, y, (190, 110, 255))
        elif el == 'pena' and r < 0.035:
            cv.put(x, y, (40, 30, 36))
            if cv.ok(x - 1, y + 1) and cv.is_empty(x - 1, y + 1):
                cv.put(x - 1, y + 1, (200, 40, 50))
        elif el == 'terra' and r < 0.05 and y > cv.seg.oy + 26:
            cv.put(x, y, (150, 120, 70))
        elif el == 'vento' and r < 0.03:
            cv.put(x, y, (220, 255, 170))
        elif el == 'ouro' and r < 0.035:
            cv.put(x, y, (255, 214, 90))


# ----- cabeças --------------------------------------------------------------
# Coordenadas relativas ao canto do chapéu (ox, oy). O rosto do sprite fica em
# x 7..11, y 8..10; a nuca em x 4; a gola começa em y 9.
# Cada linha é (y, x inicial, texto). Letras:
#   H h i  cabelo escuro, meio, luz      F f k  pele clara, meio, escura
#   e      olho                          A a    destaque claro, escuro
#   B      destaque 2 (ouro, anel)       K m    máscara escura, meio
#   X      apaga                         .      não mexe
# 'frente' pinta por cima do chapéu e do rosto; 'atras' só no vazio.
HEADS = {
    # Musashi: coque no alto da nuca, preso com fita vermelha.
    'coque': {'frente': [
        (2, 5, "HH"),
        (3, 4, "HhiH"),
        (4, 5, "aA"),
        (5, 5, "HHhiH"),
        (6, 4, "HHHHhiH"),
        (7, 4, "HHHHHHHF"),
        (8, 4, "HHHfFFeF"),
    ]},
    # Hanzo: coque grande de cabelo branco, testa alta, sem barba.
    'mestre': {'frente': [
        (1, 4, "HHH"),
        (2, 3, "HhiiH"),
        (3, 4, "HhhH"),
        (4, 5, "aA"),
        (5, 5, "HHhiH"),
        (6, 4, "HHHhiiF"),
        (7, 4, "HHHhFFFF"),
        (8, 4, "HHHfFkeF"),
    ]},
    # Raijin: cabelo curto com dois tufos de chifre e faixa amarela.
    'touro': {'frente': [
        (1, 4, "H"), (1, 10, "H"),
        (2, 4, "Hh"), (2, 9, "hH"),
        (3, 4, "HhHHiH"),
        (4, 3, "HHHHhiiH"),
        (5, 3, "HHHHHHHHH"),
        (6, 3, "aAAAAAAAA"),
        (7, 3, "HHHHHfFFF"),
        (8, 4, "HHHfFFeF"),
    ], 'atras': [
        [(6, 1, "AA"), (7, 0, "Aa"), (8, -1, "Aa"), (9, -1, "a")],
        [(6, 1, "AA"), (7, 1, "aA"), (8, 0, "aA"), (9, 0, "a")],
    ]},
    # Shizuku: franja e rabo de cavalo alto.
    'rabo': {'frente': [
        (3, 4, "A"),
        (4, 4, "aHHhH"),
        (5, 4, "HHHHhiH"),
        (6, 4, "HHHHHHiH"),
        (7, 4, "HHHHHHHH"),
        (8, 4, "HHHfFFeH"),
        (9, 4, "H"),
    ], 'atras': [
        [(3, 2, "HH"), (4, 1, "HH"), (5, 0, "HHH"), (6, 0, "HH"), (7, -1, "HH"), (8, -1, "HH"), (9, -1, "H"), (10, -2, "H")],
        [(3, 2, "HH"), (4, 1, "HH"), (5, 1, "HHH"), (6, 0, "HH"), (7, 0, "HH"), (8, -1, "HH"), (9, -1, "H"), (10, -1, "H")],
    ]},
    # Kage: capuz ninja, só a fresta dos olhos, fitas roxas atrás.
    'capuz': {'frente': [
        (3, 6, "KK"),
        (4, 5, "KKmK"),
        (5, 4, "KKKKmK"),
        (6, 4, "KKKKKmmK"),
        (7, 4, "KAAAAAAA"),
        (8, 4, "KKKKFeFF"),
        (9, 8, "KKKK"),
        (10, 8, "KKK"),
    ], 'fitas': [(7, 3, 11), (8, 3, 8)]},
    # Hayate: cabelo espetado varrido pelo vento.
    'vento': {'frente': [
        (3, 7, "H"),
        (4, 5, "HHhH"),
        (5, 2, "HH.HHHhiH"),
        (6, 1, "HHHHHHHhiHH"),
        (7, 3, "HH.HHHHHHF"),
        (8, 4, "HHHfFFeF"),
    ]},
    # Genbu: careca, bigode e barbicha grisalhos.
    'careca': {'frente': [
        (5, 5, "fFFFf"),
        (6, 4, "fFFiFFf"),
        (7, 4, "fFFFFFFF"),
        (8, 4, "kffkFFeF"),
        (10, 8, "hhh"),
        (11, 9, "hh"),
    ]},
    # Enjin: cabelo em chamas, alto.
    'chamas': {'frente': [
        (0, 8, "H"),
        (1, 6, "H.Hh"),
        (2, 5, "HhHhi"),
        (3, 3, "H.HhhiH"),
        (4, 2, "HHHhhiiH"),
        (5, 3, "HHHHhhiH"),
        (6, 3, "HHHHHhhiH"),
        (7, 4, "HHHHHHhF"),
        (8, 4, "HHHfFFeF"),
    ]},
    # Suiren: cabelo curto e faixa turquesa com pontas soltas.
    'faixa': {'frente': [
        (4, 5, "HHhH"),
        (5, 4, "HHHhiiH"),
        (6, 4, "AAAAAAAA"),
        (7, 4, "HHHHHHHF"),
        (8, 4, "HHHfFFeF"),
    ], 'atras': [
        [(6, 2, "aA"), (7, 0, "aA"), (8, -1, "a")],
        [(6, 2, "aA"), (7, 1, "aa"), (8, 0, "a"), (8, -2, "a")],
    ]},
    # Karasu: cabelo em penas para trás, olho vermelho.
    'corvo': {'frente': [
        (4, 5, "HHHH"),
        (5, 2, "HH.HHHhiH"),
        (6, 0, "HHHHHHHHhiH"),
        (7, 2, "HHHHHHHHHF"),
        (8, 3, "HHHHfFeF"),
        (9, 2, "HH"),
    ]},
    # Jinshi: cabelo grisalho comprido caindo nas costas, barba.
    'eremita': {'frente': [
        (4, 5, "HHhH"),
        (5, 4, "HHHhiH"),
        (6, 3, "HHHHHhiH"),
        (7, 3, "HHHHHHHF"),
        (8, 3, "HHHHfFeF"),
        (9, 3, "H"),
        (10, 8, "hhh"),
        (11, 8, "hhh"),
        (12, 9, "h"),
    ], 'atras': [
        [(9, 2, "HH"), (10, 1, "Hh"), (11, 1, "Hh"), (12, 1, "Hh"), (13, 1, "H"), (14, 1, "H")],
        [(9, 2, "HH"), (10, 1, "Hh"), (11, 0, "Hh"), (12, 0, "Hh"), (13, 0, "H"), (14, 1, "H")],
    ]},
    # Oboro: rabo de cavalo alto e comprido, anel de ouro, mecha no rosto.
    'rabo_longo': {'frente': [
        (2, 4, "HH"),
        (3, 3, "HBH"),
        (4, 4, "HHHhH"),
        (5, 4, "HHHHhiH"),
        (6, 4, "HHHHHHiH"),
        (7, 4, "HHHHHHHHH"),
        (8, 4, "HHHfFFeH"),
        (9, 11, "H"),
    ], 'atras': [
        [(2, 2, "HH"), (3, 1, "Hh"), (4, 0, "Hh"), (5, -1, "Hh"), (6, -1, "Hh"), (7, -2, "Hh"),
         (8, -2, "Hh"), (9, -3, "Hh"), (10, -3, "H"), (11, -4, "H")],
        [(2, 2, "HH"), (3, 1, "Hh"), (4, 0, "Hh"), (5, 0, "Hh"), (6, -1, "Hh"), (7, -1, "Hh"),
         (8, -2, "Hh"), (9, -2, "Hh"), (10, -3, "H"), (11, -3, "H")],
    ]},
}

# Rosto de quem fica com chapéu (só o que aparece debaixo da aba).
FACES = {
    'barba': [(9, 8, "h"), (10, 8, "HHH"), (11, 9, "HH")],
    'olho_raio': [(8, 10, "e")],
}


def head_paintable(cv, x, y):
    if not cv.ok(x, y):
        return False
    return cv.seg.lab[y, x] in (NONE, HATL, HATFX, HAIR, FACE)


def paint_rows(cv, rows, pal, behind=False):
    s = cv.seg
    for ry, rx, txt in rows:
        for i, k in enumerate(txt):
            if k == '.':
                continue
            x, y = s.ox + rx + i, s.oy + ry
            if behind:
                if k != 'X':
                    cv.behind(x, y, pal[k])
                continue
            if not head_paintable(cv, x, y):
                continue
            if k == 'X':
                cv.clear(x, y)
            else:
                cv.put(x, y, pal[k])


def head_palette(ch):
    return {
        'H': ch['cabelo'][0], 'h': ch['cabelo'][1], 'i': ch['cabelo'][2],
        'F': ch['pele'][0], 'f': ch['pele'][1], 'k': ch['pele'][2],
        'e': ch.get('olho', (24, 16, 20)),
        'A': ch['destaque'][0], 'a': ch['destaque'][1],
        'B': ch.get('destaque2', ch['destaque'][0]),
        'K': ch.get('mascara', [(30, 30, 36), (50, 50, 60)])[0],
        'm': ch.get('mascara', [(30, 30, 36), (50, 50, 60)])[1],
    }


def draw_head(cv, ch, idx):
    head = HEADS[ch['cabeca']]
    pal = head_palette(ch)
    for ry, rx, n in head.get('fitas', []):
        ribbon(cv, cv.seg.ox + rx, cv.seg.oy + ry, n, ch['destaque'], idx + ry, droop=0.2, amp=1.1, thick=1)
    if 'atras' in head:
        variants = head['atras']
        paint_rows(cv, variants[(idx // 2) % len(variants)], pal, behind=True)
    paint_rows(cv, head['frente'], pal)


def remove_hat(cv):
    L = cv.seg.lab
    for y, x in zip(*np.nonzero((L == HATL) | (L == HATFX))):
        cv.clear(x, y)


RAIDEN_HAT = [
    (2, 8, "LLMD"),
    (3, 5, "LLLLLMMDDD"),
    (4, 2, "LLLLLLLLMMMMDDDD"),
    (5, -1, "LLLLLLLLLLLMMMMMDDDDD"),
    (6, -4, "iLLLLLLLLLLLLLLMMMMMMDDDDD"),
    (7, -3, "DDDDDDDDDDDDDDDDDDDDDDD"),
]


def raiden_hat(cv, ch):
    """Chapéu do Raiden: largo e baixo, cobre os olhos, que brilham por baixo."""
    s = cv.seg
    hp = ch['chapeu_cor']
    pal = {'L': hp[0], 'M': hp[1], 'D': hp[2], 'i': hp[0]}
    remove_hat(cv)
    for ry, rx, txt in RAIDEN_HAT:
        for i, k in enumerate(txt):
            x, y = s.ox + rx + i, s.oy + ry
            if cv.ok(x, y) and cv.seg.lab[y, x] in (NONE, HATL, HATFX, HAIR):
                cv.put(x, y, pal[k])
    # cabelo que o chapéu original escondia e o novo não cobre
    paint_rows(cv, [(8, 4, "HHH")], head_palette(ch))


# ----- acessórios atrás do corpo ---------------------------------------------
def ribbon(cv, x0, y0, length, cols, idx, droop=0.25, amp=1.2, thick=2, speed=1.3):
    """Tira ondulando para trás (esquerda), presa em (x0, y0)."""
    ph = idx * speed
    for i in range(length):
        x = x0 - i
        yy = y0 + i * droop + amp * math.sin(i * 0.55 - ph) * min(1.0, i / 4.0)
        y = int(math.floor(yy + 0.5))
        for t in range(thick if i < length - 2 else 1):
            cv.behind(x, y + t, cols[0] if t == 0 else cols[1])


SHELL = [
    "...aaaa...",
    "..abbbba..",
    ".abAAAAba.",
    "abAAaaAAba",
    "aAAaAAaAAa",
    "aAAaAAaAAa",
    "aAAAaaAAAa",
    "aAaAAAAaAa",
    ".aAAAAAAa.",
    "..aaaaaa..",
]


def accessories(cv, ch, anim, idx):
    s = cv.seg
    for acc in ch.get('acessorios', []):
        if acc == 'cachecol':
            ribbon(cv, s.ox + 4, s.oy + 10, 15, ch['destaque'], idx, droop=0.15, amp=1.4)
            ribbon(cv, s.ox + 3, s.oy + 11, 9, [ch['destaque'][1], ch['destaque'][1]], idx + 1, droop=0.3, amp=1.0, thick=1)
        elif acc == 'casco':
            pal = {'a': ch['destaque'][1], 'A': ch['destaque'][0], 'b': ch.get('destaque2', ch['destaque'][1])}
            for ry, row in enumerate(SHELL):
                for rx, k in enumerate(row):
                    if k != '.':
                        cv.behind(s.ox - 6 + rx, s.oy + 10 + ry, pal[k])
        elif acc == 'trapo':
            ribbon(cv, s.ox + 4, s.oy + 10, 10, ch['destaque'], idx, droop=0.35, amp=1.2)
            for k in range(3):
                x = s.ox + 4 - 5 - k * 2
                ribbon(cv, x, s.oy + 12 + k, 2, [ch['destaque'][1]] * 2, idx + k, droop=1.0, amp=0, thick=1)
        elif acc == 'capa':
            ribbon(cv, s.ox + 4, s.oy + 10, 12, ch['destaque'], idx, droop=0.6, amp=0.8, thick=3, speed=1.0)


# ----- armas ----------------------------------------------------------------
def weapon_ok(cv, x, y):
    """A arma nova só ocupa o vazio ou o lugar da lâmina antiga."""
    if not cv.ok(x, y):
        return False
    lb = cv.seg.lab[y, x]
    return (lb == NONE and cv.a[y, x, 3] == 0) or lb == BLADE


def perp(u):
    """Normal que aponta para cima (y menor)."""
    n = np.array([-u[1], u[0]])
    return n if n[1] < 0 or (n[1] == 0 and n[0] > 0) else -n


def stroke(cv, b, t0, t1, core, edge=None, off=(0.0, 0.0), every=2, only_empty=False, mark=None):
    """Reta ao longo da lâmina, de t0 a t1 (distância a partir do cabo)."""
    hx, hy = b.hilt + np.array(off)
    ux, uy = b.u
    pts = line_pts(hx + ux * t0, hy + uy * t0, hx + ux * t1, hy + uy * t1)
    n = perp(b.u)
    side = (int(round(n[0])), int(round(n[1])))
    if side == (0, 0):
        side = (0, -1)
    done = []
    for i, (x, y) in enumerate(pts):
        if only_empty:
            if not (cv.ok(x, y) and cv.seg.lab[y, x] == NONE and cv.a[y, x, 3] == 0):
                continue
        elif not weapon_ok(cv, x, y):
            continue
        cv.put(x, y, core)
        done.append((x, y))
        if mark is not None:
            mark.add((x, y))
        if edge is not None and i % every == 0:
            ex, ey = x + side[0], y + side[1]
            if cv.ok(ex, ey) and cv.seg.lab[ey, ex] == NONE and cv.a[ey, ex, 3] == 0:
                cv.put(ex, ey, edge)
                if mark is not None:
                    mark.add((ex, ey))
    return done


def blade_fx(cv, ch, anim, idx, pixels):
    el = ch.get('elemento')
    for (x, y) in sorted(pixels):
        r = hsh('b', anim, idx, x, y)
        if el == 'fogo':
            if r < 0.5 and cv.is_empty(x, y - 1):
                cv.put(x, y - 1, (255, 150, 40) if r < 0.3 else (255, 90, 30))
                if r < 0.15 and cv.is_empty(x, y - 2):
                    cv.put(x, y - 2, (255, 214, 100))
        elif el == 'raio':
            if r < 0.2:
                dx, dy = ((1, -1), (-1, 1), (1, 1), (-1, -1))[int(r * 100) % 4]
                if cv.is_empty(x + dx, y + dy):
                    cv.put(x + dx, y + dy, (120, 200, 255))
                    if r < 0.1 and cv.is_empty(x + 2 * dx, y):
                        cv.put(x + 2 * dx, y, (235, 248, 255))
        elif el == 'roxo':
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                if hsh('g', anim, idx, x, y, dx, dy) < 0.16 and cv.is_empty(x + dx, y + dy):
                    cv.put(x + dx, y + dy, (96, 40, 160))
        elif el == 'agua':
            if r < 0.12:
                dx, dy = ((0, -2), (1, -1), (-1, 1), (0, 2))[int(r * 100) % 4]
                if cv.is_empty(x + dx, y + dy):
                    cv.put(x + dx, y + dy, (150, 235, 255))


def weapons(cv, ch, anim, idx):
    spec = ch.get('arma', {'tipo': 'katana'})
    kind = spec['tipo']
    s = cv.seg
    core, edge = ch['lamina']
    new_px = set()
    for b in s.blades:
        full = b.far
        if full < 5:
            continue
        own = [(x, y) for (x, y) in b.pts]
        if kind in ('katana', 'dupla'):
            new_px.update(own)
            if spec.get('escala', 1.0) > 1.0:
                stroke(cv, b, full, full * spec['escala'], core, edge, mark=new_px)
        elif kind == 'odachi':
            new_px.update(own)
            stroke(cv, b, full - 1, full * spec.get('escala', 1.45), core, edge, every=2, mark=new_px)
        elif kind == 'pesada':
            new_px.update(own)
            stroke(cv, b, full - 1, full * spec.get('escala', 1.15), core, edge, mark=new_px)
            # lâmina larga: uma fileira a mais do lado do fio
            n = perp(b.u)
            for (x, y) in list(new_px):
                for k in (1, 2) if spec.get('largura', 2) > 2 else (1,):
                    ex, ey = int(round(x - n[0] * k)), int(round(y - n[1] * k))
                    if b.dist.get((x, y), 99) > 2 and cv.ok(ex, ey) and cv.seg.lab[ey, ex] == NONE and cv.a[ey, ex, 3] == 0:
                        cv.put(ex, ey, spec.get('cor_largura', edge))
        elif kind == 'florete':
            for (x, y) in own:
                if s.c[y, x] in 'Lg':
                    erase_px(cv, x, y)
                else:
                    new_px.add((x, y))
            stroke(cv, b, full - 1, full * spec.get('escala', 1.2), core, None, mark=new_px)
            # copo da empunhadura
            n = perp(b.u)
            hx, hy = b.hilt + b.u * 1.5
            for k in (-1, 0, 1):
                x, y = int(round(hx + n[0] * k)), int(round(hy + n[1] * k))
                if cv.ok(x, y) and cv.seg.lab[y, x] in (NONE, BLADE, HANDLE):
                    cv.put(x, y, ch['destaque'][0] if k else ch['destaque'][1])
        elif kind in ('adaga', 'curta'):
            keep = spec.get('comprimento', 7)
            for (x, y) in own:
                if b.dist[(x, y)] > keep:
                    erase_px(cv, x, y)
                else:
                    new_px.add((x, y))
            if b.near > keep:
                continue
            if b.far < keep:
                stroke(cv, b, b.far, keep, core, edge, mark=new_px)
            if spec.get('guarda'):
                n = perp(b.u)
                g = b.hilt + b.u * 1.0
                for k in (-1, 1):
                    x, y = int(round(g[0] + n[0] * k)), int(round(g[1] + n[1] * k))
                    if cv.ok(x, y) and cv.seg.lab[y, x] in (NONE, BLADE):
                        cv.put(x, y, spec['guarda'])
        elif kind in ('lanca', 'cajado'):
            shaft = spec['haste']
            tip = full * spec.get('escala', 1.15)
            for (x, y) in own:
                erase_px(cv, x, y)
            head = spec.get('ponta', 5) if kind == 'lanca' else 2
            stroke(cv, b, -spec.get('atras', 14), tip - head, shaft[0], shaft[1], every=2, mark=new_px)
            if kind == 'lanca':
                stroke(cv, b, tip - head, tip, core, edge, every=1, mark=new_px)
                # alargamento da ponta (losango)
                n = perp(b.u)
                mid = b.hilt + b.u * (tip - head + 1.5)
                for k in (-1, 1):
                    x, y = int(round(mid[0] + n[0] * k)), int(round(mid[1] + n[1] * k))
                    if cv.ok(x, y) and cv.seg.lab[y, x] == NONE and cv.a[y, x, 3] == 0:
                        cv.put(x, y, edge)
                        new_px.add((x, y))
            else:
                cap = spec.get('ponteira', core)
                stroke(cv, b, tip - 2, tip, cap, None, mark=new_px)
                stroke(cv, b, -spec.get('atras', 14) - 2, -spec.get('atras', 14), cap, None, mark=new_px)
        elif kind == 'garras':
            for (x, y) in own:
                erase_px(cv, x, y)
            if b.near > 3:
                continue
            ang = math.atan2(b.u[1], b.u[0])
            n = perp(b.u)
            size = spec.get('comprimento', 8)
            for j, da in enumerate((-0.28, 0.0, 0.28)):
                a = ang + da
                u2 = np.array([math.cos(a), math.sin(a)])
                p0 = b.hilt + b.u * 1.0 + n * (j - 1) * 0.6
                ln = size - (1 if j != 1 else 0)
                pts = line_pts(p0[0], p0[1], p0[0] + u2[0] * ln, p0[1] + u2[1] * ln)
                for i, (x, y) in enumerate(pts):
                    if cv.ok(x, y) and (cv.seg.lab[y, x] in (NONE, BLADE)):
                        cv.put(x, y, edge if i < 2 else core)
                        new_px.add((x, y))
        # segunda arma na outra mão: cópia paralela, atrás do corpo
        if spec.get('par') and kind in ('dupla', 'adaga', 'katana') and b.near <= 3:
            n = perp(b.u)
            off = -n * 3 - b.u * 2
            keep = spec.get('comprimento', full * spec.get('escala', 1.0))
            stroke(cv, b, 1, min(keep, full * spec.get('escala', 1.0)), spec.get('cor_par', edge), None,
                   off=off, only_empty=True, mark=new_px)
    if ch.get('elemento') and new_px:
        blade_fx(cv, ch, anim, idx, new_px)
    cv.weapon_px = new_px


def render(fr, seg, ch, anim, idx, keep=False):
    cv = Canvas(fr, seg)
    recolor(cv, ch)
    accessories(cv, ch, anim, idx)
    if seg.has_hat:
        if ch.get('chapeu'):
            if ch.get('chapeu') == 'raiden':
                raiden_hat(cv, ch)
            if ch.get('rosto'):
                paint_rows(cv, FACES[ch['rosto']], head_palette(ch))
        else:
            remove_hat(cv)
            draw_head(cv, ch, idx)
    weapons(cv, ch, anim, idx)
    paint_smear(cv, ch, anim, idx)
    for x0, y0, x1, y1 in seg.erase:
        cv.a[max(0, y0):y1 + 1, max(0, x0):x1 + 1] = 0
    return cv if keep else cv.a


# --------------------------------------------------------------------------
# Personagens
# --------------------------------------------------------------------------
ORIG = {
    'camisa': [(255, 255, 255), (199, 207, 221), (146, 161, 185), (101, 115, 146)],
    'hakama': [(93, 93, 93), (61, 61, 61), (39, 39, 39), (27, 27, 27), (19, 19, 19)],
    'pele': [(230, 156, 105), (191, 111, 74), (138, 72, 54)],
    'cabelo': [(20, 18, 26), (46, 44, 60), (92, 90, 120)],
    'saya': (28, 18, 28),
    'cabo': (12, 46, 68),
    'lamina': [(250, 252, 252), (199, 207, 221)],
    'rastro': [(255, 255, 255), (236, 240, 248), (199, 207, 221)],
    'destaque': [(214, 52, 52), (140, 28, 36)],
}


def char(**kw):
    d = {k: (list(v) if isinstance(v, list) else v) for k, v in ORIG.items()}
    d.update(kw)
    return d


R = rgb
CHARS = {
    # O protagonista: sem chapéu, coque com fita vermelha, katana.
    'musashi': char(
        titulo='Musashi', arma={'tipo': 'katana'},
        cabeca='coque',
    ),
    # 1. Touro. Odachi gigante. Marrom e amarelo.
    'raijin': char(
        titulo='Raijin', arma={'tipo': 'odachi', 'escala': 1.5},
        cabeca='touro',
        camisa=[R('a8703e'), R('82522a'), R('5e381c'), R('402412')],
        hakama=[R('4a3a2c'), R('382c22'), R('2a2019'), R('1f1712'), R('150f0c')],
        pele=[R('e8a878'), R('c07a4e'), R('84503a')],
        cabelo=[R('1c140e'), R('3a2a1c'), R('5e4630')],
        destaque=[R('ffd23c'), R('c08a14')], obi=R('ffd23c'),
        saya=R('3a2412'), cabo=R('7a4a14'),
        rastro=[R('fff4c8'), R('ffd84a'), R('c89a2a')], elemento='ouro',
    ),
    # 2. Água. Florete. Azul claro e ciano.
    'shizuku': char(
        titulo='Shizuku', arma={'tipo': 'florete', 'escala': 1.25},
        cabeca='rabo',
        camisa=[R('eef8ff'), R('bfe2f6'), R('86bde6'), R('5a8cc4')],
        hakama=[R('4a78b0'), R('36609a'), R('284a7c'), R('1d3862'), R('142848')],
        pele=[R('f2c29c'), R('d69a74'), R('a86a4e')],
        cabelo=[R('1a2a52'), R('2e4a82'), R('5a7cc0')],
        destaque=[R('58f0ff'), R('1aa6c8')], obi=R('58f0ff'),
        saya=R('dff4ff'), cabo=R('1aa6c8'),
        lamina=[R('f4fbff'), R('a8d8f0')],
        rastro=[R('f0fdff'), R('9ff0ff'), R('4ac0e8')], elemento='agua',
    ),
    # 3. Noite. Duas adagas que brilham roxo. Ninja preto e roxo.
    'kage': char(
        titulo='Kage', arma={'tipo': 'adaga', 'comprimento': 8, 'par': True, 'cor_par': R('9a48f0'),
                             'guarda': R('4a1c7a')},
        cabeca='capuz',
        camisa=[R('4a4660'), R('34324a'), R('252438'), R('1a1a28')],
        hakama=[R('343044'), R('26243a'), R('1c1a2c'), R('141322'), R('0d0c18')],
        pele=[R('e0a47c'), R('b87a56'), R('7e4c36')],
        cabelo=[R('141320'), R('26243a'), R('3c3a56')],
        mascara=[R('141320'), R('2a2840')],
        olho=R('e8c8ff'),
        destaque=[R('b45cff'), R('6e2cb4')], obi=R('8a3ce0'),
        saya=None, cabo=R('4a1c7a'),
        lamina=[R('f4e0ff'), R('b45cff')],
        rastro=[R('f6e6ff'), R('c88cff'), R('7a3cd8')], elemento='roxo',
    ),
    # 4. Terra. Espada pesada. Chapéu de palha, verde oliva e ocre, barba.
    'daichi': char(
        titulo='Daichi', arma={'tipo': 'pesada', 'escala': 1.15, 'largura': 2, 'cor_largura': R('8a8676')},
        chapeu='palha', chapeu_cor=[R('e0bc72'), R('b48c48'), R('7c5c2c')], rosto='barba',
        camisa=[R('c4c48a'), R('9a9a5e'), R('72743e'), R('50522a')],
        hakama=[R('6a5238'), R('503c28'), R('3a2c1e'), R('2a2016'), R('1c150e')],
        pele=[R('d49468'), R('ac6c46'), R('78462e')],
        cabelo=[R('2a1c10'), R('4a3420'), R('6c5034')],
        destaque=[R('e0a030'), R('a06c18')], obi=R('c08a2a'),
        saya=R('3a2c1e'), cabo=R('6a4a1c'),
        lamina=[R('d8d8cc'), R('9c9a8a')],
        rastro=[R('fbf0d0'), R('e0b868'), R('a47a3a')], elemento='terra',
    ),
    # 5. Vento. Katana leve. Verde claro e verde limão, cachecol.
    'hayate': char(
        titulo='Hayate', arma={'tipo': 'katana'},
        cabeca='vento', acessorios=['cachecol'],
        camisa=[R('eefce0'), R('c2eca8'), R('8ccc78'), R('5c9c54')],
        hakama=[R('4a6448'), R('384e38'), R('283a2a'), R('1c2a1e'), R('131e15')],
        pele=[R('eab088'), R('c6845e'), R('8c5640')],
        cabelo=[R('16261a'), R('2c4a30'), R('4c7a4c')],
        destaque=[R('c8ff3c'), R('7cc81c')],
        saya=R('2c4a30'), cabo=R('5c9c1c'),
        lamina=[R('f4fff0'), R('bce8b0')],
        rastro=[R('f6ffe8'), R('d4ff7a'), R('8ad04a')], elemento='vento',
    ),
    # 6. Tartaruga. Espada curta e o casco nas costas (escudo). Verde.
    'genbu': char(
        titulo='Genbu', arma={'tipo': 'curta', 'comprimento': 11},
        cabeca='careca', acessorios=['casco'],
        camisa=[R('b6d0a0'), R('88ac74'), R('5e8452'), R('3e5e38')],
        hakama=[R('3e5a40'), R('2e4632'), R('223424'), R('18261a'), R('101a12')],
        pele=[R('d8a078'), R('b07852'), R('7a4c36')],
        cabelo=[R('8a8a86'), R('b8b8b2'), R('dcdcd6')],
        destaque=[R('4c9a3c'), R('22502a')], destaque2=R('8ad06a'), obi=R('2e6a2a'),
        saya=R('22502a'), cabo=R('2e4632'),
        rastro=[R('eaffdc'), R('8ee070'), R('3e9a3a')],
    ),
    # 7. Chama. Espada de fogo. Vermelho e amarelo.
    'enjin': char(
        titulo='Enjin', arma={'tipo': 'katana'},
        cabeca='chamas',
        camisa=[R('f0584a'), R('c02a2e'), R('861a24'), R('58101c')],
        hakama=[R('4a1a16'), R('361210'), R('280d0c'), R('1c0909'), R('130606')],
        pele=[R('e6a078'), R('c07650'), R('864a34')],
        cabelo=[R('b0200c'), R('ff6a1c'), R('ffd048')],
        destaque=[R('ffb020'), R('d8501a')], obi=R('ffb020'),
        saya=R('1c0909'), cabo=R('a82a1e'),
        lamina=[R('fff0b0'), R('ff9030')],
        rastro=[R('fff4b8'), R('ffa030'), R('e04420')], elemento='fogo',
    ),
    # 8. Mar. Lança de água. Azul mar e turquesa.
    'suiren': char(
        titulo='Suiren', arma={'tipo': 'lanca', 'escala': 1.2, 'atras': 14, 'ponta': 5,
                               'haste': [R('5a9cc0'), R('24506e')]},
        cabeca='faixa',
        camisa=[R('9ed8f0'), R('4aa0d4'), R('2a70b0'), R('1c4c84')],
        hakama=[R('1e3c64'), R('172e50'), R('11223c'), R('0c182c'), R('08101e')],
        pele=[R('dca07a'), R('b47654'), R('7e4c38')],
        cabelo=[R('0e1c34'), R('1e3a64'), R('3a64a0')],
        destaque=[R('3cf0d8'), R('14a8a0')], obi=R('3cf0d8'),
        saya=None, cabo=R('14a8a0'),
        lamina=[R('d8fffa'), R('3cf0d8')],
        rastro=[R('e0fffc'), R('5cf0e0'), R('1c9cc8')], elemento='agua',
    ),
    # 9. Corvo. Garras. Preto e vermelho.
    'karasu': char(
        titulo='Karasu', arma={'tipo': 'garras', 'comprimento': 8},
        cabeca='corvo', acessorios=['trapo'],
        camisa=[R('5a5058'), R('3e363e'), R('2a242a'), R('1c181c')],
        hakama=[R('3a2a2e'), R('2c1e22'), R('201518'), R('170f11'), R('0f0a0b')],
        pele=[R('ecc0a0'), R('c89478'), R('8c6250')],
        cabelo=[R('100c10'), R('241c26'), R('3c3040')],
        olho=R('ff2a2a'),
        destaque=[R('e0202c'), R('8c1018')], obi=R('c0182a'),
        saya=None, cabo=R('3a2a2e'),
        lamina=[R('f4eef0'), R('7a1820')],
        rastro=[R('ffe0e0'), R('ff4a4a'), R('a01020')], elemento='pena',
    ),
    # 10. Tempestade. Duas espadas com raios. Preto com o chapéu do Raiden.
    'arashi': char(
        titulo='Arashi', arma={'tipo': 'dupla', 'par': True, 'cor_par': R('7cc0ff')},
        chapeu='raiden', chapeu_cor=[R('f2eee0'), R('cfc6a8'), R('948a6e')], rosto='olho_raio',
        camisa=[R('4e5264'), R('363a4a'), R('262a36'), R('1a1c26')],
        hakama=[R('2e3240'), R('222530'), R('181a24'), R('111219'), R('0a0b10')],
        pele=[R('dca880'), R('b47e5c'), R('7c5040')],
        cabelo=[R('101218'), R('20242e'), R('343a4a')],
        olho=R('b4f0ff'),
        destaque=[R('3ca8ff'), R('1a5ad0')], obi=R('3ca8ff'),
        saya=R('111219'), cabo=R('1a5ad0'),
        lamina=[R('eef8ff'), R('5cb4ff')],
        rastro=[R('f4faff'), R('7cc8ff'), R('2a6cf0')], elemento='raio',
    ),
    # 11. Montanha. Cajado de ferro. Cinza pedra e branco osso.
    'jinshi': char(
        titulo='Jinshi', arma={'tipo': 'cajado', 'escala': 1.05, 'atras': 16,
                               'haste': [R('70747e'), R('3a3c44')], 'ponteira': R('ece6d4')},
        cabeca='eremita',
        camisa=[R('d6d2c6'), R('aaa598'), R('7c786e'), R('56534c')],
        hakama=[R('4c4a46'), R('3a3936'), R('2a2927'), R('1e1d1c'), R('141413')],
        pele=[R('d0a080'), R('a87858'), R('704e3c')],
        cabelo=[R('5c5a58'), R('8a8884'), R('b8b6b0')],
        destaque=[R('ece6d4'), R('a8a292')], obi=R('ece6d4'),
        saya=None, cabo=R('4a4c54'),
        rastro=[R('fbf8ee'), R('e2dccb'), R('a49e8c')],
    ),
    # 12. Oboro. Katana de Hanzo. Roxo escuro e dourado.
    'oboro': char(
        titulo='Oboro', arma={'tipo': 'katana'},
        cabeca='rabo_longo',
        camisa=[R('8a6ab0'), R('5e4488'), R('3e2c62'), R('281c42')],
        hakama=[R('2a2030'), R('201826'), R('18121c'), R('110d14'), R('0b080d')],
        pele=[R('e6b894'), R('c08c6c'), R('845c48')],
        cabelo=[R('0e0a12'), R('221a2c'), R('3e3250')],
        destaque=[R('ffcc40'), R('b08018')], destaque2=R('ffcc40'), obi=R('ffcc40'),
        saya=R('18121c'), cabo=R('b08018'),
        lamina=[R('fffbe8'), R('e8c060')],
        rastro=[R('fff6dc'), R('b48cff'), R('6a3cc0')], elemento='ouro',
    ),
    # Hanzo: não luta; cabelo branco, sem barba, azul escuro e bainha vermelha.
    'hanzo': char(
        titulo='Hanzo', arma={'tipo': 'katana'},
        cabeca='mestre',
        camisa=[R('8ca0c8'), R('5a70a0'), R('3c4e7c'), R('283658')],
        hakama=[R('2a3450'), R('20283e'), R('181e30'), R('121624'), R('0c0f18')],
        pele=[R('e0a67e'), R('b87c5a'), R('82543e')],
        cabelo=[R('a8a8a8'), R('d4d4d0'), R('f4f4f0')],
        destaque=[R('c42a2a'), R('7a1414')], obi=R('c42a2a'),
        saya=R('a01c1c'), cabo=R('20283e'),
    ),
}
ORDER = list(CHARS)


# --------------------------------------------------------------------------
# Manifesto e alcance
# --------------------------------------------------------------------------
def read_manifest(path):
    """Lê o sprite.txt do Musashi: 'cell W H' e 'anim NOME [loop] [hold n] ...'."""
    cell, anims = None, {}
    if not os.path.exists(path):
        return cell, anims
    for line in open(path, encoding='utf-8'):
        t = line.split('#')[0].split()
        if not t:
            continue
        if t[0] == 'cell' and len(t) >= 3:
            cell = (int(t[1]), int(t[2]))
        elif t[0] == 'anim' and len(t) >= 2:
            d, k = {'_ordem': len(anims)}, 2
            while k < len(t):
                if k + 1 < len(t) and t[k + 1].lstrip('-').isdigit():
                    d[t[k]] = int(t[k + 1])
                    k += 2
                else:
                    d[t[k]] = True
                    k += 1
            anims[t[1]] = d
    return cell, anims


def read_fixes(path):
    """ajustes.txt, uma linha por correção:
        ANIM quadro chapeu ox oy          o chapéu está com o canto em (ox, oy)
        ANIM quadro semchapeu             não há chapéu na cabeça neste quadro
        ANIM quadro apaga x0 y0 x1 y1     apaga um retângulo (ex.: chapéu caindo)
    """
    fixes = {}
    if not os.path.exists(path):
        return fixes
    for line in open(path, encoding='utf-8'):
        t = line.split('#')[0].split()
        if len(t) < 3:
            continue
        f = fixes.setdefault((t[0], int(t[1])), {})
        if t[2] == 'chapeu' and len(t) >= 5:
            f['hat'] = (int(t[3]), int(t[4]))
        elif t[2] == 'semchapeu':
            f['nohat'] = True
        elif t[2] == 'apaga' and len(t) >= 7:
            f.setdefault('apaga', []).append(tuple(int(v) for v in t[3:7]))
    return fixes


def foot_anchor(fr):
    """Última linha com pixel e o meio dela: o ponto que pisa no chão."""
    al = fr[:, :, 3] > 0
    ys = np.nonzero(al.any(1))[0]
    if not len(ys):
        return CELL_W // 2, CELL_H - 1
    yb = ys.max()
    xs = np.nonzero(al[yb])[0]
    return int(round((xs.min() + xs.max()) / 2)), int(yb)


def reach(cv, anchor):
    """Ponto mais à frente da arma ou do rastro, relativo à âncora dos pés."""
    L = cv.seg.lab
    m = ((L == SMEAR) | (L == BLADE)) & (cv.a[:, :, 3] > 0)
    for (x, y) in getattr(cv, 'weapon_px', ()):
        if cv.ok(x, y) and cv.a[y, x, 3]:
            m[y, x] = True
    if not m.any():
        return None
    ys, xs = np.nonzero(m)
    xm = xs.max()
    y = int(round(ys[xs == xm].mean()))
    return int(xm - anchor[0]), int(y - anchor[1])


def contact_frame(name, info, segs):
    if 'contact' in info:
        return info['contact']
    if info or 'ATTACK' not in name.upper():
        return None
    # golpe fora do manifesto: o quadro com mais rastro
    areas = [int((sg.lab == SMEAR).sum()) for sg in segs]
    return int(np.argmax(areas)) if max(areas) > 40 else None


# --------------------------------------------------------------------------
# Folhas de conferência
# --------------------------------------------------------------------------
def _font():
    from PIL import ImageFont
    return ImageFont.load_default()


def on_bg(img, bg):
    base = Image.new('RGBA', (img.shape[1], img.shape[0]), bg + (255,))
    base.alpha_composite(Image.fromarray(img))
    return base


def trim_box(frames):
    al = np.zeros((CELL_H, CELL_W), bool)
    for f in frames:
        al |= f[:, :, 3] > 0
    ys, xs = np.nonzero(al)
    if not len(xs):
        return 0, 0, CELL_W, CELL_H
    return max(0, xs.min() - 2), max(0, ys.min() - 2), min(CELL_W, xs.max() + 3), min(CELL_H, ys.max() + 3)


def sheet_character(name, rendered, out, zoom=3, bg=(40, 38, 52)):
    from PIL import ImageDraw
    rows = []
    font = _font()
    for anim, frames in rendered:
        x0, y0, x1, y1 = trim_box(frames)
        w, h = (x1 - x0) * zoom, (y1 - y0) * zoom
        row = Image.new('RGBA', (110 + len(frames) * (w + 4), h + 16), bg + (255,))
        d = ImageDraw.Draw(row)
        d.text((4, h // 2), anim, fill=(230, 230, 240, 255), font=font)
        for i, f in enumerate(frames):
            t = on_bg(f[y0:y1, x0:x1], bg).resize((w, h), Image.NEAREST)
            row.paste(t, (110 + i * (w + 4), 0))
            d.text((110 + i * (w + 4) + 2, h + 2), str(i), fill=(150, 150, 170, 255), font=font)
        rows.append(row)
    W = max(r.width for r in rows)
    img = Image.new('RGBA', (W, sum(r.height + 6 for r in rows) + 24), bg + (255,))
    ImageDraw.Draw(img).text((6, 6), name, fill=(255, 255, 255, 255), font=font)
    y = 24
    for r in rows:
        img.paste(r, (0, y))
        y += r.height + 6
    img.save(out)


def sheet_lineup(chars, poses, out, zoom=4, bg=(40, 38, 52), gray=False):
    """Todos lado a lado: tamanho do jogo em cima, ampliado embaixo."""
    from PIL import ImageDraw
    font = _font()
    x0, y0, x1, y1 = trim_box([f for _, f in poses])
    w, h = x1 - x0, y1 - y0
    colw = max(w * zoom, 72) + 8
    img = Image.new('RGBA', (len(chars) * colw + 8, h + h * zoom + 60), bg + (255,))
    d = ImageDraw.Draw(img)
    for i, (title, f) in enumerate(poses):
        crop = f[y0:y1, x0:x1]
        if gray:
            g = (crop[:, :, 0] * 0.299 + crop[:, :, 1] * 0.587 + crop[:, :, 2] * 0.114).astype(np.uint8)
            crop = np.dstack([g, g, g, crop[:, :, 3]])
        small = on_bg(crop, bg)
        cx = 8 + i * colw
        img.paste(small, (cx + (colw - 8 - w) // 2, 6))
        big = small.resize((w * zoom, h * zoom), Image.NEAREST)
        img.paste(big, (cx + (colw - 8 - w * zoom) // 2, h + 14))
        d.text((cx + 2, h + h * zoom + 22), title, fill=(240, 240, 250, 255), font=font)
    img.save(out)


def sheet_strikes(rows, out, zoom=2, bg=(40, 38, 52)):
    """Um personagem por linha, o quadro de contato de cada golpe lado a lado."""
    from PIL import ImageDraw
    font = _font()
    if not rows or not rows[0][1]:
        return
    ncol = max(len(r[1]) for r in rows)
    w, h = CELL_W * zoom, CELL_H * zoom
    img = Image.new('RGBA', (70 + ncol * w, 16 + len(rows) * h), bg + (255,))
    d = ImageDraw.Draw(img)
    for j, (anim, _) in enumerate(rows[0][1]):
        d.text((70 + j * w + 4, 2), anim, fill=(200, 200, 215, 255), font=font)
    for i, (title, tiles) in enumerate(rows):
        d.text((4, 16 + i * h + h // 2), title, fill=(240, 240, 250, 255), font=font)
        for j, (anim, f) in enumerate(tiles):
            img.paste(on_bg(f, bg).resize((w, h), Image.NEAREST), (70 + j * w, 16 + i * h))
    img.save(out)


LABEL_COLORS = {HATL: (120, 140, 255), HATFX: (80, 90, 200), HAIR: (255, 60, 200), FACE: (255, 200, 150),
                SKIN: (220, 150, 110), SHIRT: (255, 110, 110), DARK: (80, 80, 80), SAYA: (170, 0, 170),
                HANDLE: (0, 120, 255), BLADE: (0, 255, 255), SMEAR: (255, 255, 0), OTHER: (0, 255, 0)}


def sheet_detection(strips, segs, out, zoom=2):
    """Cada parte que o script achou, em cor chapada. Serve para conferir as pranchas novas."""
    from PIL import ImageDraw
    font = _font()
    rows = []
    for anim, frames in strips:
        row = np.zeros((CELL_H, CELL_W * len(frames), 4), np.uint8)
        row[:] = (24, 24, 32, 255)
        for i, sg in enumerate(segs[anim]):
            for lb, col in LABEL_COLORS.items():
                row[:, i * CELL_W:(i + 1) * CELL_W][sg.lab == lb] = col + (255,)
            if sg.has_hat and 0 <= sg.oy < CELL_H and 0 <= sg.ox < CELL_W:
                row[sg.oy, i * CELL_W + sg.ox] = (255, 255, 255, 255)
        im = Image.fromarray(row).resize((row.shape[1] * zoom, CELL_H * zoom), Image.NEAREST)
        d = ImageDraw.Draw(im)
        d.text((4, 4), anim, fill=(255, 255, 255, 255), font=font)
        for i, sg in enumerate(segs[anim]):
            tag = f"{i}" + ("" if sg.has_hat else " sem chapéu")
            d.text((i * CELL_W * zoom + 4, CELL_H * zoom - 14), tag, fill=(200, 200, 210, 255), font=font)
        rows.append(im)
    W = max(r.width for r in rows)
    img = Image.new('RGBA', (W, sum(r.height + 4 for r in rows)), (16, 16, 22, 255))
    y = 0
    for r in rows:
        img.paste(r, (0, y))
        y += r.height + 4
    img.save(out)


def sheet_reach(name, rendered, reaches, anchor, out, zoom=3, bg=(40, 38, 52)):
    """Quadro de contato de cada golpe com a âncora (vermelho) e o alcance (ciano)."""
    from PIL import ImageDraw
    font = _font()
    tiles = []
    for anim, frames in rendered:
        r = reaches.get(anim)
        if not r or r['contato'] is None or r['alcance'] is None:
            continue
        f = frames[r['contato']]
        t = on_bg(f, bg).resize((CELL_W * zoom, CELL_H * zoom), Image.NEAREST)
        d = ImageDraw.Draw(t)
        ax = anchor[0] * zoom + zoom // 2
        d.line([(ax, 0), (ax, CELL_H * zoom)], fill=(255, 60, 60, 255))
        rx = (anchor[0] + r['alcance'][0]) * zoom + zoom // 2
        ry = (anchor[1] + r['alcance'][1]) * zoom + zoom // 2
        d.line([(rx, 0), (rx, CELL_H * zoom)], fill=(60, 230, 255, 255))
        d.ellipse([rx - 4, ry - 4, rx + 4, ry + 4], outline=(255, 255, 255, 255))
        d.text((4, 4), f"{anim} q{r['contato']}  alcance {r['alcance'][0]} px", fill=(255, 255, 255, 255), font=font)
        tiles.append(t)
    if not tiles:
        return
    img = Image.new('RGBA', (sum(t.width + 4 for t in tiles), tiles[0].height + 20), bg + (255,))
    ImageDraw.Draw(img).text((4, 4), name, fill=(255, 255, 255, 255), font=font)
    x = 0
    for t in tiles:
        img.paste(t, (x, 20))
        x += t.width + 4
    img.save(out)


# --------------------------------------------------------------------------
# Entrada e saída
# --------------------------------------------------------------------------
def load_strips(src, cell=(CELL_W, CELL_H)):
    out = []
    for name in sorted(os.listdir(src)):
        if not name.lower().endswith('.png'):
            continue
        im = np.array(Image.open(os.path.join(src, name)).convert('RGBA'))
        if im.shape[0] != cell[1] or im.shape[1] % cell[0]:
            print(f"  pulando {name}: {im.shape[1]} x {im.shape[0]} não é tira de {cell[0]} x {cell[1]}")
            continue
        n = im.shape[1] // cell[0]
        frames = [im[:, i * cell[0]:(i + 1) * cell[0]].copy() for i in range(n)]
        out.append((os.path.splitext(name)[0], frames))
    return out


def write_manifest(path, cell, anims, strips, reaches, anchor, guard):
    lines = [
        "# Gerado por tools/personagens.py a partir do sprite.txt do Musashi.",
        "# ancora: ponto dos pés (IDLE quadro 0). alcance dx dy: ponta da arma ou do",
        "# rastro no quadro de contato, a partir da âncora (dy negativo = acima dos pés).",
        f"cell {cell[0]} {cell[1]}",
        f"ancora {anchor[0]} {anchor[1]}",
    ]
    if guard:
        lines.append(f"guarda {guard[0]} {guard[1]}")
    names = sorted({a for a, _ in strips}, key=lambda a: (anims.get(a, {}).get('_ordem', 999), a))
    for a in names:
        info = anims.get(a, {})
        toks = [f"anim {a:<13}"]
        for k, v in info.items():
            if k.startswith('_'):
                continue
            toks.append(k if v is True else f"{k} {v}")
        r = reaches.get(a)
        if r and r['alcance'] is not None:
            if 'contact' not in info and r['contato'] is not None:
                toks.append(f"contact {r['contato']}")
            toks.append(f"alcance {r['alcance'][0]} {r['alcance'][1]}")
        lines.append("  ".join(toks).rstrip())
    open(path, 'w', encoding='utf-8').write("\n".join(lines) + "\n")


MARK = '.gerado'
MARK_TEXT = "Pasta escrita por tools/personagens.py. Rodar o script de novo sobrescreve os PNGs daqui.\n"


def out_dir(root, name, src):
    """Nunca sobrescreve pranchas que não foram geradas por este script."""
    d = os.path.join(root, name)
    if os.path.realpath(d) == os.path.realpath(src):
        d = os.path.join(root, name + '_gerado')
    elif os.path.isdir(d) and not os.path.exists(os.path.join(d, MARK)) and \
            any(f.lower().endswith('.png') for f in os.listdir(d)):
        print(f"  {d} já tem pranchas de outro pack; o gerado vai para {d}_gerado")
        d += '_gerado'
    os.makedirs(d, exist_ok=True)
    open(os.path.join(d, MARK), 'w').write(MARK_TEXT)
    return d


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--entrada', help='pasta com as pranchas originais (padrão: <saida>/_original, '
                    'copiada de <saida>/musashi na primeira vez)')
    ap.add_argument('--saida', default='assets/sprites', help='pasta onde sai uma subpasta por personagem')
    ap.add_argument('--so', nargs='*', help='só estes personagens')
    ap.add_argument('--folhas', action='store_true', help='também gera as folhas de conferência em <saida>/_folhas')
    ap.add_argument('--lista', action='store_true', help='mostra os personagens e sai')
    args = ap.parse_args()

    if args.lista:
        for n in ORDER:
            ch = CHARS[n]
            print(f"{n:<9} {ch['arma']['tipo']:<8} {'chapéu ' + ch['chapeu'] if ch.get('chapeu') else ch['cabeca']}")
        return
    if args.entrada is None:
        args.entrada = os.path.join(args.saida, '_original')
        legacy = os.path.join(args.saida, 'musashi')
        if not os.path.isdir(args.entrada) and os.path.isdir(legacy):
            shutil.copytree(legacy, args.entrada)
            open(os.path.join(legacy, MARK), 'w').write(MARK_TEXT)
            print(f"guardei as pranchas originais em {args.entrada}; musashi/ agora recebe o Musashi sem chapéu")
            print("  (daqui em diante, edite o manifesto em _original/sprite.txt)")
    cell, anims = read_manifest(os.path.join(args.entrada, 'sprite.txt'))
    if cell and cell != (CELL_W, CELL_H):
        sys.exit(f"o gerador foi feito para o quadro {CELL_W} x {CELL_H} do Samurai #3; o manifesto diz {cell}")
    strips = load_strips(args.entrada)
    if not strips:
        sys.exit(f"nenhuma prancha em {args.entrada}")
    fixes = read_fixes(os.path.join(args.entrada, 'ajustes.txt'))
    names = args.so or ORDER
    for n in names:
        if n not in CHARS:
            sys.exit(f"personagem desconhecido: {n} (use --lista)")

    print(f"lendo {len(strips)} pranchas de {args.entrada}")
    segs = {}
    for a, frames in strips:
        segs[a] = [segment(f, fixes.get((a, i))) for i, f in enumerate(frames)]
        unk = set().union(*(sg.unknown for sg in segs[a]))
        nohat = [i for i, sg in enumerate(segs[a]) if not sg.has_hat]
        note = []
        if unk:
            note.append(f"{len(unk)} cores fora da paleta ficam como estão")
        if nohat:
            note.append(f"chapéu não achado nos quadros {nohat}")
        print(f"  {a}: {len(frames)} quadros" + (" (" + "; ".join(note) + ")" if note else ""))

    by_name = dict(strips)
    ref = by_name.get('IDLE', strips[0][1])[0]
    anchor = foot_anchor(ref)
    fol = os.path.join(args.saida, '_folhas')
    if args.folhas:
        os.makedirs(fol, exist_ok=True)
        sheet_detection(strips, segs, os.path.join(fol, 'deteccao.png'))

    poses, strikes, all_reaches = [], [], {}
    pose_anim = 'IDLE' if 'IDLE' in by_name else strips[0][0]
    for n in names:
        ch = CHARS[n]
        d = out_dir(args.saida, n, args.entrada)
        rendered, reaches = [], {}
        for a, frames in strips:
            cvs = [render(f, sg, ch, a, i, keep=True) for i, (f, sg) in enumerate(zip(frames, segs[a]))]
            outs = [cv.a for cv in cvs]
            Image.fromarray(np.concatenate(outs, 1)).save(os.path.join(d, a + '.png'))
            rendered.append((a, outs))
            k = contact_frame(a, anims.get(a, {}), segs[a])
            r = {'contato': k, 'alcance': None}
            if k is not None and k < len(cvs):
                r['alcance'] = reach(cvs[k], anchor)
            reaches[a] = r
        all_reaches[n] = reaches
        guard = reaches.get('DEFEND', {}).get('alcance')
        write_manifest(os.path.join(d, 'sprite.txt'), (CELL_W, CELL_H), anims, strips, reaches, anchor, guard)
        pose_frames = rendered[[a for a, _ in strips].index(pose_anim)][1]
        poses.append((ch['titulo'], next((f for f in pose_frames if f[:, :, 3].any()), pose_frames[0])))
        strikes.append((ch['titulo'], [(a, fr[reaches[a]['contato']]) for a, fr in rendered
                                       if reaches[a]['contato'] is not None and reaches[a]['contato'] < len(fr)]))
        if args.folhas:
            sheet_character(ch['titulo'], rendered, os.path.join(fol, n + '.png'))
            sheet_reach(ch['titulo'], rendered, reaches, anchor, os.path.join(fol, 'alcance_' + n + '.png'))
        print(f"  {n}: {d}")
    if args.folhas:
        sheet_lineup(names, poses, os.path.join(fol, 'elenco.png'))
        sheet_lineup(names, poses, os.path.join(fol, 'elenco_pb.png'), gray=True)
        sheet_strikes(strikes, os.path.join(fol, 'golpes.png'))
        print(f"folhas em {fol}")
    ms = all_reaches.get('musashi', all_reaches[names[-1]])
    print("distância entre as âncoras dos dois no quadro de contato = alcance do golpe + guarda do Musashi:")
    for a in sorted(ms):
        if ms[a]['alcance'] is not None and a != 'DEFEND':
            g = ms.get('DEFEND', {}).get('alcance')
            extra = f" + {g[0]} = {ms[a]['alcance'][0] + g[0]} px" if g else " + guarda (falta a prancha DEFEND)"
            print(f"  {a:<14} {ms[a]['alcance'][0]} px{extra}")


if __name__ == '__main__':
    main()
