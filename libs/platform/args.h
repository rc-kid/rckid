#pragma once

#include <string>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <unordered_map>

#include <platform.h>

#include "string_utils.h"

/** Very simple argument parser. 
 
    Takes argc & argv and converts it to a hashmap
 */
class Args {
public:
    class BaseArg {
    public:
        char const * name() const { return name_; }
        bool isPositional() const { return name_ == nullptr; }
        bool isDefault() const { return isDefault_; }
    protected:
        BaseArg() = default;
        BaseArg(char const * name): name_{name} {} 
        virtual void parse(char const * input) = 0;
    private:
        friend class Args;
        char const * name_ = nullptr;
        bool isDefault_ = true;
    }; // Args::BaseArg

    template<typename T>
    class Arg : public BaseArg {
    public:
        Arg(T value): value_{std::move(value)} {}
        Arg(char const * name, T value): BaseArg{name}, value_{std::move(value)} {}

        T const & value() const { return value_; }
        void setValue(T value) { value_ = std::move(value); }
    protected:
        void parse(char const * input) override;
    private:
        T value_;
    }; // Args::Arg<T>

    /** Parses the commandline arguments into the given args. 
     
        The arguments given are expected to be first positional, and then named. All positional arguments are required and all named arguments are not. For named arguments, their name must start with '--' and their value is in the next argument. If there is no next argument, or if the next argument starts with '--' then their value will be reported to the parse function as nullptr. 
     */
    static void parse(int argc, char * argv[], std::initializer_list<std::reference_wrapper<BaseArg>> args) {
        std::vector<std::reference_wrapper<BaseArg>> positional;
        std::unordered_map<std::string, std::reference_wrapper<BaseArg>> keyword;
        for (BaseArg & arg : args) {
            if (arg.isPositional()) {
                if (! keyword.empty())
                    throw std::invalid_argument{"Positional argument provided after keyword args"};
                positional.push_back(arg);
            } else {
                std::string name = arg.name();
                auto i = keyword.find(name);
                if (i != keyword.end())
                    throw std::invalid_argument{"Duplicate keyword argument"};
                keyword.insert(std::make_pair(name, std::ref(arg)));
            }
        }
        // now parse the arguments, first positional ones, which are required
        size_t i = 0;
        while (i < positional.size()) {
            if (i + 1 >= static_cast<size_t>(argc))
                throw std::invalid_argument("Too few arguments specified");
            positional[i].get().parse(argv[i + 1]);
            positional[i].get().isDefault_ = false;
            ++i;
        }
        // if there are any other arguments, they must be named
        ++i; // since positiona;l were 0 indexed now we move to 1 indexed
        while (i < static_cast<size_t>(argc)) {
            if (!startsWith(argv[i], "--"))
                throw std::invalid_argument("Keyword argument does not start with --");
            std::string name{argv[i] + 2};
            auto arg = keyword.find(name);
            if (arg == keyword.end())
                throw std::invalid_argument(std::string{"Unknown keyword argument "} + name); 
            // move to the value
            ++i;
            if (i >= static_cast<size_t>(argc) || startsWith(argv[i], "--")) {
                arg->second.get().parse(nullptr);
            } else {
                arg->second.get().parse((argv[i]));
                ++i;
            }
            if (! arg->second.get().isDefault_)
                throw std::invalid_argument("Duplicate keyword argument definition");
            arg->second.get().isDefault_ = false;
        }
    }

}; // Args

template<>
inline void Args::Arg<char const *>::parse(char const * input) {
    value_ = input;
}

template<>
inline void Args::Arg<std::string>::parse(char const * input) {
    value_ = input;
}

template<>
inline void Args::Arg<int>::parse(char const * input) {
    char * x;
    int result = std::strtol(input, & x, 10);
    if (*x != '\0')
        throw std::invalid_argument{"Not valid int"};
    value_ = result;
}

template<>
inline void Args::Arg<unsigned>::parse(char const * input) {
    char * x;
    unsigned result = std::strtoul(input, & x, 10);
    if (*x != '\0')
        throw std::invalid_argument{"Not valid unsigned"};
    value_ = result;
}

template<>
inline void Args::Arg<double>::parse(char const * input) {
    char * x;
    double result = std::strtod(input, & x);
    if (*x != '\0')
        throw std::invalid_argument{"Not valid double"};
    value_ = result;
}

template<>
inline void Args::Arg<bool>::parse(char const * input) {
    if (input == nullptr)
        value_ = true;
    else if ((strcmp(input, "1") == 0) || (strcmp(input, "T") == 0) || (strcmp(input, "Y") == 0))
        value_ = true;
    else if ((strcmp(input, "0") == 0) || (strcmp(input, "F") == 0) || (strcmp(input, "N") == 0))
        value_ = false;
    else 
        throw std::invalid_argument{"Not valid boolean"};
}