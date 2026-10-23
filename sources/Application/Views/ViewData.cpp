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

#include "ViewData.h"
#include "Application/Persistency/PersistenceConstants.h"
#include "BaseClasses/View.h"
#include <limits>

ViewData::ViewData(Project *project) : Observable(&observers_) {
  Load(project);
};

ViewData::~ViewData() {}

void ViewData::Load(Project *project) {
  project_ = project;
  song_ = &(project->song_);
  currentChain_ = 0;
  currentPhrase_ = 0;
  songX_ = 0;
  songY_ = 0;
  songOffset_ = 0;
  chainCol_ = 0;
  chainRow_ = 0;
  currentTable_ = 0;
  currentInstrumentID_ = 0;
  currentGroove_ = 0;
  playMode_ = PM_SONG;
  phraseCurPos_ = 0;

  for (int i = 0; i < SONG_CHANNEL_COUNT; i++) {
    songPlayPos_[i] = 0;
    chainPlayPos_[i] = 0;
    phrasePlayPos_[i] = 0;
    currentPlayChain_[i] = 0xFF;
    currentPlayPhrase_[i] = 0xFF;
  }

  sampleEditorFilename.clear();
  isShowingSampleEditorProjectPool = false;
  importViewStartDir = SAMPLES_LIB_DIR;
}

unsigned char ViewData::UpdateSongChain(int offset) {
  const int channelCount = song_->GetChannelCount();
  unsigned char *c =
      song_->data_ + songX_ + channelCount * (songOffset_ + songY_);
  updateData(c, offset, CHAIN_COUNT - 1, false);
  return *c;
}

void ViewData::SetSongChain(unsigned char value) {
  const int channelCount = song_->GetChannelCount();
  unsigned char *c =
      song_->data_ + songX_ + channelCount * (songOffset_ + songY_);
  *c = value;
}

uint32_t ViewData::GetSongOffset() const {
  return static_cast<uint32_t>(songOffset_);
}

void ViewData::SetSongOffset(uint32_t offset) {
  const uint32_t previousOffset = GetSongOffset();
  if (offset > static_cast<uint32_t>(SONG_ROW_COUNT)) {
    songOffset_ = SONG_ROW_COUNT;
  } else {
    songOffset_ = static_cast<int>(offset);
  }
  checkSongBoundaries();
  NotifySongOffsetChanged(previousOffset);
}

void ViewData::UpdateSongOffset(int32_t offset) {
  const uint32_t previousOffset = GetSongOffset();
  const int64_t nextOffset = static_cast<int64_t>(songOffset_) + offset;
  if (nextOffset > std::numeric_limits<int32_t>::max()) {
    songOffset_ = std::numeric_limits<int32_t>::max();
  } else if (nextOffset < std::numeric_limits<int32_t>::min()) {
    songOffset_ = std::numeric_limits<int32_t>::min();
  } else {
    songOffset_ = static_cast<int>(nextOffset);
  }
  checkSongBoundaries();
  NotifySongOffsetChanged(previousOffset);
}

void ViewData::UpdateSongCursor(int dx, int dy) {
  const uint32_t previousOffset = GetSongOffset();
  songX_ += dx;
  songY_ += dy;
  checkSongBoundaries();
  NotifySongOffsetChanged(previousOffset);
}

void ViewData::RefreshSongViewport() {
  const uint32_t offset = GetSongOffset();
  SongViewportEvent event = {
      .previousOffset_ = offset,
      .offset_ = offset,
  };
  SetChanged();
  NotifyObservers(&event);
}

void ViewData::NotifySongOffsetChanged(uint32_t previousOffset) {
  const uint32_t offset = GetSongOffset();
  if (offset == previousOffset) {
    return;
  }

  SongViewportEvent event = {
      .previousOffset_ = previousOffset,
      .offset_ = offset,
  };
  SetChanged();
  NotifyObservers(&event);
}

void ViewData::checkSongBoundaries() {
  const int channelCount = song_->GetChannelCount();
  if (songX_ > channelCount - 1)
    songX_ = channelCount - 1;
  if (songX_ < 0)
    songX_ = 0;
  if (songY_ < 0) {
    songOffset_ += songY_;
    songY_ = 0;
  };
  if (songY_ > View::songRowCount_ - 1) {
    songOffset_ += songY_ - View::songRowCount_ + 1;
    songY_ = View::songRowCount_ - 1;
  };
  if (songOffset_ > SONG_ROW_COUNT - View::songRowCount_) {
    songOffset_ = SONG_ROW_COUNT - View::songRowCount_;
  }
  if (songOffset_ < 0) {
    songOffset_ = 0;
  }
}

unsigned char *ViewData::GetCurrentSongPointer() {
  const int channelCount = song_->GetChannelCount();
  return song_->data_ + songX_ + channelCount * (songOffset_ + songY_);
};

unsigned char ViewData::UpdateChainCursorValue(int offset, int dx, int dy) {

  unsigned char *c = 0;
  unsigned char limit = 0;
  bool wrap = false;

  switch (chainCol_ + dx) {
  case 0:
    c = song_->chain_.data_ + (16 * currentChain_ + chainRow_ + dy);
    limit = PHRASE_COUNT - 1;
    wrap = false;
    break;
  case 1:
    c = song_->chain_.transpose_ + (16 * currentChain_ + chainRow_ + dy);
    limit = 0xFF;
    wrap = true;
    break;
  }
  updateData(c, offset, limit, wrap);
  return *c;
}

void ViewData::UpdateChainCursor(int dx, int dy) {
  chainCol_ += dx;
  chainRow_ += dy;
  if (chainCol_ > 1)
    chainCol_ = 1;
  if (chainCol_ < 0)
    chainCol_ = 0;
  if (chainRow_ > 15)
    chainRow_ = 15;
  if (chainRow_ < 0)
    chainRow_ = 0;
}

void ViewData::SetChainPhrase(unsigned char value) {
  unsigned char *c = song_->chain_.data_ + (16 * currentChain_ + chainRow_);
  *c = value;
}

unsigned char *ViewData::GetCurrentChainPointer() {
  return song_->chain_.data_ + (16 * currentChain_ + chainRow_);
};
