#pragma once

#include <unordered_map>

#include <rckid/string.h>
#include <rckid/stream.h>
#include <rckid/serialization.h>

/** INI format serialization and deserialization.
 
    INI-like file format is used extensively in the SDK to store user defined settings and configurations thanks to the simplicity of the format and its human readability. INI file is composed of sections, each section contains key-value pairs. On top of the standard INI specification, the SDK also supports section arrays, where multiple sections can have the same name and will be parsed as an array (such as contacts).

    ini::writer
        << Section("first section")
        << Field("key1", "value1")
        << Field("key2", 42)
        << Section("something else")

    ini::reader
        >> Section("first section")
        >> Field("key1", strValue)
        >> Field("key2", intValue)
        >> SectionArray("contacts", [&](ini::Reader & r) {
            Contact c;
            r >> Field("name", c.name) >> Field("email", c.email);
            contacts.push_back(c);
        });


 */
namespace rckid::ini {

    class Writer;
    class Reader;

    class OpenSection {
    private:
        friend class Writer;
        friend class Reader;
        friend OpenSection Section(String name);

        explicit OpenSection(String name): name_{std::move(name)} {}

        String name_;
    };

    template<typename T>
    class ReadOnlyField {
    private:
        friend class Writer;
        friend class Reader;
        template<typename U>
        friend ReadOnlyField<U> Field(String name, U const & value);

        ReadOnlyField(String name, T const & value): name_{std::move(name)}, value_{value} {}

        String name_;
        T const & value_;
    };

    template<typename T>
    class WritableField {
    private:
        friend class Writer;
        friend class Reader;
        template<typename U>
        friend WritableField<U> Field(String name, U & value);

        WritableField(String name, T & value): name_{std::move(name)}, value_{value} {}    

        String name_;
        T & value_;
    }; 

    class SectionArrayReader {
    private:
        friend class Reader;
        friend SectionArrayReader SectionArray(String name, std::function<void(Reader &)> callback);
        
        explicit SectionArrayReader(String name, std::function<void(Reader &)> callback): name_{std::move(name)}, callback_{callback} {}

        String name_;
        std::function<void(Reader &)> callback_;

    }; 

    inline OpenSection Section(String name) {
        return OpenSection{std::move(name)};
    }

    template<typename T>
    ReadOnlyField<T> Field(String name, T const & value) {
        return ReadOnlyField<T>{std::move(name), value};
    }

    template<typename T>
    WritableField<T> Field(String name, T & value) {
        return WritableField<T>{std::move(name), value};
    }

    inline SectionArrayReader SectionArray(String name, std::function<void(Reader &)> callback) {
        return SectionArrayReader{std::move(name), callback};
    }


    /** INI Writer
     
     */
    class Writer {
    public:

        explicit Writer(WriteStream & stream): stream_{stream} {}

        Writer & operator << (OpenSection && section) {
            if (openedSection_)
                stream_.writer() << "\n";
            openedSection_ = true;
            stream_.writer() << "[" << section.name_ << "]\n";
            return *this;
        }

        template<typename T>
        Writer & operator << (ReadOnlyField<T> && field) {
            storeField(field.name_, field.value_);
            return *this;
        }

        template<typename T>
        Writer & operator << (WritableField<T> && field) {
            storeField(field.name_, field.value_);
            return *this;
        }

    private:

        template<typename T>
        void storeField(String const & name, T const & value) {
            rckid::Writer wr = stream_.writer();
            wr << name << "=";
            rckid::Writer escapingWriter{[&wr](char c) {
                if (c == '\n') {
                    wr << "\\n";
                } else if (c == '\r') {
                    wr << "\\r";
                } else if (c == '\\') {
                    wr << "\\\\";
                } else {
                    wr << c;
                }
            }};
            escapingWriter << value; // writes through to the underlying stream
            wr << "\n";
        }

        WriteStream & stream_;
        bool openedSection_ = false;
    }; 

    /** INI Reader
     
     */
    class Reader {
    public:

        Reader(ReadStream & stream) {
            parseStream(stream);
        }

        Reader(rckid::Reader & reader) {
            ReaderStream rs{reader};
            parseStream(rs);
        }

        Reader(rckid::Reader && reader) {
            ReaderStream rs{reader};
            parseStream(rs);
        }
    
        Reader & operator >> (OpenSection && section) {
            auto i = sections_.find(section.name_);
            if (i == sections_.end()) {
                LOG(LL_ERROR, "INI section not found: " << section.name_);
                current_ = nullptr;
            }
            current_ = i->second.get();
            ASSERT(current_->next == nullptr); // use section arrays instead
            return *this;
        }

        template<typename T>
        Reader & operator >> (WritableField<T> && field) {
            if (current_ == nullptr)
                return *this;
            auto i = current_->fields.find(field.name_);
            if (i == current_->fields.end())
                LOG(LL_ERROR, "INI field not found: " << field.name_);
            // convert field's string value to the requested type now
            i->second.reader() >> field.value_;
            return *this;
        }

        Reader & operator >> (SectionArrayReader && sectionArray) {
            auto i = sections_.find(sectionArray.name_);
            current_ = i == sections_.end() ? nullptr : i->second.get();
            while (current_ != nullptr) {
                ParsedSection * check = current_;
                sectionArray.callback_( *this );
                ASSERT(check == current_); // check that callback did not change section
                current_ = current_->next.get();
            }
            return *this;
        }

    private:
        class ParsedSection {
        public:
            std::unordered_map<String, String> fields;
            // pointer to next section, if part of an array
            unique_ptr<ParsedSection> next;
        }; 

        void parseStream(ReadStream & stream) {
            while (!stream.eof()) {
                String line = stream.readLine();
                if (line.empty())
                    continue;
                if (line.startsWith("[")) {
                    rckid::Reader rd{line.reader(1)};
                    String sectionName = ReaderStream{rd}.readString(']');
                    current_ = new ParsedSection();
                    auto i = sections_.find(sectionName);
                    if (i == sections_.end()) {
                        sections_[sectionName] = unique_ptr<ParsedSection>(current_);
                    } else {
                        // prepend the newly created section to the existing array
                        current_->next = std::move(i->second);
                        i->second.reset(current_);
                    }
                } else {
                    if (current_ == nullptr) {
                        LOG(LL_ERROR, "INI field outside of section: " << line);
                        continue;
                    }
                    // since keys have to be alphanumeric anyways, we can already pre-escape them
                    rckid::Reader rd{[&, pos = uint32_t{0}](bool advance) mutable -> int32_t {
                        if (pos >= line.size())
                            return rckid::Reader::EOFMarker;
                        uint32_t advanceBy = 1;
                        char c = line[pos];
                        if (c == '\\' && pos + 1 < line.size()) {
                            c = line[pos + 1];
                            ++advanceBy;
                        } 
                        if (advance)
                            pos += advanceBy;
                        return static_cast<int32_t>(c);
                    }};
                    ReaderStream rs{rd};
                    String fieldName = rs.readString('=');
                    String fieldValue = rs.readString();
                    auto i = current_->fields.find(fieldName);
                    if (i != current_->fields.end())
                        LOG(LL_ERROR, "Duplicate INI field: " << fieldName);
                    current_->fields[fieldName] = fieldValue;
                }
            }
        }

        std::unordered_map<String, unique_ptr<ParsedSection>> sections_;

        ParsedSection * current_ = nullptr;
    };

    template<>
    Reader & Reader::operator >> (WritableField<String> && field) {
        if (current_ == nullptr)
            return *this;
        auto i = current_->fields.find(field.name_);
        if (i == current_->fields.end())
            LOG(LL_ERROR, "INI field not found: " << field.name_);
        field.value_ = i->second;
        return *this;
    }

} // namespace rckid