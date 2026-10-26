#include "SDLAudioDriver.h"
#include "Services/Midi/MidiService.h"
#include "System/Console/Trace.h"
#include "System/System/System.h"
#include <cstdlib>
#include <cstring>
#include <cstdint>

static void sdl_audio_callback(void *userdata, Uint8 *stream, int len) {
  SDLAudioDriver *sound = (SDLAudioDriver *)userdata;
  sound->OnChunkDone(stream, len);
}

SDLAudioDriverThread::SDLAudioDriverThread(SDLAudioDriver *driver) {
  semaphore_ = SysSemaphore::Create(0, 1024);
  driver_ = driver;
}

bool SDLAudioDriverThread::Execute() {
  while (!shouldTerminate()) {
    semaphore_->Wait();
    driver_->OnNewBufferNeeded();
    MidiService::GetInstance()->Flush();
  }
  SysSemaphore *semaphore = semaphore_;
  semaphore_ = nullptr;
  delete semaphore;
  return true;
}

void SDLAudioDriverThread::Notify() {
  if (semaphore_) {
    semaphore_->Post();
  }
}

void SDLAudioDriverThread::RequestTermination() {
  SysThread::RequestTermination();
  if (semaphore_) {
    semaphore_->Post();
  }
  SDL_Delay(10);
}

// ---------------------------------------------------------------------------

SDLAudioDriver::SDLAudioDriver(AudioSettings &settings)
    : AudioDriver(settings), unalignedMain_(nullptr), mainBuffer_(nullptr), miniBlank_(nullptr) {
  isPlaying_ = false;
  thread_ = nullptr;
  bufferPos_ = 0;
  bufferSize_ = 0;
  fragSize_ = 0;
  sampleRate_ = 44100;
  volume_ = 100;
  startTime_ = 0;
}

SDLAudioDriver::~SDLAudioDriver() {}

bool SDLAudioDriver::InitDriver() {
  SDL_AudioSpec wanted;
  SDL_AudioSpec obtained;

  wanted.freq = 44100;
  wanted.format = AUDIO_S16SYS;
  wanted.channels = 2;
  wanted.callback = sdl_audio_callback;
  wanted.samples = settings_.bufferSize_ ? settings_.bufferSize_ : 1024;
  wanted.userdata = this;

  if (SDL_OpenAudio(&wanted, &obtained) < 0) {
    Trace::Error("AUDIO", "Couldn't open SDL audio: %s", SDL_GetError());
    return false;
  }

  fragSize_ = obtained.size;
  sampleRate_ = obtained.freq;

  unalignedMain_ = (char *)malloc(fragSize_ + SOUND_BUFFER_MAX + 16);
  mainBuffer_ = (char *)(((uintptr_t)unalignedMain_ + 15) & ~((uintptr_t)15));

  Trace::Log("AUDIO", "SDL audio opened with %d samples at %d Hz",
             fragSize_ / 4, obtained.freq);

  miniBlank_ = (char *)malloc(fragSize_);
  memset(miniBlank_, 0, fragSize_);

  return true;
}

void SDLAudioDriver::CloseDriver() {
  if (miniBlank_) {
    free(miniBlank_);
    miniBlank_ = nullptr;
  }

  if (unalignedMain_) {
    free(unalignedMain_);
    unalignedMain_ = nullptr;
    mainBuffer_ = nullptr;
  }
  SDL_CloseAudio();
}

bool SDLAudioDriver::StartDriver() {
  thread_ = new SDLAudioDriverThread(this);
  thread_->Start();

  bufferPos_ = 0;
  bufferSize_ = 0;

  for (int i = 0; i < settings_.preBufferCount_; i++) {
    AddBuffer((short *)miniBlank_, fragSize_ / 4);
    MidiService::GetInstance()->AdvancePlayQueue();
  }
  if (settings_.preBufferCount_ == 0) {
    thread_->Notify();
  }

  SDL_PauseAudio(0);
  startTime_ = SDL_GetTicks();

  return true;
}

void SDLAudioDriver::StopDriver() {
  if (thread_) {
    thread_->RequestTermination();
    SysThread *thread = thread_;
    thread_ = nullptr;
    SDL_PauseAudio(1);
    delete thread;
  }
}

double SDLAudioDriver::GetStreamTime() {
  return (SDL_GetTicks() - startTime_) / 1000.0;
}

void SDLAudioDriver::OnChunkDone(Uint8 *stream, int len) {
  while (bufferSize_ - bufferPos_ < len) {
    memmove(mainBuffer_, mainBuffer_ + bufferPos_, bufferSize_ - bufferPos_);
    bufferSize_ = bufferSize_ - bufferPos_;
    bufferPos_ = 0;

    if (pool_[poolPlayPosition_].empty_) {
      memcpy(mainBuffer_ + bufferSize_, miniBlank_, fragSize_);
      bufferSize_ += fragSize_;
    } else {
      int chunkLen = pool_[poolPlayPosition_].size_;
      memcpy(mainBuffer_ + bufferSize_, pool_[poolPlayPosition_].buffer_, chunkLen);
      bufferSize_ += chunkLen;

      pool_[poolPlayPosition_].empty_ = true;
      pool_[poolPlayPosition_].size_ = 0;
    }

    onAudioBufferTick();
    MidiService::GetInstance()->Flush();

    poolPlayPosition_ = (poolPlayPosition_ + 1) % SOUND_BUFFER_COUNT;
    if (thread_) {
      thread_->Notify();
    }
  }

  memcpy(stream, mainBuffer_ + bufferPos_, len);
  bufferPos_ += len;
}

int SDLAudioDriver::GetPlayedBufferPercentage() { return 0; }

int SDLAudioDriver::GetSampleRate() { return sampleRate_ ? sampleRate_ : 44100; }
