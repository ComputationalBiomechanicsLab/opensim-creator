#include "OverlayFilesystem.h"

#include <liboscar/Platform/NativeFilesystem.h>
#include <liboscar/Utils/TemporaryDirectory.h>

#include <gtest/gtest.h>

#include <fstream>

using namespace osc;

TEST(OverlayFilesystem, can_default_construct)
{
    [[maybe_unused]] OverlayFilesystem default_constructed;
}

TEST(OverlayFilesystem, can_emplace_children)
{
    TemporaryDirectory dir1;
    TemporaryDirectory dir2;

    OverlayFilesystem overlay_filesystem;
    overlay_filesystem.emplace_lowest_priority<NativeFilesystem>(dir1.absolute_path());
    overlay_filesystem.emplace_lowest_priority<NativeFilesystem>(dir2.absolute_path());
}

TEST(OverlayFilesystem, resource_filepath_returns_correct_path)
{
    TemporaryDirectory dir1;  // highest priority (empty)
    TemporaryDirectory dir2;  // middle priority <----- the file of interest is here
    TemporaryDirectory dir3;  // lowest priority (empty)

    {
        std::ofstream file1{dir2.absolute_path() / "file1"};
    }

    OverlayFilesystem overlay_filesystem;
    overlay_filesystem.emplace_lowest_priority<NativeFilesystem>(dir1.absolute_path());
    overlay_filesystem.emplace_lowest_priority<NativeFilesystem>(dir2.absolute_path());
    overlay_filesystem.emplace_lowest_priority<NativeFilesystem>(dir3.absolute_path());

    ASSERT_EQ(overlay_filesystem.resource_filepath("file1"), std::filesystem::weakly_canonical(dir2.absolute_path() / "file1"));
}

TEST(OverlayFilesystem, resource_exists_returns_true_if_file_exists_in_some_layer)
{
    TemporaryDirectory dir1;  // foo.txt, /subdir/
    {
        std::ofstream foo{dir1.absolute_path() / "foo.txt"};
        std::filesystem::create_directory(dir1.absolute_path() / "subdir");
    }

    TemporaryDirectory dir2;  // /subdir/foo.txt, /subdir/bar.txt (no shadowing)
    {
        std::filesystem::create_directory(dir2.absolute_path() / "subdir");
        std::ofstream foo{dir2.absolute_path() / "subdir" / "foo.txt"};
        std::ofstream bar{dir2.absolute_path() / "subdir" / "bar.txt"};
    }

    TemporaryDirectory dir3;  // foo.txt, /subdir/foo.txt, /subdir/foobar.txt (two entries shadowed)
    {
        std::ofstream foo{dir3.absolute_path() / "foo.txt"};
        std::filesystem::create_directory(dir3.absolute_path() / "subdir");
        std::ofstream subfoo{dir3.absolute_path() / "subdir" / "foo.txt"};
        std::ofstream subfoobar{dir3.absolute_path() / "subdir" / "foobar.txt"};
    }

    OverlayFilesystem overlay_filesystem;
    overlay_filesystem.emplace_lowest_priority<NativeFilesystem>(dir1.absolute_path());
    overlay_filesystem.emplace_lowest_priority<NativeFilesystem>(dir2.absolute_path());
    overlay_filesystem.emplace_lowest_priority<NativeFilesystem>(dir3.absolute_path());

    ASSERT_TRUE(overlay_filesystem.resource_exists("foo.txt"));
    ASSERT_TRUE(overlay_filesystem.resource_exists("subdir/foo.txt"));
    ASSERT_TRUE(overlay_filesystem.resource_exists("subdir/bar.txt"));
    ASSERT_TRUE(overlay_filesystem.resource_exists("subdir/foobar.txt"));

    ASSERT_FALSE(overlay_filesystem.resource_exists("bar.txt"));
    ASSERT_FALSE(overlay_filesystem.resource_exists("foobar.txt"));
    ASSERT_FALSE(overlay_filesystem.resource_exists("subdir")) << "Directories are only shown during iteration: they aren't load-able resources";
    ASSERT_FALSE(overlay_filesystem.resource_exists("subdir/barfoo.txt"));
}

TEST(OverlayFilesystem, slurp_slurps_the_correct_resource_stream)
{
    // foo.txt, /subdir/
    TemporaryDirectory dir1;
    {
        std::ofstream foo{dir1.absolute_path() / "foo.txt"};
        std::filesystem::create_directory(dir1.absolute_path() / "subdir");
    }

    // /subdir/foo.txt, /subdir/bar.txt (no shadowing)
    TemporaryDirectory dir2;
    {
        std::filesystem::create_directory(dir2.absolute_path() / "subdir");
        std::ofstream foo{dir2.absolute_path() / "subdir" / "foo.txt"};
        foo << "expected content";  // <----------------------------------- data written here
        std::ofstream bar{dir2.absolute_path() / "subdir" / "bar.txt"};
    }

    // foo.txt, /subdir/foo.txt, /subdir/foobar.txt (two entries shadowed)
    TemporaryDirectory dir3;
    {
        std::ofstream foo{dir3.absolute_path() / "foo.txt"};
        std::filesystem::create_directory(dir3.absolute_path() / "subdir");
        std::ofstream subfoo{dir3.absolute_path() / "subdir" / "foo.txt"};
        foo << "shadowed";  // <------------------------------------------- shouldn't be visible
        std::ofstream subfoobar{dir3.absolute_path() / "subdir" / "foobar.txt"};
    }

    OverlayFilesystem overlay_filesystem;
    overlay_filesystem.emplace_lowest_priority<NativeFilesystem>(dir1.absolute_path());
    overlay_filesystem.emplace_lowest_priority<NativeFilesystem>(dir2.absolute_path());
    overlay_filesystem.emplace_lowest_priority<NativeFilesystem>(dir3.absolute_path());

    ASSERT_EQ(overlay_filesystem.slurp("subdir/foo.txt"), "expected content");
}
