/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/stats/counter.h>
#include <eventql/util/http/httpstats.h>

namespace eventql {

struct Z1Stats {
  stx::stats::Counter<uint64_t> num_partitions;
  stx::stats::Counter<uint64_t> num_partitions_loaded;
  stx::stats::Counter<uint64_t> replication_queue_length;
  stx::stats::Counter<uint64_t> compaction_queue_length;
  stx::stats::Counter<uint64_t> mapreduce_reduce_memory;
  stx::stats::Counter<uint64_t> mapreduce_num_map_tasks;
  stx::stats::Counter<uint64_t> mapreduce_num_reduce_tasks;
  stx::http::HTTPClientStats http_client_stats;
};

Z1Stats* z1stats();

}