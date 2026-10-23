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

#include "SampleVariable.h"
#include "SamplePool.h"

SampleVariable::SampleVariable(FourCC id) : WatchedVariable(id, 0, 0, -1) {
  SamplePool *pool = SamplePool::GetInstance();
  list_.char_ = pool->GetNameList();
  listSize_ = pool->GetNameListSize();
  pool->AddObserver(*this);
};

SampleVariable::~SampleVariable() {
  SamplePool *pool = SamplePool::GetInstance();
  pool->RemoveObserver(*this);
};

void SampleVariable::Update(Observable &o, I_ObservableData *d) {
  SamplePoolEvent *e = (SamplePoolEvent *)d;
  // If a sample was removed, update our index and notify observers so
  // instruments retarget their sample pointer.
  if (e->type_ == SPET_DELETE) {
    int currentIndex = value_.index_;

    if (currentIndex == e->index_) {
      // Our own sample was deleted: clear the selection. This is a genuine
      // change, so let observers reset their trim points as usual.
      SetInt(-1); // triggers onChange/NotifyObservers
    } else if (currentIndex > e->index_) {
      // The pool shifted our (unchanged) sample down one index. Flag the change
      // as a re-index so instruments retarget their source pointer but preserve
      // the user's trim points.
      reindexing_ = true;
      SetInt(currentIndex - 1); // triggers onChange/NotifyObservers
      reindexing_ = false;
    }
  }
  // For inserts, just refresh list pointers below
  // indices remain valid since imports append at the end
  SamplePool *pool = (SamplePool *)&o;
  list_.char_ = pool->GetNameList();
  listSize_ = pool->GetNameListSize();
};
