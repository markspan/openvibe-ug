///-------------------------------------------------------------------------------------------------
/// 
/// \file uoEntryEnumeratorTest.cpp
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#include <string>
#include <vector>

#include <fs/IEntryEnumerator.h>

#include "ovtAssert.h"

class EntryEnumeratorCallBack final : public FS::IEntryEnumeratorCallBack
{
public:
    bool callback(FS::IEntryEnumerator::IEntry& rEntry, FS::IEntryEnumerator::IAttributes& rAttributes) override
    {
        if (rAttributes.isFile()) { m_files.push_back(rEntry.getName()); }
        return true;
    }

    void release() { m_files.clear(); }

    std::vector<std::string> m_files;
};

int uoEntryEnumeratorTest(int /*argc*/, char* argv[])
{
    const std::string dataDirectory = argv[1];

    EntryEnumeratorCallBack cb;
    FS::IEntryEnumerator* enumerator = createEntryEnumerator(cb);
    enumerator->enumerate((dataDirectory + "*.txt").c_str());

    OVT_ASSERT(cb.m_files.size() == 2, "Failure to enumerate with wildcard prefix");

    cb.release();

    // test wildcard after
    enumerator->enumerate((dataDirectory + "test*").c_str());

    OVT_ASSERT(cb.m_files.size() == 2, "Failure to enumerate with wildcard suffix");

    cb.release();

    // test wildcard after
    enumerator->enumerate((dataDirectory + "t*").c_str());

    OVT_ASSERT(cb.m_files.size() == 3, "Failure to enumerate with single letter");

    cb.release();

    // test wildcard before and after
    enumerator->enumerate((dataDirectory + "*oto*").c_str());

    OVT_ASSERT(cb.m_files.size() == 1, "Failure to enumerate with englobing wildcards");
    OVT_ASSERT_STREQ(cb.m_files[0], std::string(dataDirectory + "toto.md"), "Failure to enumerate with englobing wildcards");

    cb.release();

    // test wildcard in middle
    enumerator->enumerate((dataDirectory + "test1*xt").c_str());

    OVT_ASSERT(cb.m_files.size() == 1, "Failure to enumerate with middle wildcard");
    OVT_ASSERT_STREQ(cb.m_files[0], std::string(dataDirectory + "test1.txt"), "Failure to enumerate with middle wildcard");

    cb.release();

    // error case: no wildcard
    enumerator->enumerate((dataDirectory + "t").c_str());

    OVT_ASSERT(cb.m_files.empty(), "Failure to enumerate with no wildcard");

    cb.release();

    enumerator->release();

    return EXIT_SUCCESS;
}
