#include <riscv_vector.h>

int main() {
  for (int rounds = 0; rounds < 100; rounds++) {

    int32_t a[] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                   11, 12, 13, 14, 15, 16, 17, 18, 19, 20};

    int32_t b[] = {100,  200,  300,  400,  500,  600,  700,  800,  900,  1000,
                   1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000};

    int32_t *a_ptr = a;
    int32_t *b_ptr = b;

    int vl = 20;
    int32_t result[vl];
    int32_t *result_ptr = result;

    for (size_t avl; vl > 0; vl -= avl, a_ptr += avl, b_ptr += avl) {
      avl = __riscv_vsetvl_e32m1(vl);
      vint32m1_t va = __riscv_vle32_v_i32m1(a_ptr, vl);
      vint32m1_t vb = __riscv_vle32_v_i32m1(b_ptr, vl);
      vint32m1_t vr;
      __riscv_vse32_v_i32m1(result_ptr, __riscv_vadd_vv_i32m1(va, vb, avl),
                            avl);
    }
  }
  return 0;
}