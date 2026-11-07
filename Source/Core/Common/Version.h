// Copyright 2017 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

namespace Common
{
const std::string& GetEmulatorName();
const std::string& GetScmDescStr();
const std::string& GetScmBranchStr();
const std::string& GetScmRevStr();
const std::string& GetScmRevGitStr();
const std::string& GetUserAgentStr();
const std::string& GetScmDistributorStr();
const std::string& GetScmUpdateTrackStr();
const std::string& GetNetplayDolphinVer();

// Groovy MiSTer: the revision and version name NetPlay uses to decide whether two builds can
// play together. These are the official Dolphin commit this build is based on (when known), so
// this build can play with an official Dolphin build of that same commit.
const std::string& GetNetplayRevStr();
const std::string& GetNetplayDescStr();
int GetScmCommitsAheadMaster();
}  // namespace Common
