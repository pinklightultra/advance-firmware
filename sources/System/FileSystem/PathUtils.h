/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _FILESYSTEM_PATHUTILS_H_
#define _FILESYSTEM_PATHUTILS_H_

#include "Externals/etl/include/etl/string.h"
#include "Externals/etl/include/etl/vector.h"
#include "System/FileSystem/FileSystem.h"
#include <cstddef>
#include <cstring>

namespace FileSystemPath {

using PathString = etl::string<PFILENAME_SIZE>;

namespace detail {

inline bool appendChar(PathString &out, const char ch) {
  if (out.full()) {
    return false;
  }
  out.push_back(ch);
  return true;
}

inline bool appendSpan(PathString &out, const char *data, const size_t length) {
  if (data == nullptr) {
    return false;
  }
  if ((out.size() + length) > out.capacity()) {
    return false;
  }
  out.append(data, length);
  return true;
}

inline bool segmentEquals(const char *segment, const size_t length,
                          const char *expected) {
  return std::strlen(expected) == length &&
         std::strncmp(segment, expected, length) == 0;
}

} // namespace detail

inline bool IsAbsolutePath(const char *path) {
  return (path != nullptr) && (path[0] == '/');
}

inline bool NormalizePath(const char *path, PathString &out) {
  if (path == nullptr || path[0] == '\0') {
    return false;
  }

  const bool isAbsolute = IsAbsolutePath(path);
  etl::vector<size_t, PFILENAME_SIZE / 2> segmentStarts;
  out.clear();

  if (isAbsolute && !detail::appendChar(out, '/')) {
    return false;
  }

  size_t cursor = isAbsolute ? 1 : 0;
  while (true) {
    while (path[cursor] == '/') {
      ++cursor;
    }

    const size_t segmentStart = cursor;
    while (path[cursor] != '\0' && path[cursor] != '/') {
      ++cursor;
    }
    const size_t segmentLength = cursor - segmentStart;

    if (segmentLength > 0) {
      const char *segment = path + segmentStart;
      if (detail::segmentEquals(segment, segmentLength, ".")) {
        // Drop no-op segments.
      } else if (detail::segmentEquals(segment, segmentLength, "..")) {
        if (segmentStarts.empty()) {
          return false;
        }
        out.resize(segmentStarts.back());
        segmentStarts.pop_back();
        if (out.size() > 1 && out.back() == '/') {
          out.pop_back();
        }
        if (isAbsolute && out.empty()) {
          if (!detail::appendChar(out, '/')) {
            return false;
          }
        }
      } else {
        if (isAbsolute) {
          if (out.size() > 1 && out.back() != '/') {
            if (!detail::appendChar(out, '/')) {
              return false;
            }
          }
        } else if (!out.empty()) {
          if (!detail::appendChar(out, '/')) {
            return false;
          }
        }
        const size_t outputSegmentStart = out.size();
        if (!detail::appendSpan(out, segment, segmentLength)) {
          return false;
        }
        segmentStarts.push_back(outputSegmentStart);
      }
    }

    if (path[cursor] == '\0') {
      break;
    }
    ++cursor;
  }

  if (out.empty()) {
    return detail::appendChar(out, isAbsolute ? '/' : '.');
  }

  return true;
}

inline bool JoinPath(const char *baseDir, const char *leaf, PathString &out) {
  if (!IsAbsolutePath(baseDir) || leaf == nullptr || leaf[0] == '\0' ||
      IsAbsolutePath(leaf)) {
    return false;
  }

  PathString combined;
  if (!NormalizePath(baseDir, combined)) {
    return false;
  }
  if (combined.size() > 1 && !detail::appendChar(combined, '/')) {
    return false;
  }
  if (!detail::appendSpan(combined, leaf, std::strlen(leaf))) {
    return false;
  }

  return NormalizePath(combined.c_str(), out);
}

inline bool DirName(const char *path, PathString &out) {
  PathString normalized;
  if (!NormalizePath(path, normalized)) {
    return false;
  }

  if (normalized == "/") {
    out = normalized;
    return true;
  }

  const char *lastSlash = std::strrchr(normalized.c_str(), '/');
  if (lastSlash == nullptr) {
    out = ".";
    return true;
  }

  const ptrdiff_t prefixLength = lastSlash - normalized.c_str();
  if (prefixLength <= 0) {
    out = "/";
    return true;
  }

  out.assign(normalized.c_str(), static_cast<size_t>(prefixLength));
  return true;
}

inline bool BaseName(const char *path, PathString &out) {
  PathString normalized;
  if (!NormalizePath(path, normalized)) {
    return false;
  }

  if (normalized == "/") {
    out = "/";
    return true;
  }

  const char *lastSlash = std::strrchr(normalized.c_str(), '/');
  if (lastSlash == nullptr) {
    out = normalized;
    return true;
  }

  out = lastSlash + 1;
  return !out.empty();
}

inline bool IsParentOf(const char *parent, const char *child) {
  PathString normalizedParent;
  PathString normalizedChild;
  if (!NormalizePath(parent, normalizedParent) ||
      !NormalizePath(child, normalizedChild)) {
    return false;
  }
  if (!IsAbsolutePath(normalizedParent.c_str()) ||
      !IsAbsolutePath(normalizedChild.c_str()) ||
      normalizedParent == normalizedChild) {
    return false;
  }
  if (normalizedParent == "/") {
    return normalizedChild != "/";
  }
  if (normalizedChild.size() <= normalizedParent.size()) {
    return false;
  }
  return std::strncmp(normalizedChild.c_str(), normalizedParent.c_str(),
                      normalizedParent.size()) == 0 &&
         normalizedChild[normalizedParent.size()] == '/';
}

} // namespace FileSystemPath

#endif // _FILESYSTEM_PATHUTILS_H_
