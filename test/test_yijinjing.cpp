#include <gtest/gtest.h>

#include "ft/component/yijinjing/journal/JournalReader.h"
#include "ft/component/yijinjing/journal/JournalWriter.h"
#include "ft/component/yijinjing/journal/PageProvider.h"

TEST(YIJINJING, JOURNAL) {
  auto writer = yijinjing::JournalWriter::create(".", "test_yijinjing_writer", "writer");

  writer->seekEnd();
  writer->write_frame("aaa", 3, 1, 0);

  auto reader = yijinjing::JournalReader::create(0, "reader");
  reader->addJournal(".", "test_yijinjing_writer");

  auto frame = reader->getNextFrame();
  ASSERT_TRUE(frame != nullptr);
  ASSERT_EQ(frame->getDataLength(), 3);

  char data[4];
  memcpy(data, frame->getData(), frame->getDataLength());
  data[frame->getDataLength()] = 0;
  ASSERT_STREQ(data, "aaa");
}
