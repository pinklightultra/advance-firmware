/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * DesktopFileSystem.h - Modern std::filesystem implementation for picoTracker Advance on Desktop
 */

#ifndef _DESKTOP_FILESYSTEM_H_
#define _DESKTOP_FILESYSTEM_H_

#include "System/FileSystem/FileSystem.h"
#include "System/FileSystem/I_File.h"
#include "System/FileSystem/FileHandle.h"
#include <stdio.h>
#include <string>

class DesktopFile : public I_File {
public:
  DesktopFile(FILE *f);
  virtual ~DesktopFile();

  virtual int Read(void *ptr, int size) override;
  virtual int GetC() override;
  virtual int Write(const void *ptr, int size, int nmemb) override;
  virtual void Seek(long offset, int whence) override;
  virtual long Tell() override;
  virtual int Error() override;
  virtual bool Sync() override;
  virtual void Dispose() override;

protected:
  virtual bool Close() override;

private:
  FILE *f_;
};

class DesktopFileSystem : public FileSystem {
public:
  DesktopFileSystem();
  virtual ~DesktopFileSystem();

  virtual FileHandle Open(const char *name, const char *mode) override;
  virtual bool ListDirectory(const char *absoluteDir,
                             etl::ivector<FileSystemEntry> *entries,
                             const char *filter, bool directoriesOnly,
                             bool includeHidden = false) override;
  virtual bool DeleteFile(const char *name) override;
  virtual bool DeleteDir(const char *name) override;
  virtual bool exists(const char *path) override;
  virtual bool makeDir(const char *path, bool pFlag = false) override;
  virtual bool CopyFile(const char *srcFilename,
                        const char *destFilename) override;
  virtual bool MoveFile(const char *srcFilename,
                        const char *destFilename) override;
  virtual bool isExFat() override { return false; }

private:
  std::string rootDir_;
  std::string resolvePath(const char *path);
};

#endif // _DESKTOP_FILESYSTEM_H_
