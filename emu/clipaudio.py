#!/usr/bin/env python3
"""Turn a drive.sh AUDIO_WAV capture into something worth listening to.

checkaudio.py answers "did it make a noise, when, and how loud". This
answers "does it sound right", which no test can: it trims a 60-second
capture down to just the effects, mixes to mono, drops the sample rate, and
normalises the gain so a phone speaker can resolve it. A raw capture peaks
around 0.27 with a minute of boot silence in front — unlistenable on
anything but headphones in a quiet room.

    emu/clipaudio.py capture.wav -o clip.wav
    emu/clipaudio.py new.wav --ab old.wav --burst 5 --repeat 2 -o ab.wav

The --ab form is the one that settles arguments: it lays two captures end to
end under a single shared gain, so the comparison is honest about relative
loudness instead of normalising each side to the same peak. --burst picks the
same numbered effect out of each, which is how you A/B one sound from a sweep
without listening to the other eleven.

Burst detection is checkaudio's, so both tools agree on what counts as a
sound. Stdlib only, like its sibling — no numpy, no ffmpeg, neither of which
is on the box that runs the emulator.
"""
import argparse
import array
import os
import sys
import wave

import checkaudio

TARGET_RATE = 24000   # effects are baked at 8 kHz; this is plenty
PRE = 0.03            # lead-in kept before a burst starts
TAIL = 0.30           # tail kept after it ends, so decays are not clipped
PEAK = 0.88           # normalisation target


def load_mono(path):
    """Read a wav and mix to mono by taking the louder channel.

    Paula's stereo separation puts a single-channel effect almost entirely
    on one side, so averaging the channels would halve it.
    """
    with wave.open(path, "rb") as w:
        if w.getsampwidth() != 2:
            sys.exit("%s: expected 16-bit samples, got %d-bit"
                     % (path, w.getsampwidth() * 8))
        rate, channels = w.getframerate(), w.getnchannels()
        raw = w.readframes(w.getnframes())

    s = array.array("h")
    s.frombytes(raw)
    if sys.byteorder == "big":
        s.byteswap()
    if channels == 1:
        return s, rate
    mono = array.array("h", [max(s[i:i + channels], key=abs)
                             for i in range(0, len(s) - channels + 1, channels)])
    return mono, rate


def decimate(buf, rate, target):
    """Average down to about target Hz. Integer factors only — resampling
    at a fractional ratio would need a filter to avoid aliasing, and there
    is nothing up there to alias."""
    factor = max(1, int(round(rate / float(target))))
    if factor == 1:
        return buf, rate
    out = array.array("h", [sum(buf[i:i + factor]) // factor
                            for i in range(0, len(buf) - factor + 1, factor)])
    return out, rate // factor


def segments(path, min_peak, which):
    """The selected bursts of one capture, as decimated mono samples."""
    found, _ = checkaudio.bursts(path, min_peak)
    if not found:
        sys.exit("%s: no bursts above peak %.3f — nothing to clip"
                 % (path, min_peak))
    if which is not None:
        if which < 1 or which > len(found):
            sys.exit("%s: --burst %d out of range (found %d)"
                     % (path, which, len(found)))
        found = [found[which - 1]]

    buf, rate = load_mono(path)
    buf, rate = decimate(buf, rate, TARGET_RATE)
    out = []
    for start, end, _peak in found:
        a = max(0, int((start - PRE) * rate))
        b = min(len(buf), int((end + TAIL) * rate))
        out.append(buf[a:b])
    return out, rate


def main():
    ap = argparse.ArgumentParser(
        description="Make a listenable clip from a drive.sh AUDIO_WAV capture.")
    ap.add_argument("capture")
    ap.add_argument("--ab", metavar="WAV", action="append", default=[],
                    help="another capture to append for comparison "
                         "(repeatable); all share one gain")
    ap.add_argument("-o", "--out", default="clip.wav")
    ap.add_argument("--burst", type=int,
                    help="use only the Nth burst (1-indexed) of each capture")
    ap.add_argument("--repeat", type=int, default=1,
                    help="play each selection N times (default 1)")
    ap.add_argument("--gap", type=float, default=0.5,
                    help="seconds between repeats (default 0.5)")
    ap.add_argument("--split", type=float, default=1.1,
                    help="seconds between captures in --ab mode (default 1.1)")
    ap.add_argument("--min-peak", type=float, default=0.02,
                    help="burst detection threshold (default 0.02)")
    ap.add_argument("--no-normalize", action="store_true")
    args = ap.parse_args()

    rate = None
    parts = []   # one list of segments per capture
    for path in [args.capture] + args.ab:
        segs, r = segments(path, args.min_peak, args.burst)
        if rate is None:
            rate = r
        elif r != rate:
            sys.exit("%s: sample rate %d does not match %d — captures must "
                     "come from the same emulator settings" % (path, r, rate))
        parts.append((path, segs))

    silence = lambda sec: array.array("h", [0]) * int(sec * rate)
    out = array.array("h")
    for n, (_path, segs) in enumerate(parts):
        if n:
            out.extend(silence(args.split))
        for seg in segs:
            for k in range(args.repeat):
                out.extend(seg)
                out.extend(silence(args.gap))

    if not out:
        sys.exit("nothing to write")

    gain = 1.0
    if not args.no_normalize:
        peak = max((abs(v) for v in out), default=0) or 1
        gain = int(PEAK * 32767) / peak
        out = array.array("h", [max(-32768, min(32767, int(v * gain)))
                                for v in out])

    with wave.open(args.out, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(out.tobytes())

    print("%s: %.1fs, %d Hz mono, %.2f MB (gain x%.1f)"
          % (args.out, len(out) / float(rate), rate,
             os.path.getsize(args.out) / 1e6, gain))
    for path, segs in parts:
        print("  %-40s %d segment(s)" % (os.path.basename(path), len(segs)))


if __name__ == "__main__":
    main()
