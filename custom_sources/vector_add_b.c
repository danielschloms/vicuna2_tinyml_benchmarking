// #include <riscv_vector.h>

int main() {

  for (int rounds = 0; rounds < 1000; rounds++) {

    int a[] = {rounds + 1,  rounds + 2,  rounds + 3,  rounds + 4,  rounds + 5,
               rounds + 6,  rounds + 7,  rounds + 8,  rounds + 9,  rounds + 10,
               rounds + 11, rounds + 12, rounds + 13, rounds + 14, rounds + 15,
               rounds + 16, rounds + 17, rounds + 18, rounds + 19, rounds + 20};

    int b[] = {100,  200,  300,  400,  500,  600,  700,  800,  900,  1000,
               1100, 1200, 1300, 1400, 1500, 1600, 1700, 1800, 1900, 2000};

    int result[20];

    for (int i = 0; i < 20; i++) {
      result[i] = a[i] + b[i];
    }

    volatile int r = result[10];
  }
  __asm__ volatile("jalr x0, 124(x0)");
  return 0;
}