#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <iostream>

#include "utils.h"

#define TEST(SUITE_NAME, TEST_NAME, ...) \
    class Test_ ## SUITE_NAME ## _ ## TEST_NAME : public ::Tests, ## __VA_ARGS__ { \
    private: \
        Test_ ## SUITE_NAME ## _ ## TEST_NAME (char const * suiteName, char const * testName): \
            ::Tests(__FILE__, __LINE__, suiteName, testName) { \
        } \
        void run_() override; \
        static Test_ ## SUITE_NAME ## _ ## TEST_NAME singleton_; \
    } \
    Test_ ## SUITE_NAME ## _ ## TEST_NAME ::singleton_{# SUITE_NAME, # TEST_NAME }; \
    inline void Test_ ## SUITE_NAME ## _ ## TEST_NAME ::run_() 


#define EXPECT(...) if (expect(__FILE__, __LINE__, #__VA_ARGS__, __VA_ARGS__)) {} else {}
#define EXPECT_EQ(...) if (expectEq(__FILE__, __LINE__, #__VA_ARGS__, __VA_ARGS__)) {} else {}

class Tests {
public:
    
    static int run(int argc, char * argv[]);

protected:
    Tests(char const * filename, size_t line, char const * suiteName, char const * testName):
        testName_{testName},
        filename_{filename},
        line_{line} {
            if (addTest_(suiteName, testName_, this) == false)
                throw std::invalid_argument{"Test with same name and suite already exists"};
    }

    std::string const & testName() const { return testName_; }

    template<typename T>
    bool expect(char const * filename, size_t line, char const * exprStr, T const & expr) {
        return expect(filename, line, exprStr, expr, "");
    }

    template<typename T>
    bool expect(char const * filename, size_t line, char const * exprStr, T const & expr, char const * msg) {
        ++stats_().testChecks;
        if (expr) 
            return true;
        // if this is the first failed check, print the test info first
        addFailure();
        std::cout << "    " << exprStr << " not true at " << filename << ":" << line << std::endl;
        return false;
    }

    template<typename T, typename W> 
    bool expectEq(char const * filename, size_t line, char const * expr, T const & x, W const & y) {
        return expectEq(filename, line, expr, x, y, "");
    }

    template<typename T, typename W> 
    bool expectEq(char const * filename, size_t line, char const * expr, T const & x, W const & y, char const * msg) {
        ++stats_().testChecks;
        if (x == y)
            return true;
        // if this is the first failed check, print the test info first
        addFailure();
        std::cout << "    " << expr << " not equal, found " << x << ", expected " << y << " at " << filename << ":" << line << std::endl;
        return false;
    }

private:

    std::string testName_;
    char const * filename_;
    size_t line_;

    virtual void run_() = 0;

    void testHeader() {
        std::cout << "  " << testName_ << " (" << filename_ << ":" << line_ << "):" << std::endl;
    }

    void suiteHeader() {
        std::cout << "Suite " << stats_().suite << ":" << std::endl;
    }

    void addFailure() {
        auto & stats = stats_();
        if (stats.suiteHeader == false) {
            stats.suiteHeader = true;
            suiteHeader();
        }
        if (++stats.testFails == 1)
            testHeader();
    }

    static std::unordered_map<std::string, std::unordered_map<std::string, Tests *>> & tests_() {
        static std::unordered_map<std::string, std::unordered_map<std::string, Tests *>> tests;
        return tests;
    } 

    static bool addTest_(std::string_view suiteName, std::string_view testName, Tests * test) {
        Tests * & t = tests_()[std::string{suiteName}][std::string{testName}];
        if (t != nullptr)
            return false;
        t = test;
        return true;
    }

    /** Running stats about the test suite execution. */
    struct Stats {
        size_t suites = 0;
        size_t failedSuites = 0;
        size_t totalTests = 0;
        size_t failedTests = 0;
        std::string suite;
        size_t suiteTests = 0;
        size_t suiteFails = 0;
        bool suiteHeader = false;
        size_t testChecks = 0;
        size_t testFails = 0;

        void startSuite(std::string_view suite) {
            startTest();
            suiteTests = 0;
            suiteHeader = false;
            suiteFails = 0;
            this->suite = suite;
        }

        void finishSuite() {
            if (suiteFails > 0) 
                std::cout << "  Test FAIL (" << suiteTests << " tests, " << suiteFails << " failed)" << std::endl;
            ++suites;
            if (suiteFails > 0)
                ++failedSuites;
            totalTests += suiteTests;
            failedTests += suiteFails;
        }

        void startTest() {
            testChecks = 0;
            testFails = 0;
        }

        void finishTest() {
            if (testFails > 0) {
                std::cout << "    Suite FAIL (" << testChecks << " checks, " << testFails << " failed)" << std::endl;
                suiteFails += 1;
            }
            ++suiteTests;
        }

    }; // Tests::Stats

    static Stats & stats_() {
        static Stats stats;
        return stats;
    }

}; // Tests

inline int Tests::run(int argc, char * argv[]) {
    #if (! defined TESTS)
    std::cout << "Target not compiled with -DTESTS. Tests might not be visible." << std::endl;
    #endif

    auto & stats = stats_();
    for (auto const & suite : tests_()) {
        stats.startSuite(suite.first);
        for (auto const & test : suite.second) {
            stats.startTest();
            try {
                test.second->run_();
            } catch (std::exception const & e) {
                test.second->addFailure();
                std::cout << "    exception thrown: " << e.what() << std::endl;
            } catch (...) {
                test.second->addFailure();
                std::cout << "    unknown exception thrown" << std::endl;
            }
            stats.finishTest();
        }
        stats.finishSuite();
    }
    std::cout << "All done." << std::endl;
    std::cout << "TOTAL : " << stats.suites << " suites, " << stats.failedSuites << " failed" << std::endl;
    std::cout << "        " << stats.totalTests << " tests, " << stats.failedTests << " failed" << std::endl;
    return EXIT_SUCCESS;
}

#define RUN_TESTS int main(int argc, char* argv[]) { \
    return Tests::run(argc, argv); \
}
