
#include "usvfs_basic_test.h"

const char* usvfs_basic_test::scenario_name()
{
  return SCENARIO_NAME;
}

bool usvfs_basic_test::scenario_run()
{
  ops_list(L".", true, true);
  return true;
}
