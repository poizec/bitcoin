Notable changes
===============

P2P and network changes
-----------------------

- Previously, if Bitcoin Core were listening for P2P connections, then
  it would always bind on `127.0.0.1:8334` to listen for incoming Tor
  connections. It was not possible to switch this off even if the node didn't
  use Tor. Previously, a configuration `bind=addr:port` would result in
  binding on `addr:port` and on `127.0.0.1:8334`. This has been changed and now
  `bind=addr:port` results in binding on `addr:port` only. The default behavior
  of binding to `0.0.0.0:8333` and `127.0.0.1:8334` has not been changed.

  If you are using `bind=...` without `bind=...=onion` and rely on the previous
  behavior to accept incoming Tor connections at `127.0.0.1:8334`, you need to
  make this explicit by using `bind=... bind=127.0.0.1:8334=onion`. (#22729)
