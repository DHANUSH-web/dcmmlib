#include "dcmm/dcmm.hpp"

#include <gtest/gtest.h>

#include <thread>

TEST(Engine, SeparateInstancesRunConcurrently) {
  dcmm::Engine a;
  dcmm::Engine b;
  std::thread t1([&] { (void)a.disk("/"); });
  std::thread t2([&] { (void)b.memory(); });
  t1.join();
  t2.join();
  EXPECT_GT(a.disk("/").totalBytes, 0u);
}

TEST(Engine, SameInstanceSerializesOverlappedCalls) {
  dcmm::Engine e;
  std::thread t1([&] {
    (void)e.disk("/");
    (void)e.memory();
  });
  std::thread t2([&] {
    (void)e.memory();
    (void)e.disk("/");
  });
  t1.join();
  t2.join();
}

TEST(Engine, CancelDuringScanDoesNotDeadlock) {
  dcmm::Engine e;
  std::thread t([&] { (void)e.scanJunk(); });
  e.cancel();
  t.join();
}
