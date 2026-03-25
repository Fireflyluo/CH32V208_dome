#ifndef MAIN_RUNTIME_TEST_H
#define MAIN_RUNTIME_TEST_H

#include <stdint.h>

#ifndef APP_ENABLE_MAIN_RUNTIME_TEST
#define APP_ENABLE_MAIN_RUNTIME_TEST 1
#endif

void main_runtime_test_init(void);
void main_runtime_test_process(void);

#endif /* MAIN_RUNTIME_TEST_H */
