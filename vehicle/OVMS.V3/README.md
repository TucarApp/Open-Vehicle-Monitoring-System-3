# Readme specific for Tucar's work

## Branching model

```mermaid
gitGraph
   branch openvehicles/master/Open-Vehicle-Monitoring-System-3
   commit
   commit
   branch master
   checkout openvehicles/master/Open-Vehicle-Monitoring-System-3
   commit
   commit
   commit
   checkout master
   merge openvehicles/master/Open-Vehicle-Monitoring-System-3
   branch tardis
   branch master-internal
   checkout openvehicles/master/Open-Vehicle-Monitoring-System-3
   commit
   commit
   branch tucar/feat/feat_b
   commit
   commit
   checkout openvehicles/master/Open-Vehicle-Monitoring-System-3
   commit
   commit
   commit
   checkout master-internal
   commit
   commit
   checkout openvehicles/master/Open-Vehicle-Monitoring-System-3
   commit
   commit
   checkout master
   merge openvehicles/master/Open-Vehicle-Monitoring-System-3
   branch tucar/feat/feat_a
   commit
   commit
   checkout tardis
   merge master
   checkout openvehicles/master/Open-Vehicle-Monitoring-System-3
   merge tucar/feat/feat_a
   checkout tardis
   merge tucar/feat/feat_a
   checkout openvehicles/master/Open-Vehicle-Monitoring-System-3
   commit
   checkout tardis
   commit
   checkout master-internal
   merge tardis
   commit
   checkout master
   merge openvehicles/master/Open-Vehicle-Monitoring-System-3
   checkout master-internal
   commit
   merge master
   commit
   checkout tucar/feat/feat_b
   commit
   commit
   checkout tardis
   merge master
   checkout openvehicles/master/Open-Vehicle-Monitoring-System-3
   commit
   commit
   checkout openvehicles/master/Open-Vehicle-Monitoring-System-3
   merge tucar/feat/feat_b
   commit
   checkout tardis
   merge tucar/feat/feat_b
   commit
   checkout master
   merge openvehicles/master/Open-Vehicle-Monitoring-System-3
   checkout master-internal
   merge tardis
   checkout openvehicles/master/Open-Vehicle-Monitoring-System-3
   commit
   commit
   checkout tardis
   merge master
   checkout openvehicles/master/Open-Vehicle-Monitoring-System-3
   commit
   commit
   checkout master
   merge openvehicles/master/Open-Vehicle-Monitoring-System-3
   checkout openvehicles/master/Open-Vehicle-Monitoring-System-3
   commit
   checkout master-internal
   merge tardis
```
