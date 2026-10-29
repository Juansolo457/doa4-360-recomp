#!/usr/bin/env bash
# Assemble the native Linux Steam Deck folder from tools/build-deck.sh's builds (Release and
# RelWithDebInfo; from doax2-360-recomp's): private/steamdeck/DOA4-360-SteamDeck/ with the two
# programs and their .so libraries, the launcher, doa4.toml, the README and VERSION.txt. The
# disc's files (game/) and the profile (user/) are not put in the folder: they stay on the Deck.
#   tools/steamdeck/make_linux_package.sh                          # into private/steamdeck/
#   tools/steamdeck/make_linux_package.sh --deck deck@steamdeck    # and copy it to the Deck
#   tools/steamdeck/make_linux_package.sh --deck deck@steamdeck --with-game
# --deck copies the folder to ~/recomps/DOA4-360-SteamDeck on that host over SSH; logs/, game/ and
# user/ there are left alone. --with-game also copies port/assets (your disc's files, about
# 6.8 GB) to its game/ when the Deck has no game/default.xex yet. The folder holds programs made
# from your disc and, on the Deck, its data: it never goes to GitHub or anywhere else.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
deck=""
with_game=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --deck) deck="$2"; shift ;;
    --with-game) with_game=1 ;;
    *) echo "unknown option $1" >&2; exit 2 ;;
  esac
  shift
done
if [[ $with_game -eq 1 && -z "$deck" ]]; then
  echo "--with-game needs --deck" >&2
  exit 2
fi
release="$root/port/out/build/deck-amd64-release"
debug="$root/port/out/build/deck-amd64-relwithdebinfo"
for d in "$release" "$debug"; do
  [[ -x "$d/doa4" ]] || { echo "$d/doa4 is missing: run tools/build-deck.sh (--config Release)" >&2; exit 1; }
done
out="$root/private/steamdeck/DOA4-360-SteamDeck"
rm -rf "$out"
mkdir -p "$out"
cp "$release/doa4" "$out/doa4"
cp "$debug/doa4" "$out/doa4-relwithdebinfo"
# Every shared library each program loads from beside itself ($ORIGIN).
for d in "$release" "$debug"; do
  for lib in "$d"/*.so; do cp "$lib" "$out/"; done
done
cp "$root/tools/steamdeck/doa4-linux.sh" "$root/tools/steamdeck/doa4.toml" \
   "$root/tools/steamdeck/README-SteamDeck-Linux.txt" "$out/"
chmod +x "$out/doa4" "$out/doa4-relwithdebinfo" "$out/doa4-linux.sh"
{
  echo "DOA4 360 recomp, native Linux build for the Steam Deck"
  echo "commit $(git -C "$root" rev-parse --short HEAD) ($(git -C "$root" branch --show-current)), built $(date -r "$release/doa4" '+%Y-%m-%d %H:%M')"
} > "$out/VERSION.txt"
du -sh "$out"
if [[ -n "$deck" ]]; then
  dest=recomps/DOA4-360-SteamDeck
  ssh "$deck" "mkdir -p ~/$dest"
  # The programs and libraries; logs/, game/ and user/ on the Deck are left alone.
  tar -C "$out" -cf - . | ssh "$deck" "tar -C ~/$dest -xf -"
  if [[ $with_game -eq 1 ]]; then
    if ssh "$deck" "test -f ~/$dest/game/default.xex"; then
      echo "The Deck has game/ already: not copied"
    else
      assets="$(readlink -f "$root/port/assets")"
      [[ -f "$assets/default.xex" ]] || { echo "port/assets/default.xex is missing" >&2; exit 1; }
      echo "Copying the disc's files to the Deck's game/ (about 6.8 GB)"
      ssh "$deck" "mkdir -p ~/$dest/game"
      tar -C "$assets" -cf - . | ssh "$deck" "tar -C ~/$dest/game -xf -"
    fi
  fi
  ssh "$deck" "cd ~/$dest && ls -la"
fi
