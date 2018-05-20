#include "helper.h"
#include <CUnit/CUnit.h>
#include <CUnit/Console.h>
#include <CUnit/Basic.h>
#include <CUnit/TestRun.h>
#include <CUnit/TestDB.h>
#include <stdint.h>

void cut_bits_test_aligned(void)
{
  uint32_t result;
  uint8_t csd[16] = {0xaa, 0, 0, 0, 0, 0, 0, 0, 0xaa, 0xaa, 0xaa, 0xaa, 0, 0, 0, 0xaa};
  CU_ASSERT_EQUAL(cut_bits(csd, 0, 7), 0xaa);
  CU_ASSERT_EQUAL(cut_bits(csd, 15 * 8, 15 * 8 + 7), 0xaa);
  CU_ASSERT_EQUAL(cut_bits(csd, 4 * 8, 4 * 8 + 7), 0xaa);
  CU_ASSERT_EQUAL(cut_bits(csd, 4 * 8, 5 * 8 + 7), 0xaaaa);
  CU_ASSERT_EQUAL(cut_bits(csd, 4 * 8, 6 * 8 + 7), 0xaaaaaa);
  CU_ASSERT_EQUAL(cut_bits(csd, 4 * 8, 7 * 8 + 7), 0xaaaaaaaa);
}

void cut_bits_test_small(void)
{
  uint32_t result;
  uint8_t csd[16] = {0xaa, 0, 0, 0, 0, 0, 0, 0, 0xaa, 0xaa, 0xaa, 0xaa, 0, 0, 0, 0xaa};
  CU_ASSERT_EQUAL(cut_bits(csd, 0, 6), 0xaa & 0x3f);
  CU_ASSERT_EQUAL(cut_bits(csd, 15 * 8 + 2, 15 * 8 + 7), (0xaa & 0xfc) >> 2);
  CU_ASSERT_EQUAL(cut_bits(csd, 4 * 8 + 2, 4 * 8 + 5), (0xaa & 0x3c) >> 2);
  CU_ASSERT_EQUAL(cut_bits(csd, 4 * 8 + 6, 4 * 8 + 9), 0xaa & 0xff);
  CU_ASSERT_EQUAL(cut_bits(csd, 4 * 8 + 6, 4 * 8 + 9), 0xaa & 0x00);
}

void cut_bits_test_shift(void)
{
  uint32_t result;
  uint8_t csd[16] = {0xaa, 0, 0, 0, 0, 0, 0, 0, 0xaa, 0xaa, 0xaa, 0xaa, 0, 0, 0, 0xaa};
  uint32_t exp = 0xaaaaaaaa;
  CU_ASSERT_EQUAL(cut_bits(csd, 4 * 8 - 2, 7 * 8 + 3), 0b101010101010101010101010101000);
  CU_ASSERT_EQUAL(cut_bits(csd, 4 * 8 - 1, 7 * 8 + 4), 0b010101010101010101010101010100);
  CU_ASSERT_EQUAL(cut_bits(csd, 4 * 8, 7 * 8 + 5), 0b101010101010101010101010101010);
  CU_ASSERT_EQUAL(cut_bits(csd, 4 * 8 + 2, 7 * 8 + 7), 0b010101010101010101010101010101);
  CU_ASSERT_EQUAL(cut_bits(csd, 4 * 8 + 3, 7 * 8 + 8), 0b101010101010101010101010101010);
  CU_ASSERT_EQUAL(cut_bits(csd, 4 * 8 + 4, 7 * 8 + 9), 0b001010101010101010101010101010);
}

int32_t RunAllTests(void)
{
  CU_pSuite testSuite = NULL;
  CU_initialize_registry();
  testSuite = CU_add_suite("Cut bits from 16 bytes", NULL, NULL);
  CU_add_test( testSuite, "Aligned start and end", cut_bits_test_aligned);
  CU_add_test( testSuite, "Less than 8 bits", cut_bits_test_aligned);

  CU_console_run_tests();

  CU_cleanup_registry();

  return 0;
}

int32_t main()
{
  RunAllTests();
}
  
