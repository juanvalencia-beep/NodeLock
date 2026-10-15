#include "nodelock/NodeLockEngine.h"

namespace nodelock {

NodeLockEngine::NodeLockEngine()
    : graph_(),
      validator_(graph_),
      enumerator_(graph_),
      stats_(enumerator_.countAll()),
      simulator_(enumerator_, validator_),
      analyzer_(graph_, validator_, stats_.countsByLength) {}

}  // namespace nodelock
