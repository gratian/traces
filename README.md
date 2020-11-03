# Traces and useful debug info for debugging Linux kernel problems

### futex.c: BUG_ON(!newowner) in fixup_pi_state_owner()

#### 4.9.236-rt154-00014

 * branch: https://github.com/gratian/linux/commits/stable-rt/v4.9-rt-ni-serial
 * boot messages: boot_messages-4.9.236-rt154-00014-g918126d78bde.txt
 * ftrace dump and kernel OOPS: oops_ftrace-4.9.236-rt154-00014-g918126d78bde-dump1.txt
 * ftrace dump and kernel OOPS: oops_ftrace-4.9.236-rt154-00014-g918126d78bde-dump2.txt
 * output capture from kgdb session: kgdb_session-4.9.236-rt154-00014-g918126d78bde.txt
 * .c reproducer for 4.9-rt kernels: fbomb.c

#### 5.10.0-rc1-rt1

 * boot messages, OOPS, kdb and ftrace event dump: oops_ftrace-5.10.0-rc1-rt1-00015-g10c3af983f2e-dump1.txt
 * OOPS, kdb and ftrace event dump: oops_ftrace-5.10.0-rc1-rt1-00015-g10c3af983f2e-dump2.txt
 * output capture from kgdb session: kgdb_session-5.10.0-rc1-rt1-00015-g10c3af983f2e.txt
 * .c reproducer for 5.10-rt kernels: fbomb_v2.c
