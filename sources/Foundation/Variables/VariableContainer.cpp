/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2018 Discodirt
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

#include "VariableContainer.h"
#include "config/MemorySections.h"
#include <string.h>

VariableContainer::VariableContainer(etl::ivector<Variable *> *list)
    : list_(list){};

VariableContainer::~VariableContainer(){};

PICOTRACKER_FAST_CODE("variable_container_find_id")
Variable *VariableContainer::FindVariable(FourCC id) {
  auto it = list_->begin();
  for (size_t i = 0; i < list_->size(); i++) {
    if ((*it)->GetID() == id) {
      return *it;
    }
    it++;
  }
  return NULL;
};

Variable *VariableContainer::FindVariable(const char *name) {
  auto it = list_->begin();
  for (size_t i = 0; i < list_->size(); i++) {
    if (!strcmp((*it)->GetName(), name)) {
      return *it;
    }
    it++;
  }
  return NULL;
};
