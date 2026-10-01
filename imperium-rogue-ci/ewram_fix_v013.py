#!/usr/bin/env python3
from pathlib import Path
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: ewram_fix_v013.py /path/to/imperium")

p = Path(sys.argv[1]) / "src" / "rogue_mode.c"
s = p.read_text()

def repl(old, new, label):
    global s
    if new in s:
        return
    if old not in s:
        raise SystemExit(f"missing patch anchor: {label}")
    s = s.replace(old, new, 1)

repl('#include "battle.h"\n',
     '#include "battle.h"\n#include "decompress.h"\n',
     "decompress include")

repl('static EWRAM_DATA struct RogueSaveData sRogueSave = {0};\n'
     'static EWRAM_DATA u8 sRogueSectorBuffer[SECTOR_COUNTER_OFFSET] = {0};\n',
     'static IWRAM_DATA struct RogueSaveData sRogueSave = {0};\n',
     "static rogue buffers")

repl('    memset(sRogueSectorBuffer, 0, sizeof(sRogueSectorBuffer));\n'
     '    if (TryReadSpecialSaveSector(ROGUE_SAVE_SECTOR, sRogueSectorBuffer) != SAVE_STATUS_OK)\n',
     '    memset(gDecompressionBuffer, 0, SECTOR_COUNTER_OFFSET);\n'
     '    if (TryReadSpecialSaveSector(ROGUE_SAVE_SECTOR, gDecompressionBuffer) != SAVE_STATUS_OK)\n',
     "load staging buffer")

repl('    memcpy(&sRogueSave, sRogueSectorBuffer, sizeof(sRogueSave));\n',
     '    memcpy(&sRogueSave, gDecompressionBuffer, sizeof(sRogueSave));\n',
     "load copy")

repl('    memset(sRogueSectorBuffer, 0, sizeof(sRogueSectorBuffer));\n',
     '    memset(gDecompressionBuffer, 0, SECTOR_COUNTER_OFFSET);\n',
     "write staging buffer")

repl('    memcpy(sRogueSectorBuffer, &sRogueSave, sizeof(sRogueSave));\n'
     '    TryWriteSpecialSaveSector(ROGUE_SAVE_SECTOR, sRogueSectorBuffer);\n',
     '    memcpy(gDecompressionBuffer, &sRogueSave, sizeof(sRogueSave));\n'
     '    TryWriteSpecialSaveSector(ROGUE_SAVE_SECTOR, gDecompressionBuffer);\n',
     "write copy")

p.write_text(s)

assert "sRogueSectorBuffer" not in s
assert "static IWRAM_DATA struct RogueSaveData sRogueSave" in s
assert "gDecompressionBuffer" in s
print("Applied Imperium Rogue v0.1.3 EWRAM fix.")
