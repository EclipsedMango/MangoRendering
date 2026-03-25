
#include "PropertyHolder.h"
#include "PropertyRegistry.h"

static inline bool sRegisteredPropertyHolder = [] {
    PropertyRegistry::Register("PropertyHolder", []() -> PropertyHolder* {
        return new PropertyHolder();
    });
    return true;
}();