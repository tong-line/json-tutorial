#include <gtest/gtest.h>

#include "aegis/core/version.hpp"

TEST(Version, MatchesProjectVersion) { EXPECT_EQ(aegis::version(), "0.1.0"); }
