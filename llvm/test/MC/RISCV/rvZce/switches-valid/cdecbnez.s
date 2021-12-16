# RUN: llvm-mc %s -triple=riscv32 -mattr=+zce-cdecbnez -mattr=m -riscv-no-aliases -show-encoding \
# RUN:     | FileCheck -check-prefixes=CHECK-ASM %s
# RUN: llvm-mc -filetype=obj -triple=riscv32 -mattr=+zce-cdecbnez -mattr=m < %s \
# RUN:     | llvm-objdump --mattr=experimental-zce  --mattr=m -M no-aliases -d -r - \
# RUN:     | FileCheck --check-prefixes=CHECK-OBJ %s


# CHECK-OBJ: c.decbnez s0, 0, 4
# CHECK-ASM: encoding: [0x22,0xa0]
c.decbnez s0, 1, -4