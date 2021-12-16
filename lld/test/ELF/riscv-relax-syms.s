# REQUIRES: riscv

// Check that relaxation correctly adjusts symbol addresses and sizes.

# RUN: llvm-mc -filetype=obj -triple=riscv32-unknown-elf -mattr=+relax %s -o %t.rv32.o
# RUN: llvm-mc -filetype=obj -triple=riscv64-unknown-elf -mattr=+relax %s -o %t.rv64.o
# RUN: ld.lld -Ttext=0x100000 %t.rv32.o -o %t.rv32
# RUN: ld.lld -Ttext=0x100000 %t.rv64.o -o %t.rv64

# RUN: llvm-readelf -s %t.rv32 | FileCheck %s
# RUN: llvm-readelf -s %t.rv64 | FileCheck %s

# CHECK: 100000     4 NOTYPE  LOCAL  DEFAULT     1 a
# CHECK: 100000     8 NOTYPE  LOCAL  DEFAULT     1 b
# CHECK: 100004     4 NOTYPE  LOCAL  DEFAULT     1 c
# CHECK: 100004     8 NOTYPE  LOCAL  DEFAULT     1 d
# CHECK: 100000    12 NOTYPE  GLOBAL DEFAULT     1 _start

.global _start
_start:
a:
b:
  add  a0, a1, a2
.size a, . - a
c:
d:
  call _start
.size b, . - b
.size c, . - c
  add a0, a1, a2
.size d, . - d
.size _start, . - _start
