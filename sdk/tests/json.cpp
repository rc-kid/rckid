#include <platform/tests.h>
#include <rckid/utils/json.h>

using namespace rckid;

TEST(json, emptyIsNull) {
    json::Object o;
    EXPECT(o.kind() == json::Object::Kind::Null);
}