/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * DesktopFileSystem.cpp - Modern std::filesystem implementation for picoTracker Advance on Desktop
 */

#include "DesktopFileSystem.h"
#include "System/Console/Trace.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// DesktopFile
// ---------------------------------------------------------------------------

DesktopFile::DesktopFile(FILE *f) : f_(f) {}

DesktopFile::~DesktopFile() { Close(); }

int DesktopFile::Read(void *ptr, int size) {
  if (!f_ || !ptr || size <= 0)
    return -1;
  return (int)fread(ptr, 1, size, f_);
}

int DesktopFile::GetC() {
  if (!f_)
    return -1;
  return fgetc(f_);
}

int DesktopFile::Write(const void *ptr, int size, int nmemb) {
  if (!f_ || !ptr || size <= 0 || nmemb <= 0)
    return 0;
  return (int)fwrite(ptr, size, nmemb, f_);
}

void DesktopFile::Seek(long offset, int whence) {
  if (f_) {
    fseek(f_, offset, whence);
  }
}

long DesktopFile::Tell() {
  if (!f_)
    return -1;
  return ftell(f_);
}

bool DesktopFile::Close() {
  if (f_) {
    fclose(f_);
    f_ = nullptr;
    return true;
  }
  return false;
}

int DesktopFile::Error() {
  if (!f_)
    return -1;
  return ferror(f_);
}

bool DesktopFile::Sync() {
  if (!f_)
    return false;
  return fflush(f_) == 0;
}

void DesktopFile::Dispose() {
  delete this;
}

// ---------------------------------------------------------------------------
// DesktopFileSystem
// ---------------------------------------------------------------------------

static std::string toLowerString(const std::string &s) {
  std::string res = s;
  std::transform(res.begin(), res.end(), res.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  return res;
}

DesktopFileSystem::DesktopFileSystem() {
  std::error_code ec;
  fs::path rootPath = fs::current_path(ec) / "data";
  if (!fs::exists(rootPath, ec)) {
    fs::create_directories(rootPath, ec);
  }
  rootDir_ = rootPath.lexically_normal().string();

  // Create standard directories
  fs::create_directories(rootPath / "projects", ec);
  fs::create_directories(rootPath / "samples", ec);
  fs::create_directories(rootPath / "instruments", ec);
  fs::create_directories(rootPath / "renders", ec);
  fs::create_directories(rootPath / "themes", ec);
  fs::create_directories(rootPath / "recordings", ec);

  Trace::Log("FILESYSTEM", "Desktop virtual SD root set to: %s", rootDir_.c_str());
}

DesktopFileSystem::~DesktopFileSystem() {}

std::string DesktopFileSystem::resolvePath(const char *path) {
  if (!path || path[0] == '\0') {
    return rootDir_;
  }

  std::string p(path);
  // Normalize forward/back slashes
  std::replace(p.begin(), p.end(), '\\', '/');

  // Strip leading slashes to prevent resolving outside rootDir_
  while (!p.empty() && p.front() == '/') {
    p.erase(p.begin());
  }

  fs::path rel(p);
  fs::path full = (fs::path(rootDir_) / rel).lexically_normal();
  return full.string();
}

FileHandle DesktopFileSystem::Open(const char *name, const char *mode) {
  if (!name || !mode) {
    return FileHandle();
  }

  std::string resolved = resolvePath(name);
  std::error_code ec;

  // If opening for write, ensure parent directory exists
  if (mode[0] == 'w' || mode[0] == 'a' || (strchr(mode, '+') != nullptr)) {
    fs::path parent = fs::path(resolved).parent_path();
    if (!parent.empty() && !fs::exists(parent, ec)) {
      fs::create_directories(parent, ec);
    }
  }

  // Ensure binary mode on Windows
  std::string modeStr(mode);
  if (modeStr.find('b') == std::string::npos) {
    modeStr += 'b';
  }

  FILE *f = fopen(resolved.c_str(), modeStr.c_str());
  if (!f) {
    return FileHandle();
  }

  return MakeFileHandle(new DesktopFile(f));
}

bool DesktopFileSystem::ListDirectory(const char *absoluteDir,
                                      etl::ivector<FileSystemEntry> *entries,
                                      const char *filter, bool directoriesOnly,
                                      bool includeHidden) {
  if (!entries) {
    return false;
  }
  entries->clear();

  std::string resolved = resolvePath(absoluteDir);
  std::error_code ec;

  if (!fs::exists(resolved, ec) || !fs::is_directory(resolved, ec)) {
    SetLastListDirectoryStatus(DirectoryListStatus::Error);
    return false;
  }

  SetLastListDirectoryStatus(DirectoryListStatus::Ok);

  for (const auto &dirEntry : fs::directory_iterator(resolved, ec)) {
    std::string filename = dirEntry.path().filename().string();
    if (filename.empty()) continue;

    bool isHidden = (filename[0] == '.');
    if (!includeHidden && isHidden) {
      continue;
    }

    bool isDir = dirEntry.is_directory(ec);
    if (directoriesOnly && !isDir) {
      continue;
    }

    // Filter applies to files only (directories are always listed to allow browsing)
    if (!isDir && filter && filter[0] != '\0') {
      std::string lowerName = toLowerString(filename);
      std::string lowerFilter = toLowerString(filter);
      if (lowerName.find(lowerFilter) == std::string::npos) {
        continue;
      }
    }

    if (entries->full()) {
      SetLastListDirectoryStatus(DirectoryListStatus::Truncated);
      break;
    }

    FileSystemEntry entry;
    entry.name = filename.c_str();
    entry.type = isDir ? PFT_DIR : PFT_FILE;
    entry.size = isDir ? 0 : dirEntry.file_size(ec);
    entry.isHidden = isHidden;
    entries->push_back(entry);
  }

  return true;
}

bool DesktopFileSystem::DeleteFile(const char *name) {
  if (!name) return false;
  std::string resolved = resolvePath(name);
  std::error_code ec;
  return fs::remove(resolved, ec);
}

bool DesktopFileSystem::DeleteDir(const char *name) {
  if (!name) return false;
  std::string resolved = resolvePath(name);
  std::error_code ec;
  return fs::remove_all(resolved, ec) > 0;
}

bool DesktopFileSystem::exists(const char *path) {
  if (!path) return false;
  std::string resolved = resolvePath(path);
  std::error_code ec;
  return fs::exists(resolved, ec);
}

bool DesktopFileSystem::makeDir(const char *path, bool pFlag) {
  if (!path) return false;
  std::string resolved = resolvePath(path);
  std::error_code ec;
  return fs::create_directories(resolved, ec);
}

bool DesktopFileSystem::CopyFile(const char *srcFilename, const char *destFilename) {
  if (!srcFilename || !destFilename) return false;
  std::string srcResolved = resolvePath(srcFilename);
  std::string destResolved = resolvePath(destFilename);
  std::error_code ec;
  fs::path parent = fs::path(destResolved).parent_path();
  if (!parent.empty() && !fs::exists(parent, ec)) {
    fs::create_directories(parent, ec);
  }
  return fs::copy_file(srcResolved, destResolved,
                        fs::copy_options::overwrite_existing, ec);
}

bool DesktopFileSystem::MoveFile(const char *srcFilename, const char *destFilename) {
  if (!srcFilename || !destFilename) return false;
  std::string srcResolved = resolvePath(srcFilename);
  std::string destResolved = resolvePath(destFilename);
  std::error_code ec;
  fs::path parent = fs::path(destResolved).parent_path();
  if (!parent.empty() && !fs::exists(parent, ec)) {
    fs::create_directories(parent, ec);
  }
  fs::rename(srcResolved, destResolved, ec);
  return !ec;
}
