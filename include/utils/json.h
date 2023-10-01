#pragma once 

#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>

#include "tests.h"

namespace json {

    class Value {
    public:

        class ArrayElements;

        /** Simple iterator return the array elements. 
         */
        class ArrayIterator {
        public: 

            ArrayIterator & operator ++ (){
                ++i_;
                return *this;
            }

            ArrayIterator operator ++ (int) { 
                ArrayIterator result{i_};
                ++i_;
                return result;
            }

            Value & operator*() { return **i_; }

            bool operator == (ArrayIterator const & other) const { return i_ == other.i_; }
            bool operator != (ArrayIterator const & other) const { return i_ != other.i_; }

        private:

            friend class Value::ArrayElements;

            ArrayIterator(std::vector<Value*>::iterator i):i_{i} {}

            std::vector<Value*>::iterator i_;
        }; // Value::ArrayIterator

        class ArrayElements {
        public:
            ArrayIterator begin() { return ArrayIterator{ v_.array_.begin()}; }
            ArrayIterator end() { return ArrayIterator{ v_.array_.end()}; }
        private:

            friend class Value;

            ArrayElements(Value & v):v_{v} {}

            Value & v_;
        }; // Value::ArrayElements

        enum class Kind { Null, Undefined, Bool, Int, Double, String, Array, Struct }; 

        Value(): kind_{Kind::Undefined}, bool_{false} {}

        Value(bool value): kind_{Kind::Bool}, bool_{value} {}
        Value(int value): kind_{Kind::Int}, int_{value} {}
        Value(double value): kind_{Kind::Double}, double_{value} {}
        Value(std::string const & value): kind_{Kind::String}, str_{value} {}
        Value(char const * value): kind_{Kind::String}, str_{value} {}

        Value(Value const & from): kind_{from.kind_}, bool_{false}, comment_{from.comment_} { attach(from); }
        
        Value(Value && from): kind_{from.kind_}, bool_{false}, comment_{std::move(from.comment_)} { attach(std::move(from)); }

        ~Value() { detach(); }

        static Value const & null() {
            static Value singleton{Kind::Null};
            return singleton;
        }

        static Value const & undefined() {
            static Value singleton{Kind::Undefined};
            return singleton;
        }

        static Value newArray() { return Value{Kind::Array}; }
        static Value newStruct() { return Value{Kind::Struct}; }

        Value & operator = (bool from) {
            detach();
            kind_ = Kind::Bool;
            bool_ = from;
            return *this;
        }

        Value & operator = (int from) {
            detach();
            kind_ = Kind::Int;
            int_ = from;
            return *this;
        }

        Value & operator = (double from) {
            detach();
            kind_ = Kind::Double;
            double_ = from;
            return *this;
        }

        Value & operator = (std::string const & from) {
            detach();
            kind_ = Kind::String;
            new (&str_) std::string{from};
            return *this;
        }

        Value & operator = (std::string const && from) {
            detach();
            kind_ = Kind::String;
            new (&str_) std::string{std::move(from)};
            return *this;
        }

        Value & operator = (Value const & from) {
            detach();
            kind_ = from.kind_;
            comment_ = from.comment_;
            attach(from);
            return *this;
        }

        Value & operator = (Value const && from) {
            detach();
            kind_ = from.kind_;
            comment_ = std::move(from.comment_);
            attach(std::move(from));
            return *this;
        }

        Kind kind() const { return kind_; }

        std::string const & comment() const { return comment_; }
        void setComment(std::string const & comment) { comment_ = comment; }

        size_t size() const {
            switch (kind_) {
                case Kind::String:
                    return str_.size();
                case Kind::Array:
                    return array_.size();
                case Kind::Struct:
                    return struct_.size();
                    break;
                default:
                    return 1;
            }
        }

        Value const & operator [] (size_t i) const {
            if (kind_ != Kind::Array || i >= array_.size())
                return null();
            return *(array_[i]);
        }

        Value & operator [] (size_t i) {
            if (kind_ != Kind::Array)
                throw std::runtime_error(STR("JSON value is not an array but " << kind_));
            if (i >= array_.size())
                throw std::range_error{STR("JSON array index " << i << " out of bounds " << array_.size())};
            return *(array_[i]);
        }

        Value const & operator [] (std::string const & name) const {
            if (kind_ != Kind::Struct)
                return null();
            auto i = struct_.find(name);
            return i != struct_.end() ? *(i->second) : null();
        }

        ArrayElements arrayElements() {
            if (kind_ != Kind::Array)
                throw std::runtime_error(STR("JSON value is not an array but " << kind_));
            return ArrayElements{*this};
        }

        Value & operator [] (std::string const & name) {
            if (kind_ != Kind::Struct)
                throw std::runtime_error{STR("JSON value is not a struct but " << kind_)};
            auto i = struct_.find(name);
            if (i == struct_.end())
                i = struct_.insert(std::make_pair(name, new Value{})).first;
            return *(i->second);
        }

        Value & push(Value const & element) {
            if (kind_ != Kind::Array)
                throw std::runtime_error(STR("JSON value is not an array but " << kind_));
            array_.push_back(new Value{element});
            return *this;
        }

        Value & push(Value && element) {
            if (kind_ != Kind::Array)
                throw std::runtime_error(STR("JSON value is not an array but " << kind_));
            array_.push_back(new Value{std::move(element)});
            return *this;
        }

        Value & insert(std::string const & name, Value const & value) {
            if (kind_ != Kind::Struct)
                throw std::runtime_error{STR("JSON value is not a struct but " << kind_)};
            struct_.insert(std::make_pair(name, new Value{value}));
            return *this;
        }

        Value & insert(std::string const & name, Value const && value) {
            if (kind_ != Kind::Struct)
                throw std::runtime_error{STR("JSON value is not a struct but " << kind_)};
            struct_.insert(std::make_pair(name, new Value{std::move(value)}));
            return *this;
        }

        /** Returns true of te value is struct and contains the given field (even if it is null or undefined), false otherwise. 
         */
        bool containsKey(std::string const & name) const {
            if (kind_ == Kind::Struct)
                return struct_.find(name) != struct_.end();
            return false;
        }

        template<typename T>
        T const & value() const;

        template<typename T>
        T & value();

        bool isNull() const { return kind_ == Kind::Null; }

        bool isUndefined() const { return kind_ == Kind::Undefined; }

        bool isString() const { return kind_ == Kind::String; }

        bool isArray() const { return kind_ == Kind::Array; }

        bool isStruct() const { return kind_ == Kind::Struct; }

        bool operator == (Value const & other) const {
            if (kind_ != other.kind_)
                return false;
            switch (kind_) {
                case Kind::Null:
                case Kind::Undefined:
                    return true;
                case Kind::Bool:
                    return bool_ == other.bool_;
                case Kind::Int:
                    return int_ == other.int_;
                case Kind::Double:
                    return double_ == other.double_;
                case Kind::String:
                    return str_ == other.str_;
                default:
                    UNIMPLEMENTED;
            }
        }

        std::string stringify() const {
            std::stringstream s{};
            stringify(s);
            return s.str();
        }

        void stringify(std::ostream & s) const {
            stringify(s, 0, true, true);
        }

        std::string stringifyPermissive() const {
            std::stringstream s{};
            stringifyPermissive(s);
            return s.str();
        }

        void stringifyPermissive(std::ostream &s) const {
            stringify(s, 0, false, true);
        }

    private:

        Value(Kind kind):
            kind_{kind}, bool_{false} {
            switch (kind_) {
                case Kind::Int:
                    int_ = 0;
                    break;
                case Kind::Double:
                    double_ = 0;
                    break;
                case Kind::String:
                    new (&str_) std::string{};
                    break;
                case Kind::Array:
                    new (&array_) std::vector<Value*>{};
                    break;
                case Kind::Struct:
                    new (&struct_) std::unordered_map<std::string, Value*>{};
                    break;
                default:
                    break; 
            }    
        }

        void detach() {
            switch (kind_) {
                case Kind::String:
                    str_.~basic_string();
                    break;
                case Kind::Array:
                    array_.~vector();
                    break;
                case Kind::Struct:
                    struct_.~unordered_map();
                    break;
                default:
                    break; // trivial destructors
            }
        }

        void attach(Value const & from) {
            switch (kind_) {
                case Kind::Int:
                    int_ = from.int_;
                    break;
                case Kind::Double:
                    double_ = from.double_;
                    break;
                case Kind::String:
                    new (&str_) std::string{from.str_};
                    break;
                case Kind::Array:
                    new (&array_) std::vector<Value*>{};
                    for (Value const * v : from.array_)
                        array_.push_back(new Value{*v});
                    break;
                case Kind::Struct:
                    new (&struct_) std::unordered_map<std::string, Value*>{};
                    for (auto & i : from.struct_)
                        struct_.insert(std::make_pair(i.first, new Value{*i.second}));
                    break;
                default:
                    break; 
            }
        }

        void attach(Value && from) {
            switch (kind_) {
                case Kind::Int:
                    int_ = from.int_;
                    break;
                case Kind::Double:
                    double_ = from.double_;
                    break;
                case Kind::String:
                    new (&str_) std::string{std::move(from.str_)};
                    break;
                case Kind::Array:
                    new (&array_) std::vector<Value*>{std::move(from.array_)};
                    break;
                case Kind::Struct:
                    new (&struct_) std::unordered_map<std::string, Value*>{std::move(from.struct_)};
                default:
                    break; 
            }
        }

        Kind kind_;

        std::string comment_;

        union {
            bool bool_;
            int int_;
            double double_;
            std::string str_;
            std::vector<Value*> array_;
            std::unordered_map<std::string, Value *> struct_;            
        };

        friend std::ostream & operator << (std::ostream & s, Kind kind) {
            switch (kind) {
                case Kind::Null:
                    s << "null";
                    break;
                case Kind::Undefined:
                    s << "undefined";
                    break;
                case Kind::Bool:
                    s << "bool";
                    break;
                case Kind::Int:
                    s << "int";
                    break;
                case Kind::Double:
                    s << "double";
                    break;
                case Kind::String:
                    s << "string";
                    break;
                case Kind::Array:
                    s << "array";
                    break;
                case Kind::Struct:
                    s << "struct";
                    break;
            }
            return s;
        }

        friend std::ostream & operator << (std::ostream & s, Value const & v) {
            switch (v.kind_) {
                case Kind::Null:
                    s << "null";
                    break;
                case Kind::Undefined:
                    s << "undefined";
                    break;
                case Kind::Bool:
                    s << v.bool_;
                    break;
                case Kind::Int:
                    s << v.int_;
                    break;
                case Kind::Double:
                    s << v.double_;
                    break;
                case Kind::String:
                    s << "\"" << v.str_ << "\"";
                    break;
                case Kind::Array:
                    s << "[";
                    // TODO
                    s << "]";
                    break;
                case Kind::Struct:
                    s << "{";
                    // TODO
                    s << "}";
                    break;
            }
            return s;
        }

        void stringifyNewline(std::ostream &s, size_t indent) const {
            while (indent-- != 0)
                s << '\t';
        }

        void stringifyComment(std::ostream & s, size_t indent) const {
            if (!comment_.empty()) {
                s << "/*" << comment_ << std::endl;
                stringifyNewline(s, indent);
                s <<" */" << std::endl;
                stringifyNewline(s, indent);
            }
        }

        void stringify(std::ostream &s, size_t indent, bool strict, bool showComment) const {
            if (!strict && showComment)
                stringifyComment(s, indent);
            switch (kind_) {
                case Kind::Null:
                    s << "null";
                    break;
                case Kind::Undefined:
                    s << (strict ? "null" : "undefined");
                    break;
                case Kind::Bool:
                    s << (bool_ ? "true" : "false");
                    break;
                case Kind::Int:
                    s << int_;
                    break;
                case Kind::Double:
                    s << double_;
                    break;
                case Kind::String:
                    s << '"' << str::escape(str_) << '"';
                    break;
                case Kind::Array:
                    if (array_.empty()) {
                        s << "[]";
                    } else {
                        s << "[" << std::endl;
                        for (size_t i = 0, e = array_.size(); i != e; ++i) {
                            stringifyNewline(s, indent + 1);
                            array_[i]->stringify(s, indent + 1, strict, true);
                            if (!strict || i + 1 < e)
                               s << ",";
                            s << std::endl;
                        }
                        stringifyNewline(s, indent);
                        s << "]";
                    }
                    break;
                case Kind::Struct:
                    if (struct_.empty()) {
                        s << "{}";
                    } else {
                        s << "{" << std::endl;
                        auto iter = struct_.begin();
                        for (size_t i = 0, e = struct_.size(); i != e; ++i, ++iter) {
                            stringifyNewline(s, indent + 1);
                            if (!strict && showComment)
                                iter->second->stringifyComment(s, indent + 1);
                            s << '"' << iter->first << "\" : ";
                            iter->second->stringify(s, indent + 1, strict, false);
                            if (!strict || i + 1 < e)
                               s << ",";
                            s << std::endl;
                        }
                        stringifyNewline(s, indent);
                        s << "}";
                    }
            }
        }

    }; // json::Value

    template<> 
    inline bool const & Value::value() const {
        if (kind_ != Kind::Bool)
            throw std::runtime_error{STR("JSON value " << kind_ << " is not bool")};
        return bool_;
    }

    template<> 
    inline bool & Value::value() {
        if (kind_ != Kind::Bool)
            throw std::runtime_error{STR("JSON value " << kind_ << " is not bool")};
        return bool_;
    }

    template<> 
    inline int const & Value::value() const {
        if (kind_ != Kind::Int)
            throw std::runtime_error{STR("JSON value " << kind_ << " is not int")};
        return int_;
    }

    template<> 
    inline int & Value::value() {
        if (kind_ != Kind::Int)
            throw std::runtime_error{STR("JSON value " << kind_ << " is not int")};
        return int_;
    }

    template<> 
    inline double const & Value::value() const {
        if (kind_ != Kind::Double)
            throw std::runtime_error{STR("JSON value " << kind_ << " is not double")};
        return double_;
    }

    template<> 
    inline double & Value::value() {
        if (kind_ != Kind::Double)
            throw std::runtime_error{STR("JSON value " << kind_ << " is not double")};
        return double_;
    }
    template<> 
    inline std::string const & Value::value() const {
        if (kind_ != Kind::String)
            throw std::runtime_error{STR("JSON value " << kind_ << " is not string")};
        return str_;
    }

    template<> 
    inline std::string & Value::value() {
        if (kind_ != Kind::String)
            throw std::runtime_error{STR("JSON value " << kind_ << " is not string")};
        return str_;
    }

    /** A permissive JSON parser. 
     
        A simple parser that supports the JSON format as well as some additional niceties such as comments, or keyword fields. The following grammar is used:

        JSON := ELEMENT
        ELEMENT := [ comment ] ELEMENT_NO_COMMENT 
        ELEMENT_NO_COMMENT := (null | undefined | POD | string | ARRAY | STRUCT )
        POD := bool | int | double
        ARRAY := '[' [ ELEMENT { ',' ELEMENT } [','] ] ']'
        STRUCT := '{' [ FIELD { ',' FIELD } [',' ] ] '}'
        FIELD := [ comment ] (ident | string ) '=' ELEMENT_NO_COMMENT

    */
    class Parser {
    private:

        friend Value parse(std::istream &);

        struct Token {
            enum class Kind {
                Null,
                Undefined, 
                Bool,
                Int,
                Double,
                Comment, 
                String, 
                Identifier,
                Colon,
                Comma,
                SquareOpen,
                SquareClose,
                CurlyOpen,
                CurlyClose,
                EoF
            }; // json::Parser::Token::Kind

            Token(Kind kind): kind{kind}, valueBool{false} {
                ASSERT(kind != Kind::Comment && kind != Kind::String && kind != Kind::Identifier);
            }

            Token(Kind kind, std::string && value): kind{kind}, valueStr{std::move(value)} {
                ASSERT(kind == Kind::Comment || kind == Kind::String || kind == Kind::Identifier);
            }

            Token(bool value): kind{Kind::Bool}, valueBool{value } {}
            Token(int value): kind{Kind::Int}, valueInt{value } {}
            Token(double value): kind{Kind::Double}, valueDouble{value} {}

            Token(Token && other):
                kind{other.kind} {
                switch (kind) {
                    case Kind::Int:
                        valueInt = other.valueInt;
                        break;
                    case Kind::Double:
                        valueDouble = other.valueDouble;
                        break;
                    case Kind::Comment:
                    case Kind::String:
                    case Kind::Identifier:
                        new (&valueStr) std::string{std::move(other.valueStr)};
                        break;
                    default:
                        valueBool = other.valueBool;
                }
            }

            ~Token() {
                detach();
            }

            Token & operator = (Token && other) {
                detach();
                kind = other.kind;
                switch (kind) {
                    case Kind::Int:
                        valueInt = other.valueInt;
                        break;
                    case Kind::Double:
                        valueDouble = other.valueDouble;
                        break;
                    case Kind::Comment:
                    case Kind::String:
                    case Kind::Identifier:
                        new (&valueStr) std::string{std::move(other.valueStr)};
                        break;
                    default:
                        valueBool = other.valueBool;
                }
                return *this;
            }

            Kind kind;

            union {
                bool valueBool;
                int valueInt;
                double valueDouble;
                std::string valueStr;
            };

        private:

            void detach() {
                using namespace std;
                switch (kind) {
                    case Kind::Identifier:
                    case Kind::String:
                    case Kind::Comment:
                        valueStr.~string();
                        break;
                }
            }

            friend std::ostream & operator << (std::ostream & s, Kind k) {
                switch (k) {
                    case Kind::Null:
                        s << "null";
                        break;
                    case Kind::Undefined:
                        s << "undefined";
                        break;
                    case Kind::Bool:
                        s << "bool";
                        break;
                    case Kind::Int:
                        s << "int";
                        break;
                    case Kind::Double:
                        s << "double";
                        break;
                    case Kind::Comment: 
                        s << "comment";
                        break;
                    case Kind::String: 
                        s << "string";
                        break;
                    case Kind::Identifier:
                        s << "identifier";
                        break;
                    case Kind::Colon:
                        s << ":";
                        break;
                    case Kind::Comma:
                        s << ",";
                        break;
                    case Kind::SquareOpen:
                        s << "[";
                        break;
                    case Kind::SquareClose:
                        s << "]";
                        break;
                    case Kind::CurlyOpen:
                        s << "{";
                        break;
                    case Kind::CurlyClose:
                        s << "}";
                        break;
                    case Kind::EoF:
                        s << "end of file";
                        break;
                }
                return s;
            }

        }; // json::Parser::Token

        Parser(std::istream & s): s_{s} {}

        Value ELEMENT(Token t) {
            if (t.kind == Token::Kind::Comment) {
                Value result = ELEMENT_NO_COMMENT(getNextToken());
                result.setComment(std::move(t.valueStr));
                return result;
            } else {
                return ELEMENT_NO_COMMENT(std::move(t));
            }
        }

        Value ELEMENT_NO_COMMENT(Token t) {
            switch (t.kind) {
                case Token::Kind::Null:
                    return Value::null();
                case Token::Kind::Undefined:
                    return Value::undefined();
                case Token::Kind::Bool:
                    return Value{t.valueBool};
                case Token::Kind::Int:
                    return Value{t.valueInt};
                case Token::Kind::Double:
                    return Value{t.valueDouble};
                case Token::Kind::String:
                case Token::Kind::Identifier:
                    return Value{std::move(t.valueStr)};
                case Token::Kind::SquareOpen:
                    return ARRAY();
                case Token::Kind::CurlyOpen:
                    return STRUCT();
                default:
                    throwError(STR("Expected element start, but " << t.kind << " found"));
            }
        }

        Value ARRAY() {
            Value result = Value::newArray();
            Token t = getNextToken();
            while (t.kind != Token::Kind::SquareClose) {
                result.push(ELEMENT(std::move(t)));
                t = getNextToken();
                if (t.kind == Token::Kind::Comma)
                    t = getNextToken(); 
            }
            if (t.kind != Token::Kind::SquareClose)
                throwError(STR("Expected `]`, but " << t.kind << " found"));
            return result;
        }

        Value STRUCT() {
            Value result = Value::newStruct();
            Token t = getNextToken();
            std::string comment;
            bool hasComment;
            while (t.kind != Token::Kind::CurlyClose) {
                if (t.kind == Token::Kind::Comment) {
                    hasComment = true;
                    comment = std::move(t.valueStr);
                    t = getNextToken();
                } else {
                    hasComment = false;
                }
                std::string field{FIELD(std::move(t))};
                t = getNextToken();
                if (t.kind != Token::Kind::Colon)
                    throwError(STR("Expected field assignment ':', but " << t.kind << " found"));
                Value value{ELEMENT_NO_COMMENT(getNextToken())};
                if (hasComment)
                    value.setComment(comment);
                result.insert(field, value);
                t = getNextToken();
                if (t.kind == Token::Kind::Comma)
                    t = getNextToken(); 
            }
            if (t.kind != Token::Kind::CurlyClose)
                throwError(STR("Expected '}' but " << t.kind << " found"));
            return result;
        }

        std::string FIELD(Token t) {
            if (t.kind != Token::Kind::Identifier && t.kind != Token::Kind::String)
                throwError(STR("Expected identifier or string but " << t.kind << " found"));
            return t.valueStr;
        }

        Token getNextToken() {
            char c = nextChar();
            while (isWhitespace(c))
                c = nextChar();
            tokenLine_ = line_;
            tokenCol_ = col_ - 1;
            if (eof())
                return Token{Token::Kind::EoF};
            switch (c) {
                case ':':
                    return Token{Token::Kind::Colon};
                case ',':
                    return Token{Token::Kind::Comma};
                case '[':
                    return Token{Token::Kind::SquareOpen};
                case ']':
                    return Token{Token::Kind::SquareClose};
                case '{':
                    return Token{Token::Kind::CurlyOpen};
                case '}':
                    return Token{Token::Kind::CurlyClose};
                case '/':
                    return nextComment(c);
                case '"':
                case '\'':
                    return nextString(c);
                default:
                    if (isDigit(c))
                        return nextNumber(c);
                    else if (isIdentifierStart(c))
                        return nextIdentifier(c);
                    throwError(STR("Invalid character (code " << static_cast<int>(c) << ") detected"));
            }
        }

        Token nextComment(char c) {
            std::string result{};
            switch (nextChar()) {
                case '/': // single line comment
                    while (! eof()) {
                        char c = nextChar();
                        if (c == '\n')
                            break;
                        result += c;
                    }
                    break;
                case '*': // multi-line comment 
                    while (true) {
                        if (eof())
                            throwError("Unterminated multi-line comment");
                        char c = nextChar();
                        if (c == '*') {
                            char c2 = nextChar();
                            if (c2 == '/')
                                break;
                            result += c;
                            result += c2;
                        } else {
                            result += c;
                        }
                    }
                    break;
                default:
                    throwError("Only // and /* */ comments are supported.");
            }
            str::trimRight(result);
            return Token{Token::Kind::Comment, std::move(result)};
        }

        Token nextString(char delimiter) {
            std::string result{};
            while (true) {
                if (eof())
                    throwError("Unterminated string literal");
                char c = nextChar();
                if (c == delimiter)
                    break;
                if (c == '\\') { 
                    char c = nextChar();
                    switch (c) {
                        case '"':
                        case '\'':
                        case '\\':
                            result += c;
                            break;
                        case 't':
                            result += '\t';
                            break;
                        case 'n':
                            result += '\n';
                            break;
                        case 'r':
                            result += '\r';
                            break;
                        case '\n':
                            break;
                        default:
                            throwError("Expected valid string escape sequence");
                    }
                } else {
                    result += c;
                }
            }
            return Token{Token::Kind::String, std::move(result)};
        }

        Token nextNumber(char c) {
            int result = (c - '0');
            while (isDigit(peekChar()))
                result = result * 10 + (nextChar() - '0');
            if (peekChar() == '.') {
                nextChar();
                UNIMPLEMENTED;
            } else {
                return Token{result};
            }
        }

        Token nextIdentifier(char c) {
            std::string result{c};
            while (! eof() && isIdentifier(peekChar()))
                result += nextChar();
            if (result == "null")
                return Token{Token::Kind::Null};
            else if (result == "undefined")
                return Token{Token::Kind::Undefined};
            else if (result == "true")
                return Token{true};
            else if (result == "false")
                return Token{false};
            else
                return Token{Token::Kind::Identifier, std::move(result)};
        }

        char nextChar() {
            char result = s_.get();
            if (result == '\r') {
                ++line_;
                col_ = 1;
            } else {
                ++col_;
            }
            return result;
        }

        char peekChar() { return s_.peek(); }

        bool eof() { return s_.eof(); }

        bool isDigit(char c) { return c >= '0' && c <= '9'; }
        bool isIdentifierStart(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
        bool isIdentifier(char c) { return isIdentifierStart(c) || isDigit(c); } 
        bool isWhitespace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

        [[noreturn]]
        void throwError(std::string && what) {
            throw std::runtime_error{STR(what << " at (" << tokenLine_ << ":" << tokenCol_ << ")")};
        }

        size_t tokenLine_ = 1;
        size_t tokenCol_ = 1;
        size_t line_ = 1;
        size_t col_ = 1;

        std::istream & s_;


    }; // json::Parser

    inline Value parse(std::istream & from) {
        Parser p{from};
        Value result{p.ELEMENT(p.getNextToken())};
        if (p.getNextToken().kind != Parser::Token::Kind::EoF)
            p.throwError("Extra input");
        return result;
    }

    inline Value parse(std::string const & from) {
        std::stringstream s{from};
        return parse(s);
    }

    inline Value parseFile(std::string const & filename) {
        std::ifstream f{filename};
        if (! f.good())
           throw std::runtime_error(STR("Unable to open file " << filename));
        return parse(f);
    }

} // namespace json

#ifdef TESTS

TEST(json, defaultConstructor) {
    json::Value v;
    EXPECT_EQ(v.kind(), json::Value::Kind::Undefined);
}

TEST(json, pods) {
    json::Value v1{false};
    EXPECT_EQ(v1.kind(), json::Value::Kind::Bool);
    EXPECT_EQ(v1.value<bool>(), false);
    json::Value v2{323};
    EXPECT_EQ(v2.kind(), json::Value::Kind::Int);
    EXPECT_EQ(v2.value<int>(), 323);
    json::Value v3{323.678};
    EXPECT_EQ(v3.kind(), json::Value::Kind::Double);
    EXPECT_EQ(v3.value<double>(), 323.678);
}

TEST(json, string) {
    json::Value v1{"foobar"};
    EXPECT_EQ(v1.kind(), json::Value::Kind::String);
    EXPECT_EQ(v1.value<std::string>(), "foobar");
}

TEST(json, array) {
    json::Value v = json::Value::newArray();
    EXPECT_EQ(v.kind(),  json::Value::Kind::Array);
    EXPECT_EQ(v.size(), 0);
    v.push(1);
    EXPECT_EQ(v.size(), 1);
    EXPECT_EQ(v[0].kind(), json::Value::Kind::Int);
    EXPECT_EQ(v[0].value<int>(), 1);
}

TEST(json, struct) {
    json::Value v = json::Value::newStruct();
    EXPECT_EQ(v.kind(),  json::Value::Kind::Struct);
    EXPECT_EQ(v.size(), 0);
    v["foo"] = 1;
    EXPECT_EQ(v.size(), 1);
    EXPECT_EQ(v["foo"].kind(), json::Value::Kind::Int);
    EXPECT_EQ(v["foo"].value<int>(), 1);
}

TEST(json, parseSingletons) {
    EXPECT_EQ(json::parse("null"), json::Value::null());
    EXPECT_EQ(json::parse("undefined"), json::Value::undefined());
}

TEST(json, parsePods) {
    EXPECT_EQ(json::parse("true"), json::Value{true});
    EXPECT_EQ(json::parse("false"), json::Value{false});
    EXPECT_EQ(json::parse("true"), true);
    EXPECT_EQ(json::parse("false"), false);
    EXPECT_EQ(json::parse("13"), 13);
    //EXPECT_EQ(json::parse("2.5"), 2.5);
}

TEST(json, parseString) {
    EXPECT_EQ(json::parse("\"foo\""), "foo");
    EXPECT_EQ(json::parse("'bar'"), "bar");
}

TEST(json, parseArray) {
    using namespace json;
    Value v{parse("[ 1, 2, 'str']")};
    EXPECT_EQ(v.kind(), Value::Kind::Array);
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v[0], 1);
    EXPECT_EQ(v[1], 2);
    EXPECT_EQ(v[2], "str");
    // empty
    Value v2{parse("[]")};
    EXPECT_EQ(v2.kind(), Value::Kind::Array);
    EXPECT_EQ(v2.size(), 0);
    // trailing comma
    Value v3{parse("[ 10, 22, ]")};
    EXPECT_EQ(v3.kind(), Value::Kind::Array);
    EXPECT_EQ(v3.size(), 2);
    EXPECT_EQ(v3[0], 10);
    EXPECT_EQ(v3[1], 22);
}

TEST(json, parseStruct) {
    using namespace json;
    {
        Value v{parse("{ \"foo\": 123, \"bar\" : 321}")};
        EXPECT_EQ(v.kind(), Value::Kind::Struct);
        EXPECT_EQ(v.size(), 2);
        EXPECT_EQ(v["foo"], 123);
        EXPECT_EQ(v["bar"], 321);
    }
    { // empty 
        Value v{parse("{ }")};
        EXPECT_EQ(v.kind(), Value::Kind::Struct);
        EXPECT_EQ(v.size(), 0);
    }
    { // trailing comma & idents
        Value v{parse("{ foo: 123, bar : 321, }")};
        EXPECT_EQ(v.kind(), Value::Kind::Struct);
        EXPECT_EQ(v.size(), 2);
        EXPECT_EQ(v["foo"], 123);
        EXPECT_EQ(v["bar"], 321);
    }
}

TEST(json, comments) {
    using namespace json;
    {
        Value v{parse("/*foo*/ 56")};
        EXPECT_EQ(v.comment(), "foo");
        EXPECT_EQ(v, 56);
    }
    { // array
        Value v{parse("//foo\n[ /*bar*/10, //baz\n22, ]")};
        EXPECT_EQ(v.kind(), Value::Kind::Array);
        EXPECT_EQ(v.size(), 2);
        EXPECT_EQ(v[0], 10);
        EXPECT_EQ(v[1], 22);
        EXPECT_EQ(v.comment(), "foo");
        EXPECT_EQ(v[0].comment(), "bar");
        EXPECT_EQ(v[1].comment(), "baz");
    }
    { // struct
        Value v{parse("//c1\n{ //c2\nfoo: 123, //c3\nbar : 321, }")};
        EXPECT_EQ(v.kind(), Value::Kind::Struct);
        EXPECT_EQ(v.size(), 2);
        EXPECT_EQ(v["foo"], 123);
        EXPECT_EQ(v["bar"], 321);
        EXPECT_EQ(v.comment(), "c1");
        EXPECT_EQ(v["foo"].comment(), "c2");
        EXPECT_EQ(v["bar"].comment(), "c3");
    }
}

TEST(json, stringifyStrict) {
    using namespace json;
    EXPECT_EQ(Value::null().stringify(), "null");
    EXPECT_EQ(Value::undefined().stringify(), "null");
    {
        Value v{true};
        EXPECT_EQ(v.stringify(), "true");
    }
    {
        Value v{false};
        EXPECT_EQ(v.stringify(), "false");
    }
    {
        Value v{1};
        EXPECT_EQ(v.stringify(), "1");
    }
    {
        Value v{1.45};
        EXPECT_EQ(v.stringify(), "1.45");
    }
    {
        Value v{"foo bar"};
        EXPECT_EQ(v.stringify(), "\"foo bar\"");
    }
    {
        Value v{parse("[ 1, 2, 3]")};
        EXPECT_EQ(v.stringify(), "[\n\t1,\n\t2,\n\t3\n]");
    }
    {
        Value v{parse("{ foo : 1 }")};
        EXPECT_EQ(v.stringify(), "{\n\t\"foo\" : 1\n}");
        // I'm lazy here, having more than one elemnt in struct plays funny with the ordering
    }
}

TEST(json, strinfifyPermissive) {
    using namespace json;
    EXPECT_EQ(Value::null().stringifyPermissive(), "null");
    EXPECT_EQ(Value::undefined().stringifyPermissive(), "undefined");
    {
        Value v{true};
        v.setComment("foobar is here");
        EXPECT_EQ(v.stringifyPermissive(), "/*foobar is here\n */\ntrue");
    }
    {
        Value v{parse("[ 1, 2, 3]")};
        EXPECT_EQ(v.stringifyPermissive(), "[\n\t1,\n\t2,\n\t3,\n]");
    }
    {
        Value v{parse("/*foo*/[ /*bar*/1, /* baz*/2, 3]")};
        EXPECT_EQ(v.stringifyPermissive(), "/*foo\n */\n[\n\t/*bar\n\t */\n\t1,\n\t/* baz\n\t */\n\t2,\n\t3,\n]");
    }
    {
        Value v{parse("{ foo : 1 }")};
        EXPECT_EQ(v.stringifyPermissive(), "{\n\t\"foo\" : 1,\n}");
        // I'm lazy here, having more than one elemnt in struct plays funny with the ordering
    }
    {
        Value v{parse("/* c1\n*/{ /* c2 */ foo : 1 }")};
        EXPECT_EQ(v.stringifyPermissive(), "/* c1\n */\n{\n\t/* c2\n\t */\n\t\"foo\" : 1,\n}");
    }
}

TEST(json, arrayElements) {
    using namespace json;
    Value v{parse("[ 1, 2, 3]")};
    int i = 1;
    for (auto & e : v.arrayElements()) {
        EXPECT_EQ(e.value<int>(), i);
        ++i;
    }
    EXPECT_EQ(i, 4);
}

#endif
