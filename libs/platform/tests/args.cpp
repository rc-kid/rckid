#include "../tests.h"
#include "../args.h"

TEST(platform, args_string) {
    Args::Arg<std::string> arg{""};
    char * argv[] = { "", "foobar"};
    Args::parse(2, argv, {arg});
    EXPECT(arg.isDefault() == false);
    EXPECT(arg.value() == "foobar");    
}

TEST(platform, args_int) {
    Args::Arg<int> arg1{0};
    Args::Arg<int> arg2{0};
    char * argv[] = { "", "565", "-67"};
    Args::parse(3, argv, {arg1, arg2});
    EXPECT(arg1.isDefault() == false);
    EXPECT(arg1.value() == 565);    
    EXPECT(arg2.isDefault() == false);
    EXPECT(arg2.value() == -67);    
}

TEST(platform, args_unsigned) {
    Args::Arg<unsigned> arg{0};
    char * argv[] = { "", "67262536"};
    Args::parse(2, argv, {arg});
    EXPECT(arg.isDefault() == false);
    EXPECT(arg.value() == 67262536);
}

TEST(platform, args_double) {
    Args::Arg<double> arg{0};
    char * argv[] = { "", "1.67"};
    Args::parse(2, argv, {arg});
    EXPECT(arg.isDefault() == false);
    EXPECT(arg.value() == 1.67);
}

TEST(platform, args_bool) {
    Args::Arg<bool> arg1{false};
    Args::Arg<bool> arg2{false};
    char * argv[] = { "", "T", "F"};
    Args::parse(3, argv, {arg1, arg2});
    EXPECT(arg1.isDefault() == false);
    EXPECT(arg1.value() == true);
    EXPECT(arg2.isDefault() == false);
    EXPECT(arg2.value() == false);
}

TEST(platform, args_keyword) {
    Args::Arg<int> foo{"foo", 0};
    Args::Arg<int> bar{"bar", 0};
    char * argv[] = { "", "--foo", "3", "--bar", "67"};
    Args::parse(5, argv, {foo, bar});
    EXPECT(foo.isDefault() == false);
    EXPECT(foo.value() == 3);
    EXPECT(bar.isDefault() == false);
    EXPECT(bar.value() == 67);
}

TEST(platform, args_bool_novalue) {
    Args::Arg<int> foo{"foo", 0};
    Args::Arg<bool> bar{"bar", false};
    char * argv[] = { "", "--foo", "3", "--bar"};
    Args::parse(4, argv, {foo, bar});
    EXPECT(foo.isDefault() == false);
    EXPECT(foo.value() == 3);
    EXPECT(bar.isDefault() == false);
    EXPECT(bar.value() == true);
}




