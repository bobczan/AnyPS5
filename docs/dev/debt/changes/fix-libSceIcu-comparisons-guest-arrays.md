# libSceIcu

## add Functional

- [Code-unit comparison guest buffers](../../../core/libs/prx/libSceIcu/Comparisons.cpp) (libSceIcu) - `u_strlen`, `u_strcmp`, `u_strncmp`, `u_memcmp` and `u_memcmpCodePointOrder` read the code units they compare through `GuestCopyIn_nid_no_patch`, in 64-unit windows for the counted forms and one unit at a time where the terminator or a neighbouring surrogate decides the result, so an unreadable buffer rejects through `NotImplemented_nid_no_patch` where the firmware faults on the pointer. The comparison results, including the surrogate adjustment of `u_memcmpCodePointOrder`, are unchanged. This is a host-safety fallback for invalid or partially mapped guest pointers and was not measured on a console.
