/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 */

#ifndef _BOUNDARY_TEST_UTILS_H_
#define _BOUNDARY_TEST_UTILS_H_

#include "Application/Model/Config.h"
#include "Application/Persistency/PersistencyService.h"
#include "Externals/etl/include/etl/singleton.h"
#include "Foundation/Services/ServiceRegistry.h"
#include "System/FileSystem/FakeFileSystem.h"
#include <algorithm>

struct BoundaryTestEnvironment {
  BoundaryTestEnvironment() {
    etl::singleton<Config>::destroy();
    etl::singleton<PersistencyService>::destroy();
    etl::singleton<ServiceRegistry>::destroy();
    FileSystem::Install(&fs);
  }

  ~BoundaryTestEnvironment() {
    etl::singleton<Config>::destroy();
    etl::singleton<PersistencyService>::destroy();
    etl::singleton<ServiceRegistry>::destroy();
    FileSystem::Install(nullptr);
  }

  FakeFileSystem fs;
};

inline bool operationLogHasNoChdir(const FakeFileSystem &fs) {
  return std::none_of(fs.operationLog().begin(), fs.operationLog().end(),
                      [](const FakeFileSystemOperation &entry) {
                        return entry.type == FakeFileSystemOperationType::Chdir;
                      });
}

#endif
