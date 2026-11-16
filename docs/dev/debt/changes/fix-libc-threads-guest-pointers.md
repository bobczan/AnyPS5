# libc

## add Functional

- [C11 threads guest pointers](../../../core/libs/prx/libc/src/Threads.cpp) (libc) - the `_Mtx_*`, `_Cnd_*` and `_Thrd_sleep` exports read their handle and `xtime` through `GuestCopyIn_nid_no_patch` and publish handles through `GuestCopyOut_nid_no_patch`, as `_Timespec64_get_sys`, `_Timespec64_diff_to_useconds` and `_Timespec64_diff_to_ts` do with their `timespec`. An unreadable handle or xtime throws `std::invalid_argument` where the firmware faults on the pointer, and a handle whose publication fails is deleted before the throw. This is a host-safety fallback for invalid or partially mapped guest pointers, not measured on a console. `_Pad` still reaches its own mutex, condition and started flag through direct member accesses.
