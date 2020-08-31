## Weiss
Weiss is a chess engine written using [VICE](https://www.youtube.com/watch?v=bGAfaepBco4&list=PLZ1QII7yudbc-Ky058TEaOstZHVbT-2hg) as a base, and otherwise inspired by [Ethereal](https://github.com/AndyGrant/Ethereal) and [Stockfish](https://stockfishchess.org/).

Weiss is part of the [OpenBench](http://chess.grantnet.us/index/) testing framework. You can help us out by [letting your idle CPUs run test games](https://github.com/AndyGrant/OpenBench/)!

Weiss appears in the [CCRL](https://ccrl.chessdom.com/ccrl/404/cgi/compare_engines.cgi?family=Weiss), [CEGT](http://www.cegt.net/40_4_Ratinglist/40_4_single/rangliste.html), [FastGM](http://www.fastgm.de/60-0.60.html), and [SRL](http://rebel13.nl/download/speedy-rating-list.html) rating lists, and can be seen competing in [TCEC](https://tcec-chess.com).

A big thank you to everyone in [TCEC](https://www.twitch.tv/tcec_chess_tv) chat for helping me out, as well as motivating me to give this a try to begin with!

### UCI settings

Weiss supports the following:

* #### Hash
  The size of the hash table in MB.

* #### Threads
  The number of threads to use for searching.

* #### SyzygyPath
  Path to syzygy tablebase files.

* #### NoobBook
  Allow Weiss to query and play moves suggested by [noobpwnftw's online opening database](https://www.chessdb.cn/queryc_en/).
