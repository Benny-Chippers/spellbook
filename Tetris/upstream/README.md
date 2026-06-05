# Tetris Upstream Snapshot

This directory preserves the newer raylib/host Tetris source that was pasted into
`build/tetris`. The FPGA target does not compile these files directly.

The bare-metal FPGA adapter in `../tetris_engine.c` ports the gameplay model from
`src/tetris.c`, including the falling piece grid, board collision masks, rotation
kicks, line clearing, scoring, speedup, and seven-piece bag generation.
