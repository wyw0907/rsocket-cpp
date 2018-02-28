// Copyright 2004-present Facebook. All Rights Reserved.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <yarpl/test_utils/mocks.h>

#include "rsocket/statemachine/ChannelRequester.h"
#include "rsocket/test/test_utils/MockStreamsWriter.h"

using namespace rsocket;
using namespace testing;
using namespace yarpl::mocks;

TEST(StreamsWriterTest, DelegateMock) {
  auto writer = std::make_shared<StrictMock<MockStreamsWriter>>();
  auto& impl = writer->delegateToImpl();
  EXPECT_CALL(impl, outputFrame(_));
  EXPECT_CALL(impl, shouldQueue()).WillOnce(Return(false));
  EXPECT_CALL(*writer, writeNewStream(_, _, _, _));

  auto requester = std::make_shared<ChannelRequester>(writer, 1u);
  yarpl::flowable::Subscriber<rsocket::Payload>* subscriber = requester.get();
  subscriber->onSubscribe(yarpl::flowable::Subscription::empty());
  subscriber->onNext(Payload());
}

TEST(StreamsWriterTest, NewStreamsMockWriterImpl) {
  auto writer = std::make_shared<StrictMock<MockStreamsWriterImpl>>();
  EXPECT_CALL(*writer, outputFrame(_));
  EXPECT_CALL(*writer, shouldQueue()).WillOnce(Return(false));

  auto requester = std::make_shared<ChannelRequester>(writer, 1u);
  yarpl::flowable::Subscriber<rsocket::Payload>* subscriber = requester.get();
  subscriber->onSubscribe(yarpl::flowable::Subscription::empty());
  subscriber->onNext(Payload());
}

TEST(StreamsWriterTest, QueueFrames) {
  auto writer = std::make_shared<StrictMock<MockStreamsWriter>>();
  auto& impl = writer->delegateToImpl();
  impl.shouldQueue_ = true;

  EXPECT_CALL(impl, outputFrame(_)).Times(0);
  EXPECT_CALL(impl, shouldQueue()).WillOnce(Return(true));
  EXPECT_CALL(*writer, writeNewStream(_, _, _, _));

  auto requester = std::make_shared<ChannelRequester>(writer, 1u);
  yarpl::flowable::Subscriber<rsocket::Payload>* subscriber = requester.get();
  subscriber->onSubscribe(yarpl::flowable::Subscription::empty());
  subscriber->onNext(Payload());
}

TEST(StreamsWriterTest, FlushQueuedFrames) {
  auto writer = std::make_shared<StrictMock<MockStreamsWriter>>();
  auto& impl = writer->delegateToImpl();
  impl.shouldQueue_ = true;

  EXPECT_CALL(impl, outputFrame(_)).Times(1);
  EXPECT_CALL(impl, shouldQueue()).Times(3);
  EXPECT_CALL(*writer, writeNewStream(_, _, _, _));

  auto requester = std::make_shared<ChannelRequester>(writer, 1u);
  yarpl::flowable::Subscriber<rsocket::Payload>* subscriber = requester.get();
  subscriber->onSubscribe(yarpl::flowable::Subscription::empty());
  subscriber->onNext(Payload());

  // Will queue again
  impl.sendPendingFrames();

  // Now send them actually
  impl.shouldQueue_ = false;
  impl.sendPendingFrames();
  // it will not send the pending frames twice
  impl.sendPendingFrames();
}