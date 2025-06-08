#include <platform/tests.h>
#include <rckid/utils/json.h>

using namespace rckid;

TEST(json, emptyIsNull) {
    json::Object o;
    EXPECT(o.kind() == json::Object::Kind::Null);
}

TEST(json, boolean) {
    json::Object o(true);
    EXPECT(o.kind() == json::Object::Kind::Boolean);
    EXPECT(o.asBoolean() == true);
    json::Object o2(false);
    EXPECT(o2.kind() == json::Object::Kind::Boolean);
    EXPECT(o2.asBoolean() == false);
}

TEST(json, integer) {
    json::Object o(42);
    EXPECT(o.kind() == json::Object::Kind::Integer);
    EXPECT(o.asInteger() == 42);
}

TEST(json, double) {
    json::Object o(3.14);
    EXPECT(o.kind() == json::Object::Kind::Double);
    EXPECT(o.asDouble() == 3.14);
}

TEST(json, string) {
    json::Object o("Hello world");
    EXPECT(o.kind() == json::Object::Kind::String);
    EXPECT(o.isString() == true);
    EXPECT(o.asString() == "Hello world");
}

TEST(json, array) {
    json::Object o{json::Object::Kind::Array};
    EXPECT(o.kind() == json::Object::Kind::Array);
    EXPECT(o.isArray() == true);
    EXPECT(o.size() == 0);
    o.add(json::Object{"foo"});
    EXPECT(o.isArray() == true);
    EXPECT(o.size() == 1);
    EXPECT(o[0].asString() == "foo");
    o.add(json::Object{"bar"});
    EXPECT(o.isArray() == true);
    EXPECT(o.size() == 2);
    EXPECT(o[0].asString() == "foo");
    EXPECT(o[1].asString() == "bar");
}

TEST(json, struct) {
    json::Object o{json::Object::Kind::Struct};
    EXPECT(o.kind() == json::Object::Kind::Struct);
    EXPECT(o.isStruct() == true);
    EXPECT(o.size() == 0);
    o.add("foo", json::Object{42});
    o.add("bar", json::Object{"foobar"});
    EXPECT(o.isStruct() == true);
    EXPECT(o.size() == 2);
    EXPECT(o["foo"].asInteger() == 42);
    EXPECT(o["bar"].asString() == "foobar");
}

TEST(json, parseBoolean) {
    json::Object o = json::parse("true");
    EXPECT(o.kind() == json::Object::Kind::Boolean);
    EXPECT(o.asBoolean() == true);
    
    o = json::parse("false");
    EXPECT(o.kind() == json::Object::Kind::Boolean);
    EXPECT(o.asBoolean() == false);
}

TEST(json, parseInteger) {
    json::Object o = json::parse("42");
    EXPECT(o.kind() == json::Object::Kind::Integer);
    EXPECT(o.asInteger() == 42);
    
    o = json::parse("-42");
    EXPECT(o.kind() == json::Object::Kind::Integer);
    EXPECT(o.asInteger() == -42);
}

TEST(json, parseDouble) {
    json::Object o = json::parse("3.14");
    EXPECT(o.kind() == json::Object::Kind::Double);
    EXPECT(o.asDouble() == 3.14);
    
    o = json::parse("-3.14");
    EXPECT(o.kind() == json::Object::Kind::Double);
    EXPECT(o.asDouble() == -3.14);
}

TEST(json, parseString) {
    json::Object o = json::parse("\"Hello world\"");
    EXPECT(o.kind() == json::Object::Kind::String);
    EXPECT(o.asString() == "Hello world");
    
    o = json::parse("\"\"");
    EXPECT(o.kind() == json::Object::Kind::String);
    EXPECT(o.asString() == "");
}

TEST(json, parseArray) {
    json::Object o = json::parse("[1, 2, 3]");
    EXPECT(o.kind() == json::Object::Kind::Array);
    EXPECT(o.size() == 3);
    EXPECT(o[0].asInteger() == 1);
    EXPECT(o[1].asInteger() == 2);
    EXPECT(o[2].asInteger() == 3);
    
    o = json::parse("[\"foo\", \"bar\"]");
    EXPECT(o.kind() == json::Object::Kind::Array);
    EXPECT(o.size() == 2);
    EXPECT(o[0].asString() == "foo");
    EXPECT(o[1].asString() == "bar");
}

TEST(json, parseStruct) {
    json::Object o = json::parse("{\"foo\": 42, \"bar\": \"baz\"}");
    EXPECT(o.kind() == json::Object::Kind::Struct);
    EXPECT(o.size() == 2);
    EXPECT(o["foo"].asInteger() == 42);
    EXPECT(o["bar"].asString() == "baz");
    
    o = json::parse("{}");
    EXPECT(o.kind() == json::Object::Kind::Struct);
    EXPECT(o.size() == 0);
}

TEST(json, parseNull) {
    json::Object o = json::parse("null");
    EXPECT(o.kind() == json::Object::Kind::Null);
    
    o = json::parse("undefined");
    EXPECT(o.kind() == json::Object::Kind::Null);
}

TEST(json, parseNestedStruct) {
    json::Object o = json::parse("{\"foo\": {\"bar\": 42}}");
    EXPECT(o.kind() == json::Object::Kind::Struct);
    EXPECT(o.size() == 1);
    EXPECT(o["foo"].kind() == json::Object::Kind::Struct);
    EXPECT(o["foo"]["bar"].asInteger() == 42);
    
    o = json::parse("{\"foo\": [1, 2, 3]}");
    EXPECT(o.kind() == json::Object::Kind::Struct);
    EXPECT(o.size() == 1);
    EXPECT(o["foo"].kind() == json::Object::Kind::Array);
    EXPECT(o["foo"].size() == 3);
}

TEST(json, writeBoolean) {
    json::Object o(true);
    StringWriter sw;
    sw << o;
    EXPECT(sw.str() == "true");
    
    o = json::Object(false);
    sw << o;
    EXPECT(sw.str() == "false");
}

TEST(json, writeInteger) {
    json::Object o(42);
    StringWriter sw;
    sw << o;
    EXPECT(sw.str() == "42");
    
    o = json::Object(-42);
    sw << o;
    EXPECT(sw.str() == "-42");
}

TEST(json, writeDouble) {
    json::Object o(3.14);
    StringWriter sw;
    sw << o;
    EXPECT(sw.str() == "3.14");
    
    o = json::Object(-3.14);
    sw << o;
    EXPECT(sw.str() == "-3.14");
}

TEST(json, writeString) {
    json::Object o("Hello world");
    StringWriter sw;
    sw << o;
    EXPECT(sw.str() == "\"Hello world\"");
    
    o = json::Object("");
    sw << o;
    EXPECT(sw.str() == "\"\"");

    o = json::Object("foo\"bar");
    sw << o;
    EXPECT(sw.str() == "\"foo\\\"bar\"");
}

TEST(json, writeArray) {
    json::Object o{json::Object::Kind::Array};
    o.add(json::Object{1});
    o.add(json::Object{2});
    o.add(json::Object{3});
    
    StringWriter sw;
    sw << o;
    EXPECT(sw.str() == "[\n  1,\n  2,\n  3\n]");
}

TEST(json, writeStruct) {
    json::Object o{json::Object::Kind::Struct};
    o.add("foo", json::Object{42});
    
    StringWriter sw;
    sw << o;
    EXPECT(sw.str() == "{\n  \"foo\": 42\n}");
}

TEST(json, writeNestedStruct) {
    json::Object o{json::Object::Kind::Struct};
    o.add("foo", json::Object{json::Object::Kind::Struct});
    o["foo"].add("bar", json::Object{42});
    
    StringWriter sw;
    sw << o;
    EXPECT(sw.str() == "{\n  \"foo\": {\n    \"bar\": 42\n  }\n}");
    
    o = json::parse("{\"foo\": [1, 2, 3]}");
    sw << o;
    EXPECT(sw.str() == "{\n  \"foo\": [\n    1,\n    2,\n    3\n  ]\n}");
}

TEST(json, emptyArray) {
    json::Object o{json::Object::Kind::Array};
    StringWriter sw;
    sw << o;
    EXPECT(sw.str() == "[]");
}
TEST(json, emptyStruct) {
    json::Object o{json::Object::Kind::Struct};
    StringWriter sw;
    sw << o;
    EXPECT(sw.str() == "{}");
}
