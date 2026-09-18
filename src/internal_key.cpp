#include "kv/internal_key.h"


namespace kv {

bool InternalKeyComparator::operator()(const InternalKey& lhs, const InternalKey& rhs) const {
  return (lhs.user_key != rhs.user_key
            ? lhs.user_key < rhs.user_key
            : (lhs.sequence != rhs.sequence
                 ? lhs.sequence > rhs.sequence
                 : lhs.type > rhs.type));
}

}