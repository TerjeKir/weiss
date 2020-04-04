## Weiss
Weiss is a chess engine written using [VICE](https://www.youtube.com/watch?v=bGAfaepBco4&list=PLZ1QII7yudbc-Ky058TEaOstZHVbT-2hg) as a base, and otherwise inspired by [Ethereal](https://github.com/AndyGrant/Ethereal), [Stockfish](https://stockfishchess.org/) and [Demolito](https://github.com/lucasart/Demolito).

Weiss is part of the [OpenBench](http://chess.grantnet.us/index/) testing framework. You can help us out by [letting your idle CPUs run test games](https://github.com/AndyGrant/OpenBench/)!

A big thank you to everyone in [TCEC](https://www.twitch.tv/tcec_chess_tv) chat for helping me out, as well as motivating me to give this a try to begin with!

### UCI settings

Weiss supports the following:

* #### Hash
  The size of the hash table in MB.

* #### SyzygyPath
  Path to syzygy tablebase files.

Note: Weiss will print a string indicating it supports Ponder, this is done to make cutechess GUI show pondermove accuracy. Weiss does not actually support this option.
