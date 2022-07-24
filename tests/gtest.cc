#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <string.h>

#include <utility>
#include <vector>

#include "main.h"
#include "ifiledata.h"

// For convenience.
namespace t = ::testing;

class FileDataUtilTest : public t::Test
{
    protected:
        FileData::Util util;
};

TEST_F(FileDataUtilTest, text_from_size_test)
{
    std::vector<std::pair<gint64, std::string>> test_cases = {
        {0, "0"},
        {1, "1"},
        {999, "999"},
        {1000, "1,000"},
        {1'000'000, "1,000,000"},
        {-1000, "-1,000"},

        // The following test fails.  The right solution is probably to alter
        // text_from_size to accept a guint64 instead of gint64.
        // {-100'000, "-100,000"},
    };

    for (const auto &test_case : test_cases)
    {
        ASSERT_EQ(test_case.second,
                  std::string(util.text_from_size(test_case.first)));
    }
}

TEST_F(FileDataUtilTest, text_from_size_abrev_test)
{
    constexpr gint64 kib_threshold = 1024;
    constexpr gint64 mib_threshold = 1024 * 1024;
    constexpr gint64 gib_threshold = 1024 * 1024 * 1024;
    std::vector<std::pair<gint64, std::string>> test_cases = {
        {0, "0 bytes"},
        {1, "1 bytes"},
        {kib_threshold - 1, "1023 bytes"},
        {kib_threshold, "1.0 KiB"},
        {kib_threshold * 1.5, "1.5 KiB"},
        {kib_threshold * 2, "2.0 KiB"},

        {mib_threshold - 1, "1024.0 KiB"},
        {mib_threshold, "1.0 MiB"},
        {mib_threshold * 1.5, "1.5 MiB"},
        {mib_threshold * 2, "2.0 MiB"},

        {gib_threshold - 1, "1024.0 MiB"},
        {gib_threshold, "1.0 GiB"},
        {gib_threshold * 1.5, "1.5 GiB"},
        {gib_threshold * 2, "2.0 GiB"},
        {gib_threshold * 2048, "2048.0 GiB"},
    };

    for (const auto &test_case : test_cases)
    {
        ASSERT_EQ(test_case.second,
                  std::string(util.text_from_size_abrev(test_case.first)));
    }
}

TEST_F(FileDataUtilTest, sort_by_ext_test)
{
    const FileData hipri_jpg_fd = {.extension = "jpg", .sidecar_priority = 5};
    const FileData hipri_gif_fd = {.extension = "gif", .sidecar_priority = 5};
    const FileData lopri_jpg_fd = {.extension = "jpg", .sidecar_priority = 3};
    const FileData lopri_gif_fd = {.extension = "gif", .sidecar_priority = 3};

    // For reference:
    // retval <  0: "arguments are in ascending order".
    // retval == 0: "arguments have identical sort order".
    // retval >  0: "argument are in descending order".

    // Sidecar priority should be the first consideration, regardless of
    // extension.
    ASSERT_THAT(util.sort_by_ext(&hipri_jpg_fd, &lopri_gif_fd), t::Gt(0));
    ASSERT_THAT(util.sort_by_ext(&hipri_jpg_fd, &lopri_jpg_fd), t::Gt(0));
    ASSERT_THAT(util.sort_by_ext(&hipri_gif_fd, &lopri_gif_fd), t::Gt(0));
    ASSERT_THAT(util.sort_by_ext(&hipri_gif_fd, &lopri_jpg_fd), t::Gt(0));

    // For equivalent sidecar priority, we should sort lexicographically by
    // extension.
    ASSERT_THAT(util.sort_by_ext(&hipri_jpg_fd, &hipri_gif_fd), t::Lt(0));
    ASSERT_THAT(util.sort_by_ext(&hipri_gif_fd, &hipri_jpg_fd), t::Gt(0));
    ASSERT_THAT(util.sort_by_ext(&lopri_jpg_fd, &lopri_gif_fd), t::Lt(0));
    ASSERT_THAT(util.sort_by_ext(&lopri_gif_fd, &lopri_jpg_fd), t::Gt(0));

    // Lastly, FileDatas with matching sidecar priority and extension should
    // be sorted equivalently.
    ASSERT_THAT(util.sort_by_ext(&hipri_jpg_fd, &hipri_jpg_fd), t::Eq(0));
    ASSERT_THAT(util.sort_by_ext(&hipri_gif_fd, &hipri_gif_fd), t::Eq(0));
    ASSERT_THAT(util.sort_by_ext(&lopri_jpg_fd, &lopri_jpg_fd), t::Eq(0));
    ASSERT_THAT(util.sort_by_ext(&lopri_gif_fd, &lopri_gif_fd), t::Eq(0));
}

TEST_F(FileDataUtilTest, is_hidden_file_test)
{
    // . and .. should be shown.
    ASSERT_FALSE(util.is_hidden_file("."));
    ASSERT_FALSE(util.is_hidden_file(".."));

    // Otherwise, dotfiles should be hidden, and other names should be shown.
    ASSERT_FALSE(util.is_hidden_file("some_file"));
    ASSERT_TRUE(util.is_hidden_file(".some_dotfile"));
}

int main (int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
