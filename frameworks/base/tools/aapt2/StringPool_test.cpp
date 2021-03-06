/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "StringPool.h"
#include "Util.h"

#include <gtest/gtest.h>
#include <string>

using namespace android;

namespace aapt {

TEST(StringPoolTest, InsertOneString) {
    StringPool pool;

    StringPool::Ref ref = pool.makeRef(u"wut");
    EXPECT_EQ(*ref, u"wut");
}

TEST(StringPoolTest, InsertTwoUniqueStrings) {
    StringPool pool;

    StringPool::Ref ref = pool.makeRef(u"wut");
    StringPool::Ref ref2 = pool.makeRef(u"hey");

    EXPECT_EQ(*ref, u"wut");
    EXPECT_EQ(*ref2, u"hey");
}

TEST(StringPoolTest, DoNotInsertNewDuplicateString) {
    StringPool pool;

    StringPool::Ref ref = pool.makeRef(u"wut");
    StringPool::Ref ref2 = pool.makeRef(u"wut");

    EXPECT_EQ(*ref, u"wut");
    EXPECT_EQ(*ref2, u"wut");
    EXPECT_EQ(1u, pool.size());
}

TEST(StringPoolTest, MaintainInsertionOrderIndex) {
    StringPool pool;

    StringPool::Ref ref = pool.makeRef(u"z");
    StringPool::Ref ref2 = pool.makeRef(u"a");
    StringPool::Ref ref3 = pool.makeRef(u"m");

    EXPECT_EQ(0u, ref.getIndex());
    EXPECT_EQ(1u, ref2.getIndex());
    EXPECT_EQ(2u, ref3.getIndex());
}

TEST(StringPoolTest, PruneStringsWithNoReferences) {
    StringPool pool;

    {
        StringPool::Ref ref = pool.makeRef(u"wut");
        EXPECT_EQ(*ref, u"wut");
        EXPECT_EQ(1u, pool.size());
    }

    EXPECT_EQ(1u, pool.size());
    pool.prune();
    EXPECT_EQ(0u, pool.size());
}

TEST(StringPoolTest, SortAndMaintainIndexesInReferences) {
    StringPool pool;

    StringPool::Ref ref = pool.makeRef(u"z");
    StringPool::StyleRef ref2 = pool.makeRef(StyleString{ {u"a"} });
    StringPool::Ref ref3 = pool.makeRef(u"m");

    EXPECT_EQ(*ref, u"z");
    EXPECT_EQ(0u, ref.getIndex());

    EXPECT_EQ(*(ref2->str), u"a");
    EXPECT_EQ(1u, ref2.getIndex());

    EXPECT_EQ(*ref3, u"m");
    EXPECT_EQ(2u, ref3.getIndex());

    pool.sort([](const StringPool::Entry& a, const StringPool::Entry& b) -> bool {
        return a.value < b.value;
    });


    EXPECT_EQ(*ref, u"z");
    EXPECT_EQ(2u, ref.getIndex());

    EXPECT_EQ(*(ref2->str), u"a");
    EXPECT_EQ(0u, ref2.getIndex());

    EXPECT_EQ(*ref3, u"m");
    EXPECT_EQ(1u, ref3.getIndex());
}

TEST(StringPoolTest, SortAndStillDedupe) {
    StringPool pool;

    StringPool::Ref ref = pool.makeRef(u"z");
    StringPool::Ref ref2 = pool.makeRef(u"a");
    StringPool::Ref ref3 = pool.makeRef(u"m");

    pool.sort([](const StringPool::Entry& a, const StringPool::Entry& b) -> bool {
        return a.value < b.value;
    });

    StringPool::Ref ref4 = pool.makeRef(u"z");
    StringPool::Ref ref5 = pool.makeRef(u"a");
    StringPool::Ref ref6 = pool.makeRef(u"m");

    EXPECT_EQ(ref4.getIndex(), ref.getIndex());
    EXPECT_EQ(ref5.getIndex(), ref2.getIndex());
    EXPECT_EQ(ref6.getIndex(), ref3.getIndex());
}

TEST(StringPoolTest, AddStyles) {
    StringPool pool;

    StyleString str {
        { u"android" },
        {
            Span{ { u"b" }, 2, 6 }
        }
    };

    StringPool::StyleRef ref = pool.makeRef(str);

    EXPECT_EQ(0u, ref.getIndex());
    EXPECT_EQ(std::u16string(u"android"), *(ref->str));
    ASSERT_EQ(1u, ref->spans.size());

    const StringPool::Span& span = ref->spans.front();
    EXPECT_EQ(*(span.name), u"b");
    EXPECT_EQ(2u, span.firstChar);
    EXPECT_EQ(6u, span.lastChar);
}

TEST(StringPoolTest, DoNotDedupeStyleWithSameStringAsNonStyle) {
    StringPool pool;

    StringPool::Ref ref = pool.makeRef(u"android");

    StyleString str { { u"android" } };
    StringPool::StyleRef styleRef = pool.makeRef(str);

    EXPECT_NE(ref.getIndex(), styleRef.getIndex());
}

TEST(StringPoolTest, FlattenEmptyStringPoolUtf8) {
    StringPool pool;
    BigBuffer buffer(1024);
    StringPool::flattenUtf8(&buffer, pool);

    std::unique_ptr<uint8_t[]> data = util::copy(buffer);
    android::ResStringPool test;
    ASSERT_EQ(test.setTo(data.get(), buffer.size()), android::NO_ERROR);
}

constexpr const char16_t* sLongString = u"??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????SMS?????????????????? ????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????OFF??????????????????";

TEST(StringPoolTest, FlattenUtf8) {
    StringPool pool;

    StringPool::Ref ref1 = pool.makeRef(u"hello");
    StringPool::Ref ref2 = pool.makeRef(u"goodbye");
    StringPool::Ref ref3 = pool.makeRef(sLongString);
    StringPool::StyleRef ref4 = pool.makeRef(StyleString{
            { u"style" },
            { Span{ { u"b" }, 0, 1 }, Span{ { u"i" }, 2, 3 } }
    });

    EXPECT_EQ(0u, ref1.getIndex());
    EXPECT_EQ(1u, ref2.getIndex());
    EXPECT_EQ(2u, ref3.getIndex());
    EXPECT_EQ(3u, ref4.getIndex());

    BigBuffer buffer(1024);
    StringPool::flattenUtf8(&buffer, pool);

    std::unique_ptr<uint8_t[]> data = util::copy(buffer);
    {
        android::ResStringPool test;
        ASSERT_EQ(test.setTo(data.get(), buffer.size()), android::NO_ERROR);

        EXPECT_EQ(util::getString(test, 0), u"hello");
        EXPECT_EQ(util::getString(test, 1), u"goodbye");
        EXPECT_EQ(util::getString(test, 2), sLongString);
        EXPECT_EQ(util::getString(test, 3), u"style");

        const ResStringPool_span* span = test.styleAt(3);
        ASSERT_NE(nullptr, span);
        EXPECT_EQ(util::getString(test, span->name.index), u"b");
        EXPECT_EQ(0u, span->firstChar);
        EXPECT_EQ(1u, span->lastChar);
        span++;

        ASSERT_NE(ResStringPool_span::END, span->name.index);
        EXPECT_EQ(util::getString(test, span->name.index), u"i");
        EXPECT_EQ(2u, span->firstChar);
        EXPECT_EQ(3u, span->lastChar);
        span++;

        EXPECT_EQ(ResStringPool_span::END, span->name.index);
    }
}

} // namespace aapt
