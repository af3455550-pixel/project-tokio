#include "Enemies/Enemy.h"
#include "Enemies/EnemyTypes.h"
#include "Gameplay/Level.h"
#include "Gameplay/SimContext.h"
#include "Player/Player.h"
#include "Physics/Collision.h"
#include <cmath>

namespace ink {

void Enemy::Init(const EnemyDef& d, Vec2 pos_, int id_) {
    def = &d;
    id = id_;
    pos = pos_;
    homePos = pos_;
    w = d.w;
    h = d.h;
    hp = d.hp;
    maxHp = d.hp;
    alive = true;
    score = d.score;
    brain.Reset();
    anim.frame = 0;
    bobT = 0.0;
}

std::unique_ptr<Enemy> Enemy::Create(const std::string& type) {
    if (type == "slome")
        return std::make_unique<Slome>();
    if (type == "inkbat")
        return std::make_unique<InkBat>();
    if (type == "quillgunner")
        return std::make_unique<QuillGunner>();
    if (type == "wisp")
        return std::make_unique<PaperWisp>();
    return nullptr;
}

ProjectileDef Enemy::MakeEnemyShot() const {
    ProjectileDef d;
    d.id = "enemy_shot";
    d.speed = 300.0;
    d.gravity = 0.0;
    d.life = 2.4;
    d.radius = 4.0;
    d.damage = 1.0;
    d.knockback = 200.0;
    d.parryable = true;
    d.vfx = "quill";
    d.score = 0;
    return d;
}

Vec2 Enemy::PlayerDir(SimContext& ctx) const {
    if (!ctx.player)
        return {1, 0};
    return (ctx.player->Center() - Center()).Normalized();
}

double Enemy::PlayerDist(SimContext& ctx) const {
    if (!ctx.player)
        return 1e9;
    return Dist(Center(), ctx.player->Center());
}

bool Enemy::PlayerLOS(SimContext& ctx) const {
    if (!ctx.player || !ctx.level)
        return true;
    return ctx.level->RaycastLOS(Center(), ctx.player->Center());
}

void Enemy::ContactDamage(SimContext& ctx) {
    if (!ctx.player || !ctx.player->alive || !canDamagePlayer)
        return;
    if (!Box().Overlaps(ctx.player->Box()))
        return;
    // Parryable only during the telegraphed/dangerous part of an attack.
    bool parried = attackActive && ctx.player->TryParryMelee(ctx);
    if (parried) {
        brain.Stun(def ? def->stunDuration : 1.0);
        OnStateEnter(brain.Get(), ctx);
        return;
    }
    ctx.player->Damage(def ? def->attackDamage : 1, Center(), ctx, /*hazard=*/false);
}

void Enemy::FinishAttack(SimContext& ctx) {
    attackActive = false;
    attackCdT = def ? def->attackCooldown : 1.4;
    brain.Request(BrainState::Recover);
    OnStateEnter(BrainState::Recover, ctx);
}

void Enemy::Update(double dt, SimContext& ctx) {
    if (!def)
        return;
    if (!alive) {
        deadT += dt;
        vel.y += 900.0 * dt;
        pos += vel * dt;
        return;
    }
    TickTimers(dt);
    bobT += dt;
    if (attackCdT > 0.0)
        attackCdT -= dt;

    BrainView v;
    v.dt = dt;
    if (ctx.player && ctx.player->alive) {
        v.distToPlayer = PlayerDist(ctx);
        v.hasLOS = PlayerLOS(ctx);
    } else {
        v.playerAlive = false;
        v.distToPlayer = 1e9;
        v.hasLOS = false;
    }
    v.hpf = maxHp > 0 ? static_cast<double>(hp) / static_cast<double>(maxHp) : 0.0;

    BrainState prev = brain.Get();
    brain.Tick(v, def->Brain());
    if (brain.Get() != prev)
        OnStateEnter(brain.Get(), ctx);

    UpdateAI(dt, ctx);

    // Physics
    if (def->flying) {
        if (!attackActive)
            vel.y = std::sin(bobT * 3.0) * 18.0;
        vel.y = Clamp(vel.y, -200.0, 200.0);
    } else {
        vel.y += 1400.0 * dt;
        vel.y = std::min(vel.y, 640.0);
    }
    if (ctx.level) {
        MoveResult r = MoveEntity(pos, vel, Box(), dt, *ctx.level, false);
        pos = r.pos;
        vel = r.vel;
    }
    ContactDamage(ctx);
}

void Enemy::TakeDamage(double dmg, Vec2 knock, SimContext& ctx) {
    if (!alive)
        return;
    hp -= static_cast<int>(std::ceil(dmg));
    hitFlashT = 0.12;
    vel += knock;
    if (hp <= 0) {
        hp = 0;
        alive = false;
        OnKill(ctx);
        return;
    }
    brain.DamageFlash();
    OnStateEnter(brain.Get(), ctx);
}

void Enemy::OnKill(SimContext& ctx) {
    if (!def)
        return;
    deadT = 0.0;
    vel = {vel.x * 0.3, -180.0};
    ctx.Particles().InkSplash(Center(), 0xFF26213C, 16, 220);
    ctx.Particles().Sparks(Center(), 0xFFE8B84B, 6);
    ctx.Emit({Evt::EnemyKilled, id, score, Center(), def->id});
    ctx.Emit({Evt::Shake, id, 2, Center(), "kill"});
    if (ctx.player)
        ctx.player->AddEnergy(def->energyOnKill);
    // Coin drop
    if (ctx.rng && ctx.rng->chance(def->coinChance) && ctx.level) {
        ctx.level->Collectibles().push_back(
            {"coin", "drop_" + std::to_string(id), Center(), false, 0.0});
    }
}

} // namespace ink
