# Readme specific for Tucar's work

## Branching model

```mermaid
gitGraph
   branch openvehicles
   commit
   commit
   branch master
   checkout openvehicles
   commit
   commit
   commit
   checkout master
   merge openvehicles
   branch tardis
   branch master-internal
   checkout openvehicles
   commit
   commit
   branch tucar/feat/feat_b
   commit
   commit
   checkout openvehicles
   commit
   commit
   commit
   checkout master-internal
   commit
   commit
   checkout openvehicles
   commit
   commit
   checkout master
   merge openvehicles
   branch tucar/feat/feat_a
   commit
   commit
   checkout tardis
   merge master
   checkout openvehicles
   merge tucar/feat/feat_a
   checkout tardis
   merge tucar/feat/feat_a
   checkout openvehicles
   commit
   checkout tardis
   commit
   checkout master-internal
   merge tardis
   commit
   checkout master
   merge openvehicles
   checkout master-internal
   commit
   merge master
   commit
   checkout tucar/feat/feat_b
   commit
   commit
   checkout tardis
   merge master
   checkout openvehicles
   commit
   commit
   checkout openvehicles
   merge tucar/feat/feat_b
   commit
   checkout tardis
   merge tucar/feat/feat_b
   commit
   checkout master
   merge openvehicles
   checkout master-internal
   merge tardis
   checkout openvehicles
   commit
   commit
   checkout tardis
   merge master
   checkout openvehicles
   commit
   commit
   checkout master
   merge openvehicles
   checkout openvehicles
   commit
   checkout master-internal
   merge tardis
```
