#include "abstract_read_only_operator.hpp"  // NEEDEDINCLUDE

namespace opossum {

std::shared_ptr<const Table> AbstractReadOnlyOperator::_on_execute(std::shared_ptr<TransactionContext>) {
  return _on_execute();
}

}  // namespace opossum
