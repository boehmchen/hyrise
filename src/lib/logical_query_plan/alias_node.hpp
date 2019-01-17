#pragma once

#include "abstract_lqp_node.hpp"  // NEEDEDINCLUDE
#include "enable_make_for_lqp_node.hpp"

namespace opossum {

/**
 * Assign column names to expressions
 */
class AliasNode : public EnableMakeForLQPNode<AliasNode>, public AbstractLQPNode {
 public:
  AliasNode(const std::vector<std::shared_ptr<AbstractExpression>>& expressions,
            const std::vector<std::string>& aliases);

  std::string description() const override;
  const std::vector<std::shared_ptr<AbstractExpression>>& column_expressions() const override;

  const std::vector<std::string> aliases;

 protected:
  std::shared_ptr<AbstractLQPNode> _on_shallow_copy(LQPNodeMapping& node_mapping) const override;
  bool _on_shallow_equals(const AbstractLQPNode& rhs, const LQPNodeMapping& node_mapping) const override;
};

}  // namespace opossum
