#include <string.h>

#include "unity.h"
#include "deque.h"  

void setUp(void) {}
void tearDown(void) {}

void test_deque_create(void) {
    deq_t deq;
    deq_init(&deq); 

    // Добавление элементов в деку
    int data1 = 10, data2 = 20;
    deq_push_head(&deq, &data1);
    deq_push_tail(&deq, &data2);

    // Проверка содержимого деки
    TEST_ASSERT_EQUAL_INT(2, deq.size);
    TEST_ASSERT_EQUAL_INT(10, *(int *)deq(&deq));
    TEST_ASSERT_EQUAL_INT(20, *(int *)deq_tail(&deq));

    // Извлечение элементов и проверка порядка
    int *head_data = deq_pop_head(&deq);
    TEST_ASSERT_EQUAL_INT(10, *head_data);
    int *tail_data = deq_pop_tail(&deq);
    TEST_ASSERT_EQUAL_INT(20, *tail_data);

    // Проверка на пустоту после удаления
    TEST_ASSERT_TRUE(deq_isempty(&deq));

    deq_free(&deq); // Освобождение ресурсов деки
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_deque_operations);
    return UNITY_END();
}
