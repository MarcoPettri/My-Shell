#include "myshell/lexer/lexer.hh"

#include <gtest/gtest.h>

#include <algorithm>

TEST(LexerTest, BasicTokens) {
    myshell::Lexer lexer("echo hello world");
    auto t1 = lexer.next_token();
    EXPECT_TRUE(t1.is_word_like());
    EXPECT_EQ(t1.value, "echo");

    auto t2 = lexer.next_token();
    EXPECT_TRUE(t2.is_word_like());
    EXPECT_EQ(t2.value, "hello");

    auto t3 = lexer.next_token();
    EXPECT_TRUE(t3.is_word_like());
    EXPECT_EQ(t3.value, "world");

    auto t4 = lexer.next_token();
    EXPECT_EQ(t4.kind, myshell::TokenKind::END_OF_INPUT);
}

TEST(LexerTest, OperatorsAndPipes) {
    myshell::Lexer lexer("ls -l | grep cpp > output.txt && echo done");
    std::vector<myshell::TokenKind> kinds;
    while (!lexer.at_end()) {
        auto tok = lexer.next_token();
        if (tok.kind == myshell::TokenKind::END_OF_INPUT)
            break;
        kinds.push_back(tok.kind);
    }

    EXPECT_NE(std::find(kinds.begin(), kinds.end(), myshell::TokenKind::PIPE), kinds.end());
    EXPECT_NE(std::find(kinds.begin(), kinds.end(), myshell::TokenKind::REDIRECT_OUT), kinds.end());
    EXPECT_NE(std::find(kinds.begin(), kinds.end(), myshell::TokenKind::DOUBLE_AMPERSAND),
              kinds.end());
}

TEST(LexerTest, Quotes) {
    myshell::Lexer lexer("echo 'single quoted' \"double quoted\"");
    auto t1 = lexer.next_token();
    EXPECT_EQ(t1.value, "echo");

    auto t2 = lexer.next_token();
    EXPECT_EQ(t2.value, "single quoted");

    auto t3 = lexer.next_token();
    EXPECT_EQ(t3.value, "double quoted");
}
