#include "myshell/history/history.hh"

#include <gtest/gtest.h>

TEST(HistoryTest, AddAndRetrieve) {
    myshell::history::History hist("", 10);
    hist.add("cmd1");
    hist.add("cmd2");
    hist.add("cmd3");

    EXPECT_EQ(hist.size(), 3u);
    EXPECT_EQ(hist.at(0), "cmd1");
    EXPECT_EQ(hist.at(1), "cmd2");
    EXPECT_EQ(hist.at(2), "cmd3");
}

TEST(HistoryTest, Deduplication) {
    myshell::history::History hist("", 10);
    hist.add("same");
    hist.add("same");

    EXPECT_EQ(hist.size(), 1u);
}

TEST(HistoryTest, CapacityEviction) {
    myshell::history::History hist("", 2);
    hist.add("c1");
    hist.add("c2");
    hist.add("c3");

    EXPECT_EQ(hist.size(), 2u);
    EXPECT_EQ(hist.at(0), "c2");
    EXPECT_EQ(hist.at(1), "c3");
}
