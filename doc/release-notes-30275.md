RPC
---

- The default mode for the `estimatesmartfee` RPC has been updated from `conservative` to `economical`.
  This change is due to the `conservative` mode considering a longer history of blocks, potentially
  returning a higher fee rate, this made it less responsive to short-term drops in the prevailing fee market.
  Whereas, the `economical` mode provides potentially lower estimates and is more responsive
  to short-term drops in the prevailing fee market.

- Since users typically rely on the default mode, this change is expected to reduce overestimation for many users.
  The `economical` mode aligns with current users behavior, where users often opt-in to Replace-by-Fee (RBF),
  prefer not to overestimate fees, and use fee bumping when necessary. For users requiring high confidence
  in their fee estimates at the cost of potentially overestimating, the `conservative` mode remains available.
