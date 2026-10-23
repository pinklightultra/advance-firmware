/*
 * SPDX-License-Identifier: BSD-3-Clause AND GPL-3.0-only
 *
 * Copyright (c) 2024 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 *
 * Inherited BSD-licensed material remains BSD-3-Clause.
 * New original contributions by xiphonics, inc. are GPL-3.0-only.
 * Previously granted BSD permissions are preserved. See LICENSING.md.
 */

#ifndef _STRING_VARIABLE_H_
#define _STRING_VARIABLE_H_

#include "Variable.h"
#include "WatchedVariable.h"

// Lightweight string-capable variable without heap allocations.
// Stores string data in an internal ETL string buffer.
template <size_t MaxLen = MAX_VARIABLE_STRING_LENGTH>
class StringVariable : public Variable {
public:
  explicit StringVariable(FourCC id, const char *value = "") : Variable(id, 0) {
    type_ = STRING;
    stringValue_ = &storage_;
    setStringValue(value ? value : "");
    defaultValue_ = storage_;
  }

  void Reset() override {
    setStringValue(defaultValue_.c_str());
    onChange();
  }

  bool IsModified() override { return storage_ != defaultValue_; }

  // The base implementation only copies the value unions, which would leave
  // the string storage untouched; copy the string content instead.
  void CopyFrom(Variable &other) override {
    setStringValue(other.GetString().c_str());
    onChange();
  }

protected:
  using Variable::setStringValue;

private:
  etl::string<MaxLen> storage_;
  etl::string<MaxLen> defaultValue_;
};

// Like StringVariable, but the default value is a pointer to caller-owned
// storage instead of a per-instance copy, halving the footprint when many
// instances share the same default (the caller must keep the default string
// alive for the variable's lifetime).
template <size_t MaxLen = MAX_VARIABLE_STRING_LENGTH>
class SharedDefaultStringVariable : public Variable {
public:
  SharedDefaultStringVariable(FourCC id, const char *defaultValue)
      : Variable(id, 0), defaultValue_(defaultValue) {
    type_ = STRING;
    stringValue_ = &storage_;
    setStringValue(defaultValue);
  }

  void Reset() override {
    setStringValue(defaultValue_);
    onChange();
  }

  bool IsModified() override { return storage_.compare(defaultValue_) != 0; }

  // The base implementation only copies the value unions, which would leave
  // the string storage untouched; copy the string content instead.
  void CopyFrom(Variable &other) override {
    setStringValue(other.GetString().c_str());
    onChange();
  }

private:
  etl::string<MaxLen> storage_;
  const char *defaultValue_;
};

template <size_t MaxLen = MAX_VARIABLE_STRING_LENGTH>
class StringWatchedVariable : public WatchedVariable {
public:
  explicit StringWatchedVariable(FourCC id, const char *value = "")
      : WatchedVariable(id, false) {
    type_ = STRING;
    stringValue_ = &storage_;
    setStringValue(value ? value : "");
    defaultValue_ = storage_;
  }

  void Reset() override {
    setStringValue(defaultValue_.c_str());
    onChange();
  }

  bool IsModified() override { return storage_ != defaultValue_; }

  // The base implementation only copies the value unions, which would leave
  // the string storage untouched; copy the string content instead.
  void CopyFrom(Variable &other) override {
    setStringValue(other.GetString().c_str());
    onChange();
  }

protected:
  using Variable::setStringValue;

private:
  etl::string<MaxLen> storage_;
  etl::string<MaxLen> defaultValue_;
};

#endif
