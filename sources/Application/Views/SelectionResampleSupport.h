/*
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Copyright (c) 2026 xiphonics, inc.
 *
 * This file is part of the picoTracker firmware
 */

#ifndef _SELECTION_RESAMPLE_SUPPORT_H_
#define _SELECTION_RESAMPLE_SUPPORT_H_

#include "Application/Mixer/MixerService.h"
#include "Application/Player/SelectionResampleRequest.h"
#include "UIFramework/BasicDatas/GUIRect.h"

class Project;
class View;
class ViewData;

namespace SelectionResampleSupport {

bool BuildNextProjectSamplePath(Project *project,
                                SelectionResampleRequest &request);
bool BuildNextMixdownPath(SelectionResampleRequest &request);
bool BuildSelectionRequest(const GUIRect &selectionRect,
                           const ViewData &viewData,
                           SelectionResampleRequest &request,
                           SelectionOutputIntent outputIntent);
void ClearProjectSamplePathError();
const char *GetProjectSamplePathError();
MixerServiceMode GetMixerServiceMode(SelectionOutputIntent outputIntent);
bool StartSelectionResample(View &view,
                            const SelectionResampleRequest &request);
void ShowResampleError(View &view, const char *message);

} // namespace SelectionResampleSupport

#endif
