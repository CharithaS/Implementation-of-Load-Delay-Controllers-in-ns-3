# Implementation-of-Load-Delay-Controllers-in-ns-3
## Overview
Load/Delay Controllers are a class of Active Queue Management Algorithms which take into account both the load factor, which is the ratio of packet arrival rate to the packet drain rate in the queue, and the queuing delay metrics [1]. The objective of LDC is to maintain average queuing delay under a specified target which reduces the response time without significantly affecting application throughput and link utilization. LDC has been implemented in ns-2 [2] and this repository contains the implementation of LDC in ns-3 [3].
## References
[1] Minseok Kwon, Sonia Fahmy.(2002). A Comparison of Load-based and Queue-based Active Queue Management Algorithms.Proceedings of SPIE ITCom. July 2002.

[2] http://www.isi.edu/nsnam/ns/

[3] http://www.nsnam.org/
