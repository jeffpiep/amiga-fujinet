#!/usr/bin/env python3
"""Send keysyms to the FS-UAE window on a headless X display via XTEST.

Lets a scripted (non-interactive) emulator run drive the Amiga keyboard:
ready-up prompts, menu navigation, and FS-UAE hotkeys such as F12+s
(internal screenshot). See .claude/commands/emu-build-and-boot.md
("Screenshot color accuracy" and "Driving the Amiga keyboard headless").

Usage: emukey.py <display> <token> [<token> ...]
  <token>   keysym name ('space', 'Escape', 'w', ...); combos join with
            '+' and hold left-to-right ('F12+s'); 'sleepN' pauses N tenths
            of a second.

Requires python-xlib (pip install python-xlib).
"""
import sys
import time

from Xlib import display as xdisplay
from Xlib import X, XK
from Xlib.ext import xtest


def main():
    if len(sys.argv) < 3:
        print(__doc__, file=sys.stderr)
        return 1
    disp = xdisplay.Display(sys.argv[1])
    root = disp.screen().root

    # Focus the first mapped child (FS-UAE) so SDL sees the events.
    for win in root.query_tree().children:
        attrs = win.get_attributes()
        if attrs.map_state == X.IsViewable:
            disp.set_input_focus(win, X.RevertToParent, X.CurrentTime)
            break
    disp.sync()

    for token in sys.argv[2:]:
        if token.startswith('sleep'):
            time.sleep(int(token[5:]) / 10.0)
            continue
        codes = []
        for name in token.split('+'):
            keysym = XK.string_to_keysym(name)
            keycode = disp.keysym_to_keycode(keysym)
            if keycode == 0:
                print(f'no keycode for {name}', file=sys.stderr)
            else:
                codes.append(keycode)
        for keycode in codes:
            xtest.fake_input(disp, X.KeyPress, keycode)
            disp.sync()
            time.sleep(0.08)
        for keycode in reversed(codes):
            xtest.fake_input(disp, X.KeyRelease, keycode)
            disp.sync()
            time.sleep(0.05)
        time.sleep(0.25)
    disp.close()
    return 0


if __name__ == '__main__':
    sys.exit(main())
