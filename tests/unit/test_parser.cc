#include "myshell/parser/parser.hh"

#include <gtest/gtest.h>

TEST(ParserTest, ParseSimpleCommand) {
    myshell::Lexer lexer("ls -la /tmp");
    myshell::Parser parser(lexer);

    auto res = parser.parse();
    ASSERT_TRUE(res.ok());

    const auto& script = res.value();
    ASSERT_EQ(script.commands.items.size(), 1u);

    const auto& item = script.commands.items[0];
    const auto& cmd = item.and_or.first.commands[0];
    EXPECT_EQ(cmd.words.size(), 3u);
    EXPECT_EQ(cmd.words[0], "ls");
    EXPECT_EQ(cmd.words[1], "-la");
    EXPECT_EQ(cmd.words[2], "/tmp");
}

TEST(ParserTest, ParsePipeline) {
    myshell::Lexer lexer("cat file.txt | grep error | wc -l");
    myshell::Parser parser(lexer);

    auto res = parser.parse();
    ASSERT_TRUE(res.ok());

    const auto& script = res.value();
    const auto& pipe = script.commands.items[0].and_or.first;
    EXPECT_EQ(pipe.commands.size(), 3u);
    EXPECT_EQ(pipe.commands[0].words[0], "cat");
    EXPECT_EQ(pipe.commands[1].words[0], "grep");
    EXPECT_EQ(pipe.commands[2].words[0], "wc");
}

TEST(ParserTest, ParseRedirection) {
    myshell::Lexer lexer("echo hello > out.txt");
    myshell::Parser parser(lexer);

    auto res = parser.parse();
    ASSERT_TRUE(res.ok());

    const auto& cmd = res.value().commands.items[0].and_or.first.commands[0];
    EXPECT_EQ(cmd.words.size(), 2u);
    ASSERT_EQ(cmd.redirects.size(), 1u);
    EXPECT_EQ(cmd.redirects[0].op, myshell::ast::RedirectOp::Output);
    EXPECT_EQ(cmd.redirects[0].target, "out.txt");
}
