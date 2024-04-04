#include "lib.h"

int main() {
    char c = read_char();
    print_s("The character is: ");
    print_c(c);
    print_c('\n');
    int a = read_num();
    print_s("The number is: ");
    print_d(a);
    print_c('\n');
    exit_proc();
}