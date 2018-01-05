
#include "usvfs_basic_test.h"

const char* usvfs_basic_test::scenario_name()
{
  return SCENARIO_NAME;
}

bool usvfs_basic_test::scenario_run()
{
  // Note: For regression purposes we don't really need to verify the results of our operations
  // as the usvfs_test_base postmortem_check should catch these things. At least for now these
  // verifications are left here as a "documentation" of open usvfs issues (like not having
  // proper copy_on_write, etc.).

  ops_list(LR"(.)", true, true);

  // test copy on write/delete against source "mod":

  {
    const auto& old_contents = source_contents(LR"(mod4\mfolder4\mfileoverwrite.txt)");
    verify_source_contents(LR"(mod4\mfolder4\mfileoverwrite.txt)", old_contents.c_str());
    ops_overwrite(LR"(mfolder4\mfileoverwrite.txt)", R"(mfolder4\mfileoverwrite.txt overwrite)", false);
    ops_read(LR"(mfolder4\mfileoverwrite.txt)");
    if (auto copy_on_write_implemented = false)
    {
      verify_source_contents(LR"(mod4\mfolder4\mfileoverwrite.txt)", old_contents.c_str());
      verify_source_contents(LR"(overwrite\mfolder4\mfileoverwrite.txt)", R"(mfolder4\mfileoverwrite.txt overwrite)");
    }
    else {
      verify_source_contents(LR"(mod4\mfolder4\mfileoverwrite.txt)", R"(mfolder4\mfileoverwrite.txt overwrite)");
      verify_source_existance(LR"(overwrite\mfolder4\mfileoverwrite.txt)", false);
    }
  }

  {
    const auto& old_contents = source_contents(LR"(mod4\mfolder4\mfilerewrite.txt)");
    verify_source_contents(LR"(mod4\mfolder4\mfilerewrite.txt)", old_contents.c_str());
    ops_rewrite(LR"(mfolder4\mfilerewrite.txt)", R"(mfolder4\mfilerewrite.txt rewrite)");
    ops_read(LR"(mfolder4\mfilerewrite.txt)");
    if (auto copy_on_write_implemented = false)
    {
      verify_source_contents(LR"(mod4\mfolder4\mfilerewrite.txt)", old_contents.c_str());
      verify_source_contents(LR"(overwrite\mfolder4\mfilerewrite.txt)", R"(mfolder4\mfilerewrite.txt rewrite)");
    }
    else {
      verify_source_contents(LR"(mod4\mfolder4\mfilerewrite.txt)", R"(mfolder4\mfilerewrite.txt rewrite)");
      verify_source_existance(LR"(overwrite\mfolder4\mfilerewrite.txt)", false);
    }
  }

  {
    const auto& old_contents = source_contents(LR"(mod4\mfolder4\mfilemoveover.txt)");
    verify_source_contents(LR"(mod4\mfolder4\mfilemoveover.txt)", old_contents.c_str());
    ops_overwrite(LR"(mfolder4\temp_mfilemoveover.txt)", R"(mfolder4\mfilemoveover.txt overwrite)", false);
    verify_source_contents(LR"(overwrite\mfolder4\temp_mfilemoveover.txt)", R"(mfolder4\mfilemoveover.txt overwrite)");
    ops_rename(LR"(mfolder4\temp_mfilemoveover.txt)", LR"(mfolder4\mfilemoveover.txt)", true);
    ops_read(LR"(mfolder4\mfilemoveover.txt)");
    verify_source_existance(LR"(overwrite\mfolder4\temp_mfilemoveover.txt)", false);
    verify_source_contents(LR"(mod4\mfolder4\mfilemoveover.txt)", old_contents.c_str());
    verify_source_contents(LR"(overwrite\mfolder4\mfilemoveover.txt)", R"(mfolder4\mfilemoveover.txt overwrite)");
  }

  {
    const auto& old_contents = source_contents(LR"(mod4\mfolder4\mfiledeletewrite.txt)");
    verify_source_contents(LR"(mod4\mfolder4\mfiledeletewrite.txt)", old_contents.c_str());
    ops_delete(LR"(mfolder4\mfiledeletewrite.txt)");
    ops_overwrite(LR"(mfolder4\mfiledeletewrite.txt)", R"(mfolder4\mfiledeletewrite.txt overwrite)", false);
    ops_read(LR"(mfolder4\mfiledeletewrite.txt)");
    verify_source_contents(LR"(overwrite\mfolder4\mfiledeletewrite.txt)", R"(mfolder4\mfiledeletewrite.txt deletewrite)");
    if (auto proper_delete_implemented = false)
      verify_source_contents(LR"(mod4\mfolder4\mfiledeletewrite.txt)", old_contents.c_str());
    else
      verify_source_existance(LR"(mod4\mfolder4\mfiledeletewrite.txt)", false);
  }

  {
    const auto& old_contents = source_contents(LR"(mod4\mfolder4\mfiledeletemove.txt)");
    verify_source_contents(LR"(mod4\mfolder4\mfiledeletemove.txt)", old_contents.c_str());
    ops_delete(LR"(mfolder4\mfiledeletemove.txt)");
    ops_overwrite(LR"(mfolder4\temp_mfiledeletemove.txt)", R"(mfolder4\mfiledeletemove.txt overwrite)", false);
    verify_source_contents(LR"(overwrite\mfolder4\temp_mfiledeletemove.txt)", R"(mfolder4\mfiledeletemove.txt overwrite)");
    ops_rename(LR"(mfolder4\temp_mfiledeletemove.txt)", LR"(mfolder4\mfiledeletemove.txt)", false);
    ops_read(LR"(mfolder4\mfiledeletemove.txt)");
    verify_source_existance(LR"(overwrite\mfolder4\temp_mfiledeletemove.txt)", false);
    verify_source_contents(LR"(overwrite\mfolder4\mfiledeletemove.txt)", R"(mfolder4\mfiledeletemove.txt overwrite)");
    if (auto proper_delete_implemented = false)
      verify_source_contents(LR"(mod4\mfolder4\mfiledeletemove.txt)", old_contents.c_str());
    else
      verify_source_existance(LR"(mod4\mfolder4\mfiledeletemove.txt)", false);
  }

  // test copy on write/delete/move against original mount files:

  {
    const auto& old_contents = mount_contents(LR"(rfolder\rfilerewrite.txt)");
    ops_rewrite(LR"(rfolder\rfilerewrite.txt)", R"(rfolder\rfilerewrite.txt rewrite)");
    ops_read(LR"(rfolder\rfilerewrite.txt)");
    if (auto copy_on_write_implemented = false)
    {
      verify_source_contents(LR"(overwrite\rfolder\rfilerewrite.txt)", R"(rfolder\rfilerewrite.txt rewrite)");
      verify_mount_contents(LR"(rfolder\rfilerewrite.txt)", old_contents.c_str());
    }
    else {
      verify_source_existance(LR"(overwrite\rfolder\rfilerewrite.txt)", false);
      verify_mount_contents(LR"(rfolder\rfilerewrite.txt)", R"(rfolder\rfilerewrite.txt rewrite)");
    }
  }

  {
    const auto& old_contents = mount_contents(LR"(rfolder\rfiledelete.txt)");
    ops_delete(LR"(rfolder\rfiledelete.txt)");
    ops_read(LR"(rfolder\rfiledelete.txt)", false);
    if (auto proper_delete_implemented = false)
      verify_mount_contents(LR"(rfolder\rfiledelete.txt)", old_contents.c_str());
    else
      verify_mount_existance(LR"(rfolder\rfiledelete.txt)", false);
  }

  {
    const auto& old_contents = mount_contents(LR"(rfolder\rfileoldname.txt)");
    ops_rename(LR"(rfolder\rfileoldname.txt)", LR"(rfolder\rfilenewname.txt)", false, false);
    ops_read(LR"(rfolder\rfileoldname.txt)", false);
    ops_read(LR"(rfolder\rfilenewname.txt)");
    verify_source_contents(LR"(overwrite\rfolder\rfilenewname.txt)", old_contents.c_str());
    verify_mount_existance(LR"(rfolder\rfilenewname.txt)", false);
    if (auto copy_on_move_implemented = false)
      verify_mount_contents(LR"(rfolder\rfileoldname.txt)", old_contents.c_str());
    else
      verify_mount_existance(LR"(rfolder\rfileoldname.txt)", false);
  }

  ops_list(LR"(.)", true, true);

  return true;
}
