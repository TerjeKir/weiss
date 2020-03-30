## Weiss
Weiss is a UCI chess engine built with VICE by Bluefever as a base.

If you are interested in getting into chess programming, check out his video tutorials on youtube\
https://www.youtube.com/playlist?list=PLZ1QII7yudbc-Ky058TEaOstZHVbT-2hg

Also inspired by Ethereal, Stockfish and Demolito. 

Weiss is part of a testing framework maintained by Andrew Grant of Ethereal. You can help us out by letting your idle CPUs run test games:\
http://chess.grantnet.us/index/

Big thank you to everyone in TCEC chat for helping me out with both chess specifics as well as general programming!

### UCI settings

Weiss supports the following:

* #### Hash
  The size of the hash table in MB.

* #### SyzygyPath
  Path to syzygy tablebase files.

Note: Weiss will print a string indicating it supports Ponder, this is done to make cutechess GUI show pondermove accuracy. Weiss does not actually support this option.
