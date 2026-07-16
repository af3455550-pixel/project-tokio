#include "Bosses/Boss.h"
#include "Bosses/BossBarnaby.h"
#include "Bosses/BossPatch.h"
#include "Gameplay/Level.h"
#include "Core/Json.h"
#include "Gameplay/SimContext.h"
#include "Player/Player.h"
#include "Physics/Collision.h"
#include <cmath>

namespace ink {

std::unique_ptr<Boss> Boss::Create(const std::string& id) {
    if (id == "patchling")
        return std::make_unique<Patch>();
    if (id == "barnaby")
        return std::make_unique<Barnaby>();
    if (id == "patch")
        return std::make_unique<Patch>();
    return nullptr;
}

void Boss::Init(const BossDef& d, Vec2 pos_, int id_) {
    def = &d;
    id = id_;
    pos = pos_;
    spawnPos_ = pos_;
    w = d.w;
    h = d.h;
    hp = d.maxHp;
    maxHp = d.maxHp;
    alive = true;
    score = d.score;
    inIntro = false;
    inDefeat = false;
    phase = 0;
}

void Boss::StartIntro(SimContext& ctx) {
    inIntro = true;
    introT = 0.0;
    introFrom_ = {pos.x + 240.0, pos.y};
    pos = introFrom_;
    ctx.Emit({Evt::BossIntro, id, 0, Center(), def ? def->id : ""});
    ctx.Emit({Evt::Shake, id, 3, Center(), "bossintro"});
}

void Boss::Update(double dt, SimContext& ctx) {
    if (!def)
        return;
    TickTimers(dt);

    if (inIntro) {
        introT += dt;
        double t = SmoothStep(std::min(1.0, introT / def->introTime));
        pos.x = Lerp(introFrom_.x, spawnPos_.x, t);
        if (introT >= def->introTime) {
            inIntro = false;
            pos = spawnPos_;
            ctx.Emit({Evt::BossPhase, id, 0, Center(), "start"});
        }
        return;
    }
    if (inDefeat) {
        defeatT += dt;
        vel.y += 900.0 * dt;
        pos += vel * dt;
        if (static_cast<int>(defeatT * 10.0) % 3 == 0)
            ctx.Particles().InkSplash(Center() + Vec2{ctx.GetRng().range(-20, 20), ctx.GetRng().range(-24, 24)},
                                      0xFF26213C, 6, 160);
        if (defeatT >= def->defeatTime) {
            alive = false;
            ctx.Emit({Evt::BossDefeated, id, score, Center(), def->id});
            ctx.Particles().InkSplash(Center(), 0xFF26213C, 30, 320);
            ctx.Particles().Stars(Center(), 0xFFFFE08A, 24);
            ctx.Emit({Evt::Shake, id, 10, Center(), "bossdefeat"});
        }
        return;
    }
    if (transitionT > 0.0) {
        transitionT -= dt;
        vel *= std::max(0.0, 1.0 - 6.0 * dt);
        if (ctx.frame % 4 == 0)
            ctx.Particles().Sparks(Center() + Vec2{ctx.GetRng().range(-24, 24), ctx.GetRng().range(-30, 30)},
                                   0xFF7FD4FF, 2);
        return;
    }

    fightTime += dt;
    UpdateCombat(dt, ctx);

    // Gravity + ground
    vel.y += 1500.0 * dt;
    vel.y = std::min(vel.y, 800.0);
    if (ctx.level) {
        MoveResult r = MoveEntity(pos, vel, Box(), dt, *ctx.level, false);
        pos = r.pos;
        vel = r.vel;
        const Rect b = ctx.level->Data().bounds;
        pos.x = Clamp(pos.x, b.Left() + 2.0, b.Right() - w - 2.0);
    }
}

void Boss::TakeDamage(double dmg, Vec2 knock, SimContext& ctx) {
    if (!def || !Vulnerable())
        return;
    hp -= static_cast<int>(std::ceil(dmg));
    hitFlashT = 0.1;
    (void)knock; // bosses barely flinch
    if (hp <= 0) {
        hp = 0;
        StartDefeat(ctx);
        return;
    }
    PhaseCheck(ctx);
}

void Boss::PhaseCheck(SimContext& ctx) {
    if (!def || def->phases.empty())
        return;
    int newPhase = 0;
    for (std::size_t i = 0; i < def->phases.size(); ++i)
        if (Hpf() <= def->phases[i].hpFrac)
            newPhase = static_cast<int>(i) + 1;
    if (newPhase > phase)
        PhaseUp(ctx);
    else if (newPhase < phase)
        phase = newPhase;
}

void Boss::PhaseUp(SimContext& ctx) {
    ++phase;
    transitionT = 1.2;
    vel = {0.0, -160.0};
    ctx.Emit({Evt::BossPhase, id, phase, Center(), def->phases.size() > static_cast<std::size_t>(phase)
                                                      ? def->phases[phase].label
                                                      : "phase"});
    ctx.Emit({Evt::Shake, id, 6, Center(), "phaseup"});
    ctx.Particles().Shockwave(Center(), 0xFF7FD4FF);
    ctx.Particles().InkSplash(Center(), 0xFF26213C, 20, 260);
}

void Boss::StartDefeat(SimContext& ctx) {
    inDefeat = true;
    defeatT = 0.0;
    vel = {0.0, -220.0};
    ctx.Emit({Evt::SlowMo, id, 300, Center(), "defeat"});
    ctx.Emit({Evt::Shake, id, 8, Center(), "defeat"});
}

const char* Boss::PhaseLabel() const {
    if (!def || def->phases.empty())
        return "PHASE 1";
    std::size_t idx = static_cast<std::size_t>(phase);
    if (idx < def->phases.size())
        return def->phases[idx].label.c_str();
    return def->phases.back().label.c_str();
}

// ------------------------------------------------------------------ Book ----
static BossDef BossDefFromJson(const Json& j) {
    BossDef d;
    d.id = j.Find("id") ? j.Find("id")->AsString() : "boss";
    d.name = j.Find("name") ? j.Find("name")->AsString() : d.id;
    d.sprite = j.Find("sprite") ? j.Find("sprite")->AsString() : d.id;
    d.musicId = j.Find("music") ? j.Find("music")->AsString() : d.id;
    auto n = [&](const char* k, double* dst) {
        if (const Json* v = j.Find(k))
            *dst = v->AsNumber(*dst);
    };
    auto i = [&](const char* k, int* dst) {
        if (const Json* v = j.Find(k))
            *dst = v->AsInt(*dst);
    };
    auto b = [&](const char* k, bool* dst) {
        if (const Json* v = j.Find(k))
            *dst = v->AsBool(*dst);
    };
    i("maxHp", &d.maxHp);
    n("w", &d.w);
    n("h", &d.h);
    i("score", &d.score);
    i("energyOnKill", &d.energyOnKill);
    n("introTime", &d.introTime);
    n("defeatTime", &d.defeatTime);
    b("miniBoss", &d.miniBoss);
    const Json* ph = j.Find("phases");
    if (ph && ph->IsArray()) {
        for (const auto& jp : ph->Items()) {
            BossPhaseDef p;
            p.hpFrac = jp.Find("hpFrac") ? jp.Find("hpFrac")->AsNumber(0.66) : 0.66;
            p.musicLayer = jp.Find("musicLayer") ? jp.Find("musicLayer")->AsInt(1) : 1;
            p.label = jp.Find("label") ? jp.Find("label")->AsString() : "PHASE";
            d.phases.push_back(p);
        }
    }
    return d;
}

bool BossBook::LoadJson(const std::string& json, std::string* err) {
    defs_.clear();
    Json root = Json::Parse(json, err);
    const Json* arr = root.IsArray() ? &root : root.Find("bosses");
    if (!arr || !arr->IsArray()) {
        if (err)
            *err = "bosses.json: expected array of bosses";
        return false;
    }
    for (const auto& j : arr->Items())
        defs_.push_back(BossDefFromJson(j));
    return !defs_.empty();
}

const BossDef* BossBook::Get(const std::string& id) const {
    for (const auto& d : defs_)
        if (d.id == id)
            return &d;
    return nullptr;
}

} // namespace ink
