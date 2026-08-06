#!/usr/bin/env python3
"""Report the non-silent bursts in a .wav captured by drive.sh's AUDIO_WAV.

Sound is the one thing a screenshot cannot verify, and on FS-UAE it is not
enough to know the code path ran: the emulator drops the AUDxVOL write
audio.device performs at CMD_WRITE, so an effect can play at volume 0 and be
perfectly silent while every log line looks right (see gksound.c's
pokeVolume(), and the strategic-plan Lessons Learned entry for 2026-07-08).
This turns "did it make a noise, when, and how loud" into a number.

    emu/checkaudio.py capture.wav [--min-peak 0.02] [--expect N]

Exits non-zero if --expect is given and a different number of bursts is
found, so it can gate a scripted run. Stdlib only — no numpy on the box that
runs the emulator.
"""
import array
import sys
import wave

GAP_SECONDS = 0.25   # silence this long ends a burst
WINDOW_MS = 10       # peak is measured per window, not per sample


def bursts(path, min_peak):
    with wave.open(path, "rb") as w:
        if w.getsampwidth() != 2:
            sys.exit("expected 16-bit samples, got %d-bit"
                     % (w.getsampwidth() * 8))
        rate, channels = w.getframerate(), w.getnchannels()
        frames = w.readframes(w.getnframes())

    samples = array.array("h")
    samples.frombytes(frames)
    if sys.byteorder == "big":
        samples.byteswap()

    # Mono-ise by taking the louder channel; Paula's stereo separation puts
    # a single-channel effect almost entirely on one side.
    if channels > 1:
        mono = [max(abs(samples[i + c]) for c in range(channels))
                for i in range(0, len(samples) - channels + 1, channels)]
    else:
        mono = [abs(s) for s in samples]

    win = max(1, rate * WINDOW_MS // 1000)
    gap_windows = max(1, int(GAP_SECONDS * rate / win))

    found, cur, quiet = [], None, 0
    for i in range(0, len(mono) - win + 1, win):
        peak = max(mono[i:i + win]) / 32768.0
        t = i / float(rate)
        if peak >= min_peak:
            quiet = 0
            if cur is None:
                cur = [t, t, peak]
            else:
                cur[1], cur[2] = t, max(cur[2], peak)
        elif cur is not None:
            quiet += 1
            if quiet >= gap_windows:
                found.append(cur)
                cur, quiet = None, 0
    if cur is not None:
        found.append(cur)
    return found, len(mono) / float(rate)


def main():
    args = sys.argv[1:]
    if not args:
        sys.exit(__doc__)
    path = args[0]
    min_peak = 0.02
    expect = None
    if "--min-peak" in args:
        min_peak = float(args[args.index("--min-peak") + 1])
    if "--expect" in args:
        expect = int(args[args.index("--expect") + 1])

    found, duration = bursts(path, min_peak)
    print("%s: %.1fs captured, %d burst(s) above peak %.3f"
          % (path, duration, len(found), min_peak))
    for n, (start, end, peak) in enumerate(found, 1):
        print("  %2d  %7.2fs - %7.2fs  (%5.0f ms)  peak %.3f"
              % (n, start, end, (end - start) * 1000, peak))
    if not found:
        print("  (silence)")

    if expect is not None and len(found) != expect:
        sys.exit("FAIL: expected %d burst(s), found %d" % (expect, len(found)))


if __name__ == "__main__":
    main()
