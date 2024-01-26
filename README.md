<div align="center">

  [![Build][build-badge]][build-link]
  [![License][license-badge]][license-link]
  <br>
  [![Release][release-badge]][release-link]
  [![Commits][commits-badge]][commits-link]
  <br>
  [![OpenBench][openbench-badge]][openbench-link]
  [![Discord][discord-badge]][discord-link]
</div>

## Weiss
The name is an homage to [VICE](https://www.youtube.com/watch?v=bGAfaepBco4&list=PLZ1QII7yudbc-Ky058TEaOstZHVbT-2hg) which I used as a base for this project.

Weiss appears in most rating lists, and can be seen competing at [TCEC](https://tcec-chess.com) and [Chess.com](https://www.chess.com/computer-chess-championship).

### UCI settings

* #### Hash
  The size of the hash table in MB.

* #### Threads
  The number of threads to use for searching.

* #### SyzygyPath
  Path to syzygy tablebase files. Uses [Pyrrhic](https://github.com/AndyGrant/Pyrrhic) library.

* #### MultiPV
  Output the N best lines when searching. Leave at 1 for best performance.

* #### UCI_Chess960
  An option handled by your GUI. If true, Weiss will play Chess960.

* #### NoobBook
  Allow Weiss to query and play moves suggested by [noobpwnftw's online opening database](https://www.chessdb.cn/queryc_en/).

* #### NoobBookMode
  Sets the query mode to use (see [dbcn docs](https://www.chessdb.cn/cloudbookc_api_en.html)):
    - best - chooses randomly from the moves with scores close to the best. Stops if all moves' scores are below a threshold.
    - all - chooses the move with the highest score (first in the returned list). Only stops if there are no moves with scores.

* #### NoobBookLimit
  Limit the use of NoobBook to the first x moves of the game. Only relevant with NoobBook set to true.

* #### OnlineSyzygy
  Allow Weiss to query online 7 piece Syzygy tablebases [hosted by lichess](https://tablebase.lichess.ovh).


[build-link]:      https://github.com/TerjeKir/Weiss/actions/workflows/make.yml
[commits-link]:    https://github.com/TerjeKir/Weiss/commits/master
[discord-link]:    https://discord.gg/WJJcCPTyBJ
[license-link]:    https://github.com/TerjeKir/weiss/blob/master/COPYING.txt
[openbench-link]:  http://chess.grantnet.us/index/
[release-link]:    https://github.com/TerjeKir/Weiss/releases/latest

[build-badge]:     https://img.shields.io/github/actions/workflow/status/TerjeKir/Weiss/make.yml?branch=master&style=for-the-badge&label=Weiss&logo=github
[commits-badge]:   https://img.shields.io/github/commits-since/TerjeKir/Weiss/latest?style=for-the-badge
[discord-badge]:   https://img.shields.io/discord/759496923324874762?style=for-the-badge&label=discord&logo=Discord
[license-badge]:   https://img.shields.io/github/license/TerjeKir/Weiss?style=for-the-badge&label=license&color=success
[openbench-badge]: https://img.shields.io/website?style=for-the-badge&label=OpenBench&down_color=red&down_message=Offline&up_color=success&up_message=Online&url=http%3A%2F%2Fchess.grantnet.us%2Findex
[release-badge]:   https://img.shields.io/github/v/release/TerjeKir/Weiss?style=for-the-badge&label=official%20release
