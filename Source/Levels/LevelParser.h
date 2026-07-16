#pragma once
// ASCII level format. A level file looks like:
//
//   [level]
//   id=meadows_01
//   name=The Whispering Fields
//   world=meadows
//   music=meadows
//   exit=The meadow gate swings shut.
//   [map]
//   ..........................................
//   ...
//   [entities]
//   spawn slome 300 128
//   npc birdie 260 112
//   plat 480 208 560 208 40
//
// Map legend:
//   '.' empty   '#' solid   '=' one-way   '^' hazard   '?' breakable
//   'P' player spawn  'C' checkpoint  'D' door (solid until boss dead)
//   'i' ink coin  'f' film strip  'm' master frame  's' hidden stamp
//   'B' boss spawn (marks boss arena start)  'b' mini-boss spawn
#include "Gameplay/Level.h"
#include <string>

namespace ink {

bool ParseLevelText(const std::string& text, LevelData& out, std::string* err = nullptr);
std::string ReadFile(const std::string& path, std::string* err = nullptr);

} // namespace ink
