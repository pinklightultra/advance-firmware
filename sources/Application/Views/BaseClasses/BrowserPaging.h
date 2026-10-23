/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

#ifndef _BROWSER_PAGING_H_
#define _BROWSER_PAGING_H_

#include <algorithm>
#include <cstddef>

namespace BrowserPaging {

inline void Warp(size_t &currentIndex, size_t &topIndex, size_t totalItems,
                 size_t pageSize, bool goUp, bool page = false) {
  if (totalItems == 0) {
    currentIndex = 0;
    topIndex = 0;
    return;
  }

  if (page) {
    if (goUp) {
      if (currentIndex >= pageSize) {
        currentIndex -= pageSize;
      } else {
        currentIndex = 0;
      }
      if (topIndex >= pageSize) {
        topIndex -= pageSize;
      } else {
        topIndex = 0;
      }
    } else {
      if (currentIndex + pageSize < totalItems) {
        currentIndex += pageSize;
      } else {
        currentIndex = totalItems - 1;
      }
      if (totalItems > pageSize) {
        const size_t maxTop = totalItems - pageSize;
        topIndex = std::min(topIndex + pageSize, maxTop);
      } else {
        topIndex = 0;
      }
    }
  } else {
    if (goUp) {
      if (currentIndex > 0) {
        currentIndex--;
        if (currentIndex < topIndex) {
          topIndex = currentIndex;
        }
      }
    } else {
      if (currentIndex < totalItems - 1) {
        currentIndex++;
        if (currentIndex >= topIndex + pageSize) {
          topIndex = currentIndex - pageSize + 1;
        }
      }
    }
  }

  // Ensure invariants: topIndex <= currentIndex < topIndex + pageSize
  if (currentIndex < topIndex) {
    topIndex = currentIndex;
  } else if (currentIndex >= topIndex + pageSize) {
    topIndex = currentIndex - pageSize + 1;
  }
}

} // namespace BrowserPaging

#endif // _BROWSER_PAGING_H_
