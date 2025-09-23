#pragma once

#include <memory.h>

#include "stream.h"

namespace rckid::ini {

    /** Parser for simplified INI  */
    class Reader {
    public:
        Reader(std::unique_ptr<ReadStream> from):
            from_{std::move(from)} {
        }

        bool eof() const { return from_->eof(); }

        String nextSection() {
            while (!eof()) {
                if (line_.empty())
                    line_ = from_->readLine();
                if (line_.startsWith("[") && line_.endsWith("]")) {
                    String result = line_.substr(1, line_.size() - 2);
                    line_ = "";
                    return result;
                }
            }
            return "";
        }

        std::pair<String, String> nextValue() {
            while (!eof()) {
                if (line_.empty())
                    line_ = from_->readLine();
                if (line_.empty())
                    continue;
                // if this is section header, stop and return empty pair
                if (line_.startsWith("["))
                    return std::make_pair("", "");
                uint32_t eq = line_.find('=');
                if (eq != String::NPOS) {
                    String name = line_.substr(0, eq);
                    String value = line_.substr(eq + 1);
                    line_ = "";
                    return std::make_pair(name, value);
                }
                line_ = "";
            }
            return std::make_pair("","");
        }

    private:
        String line_;
        std::unique_ptr<ReadStream> from_;

    }; // rckid::ini::Reader

    /** A simple writer for the ini files. Contains shorthand functions for writing ini sections and values.
     */
    class Writer {
    public:
        Writer(std::unique_ptr<WriteStream> into):
            into_{std::move(into)} {
        }

        void writeSection(String const & name) {
            into_->writer() << "[" << name << "]\n";
        }

        void writeValue(String const & name, String const & value) {
            into_->writer() << name << "=" << value << "\n";
        }   

    private:

        std::unique_ptr<WriteStream> into_;

    }; // rckid::ini::Writer


} // namespace rckid::ini