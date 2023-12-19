#include <stdio.h>


typedef enum FooFilter {
    FOO_FILTER_ONE   = 1 << 0,
    FOO_FILTER_TWO   = 1 << 1,
    FOO_FILTER_THREE = 1 << 2,
} FooFilter;


void do_something(FooFilter filter_flags) {
  printf("%d\n", filter_flags);
}
