  /**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <eventql/util/SHA1.h>
#include <eventql/sql/runtime/defaultruntime.h>
#include <eventql/transport/native/client_tcp.h>

namespace csql {

class GroupByExpression : public TableExpression {
public:

  static const size_t kOutputBatchSize = 1024;

  GroupByExpression(
      Transaction* txn,
      ExecutionContext* execution_context,
      Vector<ValueExpression> select_expressions,
      Vector<ValueExpression> group_expressions,
      ScopedPtr<TableExpression> input);

  ~GroupByExpression();

  ReturnCode execute() override;
  ReturnCode nextBatch(SVector* columns, size_t* len) override;

  size_t getColumnCount() const override;
  SType getColumnType(size_t idx) const override;

protected:
  Transaction* txn_;
  ExecutionContext* execution_context_;
  Vector<ValueExpression> select_exprs_;
  Vector<ValueExpression> group_exprs_;
  ScopedPtr<TableExpression> input_;
  HashMap<SHA1Hash, Vector<VM::Instance>> groups_;
  HashMap<SHA1Hash, Vector<VM::Instance>>::iterator groups_iter_;
  ScratchMemory scratch_;
  VMStack vm_stack_;
};

class PartialGroupByExpression : public TableExpression {
public:

  static const size_t kOutputBatchSize = 1024;

  PartialGroupByExpression(
      Transaction* txn,
      Vector<ValueExpression> select_expressions,
      Vector<ValueExpression> group_expressions,
      SHA1Hash expression_fingerprint,
      ScopedPtr<TableExpression> input);

  ~PartialGroupByExpression();

  ReturnCode execute() override;
  ReturnCode nextBatch(SVector* columns, size_t* len) override;

  size_t getColumnCount() const override;
  SType getColumnType(size_t idx) const override;

  Option<SHA1Hash> getCacheKey() const override;

protected:
  Transaction* txn_;
  Vector<ValueExpression> select_exprs_;
  Vector<ValueExpression> group_exprs_;
  SHA1Hash expression_fingerprint_;
  ScopedPtr<TableExpression> input_;
  HashMap<SHA1Hash, Vector<VM::Instance>> groups_;
  HashMap<SHA1Hash, Vector<VM::Instance>>::iterator groups_iter_;
  ScratchMemory scratch_;
  VMStack vm_stack_;
};

class GroupByMergeExpression : public TableExpression {
public:

  static const size_t kOutputBatchSize = 1024;

  GroupByMergeExpression(
      Transaction* txn,
      ExecutionContext* execution_context,
      Vector<ValueExpression> select_expressions,
      eventql::ProcessConfig* config,
      eventql::ConfigDirectory* config_dir,
      size_t max_concurrent_tasks,
      size_t max_concurrent_tasks_per_host);

  ~GroupByMergeExpression();

  ReturnCode execute() override;
  ReturnCode nextBatch(SVector* columns, size_t* len) override;

  size_t getColumnCount() const override;
  SType getColumnType(size_t idx) const override;

  void addPart(GroupByNode* node, std::vector<std::string> hosts);

protected:
  Transaction* txn_;
  ExecutionContext* execution_context_;
  Vector<ValueExpression> select_exprs_;
  eventql::native_transport::TCPAsyncClient rpc_scheduler_;
  HashMap<SHA1Hash, Vector<VM::Instance>> groups_;
  HashMap<SHA1Hash, Vector<VM::Instance>>::iterator groups_iter_;
  ScratchMemory scratch_;
  VMStack vm_stack_;
  size_t num_parts_;
};

}
