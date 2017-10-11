/**
 * Copyright Soramitsu Co., Ltd. 2017 All Rights Reserved.
 * http://soramitsu.co.jp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "module/irohad/ordering/ordering_mocks.hpp"

#include <grpc++/grpc++.h>
#include "logger/logger.hpp"
#include "module/irohad/ametsuchi/ametsuchi_mocks.hpp"
#include "ordering/impl/ordering_service_impl.hpp"

using namespace iroha;
using namespace iroha::ordering;
using namespace iroha::model;
using namespace iroha::network;
using namespace iroha::ametsuchi;
using namespace std::chrono_literals;

using ::testing::_;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::DoAll;
using ::testing::AtLeast;
using ::testing::Return;

class OrderingServiceTest : public OrderingTest {
 public:
  OrderingServiceTest() {
    fake_gate =
        static_cast<ordering::MockOrderingGate *>(gate_transport_service.get());
  }

  void SetUp() override {}

  void start() override {
    OrderingTest::start();
    client = proto::OrderingService::NewStub(
        grpc::CreateChannel(address, grpc::InsecureChannelCredentials()));
  }

  void send() {
    grpc::ClientContext context;
    google::protobuf::Empty reply;
    client->SendTransaction(&context, iroha::protocol::Transaction(), &reply);
  }

  ordering::MockOrderingGate *fake_gate;
  std::unique_ptr<iroha::ordering::proto::OrderingService::Stub> client;
  std::condition_variable cv;
  std::mutex m;
};

TEST_F(OrderingServiceTest, ValidWhenProposalSizeStrategy) {
  // Init => proposal size 5 => 2 proposals after 10 transactions

  std::shared_ptr<MockPeerQuery> wsv = std::make_shared<MockPeerQuery>();
  EXPECT_CALL(*wsv, getLedgerPeers())
      .WillRepeatedly(Return(std::vector<Peer>{peer}));

  const size_t max_proposal = 5;
  const size_t commit_delay = 1000;
  service =
      std::make_shared<OrderingServiceImpl>(wsv, max_proposal, commit_delay);

  EXPECT_CALL(*fake_gate, onProposal(_, _, _)).Times(2);

  size_t call_count = 0;
  ON_CALL(*fake_gate, onProposal(_, _, _))
      .WillByDefault(InvokeWithoutArgs([&] {
        ++call_count;
        cv.notify_one();
        return grpc::Status::OK;
      }));

  start();

  for (size_t i = 0; i < 10; ++i) {
    send();
  }

  std::unique_lock<std::mutex> lock(m);
  cv.wait_for(lock, 10s, [&] { return call_count == 2; });
}

TEST_F(OrderingServiceTest, ValidWhenTimerStrategy) {
  // Init => proposal timer 400 ms => 10 tx by 50 ms => 2 proposals in 1 second

  std::shared_ptr<MockPeerQuery> wsv = std::make_shared<MockPeerQuery>();
  EXPECT_CALL(*wsv, getLedgerPeers())
      .WillRepeatedly(Return(std::vector<Peer>{peer}));

  const size_t max_proposal = 100;
  const size_t commit_delay = 400;
  service =
      std::make_shared<OrderingServiceImpl>(wsv, max_proposal, commit_delay);

  EXPECT_CALL(*fake_gate, onProposal(_, _, _)).Times(2);
  ON_CALL(*fake_gate, onProposal(_, _, _))
      .WillByDefault(InvokeWithoutArgs([&] {
        cv.notify_one();
        return grpc::Status::OK;
      }));

  start();

  for (size_t i = 0; i < 8; ++i) {
    send();
  }

  std::unique_lock<std::mutex> lk(m);
  cv.wait_for(lk, 10s);

  send();
  send();
  cv.wait_for(lk, 10s);
}
