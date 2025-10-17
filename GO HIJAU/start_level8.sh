#!/usr/bin/env bash
set -e

# Give the desktop a moment to settle
sleep 5

# Open a terminal that runs your script and stays open
/usr/bin/lxterminal \
  --title="QRTerm" \
  --command="bash -lc 'python3 /home/techcapitalmyro/level8.py; exec bash'" &

# After the window appears, bring it to the front/focus (don’t fail if not found)
sleep 1
/usr/bin/xdotool search --name QRTerm windowactivate --sync || true
