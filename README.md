## Weiss
Weiss is a chess engine written using VICE by Bluefever as a base, and otherwise inspired by Ethereal, Stockfish and Demolito.

Weiss is part of the OpenBench testing framework. You can help us out by letting your idle CPUs run test games:\
http://chess.grantnet.us/index/

Big thank you to everyone in TCEC chat for helping me out with both chess specifics as well as general programming!

### UCI settings

Weiss supports the following:

* #### Hash
  The size of the hash table in MB.

* #### SyzygyPath
  Path to syzygy tablebase files.

Note: Weiss will print a string indicating it supports Ponder, this is done to make cutechess GUI show pondermove accuracy. Weiss does not actually support this option.
