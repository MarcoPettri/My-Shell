#include "myshell/builtins/builtin.hh"
#include "myshell/execution/executor.hh"
#include "myshell/lexer/lexer.hh"
#include "myshell/parser/parser.hh"
#include "myshell/platform/posix/environment.hh"

#include <gtest/gtest.h>

TEST(ExecutionTest, EchoBuiltin) {
    myshell::platform::Environment env;
    myshell::builtins::BuiltinRegistry builtins;
    myshell::execution::Executor exec(env, builtins);

    myshell::Lexer lexer("echo test");
    myshell::Parser parser(lexer);
    auto script = parser.parse();
    ASSERT_TRUE(script.ok());

    auto res = exec.execute(script.value());
    ASSERT_TRUE(res.ok());
    EXPECT_EQ(res.value(), 0);
}

TEST(ExecutionTest, VariableAssignment) {
    myshell::platform::Environment env;
    myshell::builtins::BuiltinRegistry builtins;
    myshell::execution::Executor exec(env, builtins);

    myshell::Lexer lexer("FOO=bar");
    myshell::Parser parser(lexer);
    auto script = parser.parse();
    ASSERT_TRUE(script.ok());

    auto res = exec.execute(script.value());
    ASSERT_TRUE(res.ok());
    EXPECT_EQ(res.value(), 0);

    auto val = env.get("FOO");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "bar");
}
