#pragma once

#include "../rckid.h"
#include "stream.h"

namespace rckid::json {

    /** JSON object. 
     */
    class Object {
    public:
        enum class Kind {
            Null,
            Boolean,
            Integer, 
            Double,
            String,
            Array,
            Struct,
        }; 

        class Iterator {
        public:
            Iterator(Object & object, uint32_t index) : object_{object}, index_{index} {}

            Object & operator * () { return *get(); }

            Object * operator -> () { return get(); }

            Iterator & operator ++ () {
                if (index_ < object_.size())
                    ++index_;
                return *this;
            }

            bool operator == (Iterator const & other) const {
                return &object_ == &other.object_ && index_ == other.index_;
            }

            bool operator != (Iterator const & other) const {
                return !(*this == other);
            }
        private:

            Object * get() {
                if (object_.isArray()) {
                    return &object_.array_[index_];
                } else {
                    auto it = object_.struct_.begin();
                    std::advance(it, index_);
                    return &it->second;
                }
            }

            Object & object_;
            uint32_t index_;
        }; // json::Object::Iterator

        Object() : kind_{Kind::Null} {}

        Object(Object && other) : kind_{other.kind_} {
            switch (kind_) {
                case Kind::Null:
                    break;
                case Kind::Boolean:
                    boolean_ = other.boolean_;
                    break;
                case Kind::Integer:
                    int_ = other.int_;
                    break;
                case Kind::Double:
                    double_ = other.double_;
                    break;
                case Kind::String:
                    new (&string_) String(std::move(other.string_));
                    break;
                case Kind::Array:
                    new (&array_) std::vector<Object>(std::move(other.array_));
                    break;
                case Kind::Struct:
                    new (&struct_) std::unordered_map<String, Object>(std::move(other.struct_));
                    break;
            }
            other.kind_ = Kind::Null;
        }

        explicit Object(bool value) : kind_{Kind::Boolean}, boolean_{value} {}
        explicit Object(int32_t value) : kind_{Kind::Integer}, int_{value} {}
        explicit Object(double value) : kind_{Kind::Double}, double_{value} {}
        explicit Object(String const & value) : kind_{Kind::String}, string_{value} {}
        explicit Object(char const * value) : kind_{Kind::String}, string_{value} {}

        explicit Object(Kind kind) : kind_{kind} {
            switch (kind_) {
                case Kind::Null:
                    break;
                case Kind::Boolean:
                    boolean_ = false; // default value
                    break;
                case Kind::Integer:
                    int_ = 0; // default value
                    break;
                case Kind::Double:
                    double_ = 0.0; // default value
                    break;
                case Kind::String:
                    new (&string_) String(); // default empty string
                    break;
                case Kind::Array:
                    new (&array_) std::vector<Object>(); // default empty array
                    break;
                case Kind::Struct:
                    new (&struct_) std::unordered_map<String, Object>(); // default empty struct
                    break;
            }
        }

        Object & operator = (Object && other) {
            if (this != &other) {
                release();
                // move the other
                kind_ = other.kind_;
                switch (kind_) {
                    case Kind::Null:
                        break;
                    case Kind::Boolean:
                        boolean_ = other.boolean_;
                        break;
                    case Kind::Integer:
                        int_ = other.int_;
                        break;
                    case Kind::Double:
                        double_ = other.double_;
                        break;
                    case Kind::String:
                        new (&string_) String(std::move(other.string_));
                        break;
                    case Kind::Array:
                        new (&array_) std::vector<Object>(std::move(other.array_));
                        break;
                    case Kind::Struct:
                        new (&struct_) std::unordered_map<String, Object>(std::move(other.struct_));
                        break;
                }
                other.kind_ = Kind::Null; // mark the other as empty
            }
            return *this;
        }

        ~Object() {
            release();
        }

        bool isNull() const { return kind_ == Kind::Null; }
        bool isBoolean() const { return kind_ == Kind::Boolean; }
        bool isInteger() const { return kind_ == Kind::Integer; }
        bool isDouble() const { return kind_ == Kind::Double; }
        bool isString() const { return kind_ == Kind::String; }
        bool isArray() const { return kind_ == Kind::Array; }
        bool isStruct() const { return kind_ == Kind::Struct; }

        Kind kind() const {
            return kind_;
        }

        int32_t asInteger() const {
            ASSERT(kind_ == Kind::Integer);
            return int_;
        }

        double asDouble() const {
            ASSERT(kind_ == Kind::Double);
            return double_;
        }

        bool asBoolean() const {
            ASSERT(kind_ == Kind::Boolean);
            return boolean_;
        }

        String asString() const {
            ASSERT(kind_ == Kind::String);
            return string_;
        }

        String asStringOrDefault(String defaultValue) const {
            if (kind_ == Kind::String)
                return string_;
            else 
                return defaultValue;
        }

        bool has(String const & key) const {
            ASSERT(kind_ == Kind::Struct);
            return struct_.find(key) != struct_.end();
        }

        uint32_t size() const {
            switch (kind_) {
                case Kind::Null:
                case Kind::Boolean:
                case Kind::Integer:
                case Kind::Double:
                    return 0; // these types have no size
                case Kind::String:
                    return string_.size();
                case Kind::Array:
                    return array_.size();
                case Kind::Struct:
                    return struct_.size();
            }
            UNREACHABLE; // should never reach here
        }

        Object & operator[](uint32_t index) {
            ASSERT(kind_ == Kind::Array);
            return array_[index];
        }

        Object & operator[](String const & key) {
            ASSERT(kind_ == Kind::Struct);
            return struct_[key];
        }

        Object const & operator[](uint32_t index) const {
            ASSERT(kind_ == Kind::Array);
            return array_[index];
        }

        Object const & operator[](String const & key) const {
            ASSERT(kind_ == Kind::Struct);
            return struct_.find(key)->second;
        }

        Iterator begin() { return Iterator{*this, 0}; }

        Iterator end() { return Iterator{*this, size()}; }

        void add(Object && object) {
            ASSERT(kind_ == Kind::Array);
            array_.push_back(std::move(object));
        }

        void add(String const & key, Object && object) {
            ASSERT(kind_ == Kind::Struct);
            struct_[key] = std::move(object);
        }

        void add(char const * key, Object && object) {
            ASSERT(kind_ == Kind::Struct);
            struct_[String{key}] = std::move(object);
        }


    private:

        friend Writer & operator << (Writer & writer, Object const & object) {
            object.write(writer);
            return writer;
        }

        friend Writer && operator << (Writer && writer, Object const & object) {
            object.write(writer);
            return std::move(writer);
        }

        void write(Writer & w, uint32_t indent = 0) const {
            switch (kind_) {
                case Kind::Null:
                    w << "null";
                    break;
                case Kind::Boolean:
                    w << (boolean_ ? "true" : "false");
                    break;
                case Kind::Integer:
                    w << int_;
                    break;
                case Kind::Double:
                    w << double_;
                    break;
                case Kind::String:
                    w << '"';
                    for (uint32_t i = 0, e = string_.size(); i != e; ++i) {
                        char c = string_[i];
                        switch (c) {
                            case '\n':
                                w << "\\n";
                                break;
                            case '\r':
                                w << "\\r";
                                break;
                            case '\t':
                                w << "\\t";
                                break;
                            case '\\':
                            case '"':
                            case '\'':
                                w << '\\' << c;
                                break; // these characters will be escaped below
                            default:
                                w << c; 
                        }
                    }
                    w << '"';
                    break;
                case Kind::Array: {
                    if (array_.size() == 0) {
                        w << "[]";
                        break;
                    }
                    w << "[\n";
                    indent += 2;
                    for (size_t i = 0; i < array_.size(); ++i) {
                        if (i > 0)
                            w << ",\n";
                        writeIndent(w, indent);
                        array_[i].write(w, indent);
                    }
                    indent -= 2;
                    w << "\n";
                    writeIndent(w, indent);
                    w << ']';
                    break;
                }
                case Kind::Struct: {
                    if (struct_.size() == 0) {
                        w << "{}";
                        break;
                    }
                    w << "{\n";
                    indent += 2;
                    bool first = true;
                    for (auto const & pair : struct_) {
                        if (!first) 
                            w << ",\n";
                        writeIndent(w, indent);
                        first = false;
                        w << '"' << pair.first << "\": "; 
                        pair.second.write(w, indent);
                    }
                    indent -= 2;
                    w << "\n";
                    writeIndent(w, indent);
                    w << '}';
                    break;
                }
            }
        }

        void writeIndent(Writer & w, uint32_t indent) const {
            for (uint32_t i = 0; i < indent; ++i)
                w << ' ';
        }

        void release() {
            if (kind_ == Kind::String) {
                string_.~String();
            } else if (kind_ == Kind::Array) {
                array_.~vector();
            } else if (kind_ == Kind::Struct) {
                struct_.~unordered_map();
            }
            kind_ = Kind::Null; // reset to null
        }

        Kind kind_;
        union {
            bool boolean_;
            int32_t int_;
            double double_;
            String string_;
            std::vector<Object> array_;
            std::unordered_map<String, Object> struct_;
        };
    }; 

    /** 
    
        This follows a very simple grammer:

        JSON := Array | Struct | String | Number | boolean | null
        ARRAY := '[' (JSON (',' JSON)*)? ']'
        STRUCT := '{' (STRING ':' JSON (',' STRING ':' JSON)*)? '}'
        STRING := '"' (any character except '"')* '"'
        NUMBER := integer | double 
        BOOLEAN : true | false
        NULL : null | undefined

      */
    class Parser {
    public:
        /** Parses JSON value from the given stream and returns the resulting JSON object. 
         */
        static Object parse(ReadStream & stream) {
            Parser p{stream};
            p.next();
            return p.parseObject();
        }

    private:

        Object parseObject() {
            skipWhitespace();
            char t = top();
            if (t == '{')
                return parseStruct();
            else if (t == '[')
                return parseArray();
            else if (t == '"')
                return parseString();
            else if (t == '-' || (t >= '0' && t <= '9'))
                return parseNumber();
            else
                return parseIdentifier();
        }
        
        Object parseStruct() {
            Object result{Object::Kind::Struct};
            next(); // skip the opening brace
            skipWhitespace();
            while (! eof() && top() != '}') {
                if (top() != '"') {
                    LOG(LL_ERROR, "Expected string key");
                    return Object{};
                }
                String key = parseString().asString();
                skipWhitespace();
                if (top() != ':') {
                    LOG(LL_ERROR, "Expected colon");
                    return Object{};
                }
                next(); // skip the colon
                skipWhitespace();
                result.add(key, parseObject());
                skipWhitespace();
                if (top() == ',') {
                    next(); // skip the comma
                    skipWhitespace();
                } else if (top() != '}') {
                    LOG(LL_ERROR, "Expected closing brace or comma");
                    return Object{};
                }
            }
            if (top() == '}')
                next(); // skip the closing brace
            return result;
        }

        Object parseArray() {
            Object result{Object::Kind::Array};
            next(); // skip the opening bracket
            skipWhitespace();
            while (! eof() && top() != ']') {
                result.add(parseObject());
                skipWhitespace();
                if (top() == ',') {
                    next(); // skip the comma
                    skipWhitespace();
                } else if (top() != ']') {
                    LOG(LL_ERROR, "Expected closing bracket or comma");
                    return Object{};
                }
            }
            if (top() == ']')
                next(); // skip the closing bracket
            return result;
        }
        Object parseString() {
            String result;
            next(); // skip the opening quote
            while (! eof() && top() != '"') {
                if (top() == '\\') {
                    next(); // skip the escape character
                    if (eof()) {
                        LOG(LL_ERROR, "Unterminated string escape sequence");
                        return Object{};
                    }
                    char escaped = top();
                    switch (escaped) {
                        case 'n': result += '\n'; break;
                        case 'r': result += '\r'; break;
                        case 't': result += '\t'; break;
                        case '"': result += '"'; break;
                        case '\\': result += '\\'; break;
                        default: 
                            LOG(LL_ERROR, "Unknown escape sequence: \\" << escaped);
                            result += escaped; // permissively just add the escaped character
                    }
                } else {
                    result += top();
                }
                next();
            }
            if (top() != '"') {
                LOG(LL_ERROR, "Unterminated string");
                return Object{};
            }
            next(); // skip the closing quote
            return Object{result};
        }

        Object parseNumber() {
            String number;
            bool isDouble = false;
            while (! eof() && (top() == '-' || (top() >= '0' && top() <= '9') || top() == '.' || top() == 'e' || top() == 'E')) {
                if (top() == '.') {
                    if (isDouble) {
                        LOG(LL_ERROR, "Multiple decimal points");
                        return Object{};
                    }
                    isDouble = true;
                }
                number += top();
                next();
            }
            if (isDouble) {
                return Object{atof(number.c_str())};
            } else {
                return Object{atoi(number.c_str())};
            }
        }

        // parses identifier, which can be boolean (true/false), null, or undefined
        Object parseIdentifier() {
            String identifier;
            while (! eof() && (isalpha(top()) || top() == '_')) {
                identifier += top();
                next();
            }
            if (identifier == "true") {
                return Object{true};
            } else if (identifier == "false") {
                return Object{false};
            } else if (identifier == "null" || identifier == "undefined") {
                return Object{};
            } else {
                LOG(LL_ERROR, "Unknown identifier: " << identifier);
                return Object{};
            }
        }

        bool eof() { return stream_.eof() && t_ == 0; }

        char top() { return t_; }

        char pop() {
            char result = t_;
            next();
            return result;
        }

        void skipWhitespace() {
            while (! eof() && (t_ == ' ' || t_ == '\n' || t_ == '\r' || t_ == '\t'))
                next();
        }

        void next() {
            if (stream_.read(reinterpret_cast<uint8_t*>(&t_), 1) != 1)
                t_ = 0;
        }

        Parser(ReadStream & stream) : stream_{stream} {}

        ReadStream & stream_;
        char t_ = 0;
    }; // rckid::json::Parser

    /** Parses JSON value from the given stream and returns the resulting JSON object. 
     */
    Object parse(ReadStream & stream) {
        return Parser::parse(stream);
    }

    /** Parses JSON object from given buffer. Internally wraps the buffer in a read stream and calls the streamed parser.
     */
    Object parse(char const * json) {
        auto s = MemoryReadStream(reinterpret_cast<uint8_t const *>(json), strlen(json));
        return parse(s);
    }

} // namespace rckid::json