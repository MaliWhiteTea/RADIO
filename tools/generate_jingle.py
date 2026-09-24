#!/usr/bin/env python3
"""Bir ses dosyasından ESP32 DAC için mono unsigned 8-bit PCM üretir."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path, help="Kaynak WAV/MP3 dosyası")
    parser.add_argument("--rate", type=int, default=16_000, help="Örnekleme hızı")
    parser.add_argument(
        "--raw",
        type=Path,
        default=Path("tools/jingle_16k_u8.pcm"),
        help="Üretilecek ham PCM dosyası",
    )
    parser.add_argument(
        "--header",
        type=Path,
        default=Path("JingleData.h"),
        help="Üretilecek Arduino başlık dosyası",
    )
    return parser.parse_args()


def write_header(path: Path, pcm: bytes, sample_rate: int) -> None:
    rows = []
    for offset in range(0, len(pcm), 16):
        values = ", ".join(f"0x{value:02X}" for value in pcm[offset : offset + 16])
        rows.append(f"  {values},")

    duration = len(pcm) / sample_rate
    contents = "\n".join(
        [
            "#pragma once",
            "#include <Arduino.h>",
            "",
            f"// {sample_rate / 1000:g} kHz, unsigned 8-bit mono ({duration:.3f} sn)",
            f"static const uint16_t JINGLE_SAMPLE_RATE = {sample_rate};",
            f"static const size_t JINGLE_LEN = {len(pcm)};",
            "static const uint8_t JINGLE_PCM[JINGLE_LEN] PROGMEM = {",
            *rows,
            "};",
            "",
        ]
    )
    path.write_text(contents, encoding="utf-8")


def main() -> None:
    args = parse_args()
    if args.rate <= 0 or args.rate > 65_535:
        raise SystemExit("Örnekleme hızı 1..65535 arasında olmalıdır")

    args.raw.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [
            "ffmpeg",
            "-hide_banner",
            "-loglevel",
            "error",
            "-y",
            "-i",
            str(args.source),
            "-vn",
            "-ac",
            "1",
            "-ar",
            str(args.rate),
            "-c:a",
            "pcm_u8",
            "-f",
            "u8",
            str(args.raw),
        ],
        check=True,
    )

    pcm = args.raw.read_bytes()
    if not pcm:
        raise SystemExit("FFmpeg boş PCM üretti")
    write_header(args.header, pcm, args.rate)
    print(
        f"{args.raw}: {len(pcm)} byte, {args.rate} Hz, "
        f"{len(pcm) / args.rate:.3f} sn"
    )
    print(f"{args.header} güncellendi")


if __name__ == "__main__":
    main()
