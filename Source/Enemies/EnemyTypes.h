#pragma once
// Concrete enemy archetypes for the vertical slice (§20). Each tests a
// different AI behavior: melee pouncer, flying swooper, ranged kiter, swarm
// diver. The full 50+ roster plugs into the same base class + JSON data.
#include "Enemies/Enemy.h"

namespace ink {

// Melee: patrols, telegraphs, lunges. Contact damage.
class Slome : public Enemy {
public:
    std::string ArtName() const override;
    int ArtFrame() const override;
protected:
    void OnStateEnter(BrainState s, SimContext& ctx) override;
    void UpdateAI(double dt, SimContext& ctx) override;
};

// Flying: hovers, telegraphs, swoops in a straight line.
class InkBat : public Enemy {
public:
    std::string ArtName() const override;
    int ArtFrame() const override;
protected:
    void OnStateEnter(BrainState s, SimContext& ctx) override;
    void UpdateAI(double dt, SimContext& ctx) override;
    double hoverBaseY_ = 0.0;
    Vec2 swoopDir_{1, 0};
};

// Ranged: keeps distance, fires parryable quill shots.
class QuillGunner : public Enemy {
public:
    std::string ArtName() const override;
    int ArtFrame() const override;
protected:
    void OnStateEnter(BrainState s, SimContext& ctx) override;
    void UpdateAI(double dt, SimContext& ctx) override;
};

// Swarm: weak, fast, dives in a wavy line. Contact damage.
class PaperWisp : public Enemy {
public:
    std::string ArtName() const override;
    int ArtFrame() const override;
protected:
    void OnStateEnter(BrainState s, SimContext& ctx) override;
    void UpdateAI(double dt, SimContext& ctx) override;
};

} // namespace ink
