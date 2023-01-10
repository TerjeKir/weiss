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
The name is an homage to [VICE](https://www.youtube.com/watch?v=bGAfaepBco4&list=PLZ1QII7yudbc-Ky058TEaOstZHVbT-2hg) which I used as a base for this project. Development is inspired by [Ethereal](https://github.com/AndyGrant/Ethereal) and [Stockfish](https://stockfishchess.org/).

Weiss appears in the [CCRL](https://ccrl.chessdom.com/ccrl/404/cgi/compare_engines.cgi?family=Weiss), [CEGT](http://www.cegt.net/40_4_Ratinglist/40_4_single/rangliste.html), and [FastGM](http://www.fastgm.de/60-0.60.html) rating lists, and can be seen competing in [TCEC](https://tcec-chess.com).

Weiss is part of the [OpenBench](http://chess.grantnet.us/index/) testing framework.

### UCI settings

* #### Hash
  The size of the hash table in MB.

* #### Threads
  The number of threads to use for searching.

* #### SyzygyPath
  Path to syzygy tablebase files.

* #### MultiPV
  Output the N best lines when searching. Leave at 1 for best performance.

* #### UCI_Chess960
  An option handled by your GUI. If true, Weiss will play Chess960.

* #### NoobBook
  Allow Weiss to query and play moves suggested by [noobpwnftw's online opening database](https://www.chessdb.cn/queryc_en/).

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
