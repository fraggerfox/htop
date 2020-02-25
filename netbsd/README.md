NetBSD support in htop(1)
===

This implementation makes NetBSD use htop(1) without the need of mount_procfs(8).

The implementation has been copied over from the OpenBSD implemention in
htop(1).

Make NetBSD no longer masquerade as Linux.

Build notes
---

* Make sure python interpreter is correct in `scripts/MakeHeader.py`

What works
---

* Builds in NetBSD without any additional patch
* Uses sysctl(3) and kvm(3) to get basic information
* Shows basic meters CPU / Memory / Swap
* Shows basic process listing

What does *NOT* work
---

* Memory being split into used/buffers/cache

What needs to be tested
---

* Basic features of htop(1) like kill / nice / sort / search works as expected
* If the meter displays are working as expected

TODO
---

* Clean up the implementation, unused variables etc
* Remove unused / irrelevant files and useless / unused code
* Use kvm_getlwps(3) to get thread information
* Implement / fix proper Memory usage display
* Make package in pkgsrc-wip for better testing
* Fix the `kproc->p_stat` switch...case with correct values
