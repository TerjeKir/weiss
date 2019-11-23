## weiss
weiss is a UCI chess engine built with VICE by Bluefever as a base.

If you are interested in getting into chess programming, check out his video tutorials on youtube\
https://www.youtube.com/playlist?list=PLZ1QII7yudbc-Ky058TEaOstZHVbT-2hg


Also inspired by Ethereal and Stockfish. Help them out by letting your idle CPUs run test games:\
http://chess.grantnet.us/index/

http://tests.stockfishchess.org/tests

Big thank you to everyone in TCEC chat for helping me out with both chess specifics as well as general programming!

### UCI settings

weiss supports the following:

* #### Hash
  The size of the hash table in MB.

* #### SyzygyPath
  Path to syzygy tablebase files.

Note: weiss will print a string indicating it supports Ponder, this is done to make cutechess GUI show pondermove accuracy. weiss does not actually support this option.