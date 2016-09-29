/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Laura Schlimmer <laura@eventql.io>
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/cli/commands/table_import.h>
#include <eventql/util/cli/flagparser.h>
#include "eventql/util/logging.h"
#include "eventql/transport/native/frames/insert.h"
#include "eventql/transport/native/frames/error.h"

namespace eventql {
namespace cli {

const String TableImport::kName_ = "table-import";
const String TableImport::kDescription_ = "Import json or csv data to a table.";

TableImport::TableImport(
    RefPtr<ProcessConfig> process_cfg) :
    process_cfg_(process_cfg) {}

Status TableImport::execute(
    const std::vector<std::string>& argv,
    FileInputStream* stdin_is,
    OutputStream* stdout_os,
    OutputStream* stderr_os) {
  ::cli::FlagParser flags;

  flags.defineFlag(
      "database",
      ::cli::FlagParser::T_STRING,
      true,
      "d",
      NULL,
      "database",
      "<string>");

  flags.defineFlag(
      "table",
      ::cli::FlagParser::T_STRING,
      true,
      "t",
      NULL,
      "table name",
      "<string>");

  flags.defineFlag(
      "host",
      ::cli::FlagParser::T_STRING,
      true,
      "h",
      NULL,
      "host name",
      "<string>");

  flags.defineFlag(
      "port",
      ::cli::FlagParser::T_INTEGER,
      true,
      "p",
      NULL,
      "port",
      "<integer>");

  flags.defineFlag(
      "file",
      ::cli::FlagParser::T_STRING,
      true,
      "f",
      NULL,
      "file path",
      "<string>");

  flags.defineFlag(
      "connections",
      ::cli::FlagParser::T_INTEGER,
      false,
      "c",
      "10",
      "number of connections",
      "<num>");

  std::unique_ptr<FileInputStream> is;
  try {
    flags.parseArgv(argv);
    is = FileInputStream::openFile(flags.getString("file"));

  } catch (const Exception& e) {

    stderr_os->write(StringUtil::format(
        "$0: $1\n",
       e.getTypeName(),
        e.getMessage()));
    return Status(e);
  }

  database_ = flags.getString("database");
  table_ = flags.getString("table");

  /* auth data */
  std::vector<std::pair<std::string, std::string>> auth_data;
  if (process_cfg_->hasProperty("client.user")) {
    auth_data.emplace_back(
        "user",
        process_cfg_->getString("client.user").get());
  }

  if (process_cfg_->hasProperty("client.password")) {
    auth_data.emplace_back(
        "password",
        process_cfg_->getString("client.").get());
  }

  if (process_cfg_->hasProperty("client.auth_token")) {
    auth_data.emplace_back(
        "auth_token",
        process_cfg_->getString("client.auth_token").get());
  }

  auto host = flags.getString("host");
  auto port = flags.getInt("port");

  client_ = new native_transport::TCPClient(nullptr, nullptr);
  /* connect */
  auto rc = client_->connect(
      host,
      port,
      false,
      auth_data);
  if (!rc.isSuccess()) {
    return rc;
  }

  std::vector<std::string> batch;
  std::string line;
  uint64_t cur_size = 0;
  while (is->readLine(&line)) {
    if (cur_size + line.size() > kDefaultBatchSize) {
      auto rc = uploadBatch(batch);
      if (!rc.isSuccess()) {
        client_->close();
        return rc;
      }

      batch.clear();
      cur_size = 0;
    }

    batch.emplace_back(line);
    cur_size += line.size();
    line.clear();
  }

  if (batch.size() > 0) {
    auto rc = uploadBatch(batch);
    if (!rc.isSuccess()) {
      client_->close();
      return rc;
    }
  }

  client_->close();
  return Status::success();
}

Status TableImport::uploadBatch(std::vector<std::string> batch) {
  /* insert */
  native_transport::InsertFrame i_frame;
  i_frame.setDatabase(database_);
  i_frame.setTable(table_);
  i_frame.setRecordEncoding(EVQL_INSERT_CTYPE_JSON);

  for (auto l : batch) {
    i_frame.addRecord(l);
  }

  auto rc = client_->sendFrame(&i_frame, 0);
  if (!rc.isSuccess()) {
    return rc;
  }

  uint16_t ret_opcode = 0;
  uint16_t ret_flags;
  std::string ret_payload;
  while (ret_opcode != EVQL_OP_ACK) {
    auto rc = client_->recvFrame(
        &ret_opcode,
        &ret_flags,
        &ret_payload,
        kMicrosPerSecond); // FIXME

    if (!rc.isSuccess()) {
      return rc;
    }

    switch (ret_opcode) {
      case EVQL_OP_ACK:
      case EVQL_OP_HEARTBEAT:
        continue;
      case EVQL_OP_ERROR: {
        native_transport::ErrorFrame eframe;
        eframe.parseFrom(ret_payload.data(), ret_payload.size());
        return ReturnCode::error("ERUNTIME", eframe.getError());
      }
      default:
        return ReturnCode::error("ERUNTIME", "unexpected opcode");
    }
  }

  return Status::success();
}

const String& TableImport::getName() const {
  return kName_;
}

const String& TableImport::getDescription() const {
  return kDescription_;
}

void TableImport::printHelp(OutputStream* stdout_os) const {
  stdout_os->write(StringUtil::format(
      "\nevqlctl-$0 - $1\n\n", kName_, kDescription_));

  stdout_os->write(
      "Usage: evqlctl table-import [OPTIONS]\n"
      "   -c, --connections <num>     Number of concurrent connections\n"
      "  -d, --database <db>          Select a database.\n"
      "  -t, --table <tbl>            Select a destination table.\n"
      "  -f, --file <file>            Set the path of the file to import.\n"
      "  -h, --host <hostname>        Set the EventQL hostname.\n"
      "  -p, --port <port>            Set the EventQL port.\n");
}

} // namespace cli
} // namespace eventql

