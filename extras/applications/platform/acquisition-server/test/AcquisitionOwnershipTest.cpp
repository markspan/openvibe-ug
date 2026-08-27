// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>

#include <gtest/gtest.h>

#include "Acquisition.h"

namespace {

constexpr uint64_t LOCAL  = 0;
constexpr uint64_t REMOTE = 42;
constexpr uint64_t OTHER  = 7;

}

// An acquisition a client made for itself is that client's to drive, and nobody else's.
TEST(AcquisitionOwnershipTest, ExclusiveIsControllableByItsOwnerOnly)
{
    Acquisition acquisition;
    acquisition.setOwner(REMOTE);

    EXPECT_FALSE(acquisition.isShared());
    EXPECT_TRUE(acquisition.isOwnedBy(REMOTE));
    EXPECT_TRUE(acquisition.isControllableBy(REMOTE));

    EXPECT_FALSE(acquisition.isOwnedBy(OTHER));
    EXPECT_FALSE(acquisition.isControllableBy(OTHER));
    EXPECT_FALSE(acquisition.isControllableBy(LOCAL));
}

// The acquisition the application sets up at startup belongs to nobody, so the user interface and a
// remote client may both drive it.
TEST(AcquisitionOwnershipTest, SharedIsControllableByAnyone)
{
    Acquisition acquisition;   // no owner set

    EXPECT_TRUE(acquisition.isShared());
    EXPECT_TRUE(acquisition.isControllableBy(LOCAL));
    EXPECT_TRUE(acquisition.isControllableBy(REMOTE));
    EXPECT_TRUE(acquisition.isControllableBy(OTHER));
}

// Shared means unowned, not owned-by-everyone: asking who owns it must not answer "you do".
TEST(AcquisitionOwnershipTest, SharedIsOwnedByNobody)
{
    Acquisition acquisition;

    EXPECT_FALSE(acquisition.isOwnedBy(LOCAL));
    EXPECT_FALSE(acquisition.isOwnedBy(REMOTE));
}
