#!/usr/bin/env python3
"""Grava amostras automáticas das lutas, com os efeitos de impacto do jogo."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
NAMES = ('daichi', 'genbu', 'hayate', 'shizuku', 'enjin', 'arashi',
         'raizo', 'garfiel', 'suiren', 'karasu', 'yoru', 'jinshi', 'oboro')
SOUND = {1: 3, 2: 2, 3: 1}  # J_RUIM, J_BOM, J_PERFEITO -> SoundId


def run(cmd, **kwargs):
    return subprocess.run(cmd, check=True, **kwargs)


def record(master, seconds, output, phase, temp):
    frames = temp / 'frames'
    frames.mkdir()
    sfx = temp / 'sfx'
    sfx.mkdir()
    env = os.environ.copy()
    env.update(APARA_AUTO='1', APARA_LOG_IMPACTOS='1', APARA_SFX=str(sfx), APARA_REC_FPS='60', APARA_LOG_POSTURAS='1')
    cmd = [str(ROOT / 'apara'), '--master', str(master), '--duel']
    if phase: cmd += ['--fase', str(phase)]
    cmd += ['--rec', str(frames), '0', str(seconds)]
    game = run(cmd, cwd=ROOT, env=env, capture_output=True, text=True)
    hits = [(float(t), int(j)) for t, j in re.findall(
        r'^IMPACTO contato ([0-9.]+) julgamento ([0-9]+)', game.stderr, re.M)]
    if not list(frames.glob('q*.png')):
        raise RuntimeError(f'nenhum quadro para {NAMES[master - 1]}')
    ff = ['ffmpeg', '-y', '-loglevel', 'error', '-framerate', '60',
          '-i', str(frames / 'q%04d.png'), '-f', 'lavfi', '-t', str(seconds),
          '-i', 'anullsrc=r=44100:cl=mono']
    labels = ['[1:a]']
    filters = []
    # Amostra com os sons básicos sintetizados: inclui aviso/gesto/corte,
    # respeitando o pitch da postura, além dos impactos. Não grava a trilha.
    events = [(at, SOUND.get(judgement, 3), 1.0, 1.0) for at, judgement in hits]
    events += [(float(at), int(sound), float(volume), float(pitch))
               for at, sound, volume, pitch in re.findall(
        r'^POSTURA tempo=([0-9.]+) fonte=\w+ som=(\d+) volume=([0-9.]+) pitch=([0-9.]+)', game.stderr, re.M)]
    for index, (at, sound, volume, pitch) in enumerate(events, 2):
        wav = sfx / f'sfx_{sound:02d}.wav'
        if not wav.exists(): continue
        ff += ['-i', str(wav)]
        labels.append(f'[s{index}]')
        filters.append(f'[{index}:a]asetrate={round(44100 * pitch)},aresample=44100,volume={volume},adelay={round(at * 1000)}:all=1[s{index}]')
    filters.append(''.join(labels) + f'amix=inputs={len(labels)}:duration=first:normalize=0,volume=0.55[a]')
    ff += ['-filter_complex', ';'.join(filters), '-map', '0:v', '-map', '[a]',
           '-c:v', 'libx264', '-pix_fmt', 'yuv420p', '-crf', '20',
           '-c:a', 'aac', '-b:a', '160k', '-t', str(seconds), str(output)]
    run(ff, cwd=ROOT)
    return len(hits)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seconds', type=float, default=6)
    parser.add_argument('--out', type=Path, default=ROOT / 'videos-lutas')
    parser.add_argument('--phase', type=int, choices=(1, 2, 3), default=3, help='fase do Oboro')
    parser.add_argument('--masters', type=int, nargs='*', default=list(range(1, 14)))
    args = parser.parse_args()
    if args.seconds <= 0 or any(m < 1 or m > 13 for m in args.masters):
        parser.error('segundos positivos e mestres de 1 a 13')
    args.out.mkdir(parents=True, exist_ok=True)
    for master in args.masters:
        name = NAMES[master - 1]
        phase = args.phase if master == 13 else None
        with tempfile.TemporaryDirectory(prefix='apara-video-') as directory:
            output = args.out / f'{master:02d}_{name}.mp4'
            hits = record(master, args.seconds, output, phase, Path(directory))
            print(f'{output}: {hits} impactos', flush=True)


if __name__ == '__main__':
    main()
