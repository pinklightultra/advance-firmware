/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _BROWSER_PATH_UTILS_H_
#define _BROWSER_PATH_UTILS_H_

#include "Application/Persistency/PersistenceConstants.h"
#include "Externals/etl/include/etl/string_stream.h"
#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/PathUtils.h"

namespace BrowserPathUtils {

enum class NextIndexedFilePathStatus : uint8_t {
  Found = 0,
  Exhausted,
  Error,
};

inline bool BuildProjectDirectory(const char *projectName,
                                  FileSystemPath::PathString &projectDir) {
  if (projectName == nullptr || projectName[0] == '\0') {
    return false;
  }

  return FileSystemPath::JoinPath(PROJECTS_DIR, projectName, projectDir);
}

inline bool
BuildProjectSamplesDirectory(const char *projectName,
                             FileSystemPath::PathString &projectSamplesDir) {
  FileSystemPath::PathString projectDir;
  if (!BuildProjectDirectory(projectName, projectDir)) {
    return false;
  }

  return FileSystemPath::JoinPath(projectDir.c_str(), PROJECT_SAMPLES_DIR,
                                  projectSamplesDir);
}

inline NextIndexedFilePathStatus
BuildNextIndexedFilePath(const char *directory, const char *prefix,
                         const char *extension, uint32_t maxIndex,
                         FileSystemPath::PathString &outputPath) {
  if (directory == nullptr || prefix == nullptr || extension == nullptr ||
      directory[0] == '\0' || prefix[0] == '\0' || extension[0] == '\0' ||
      maxIndex == 0) {
    return NextIndexedFilePathStatus::Error;
  }

  FileSystem *fs = FileSystem::GetInstance();
  if (fs == nullptr) {
    return NextIndexedFilePathStatus::Error;
  }

  uint32_t width = 1;
  for (uint32_t value = maxIndex; value >= 10; value /= 10) {
    ++width;
  }

  for (uint32_t index = 1; index <= maxIndex; ++index) {
    etl::string<32> leafName;
    etl::string_stream leafNameStream(leafName);
    leafNameStream << prefix << etl::setfill('0') << etl::setw(width) << index
                   << extension;
    if (leafName.is_truncated()) {
      return NextIndexedFilePathStatus::Error;
    }

    if (!FileSystemPath::JoinPath(directory, leafName.c_str(), outputPath)) {
      return NextIndexedFilePathStatus::Error;
    }

    if (!fs->exists(outputPath.c_str())) {
      return NextIndexedFilePathStatus::Found;
    }
  }

  return NextIndexedFilePathStatus::Exhausted;
}

inline bool ResolveParentDirectory(const char *currentDir, const char *rootDir,
                                   FileSystemPath::PathString &parentDir) {
  FileSystemPath::PathString normalizedCurrentDir;
  FileSystemPath::PathString normalizedRootDir;
  if (!FileSystemPath::NormalizePath(currentDir, normalizedCurrentDir) ||
      !FileSystemPath::NormalizePath(rootDir, normalizedRootDir) ||
      !FileSystemPath::IsAbsolutePath(normalizedCurrentDir.c_str()) ||
      !FileSystemPath::IsAbsolutePath(normalizedRootDir.c_str())) {
    return false;
  }

  if (normalizedCurrentDir == normalizedRootDir) {
    return false;
  }

  if (normalizedRootDir != "/" &&
      !FileSystemPath::IsParentOf(normalizedRootDir.c_str(),
                                  normalizedCurrentDir.c_str())) {
    return false;
  }

  if (!FileSystemPath::DirName(normalizedCurrentDir.c_str(), parentDir)) {
    return false;
  }

  if (normalizedRootDir != "/" && parentDir.size() < normalizedRootDir.size()) {
    parentDir = normalizedRootDir;
  }

  return true;
}

inline bool IsCurrentProjectImportDirectory(const char *candidateDir,
                                            const char *currentProjectName) {
  FileSystemPath::PathString projectDir;
  FileSystemPath::PathString normalizedCandidateDir;
  if (!BuildProjectDirectory(currentProjectName, projectDir)) {
    return false;
  }

  if (!FileSystemPath::NormalizePath(candidateDir, normalizedCandidateDir)) {
    return false;
  }

  return normalizedCandidateDir == projectDir;
}

inline bool PrependParentEntry(const char *currentDir, const char *rootDir,
                               etl::ivector<FileSystemEntry> *entries) {
  if (entries == nullptr) {
    return false;
  }

  FileSystemPath::PathString parentDir;
  if (!ResolveParentDirectory(currentDir, rootDir, parentDir)) {
    return false;
  }

  if (entries->full()) {
    entries->pop_back();
  }

  FileSystemEntry parentEntry;
  parentEntry.name = "..";
  parentEntry.type = PFT_DIR;
  parentEntry.size = 0;
  parentEntry.isHidden = false;
  entries->insert(entries->begin(), parentEntry);
  return true;
}

} // namespace BrowserPathUtils

#endif
