Fathom
======

Fathom is a stand-alone Syzygy tablebase probing tool.  The aims of Fathom
are:

* To make it easy to integrate the Syzygy tablebases into existing chess
  engines;
* To make it easy to create stand-alone applications that use the Syzygy
  tablebases;

Fathom is compilable under either C99 or C++ and supports a variety of
platforms, including at least Windows, Linux, and MacOS.

Tool
----

Fathom includes a stand-alone command-line syzygy probing tool `fathom`.  To
probe a position, simply run the command:

    fathom --path=<path-to-TB-files> "FEN-string"

The tool will print out a PGN representation of the probe result, including:

* Result: "1-0" (white wins), "1/2-1/2" (draw), or "0-1" (black wins)
* The Win-Draw-Loss (WDL) value for the next move: "Win", "Draw", "Loss",
  "CursedWin" (win but 50-move draw) or "BlessedLoss" (loss but 50-move draw)
* The Distance-To-Zero (DTZ) value (in plys) for the next move
* WinningMoves: The list of all winning moves
* DrawingMoves: The list of all drawing moves
* LosingMoves: The list of all losing moves
* A pseudo "principal variation" of Syzygy vs. Syzygy for the input position.

For more information, run the following command:

    fathom --help

Programming API
---------------

Fathom provides a simple API. Following are the main function calls:

* `tb_init` initializes the tablebases.
* `tb_free` releases any resources allocated by Fathom.
* `tb_probe_wdl` probes the Win-Draw-Loss (WDL) table for a given position.
* `tb_probe_root` probes the Distance-To-Zero (DTZ) table for the given
   position. It returns a recommended move, and also a list of unsigned
   integers, each one encoding a possible move and its DTZ and WDL values.
* `tb_probe_root_dtz` probes the Distance-To-Zero (DTZ) at the root position.
   It returns a score and a rank for each possible move.
* `tb_probe_root_wdl` probes the Win-Draw-Loss (WDL) at the root position.
   it returns a score and a rank for each possible move.

Fathom does not require the callee to provide any additional functionality
(e.g. move generation). A simple set of chess-related functions including move
generation is provided in file tbchess.c. However, chess engines can opt to
replace some of this functionality for better performance (see below).

Chess Engines
-------------

Chess engines can use `tb_probe_wdl` to get the WDL value during search.
This function is thread safe (unless TB_NO_THREADS is set). The various
"probe_root" functions are intended for probing only at the root node
and are not thread-safe.

Chess engines can opt for a tighter integration of Fathom by configuring
`tbconfig.h`.  Specifically, the chess engines can define `TB_*_ATTACKS`
macros that replace the default attack functions with the engine's own definitions,
avoiding duplication of functionality.

History and Credits
-------------------

The Syzygy tablebases were created by Ronald de Man. This original version of Fathom
(https://github.com/basil00/Fathom) combined probing code from Ronald de Man, originally written for
Stockfish, with chess-related functions and other support code from Basil Falcinelli.
This repository was originaly a fork of that codebase, with additional modifications
by Jon Dart.

However, the current release of Fathom is not derived directly from the probing code
written for Stockfish, but from tbprobe.c, which is a component of the Cfish chess engine
(https://github.com/syzygy1/Cfish), a Stockfish derivative. tbprobe.c was written
by Ronald de Man and released for unrestricted distribution and use.

Fathom replaces the Cfish board representation and move generation code used in tbprobe.c
with simpler code from the original Fathom source by Basil. The code has been reorganized
so that tbchess.c contains all move generation and most chess-related typedefs and
functions, while tbprobe.c contains all the tablebase probing code. The code replacement and
reorganization was done by Jon Dart.

License
-------

(C) 2013-2015 Ronald de Man (original code)
(C) 2015 basil (new modifications)
(C) 2016-2019 Jon Dart (additional modifications)

This version of Fathom is released under the MIT License:

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

