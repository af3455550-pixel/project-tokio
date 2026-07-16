#include "AI/AIBrain.h"
#include <algorithm>

namespace ink {

const char* BrainStateName(BrainState s) {
    switch (s) {
    case BrainState::Idle: return "IDLE";
    case BrainState::Patrol: return "PATROL";
    case BrainState::Notice: return "NOTICE";
    case BrainState::Chase: return "CHASE";
    case BrainState::Attack: return "ATTACK";
    case BrainState::Recover: return "RECOVER";
    case BrainState::Stunned: return "STUNNED";
    case BrainState::Damaged: return "DAMAGED";
    case BrainState::Retreat: return "RETREAT";
    case BrainState::Dead: return "DEAD";
    }
    return "?";
}

} // namespace ink
