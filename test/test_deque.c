#include <string.h>

#include "unity.h"
#include "deque.h"  

void setUp(void) {}
void tearDown(void) {}

void test_deque_create(void) {
    
    deq_t *deq = deque_create(); 

    // Добавление элементов в деку
    int data1 = 10, data2 = 20;
    deq_push_head(deq, &data1);
    deq_push_tail(deq, &data2);

    // Проверка содержимого деки
		{
    deq_entry_t *entry;
		TEST_ASSERT_EQUAL_INT(2, deq->size);
    entry = deq_get_head(deq);
    TEST_ASSERT_EQUAL_INT(10, *(int *)entry->data);
    entry = deq_get_tail(deq);
    TEST_ASSERT_EQUAL_INT(20, *(int *)entry->data);
		}

		//проверяем pop функции
		{
		deq_entry_t *entry;
		TEST_ASSERT_EQUAL_INT(2, deq->size);
    entry = deq_pop_head(deq);
    TEST_ASSERT_EQUAL_INT(10, *(int *)entry->data);
    entry = deq_pop_tail(deq);
    TEST_ASSERT_EQUAL_INT(20, *(int *)entry->data);
		}

    // Проверка на пустоту после удаления
    TEST_ASSERT_TRUE(deq_isempty(deq));

    deq_free(deq); // Освобождение ресурсов деки
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_deque_create);
    return UNITY_END();
}
