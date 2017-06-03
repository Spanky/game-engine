#include "StableHeaders.h"
#include "GO_ComponentAccessControl.h"

std::vector<std::shared_ptr<GO::ComponentAccessFlags>> GO::ComponentAccessControl::ourOutstandingFlags;
std::mutex GO::ComponentAccessControl::ourStorageMutex;