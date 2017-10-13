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

#include "gflags_config.hpp"
#include "main/config/flags.hpp"
#include "util/filesystem.hpp"

namespace iroha {
  namespace config {

    using iroha::filesystem::util::read_file;

    GFlagsConfig::GFlagsConfig() { load(); }

    void GFlagsConfig::load() {
      gflags::ReparseCommandLineNonHelpFlags();

      // these parameters are already validated.
      this->redis_.host = FLAGS_redis_host;
      this->redis_.port = static_cast<uint16_t>(FLAGS_redis_port);

      this->pg_.host = FLAGS_postgres_host;
      this->pg_.port = static_cast<uint16_t>(FLAGS_postgres_port);
      this->pg_.username = FLAGS_postgres_username;
      this->pg_.password = FLAGS_postgres_password;

      this->crypto_.private_key = read_file(FLAGS_private_key);
      this->crypto_.public_key = read_file(FLAGS_public_key);

      this->db_.path = FLAGS_dbpath;

      this->options_.genesis_block =
          filesystem::util::read_file(FLAGS_genesis_block);

      this->loaded_ = true;
    }
  }  // namespace config
}  // namespace iroha
