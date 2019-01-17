#pragma once

#include "base_non_query_node.hpp"  // NEEDEDINCLUDE
#include "enable_make_for_lqp_node.hpp"

namespace opossum {

class AbstractExpression;

/**
 * Node type to represent updates (i.e., invalidation and inserts) in a table.
 */
class UpdateNode : public EnableMakeForLQPNode<UpdateNode>, public BaseNonQueryNode {
 public:
  explicit UpdateNode(const std::string& table_name);

  std::string description() const override;

  const std::string table_name;

 protected:
  std::shared_ptr<AbstractLQPNode> _on_shallow_copy(LQPNodeMapping& node_mapping) const override;
  bool _on_shallow_equals(const AbstractLQPNode& rhs, const LQPNodeMapping& node_mapping) const override;
};

}  // namespace opossum
