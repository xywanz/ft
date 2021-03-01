// Copyright [2020] <Copyright Kevin, kevin.lau.gd@gmail.com>

#include <gtest/gtest.h>

#include <future>
#include <thread>

#include "component/pubsub/zmq_socket.h"

using ft::pubsub::ZmqSocket;

void PublisherThread() {
  //  Prepare publisher
  ZmqSocket publisher;
  publisher.Bind("ipc://test.ipc");

  // Give the subscribers a chance to connect, so they don't lose any messages
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  for (int i = 0; i < 10; ++i) {
    //  Write three messages, each with an envelope and content
    publisher.SendMsg("A", "Message in A envelope");
    publisher.SendMsg("B", "Message in B envelope");
    publisher.SendMsg("C", "Message in C envelope");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void SubscriberThread1() {
  //  Prepare subscriber
  ZmqSocket subscriber;
  subscriber.Connect("ipc://test.ipc");

  //  Thread2 opens "A" and "B" envelopes
  subscriber.Subscribe("A");
  subscriber.Subscribe("B");

  std::string topic;
  std::string msg;

  for (int i = 0; i < 10; ++i) {
    // Receive all parts of the message
    ASSERT_TRUE(subscriber.RecvMsg(&topic, &msg));
    // std::cout << "Thread2: [" << topic << "] " << msg << std::endl;
  }
}

void SubscriberThread2() {
  //  Prepare our context and subscriber
  ZmqSocket subscriber;
  subscriber.Connect("ipc://test.ipc");

  //  Thread3 opens ALL envelopes
  subscriber.Subscribe("");

  std::string topic;
  std::string msg;

  for (int i = 0; i < 10; ++i) {
    // Receive all parts of the message
    ASSERT_TRUE(subscriber.RecvMsg(&topic, &msg));
    // std::cout << "Thread3: [" << topic << "] " << msg << std::endl;
  }
}

TEST(ZmqSocketTest, Case_0) {
  /*
   * No I/O threads are involved in passing messages using the inproc transport.
   * Therefore, if you are using a ØMQ context for in-process messaging only you
   * can initialise the context with zero I/O threads.
   *
   * Source: http://api.zeromq.org/4-3:zmq-inproc
   */

  auto thread1 = std::async(std::launch::async, PublisherThread);

  // Give the publisher a chance to bind, since inproc requires it
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  auto thread2 = std::async(std::launch::async, SubscriberThread1);
  auto thread3 = std::async(std::launch::async, SubscriberThread2);
  thread1.wait();
  thread2.wait();
  thread3.wait();

  /*
   * Output:
   *   An infinite loop of a mix of:
   *     Thread2: [A] Message in A envelope
   *     Thread2: [B] Message in B envelope
   *     Thread3: [A] Message in A envelope
   *     Thread3: [B] Message in B envelope
   *     Thread3: [C] Message in C envelope
   */
}
