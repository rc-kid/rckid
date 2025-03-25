#pragma once

#include <vector>
#include <unordered_map>
#include <sstream>
#include <iostream>
#include <cstdint>

#if (! defined STR)
#define STR(...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()
#endif

/** \page helpersTests Tests
    \brief Unittests infrastructure. 

    This header file provides standalone implementation for unit tests. New tests can be added using the `TEST` macro, which takes suite name and test name itself (both has to be unique within all suites or tests in a suite) and is followed by a code block that runs the test. 

    If the first command in the test body is the macro `TEST_SKIP` then the test will be skipped and its checks will not be executed. Inside the body, various conditions can be _checked_, or _expected_. Checks test for certain condition and stop executing the test immediately upon failure, while expects will only report error, but continue running the rest of the test. In any case, a failure in one test does not stop execution of other tests. 

    Any single expression that evaluates to boolean can be tested via the `CHECK` or `EXPECT` macros. If two expressions are given to the macros instead, they are evaluated and compared against, with the first expression being the actual value and the second expression the expected result. 

    `CHECK_THROWS` and `EXPECT_THROWS` are macros that take two arguments, a type and a code. They test that the evaluaton of the code throws exception of the specified type and fail if none, or different exception is thrown. 

    The Test class provides a simple test runner that should be called from the `main()` function and that runs all tests and reports the total, skipped and failed numbers of tests as well as total and failed checks.

 */

#define TEST(SUITE_NAME, TEST_NAME) \
    class Test_ ## SUITE_NAME ## _ ## TEST_NAME : public ::Test { \
    private: \
        Test_ ## SUITE_NAME ## _ ## TEST_NAME (): \
            Test{#SUITE_NAME, #TEST_NAME} {} \
        void run_() override; \
        static Test_ ## SUITE_NAME ## _ ## TEST_NAME Singleton_; \
    } \
    Test_ ## SUITE_NAME ## _ ## TEST_NAME :: Singleton_{}; \
    inline void Test_ ## SUITE_NAME ## _ ## TEST_NAME :: run_()

#define TEST_SKIP throw ::Test::Skip{};

#define CHECK(...) if (! expect(__FILE__, __LINE__, #__VA_ARGS__, __VA_ARGS__)) throw ::Test::CheckFailure{}; 

#define EXPECT(...) expect(__FILE__, __LINE__, #__VA_ARGS__, __VA_ARGS__)

#define CHECK_THROWS(TYPE, ...) \
  try { \
    __VA_ARGS__; \
    expectedExceptionNotRaised(__FILE__, __LINE__, #TYPE); \
    throw ::Test::CheckFailure{}; \
  } catch (TYPE const &) { \
    expect(__FILE__, __LINE__, "", true); \
  } catch (...) { \
    expectedExceptionMismatch(__FILE__, __LINE__, #TYPE); \
    throw ::Test::CheckFailure{}; \
  } 

#define EXPECT_THROWS(TYPE, ...) \
  try { \
    __VA_ARGS__; \
    expectedExceptionNotRaised(__FILE__, __LINE__, #TYPE); \
  } catch (TYPE const &) { \
    expect(__FILE__, __LINE__, "", true); \
  } catch (...) { \
    expectedExceptionMismatch(__FILE__, __LINE__, #TYPE); \
  } 

class Test {
public:

    class CheckFailure { }; // Test::CheckFailure

    class Skip {}; // Test::Skip

    static int RunAll([[maybe_unused]] int argc, [[maybe_unused]] char * argv[]) {
        // TODO actually implement logging, disabling test suites, etc. 
        size_t testIndex = 0;
        //std::cout << "Running " << TotalTests_ << " tests..." << std::flush;
        for (auto & suite : Suites_) {
            //std::cout << "Running suite " << suite.first << "..." << std::endl;
            for (Test * t : suite.second.tests) {
                std::cout << "\r(" << ++testIndex << "/" << TotalTests_ << "): " << t->suiteName << " - " << t->testName << "\033[K" << std::flush;
                size_t oldFailed = FailedChecks_;
                try {
                    t->run_();
                } catch (CheckFailure const &) {
                    // test failed, try next one
                } catch (Skip const &) {
                    SkippedTests_++;
                } catch (std::exception const & e) {
                    std::cout << "Failed after throwing: " << e.what() << std::endl;
                    ++FailedTests_;
                    break;
                } catch (...) {
                    std::cout << "Failed after throwing unknown exception" << std::endl;
                    ++FailedTests_;
                    break;
                }
                if (oldFailed != FailedChecks_)
                    ++FailedTests_;
            }
        }
        if (FailedTests_ == 0) {
            std::cout << "\rPASS: total tests:   " << TotalTests_ << " \033[K" << std::endl;
            std::cout << "      skipped tests: " << SkippedTests_ << std::endl;
            std::cout << "      total checks:  " << TotalChecks_ << std::endl;
            return EXIT_SUCCESS;
        } else {
            std::cout << "\rFAIL: total tests:   " << TotalTests_ << " \033[K" << std::endl;
            std::cout << "      skipped tests: " << SkippedTests_ << std::endl;
            std::cout << "      failed tests:  " << FailedTests_ << std::endl;
            std::cout << "      total checks:  " << TotalChecks_ << std::endl;
            std::cout << "      failed checks: " << FailedChecks_ << std::endl;
            return EXIT_FAILURE;
        }
    }

protected:

    char const * const suiteName;
    char const * const testName;

    Test(char const * suiteName, char const * testName):
        suiteName{suiteName},
        testName{testName} {
        Suites_[suiteName].tests.push_back(this);
        ++TotalTests_;
    }

    template<typename T>
    bool expect(char const * file, size_t line, char const * expr, T const & x) {
        ++TotalChecks_;
        if (!x) {
            ++FailedChecks_;
            std::cout << "\n" << file << "(" << line << "): " << expr << " not true" << std::endl;
            return false;
        } else {
            return true;
        }
    }

    template<typename T, typename W>
    bool expect(char const * file, size_t line, char const * expr, T actual, W expected) {
        ++TotalChecks_;
        if (actual != static_cast<T>(expected)) {
            ++FailedChecks_;
            std::cout << "\n" << file << "(" << line << "): " << expr << " evaluates to " << actual << " when " << expected << " expected" << std::endl;
            return false;
        } else {
            return true;
        }
    }

    void expectedExceptionNotRaised(char const * file, size_t line, char const * ex) {
        ++FailedChecks_;
        std::cout << "\n" << file << "(" << line << "): Expected exception " << ex << " but none thrown" << std::endl;
    }

    void expectedExceptionMismatch(char const * file, size_t line, char const * ex) {
        ++FailedChecks_;
        std::cout << "\n" << file << "(" << line << "): Expected exception " << ex << " but other exception thrown" << std::endl;
    }

private:

    virtual void run_() = 0;

    static inline size_t TotalTests_ = 0;
    static inline size_t PassedTests_ = 0;
    static inline size_t SkippedTests_ = 0;
    static inline size_t FailedTests_ = 0;
    static inline size_t TotalChecks_ = 0;
    static inline size_t FailedChecks_ = 0;

    struct Suite {
        std::vector<Test*> tests;
    }; // Test::Suite

    static inline std::unordered_map<std::string, Suite> Suites_;

}; // Test

template<>
bool inline Test::expect(char const * file, size_t line, char const * expr, uint8_t const * actual, uint8_t const * expected) {
    ++TotalChecks_;
    if (actual != expected) {
        ++FailedChecks_;
        std::cout << "\n" << file << "(" << line << "): " << expr << " evaluates to " << (size_t)actual << " when " << (size_t)expected << " expected" << std::endl;
        return false;
    } else {
        return true;
    }
}
