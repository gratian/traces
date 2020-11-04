# Traces and useful debug info for debugging Linux kernel problems

### futex.c: BUG_ON(!newowner) in fixup_pi_state_owner()

#### 4.9.236-rt154-00014

 * boot messages: boot_messages-4.9.236-rt154-00014-g918126d78bde.txt
 * ftrace dump and kernel OOPS: oops_ftrace-4.9.236-rt154-00014-g918126d78bde-dump1.txt
 * ftrace dump and kernel OOPS: oops_ftrace-4.9.236-rt154-00014-g918126d78bde-dump2.txt
 * output capture from kgdb session: kgdb_session-4.9.236-rt154-00014-g918126d78bde.txt
 * .c reproducer for 4.9-rt kernels: fbomb.c
 * kernel branch: https://github.com/gratian/linux/tree/stable-rt/v4.9-rt-ni-serial
 * kernel branch (with breakpoint): https://github.com/gratian/linux/tree/stable-rt/v4.9-rt-ni-serial-kgdb_bp

#### 5.10.0-rc1-rt1

 * boot messages, OOPS, kdb and ftrace event dump: oops_ftrace-5.10.0-rc1-rt1-00015-g10c3af983f2e-dump1.txt
 * OOPS, kdb and ftrace event dump: oops_ftrace-5.10.0-rc1-rt1-00015-g10c3af983f2e-dump2.txt
 * output capture from kgdb session: kgdb_session-5.10.0-rc1-rt1-00015-g10c3af983f2e.txt
 * .c reproducer for 5.10-rt kernels: fbomb_v2.c
 * kernel branch: https://github.com/gratian/linux/tree/devel-rt/v5.10-rc1-rt1-ni-serial

#### 5.10.0-rc2-rt4

 * oops_ftrace-5.10.0-rc2-rt4-00014-gc6d6e2d39cef-dump1.txt
 * kernel branch: https://github.com/gratian/linux/tree/devel-rt/v5.10-rc2-rt4-ni-serial
