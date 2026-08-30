// Game implementation — the orchestrator. Keep it lean: sim systems own
// their behavior; the Game only drives frames, maps events to feedback and
// renders. No gameplay logic lives here beyond wiring (§94).
#include "Game/Game.h"

#include "Art/Background.h"
#include "Core/Log.h"
#include "Levels/LevelParser.h"

#include <SDL.h>
#include <SDL_scancode.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

namespace ink {

namespace {
std::string ReadAll(const std::string& path) {
    return ReadFile(path);
}
std::string Pad2(int n) {
    char b[8];
    std::snprintf(b, sizeof(b), "_%02d", n);
    return b;
}
std::string ArtKey(const std::string& base, int frame) { return base + Pad2(frame); }
} // namespace

// =========================================================================
// Init
// =========================================================================
bool Game::Init(Application& app, std::string* err) {
    app_ = &app;
    cfg_ = app.Config();
    assetsDir_ = cfg_.assetsDir;

    if (!renderer_.Init(app.Renderer(), err))
        return false;
    cam_.SetViewport(cfg_.width, cfg_.height);

    BuildMiloArt(bank_);
    BuildEnemyArt(bank_);
    BuildBossArt(bank_);
    BuildNpcArt(bank_);
    BuildTileArt(bank_);
    BuildPickupArt(bank_);
    bank_.Build([&](int w, int h, const uint32_t* rgba) { return renderer_.CreateTexture(w, h, rgba); });
    if (bank_.TextureId() < 0)
        INK_LOG_ERROR("sprite atlas build failed");

    app.SetEventSink([this](const SDL_Event& e) {
        input_.HandleEvent(e);
        OnRawEvent(e);
    });
    input_.Init([](const SDL_Event&) {});

    audio_.Init(cfg_.muteAudio || cfg_.headless, err);
    {
        std::vector<MusicTrack> tracks;
        LoadAllMusic(tracks);
        for (auto& t : tracks)
            audio_.LoadTrack(t);
    }

    if (!LoadData(err))
        return false;

    // Load settings from the latest save if any exist.
    for (int s = 0; s < SaveSystem::kSlots; ++s)
        if (saves_.Exists(s)) {
            saves_.LoadSettingsOnly(progress_, err);
            break;
        }
    ApplySettings();

    // Menus.
    std::vector<std::string> ti;
    bool anySave = false;
    for (int s = 0; s < SaveSystem::kSlots; ++s)
        anySave |= saves_.Exists(s);
    titleMenu_.SetItems(std::vector<std::string>{
        anySave ? "CONTINUE" : "CONTINUE (NO SAVE)", "NEW GAME", "OPTIONS", "QUIT"});
    titleMenu_.SetOnSelect([this, anySave](int i) {
        audio_.PlaySfx(AudioEngine::SfxUiSelect);
        if (i == 0) {
            if (anySave)
                ContinueGame(saves_.LatestSlot());
            else
                GoTitle();
        } else if (i == 1) {
            screen_ = Screen::Slots;
            screenT_ = 0.0;
        } else if (i == 2) {
            screen_ = Screen::Options;
            screenT_ = 0.0;
            optionsRow_ = 0;
        } else {
            quitRequested_ = true;
            app_->RequestQuit();
        }
    });
    slotsMenu_.SetItems(std::vector<std::string>{"SLOT 1", "SLOT 2", "SLOT 3"});
    slotsMenu_.SetOnSelect([this](int i) {
        audio_.PlaySfx(AudioEngine::SfxUiSelect);
        StartNewGame(i);
    });
    pauseMenu_.SetItems(std::vector<std::string>{"RESUME", "RESTART LEVEL", "QUIT TO TITLE"});
    pauseMenu_.SetOnSelect([this](int i) {
        audio_.PlaySfx(AudioEngine::SfxUiSelect);
        if (i == 0) {
            screen_ = Screen::Playing;
            audio_.SetMusicPlaying(true);
        } else if (i == 1) {
            LoadLevel(progress_.currentLevel);
            screen_ = Screen::Playing;
            audio_.SetMusicPlaying(true);
        } else {
            GoTitle();
        }
    });

    events_.Listen([this](const GameEvent& e) { OnGameEvent(e); });
    quests_.SetEvents(&events_);

    // Headless QA hooks. INK_AUTOTEST=1 new game; =2 continue latest save.
    if (const char* v = std::getenv("INK_AUTOTEST"))
        autotest_ = (v[0] == '1' || v[0] == '2' || v[0] == 'c');
    if (const char* v = std::getenv("INK_SHOTS")) {
        std::string s = v;
        std::size_t i = 0;
        while (i < s.size()) {
            std::size_t c = s.find(',', i);
            std::string part = s.substr(i, c == std::string::npos ? std::string::npos : c - i);
            std::size_t c2 = part.find(':');
            if (c2 != std::string::npos)
                shots_.push_back({std::atoi(part.substr(0, c2).c_str()), part.substr(c2 + 1)});
            if (c == std::string::npos)
                break;
            i = c + 1;
        }
    }

    screen_ = Screen::Title;
    audio_.PlayMusic("menu");
    if (autotest_) {
        slot_ = 0;
        const char* v = std::getenv("INK_AUTOTEST");
        if (v && (std::strcmp(v, "2") == 0 || std::strcmp(v, "c") == 0)) {
            INK_LOG_INFO("autotest: continuing latest save");
            ContinueGame(saves_.LatestSlot());
        } else if (v && std::strcmp(v, "3") == 0) {
            // QA: skip straight to the ending + credit roll.
            StartNewGame(0);
            endingKind_ = "normal";
            screen_ = Screen::Ending;
            screenT_ = 0.0;
            creditsEndT_ = 3.5 + Screens::CreditsRollSeconds(credits_, cfg_.height) + 2.0;
        } else {
            StartNewGame(0);
        }
    }
    INK_LOG_INFO("Game initialised ({} frames of art, {} enemies, {} bosses, {} weapons)",
                 static_cast<long>(bank_.FrameCount()), static_cast<long>(enemyBook_.All().size()),
                 static_cast<long>(bossBook_.All().size()), static_cast<long>(weapons_.All().size()));
    return true;
}

bool Game::LoadData(std::string* err) {
    auto load = [&](const char* name, auto& book, auto fn) {
        std::string p = assetsDir_ + "/Data/" + name;
        if (!fs::exists(fs::path(p))) {
            *err = std::string("missing data file: ") + p;
            return false;
        }
        if (!fn(ReadAll(p), err)) {
            *err = std::string("failed to load ") + p + ": " + *err;
            return false;
        }
        return true;
    };
    std::string e;
    if (!load("weapons.json", weapons_, [&](const std::string& t, std::string* e2) { return weapons_.LoadJson(t, e2); }) ||
        !load("enemies.json", enemyBook_, [&](const std::string& t, std::string* e2) { return enemyBook_.LoadJson(t, e2); }) ||
        !load("bosses.json", bossBook_, [&](const std::string& t, std::string* e2) { return bossBook_.LoadJson(t, e2); }) ||
        !load("worlds.json", worlds_, [&](const std::string& t, std::string* e2) { return worlds_.LoadJson(t, e2); }))
        return false;
    // Optional data (defaults are fine if absent).
    auto optional = [&](const char* name, auto fn) {
        std::string p = assetsDir_ + "/Data/" + name;
        if (fs::exists(fs::path(p))) {
            std::string e2;
            fn(ReadAll(p), &e2);
        }
    };
    optional("charms.json", [&](const std::string& t, std::string* e2) { charmBook_.LoadJson(t, e2); });
    optional("dialogue.json", [&](const std::string& t, std::string* e2) { dialogue_.LoadJson(t, e2); });
    optional("quests.json", [&](const std::string& t, std::string* e2) { quests_.LoadJson(t, e2); });
    optional("achievements.json", [&](const std::string& t, std::string* e2) { achievements_.LoadJson(t, e2); });
    optional("credits.json", [&](const std::string& t, std::string* e2) {
        Json j = Json::Parse(t, e2);
        if (j.IsNull() || !e2->empty())
            return;
        credits_ = CreditsData{};
        if (const Json* x = j.Find("title"))
            credits_.title = x->AsString("INKBOUND");
        if (const Json* x = j.Find("subtitle"))
            credits_.subtitle = x->AsString("THE LAST REEL");
        if (const Json* roll = j.Find("roll"))
            for (std::size_t i = 0; i < roll->Size(); ++i) {
                const Json& it = roll->At(i);
                CreditEntry e;
                if (const Json* x = it.Find("heading"))
                    e.heading = x->AsString();
                if (const Json* x = it.Find("role"))
                    e.role = x->AsString();
                if (const Json* x = it.Find("name"))
                    e.name = x->AsString();
                credits_.roll.push_back(std::move(e));
            }
        INK_LOG_INFO("credits loaded: {} entries", static_cast<long>(credits_.roll.size()));
    });
    (void)e;
    return true;
}

void Game::ApplySettings() {
    const auto& s = progress_.settings;
    audio_.SetMasterVolume(s.masterVol);
    audio_.SetMusicVolume(s.musicVol);
    audio_.SetSfxVolume(s.sfxVol);
    cam_.SetShakeScale(s.reducedShake ? 0.3 : 1.0);
    fontScale_ = s.fontScale;
    filmFx_ = s.filmFx;
    reducedFlash_ = s.reducedFlash;
}

// =========================================================================
// Progress / level flow
// =========================================================================
void Game::StartNewGame(int slot) {
    slot_ = slot;
    GameProgress fresh;
    // keep the player's settings
    fresh.settings = progress_.settings;
    fresh.saveName = "Milo";
    fresh.currentLevel = "meadows_01";
    fresh.currentWorld = "meadows";
    progress_ = fresh;
    counters_ = MetaCounters{};
    achievements_.SetUnlocked({});
    quests_.ResetForLevel();
    SaveNow();
    LoadLevel(progress_.currentLevel);
    screen_ = Screen::Playing;
    StartLevelCine();
}

void Game::ContinueGame(int slot) {
    slot_ = slot;
    std::string err;
    if (!saves_.Load(slot, progress_, &err)) {
        INK_LOG_ERROR("Load failed: {}", err);
        StartNewGame(slot);
        return;
    }
    ApplySettings();
    counters_ = MetaCounters{};
    counters_.levelId = progress_.currentLevel;
    achievements_.SetUnlocked(progress_.achievements);
    quests_.ResetForLevel();
    LoadLevel(progress_.currentLevel);
    screen_ = Screen::Playing;
}

void Game::LoadLevel(const std::string& levelId) {
    // Find the world + level file.
    std::string file;
    std::string worldId;
    for (const auto& w : worlds_.Worlds()) {
        for (const auto& l : w.levels) {
            if (l.id == levelId) {
                file = l.file;
                worldId = w.id;
                break;
            }
        }
    }
    if (file.empty()) {
        INK_LOG_ERROR("unknown level id: {}", levelId);
        file = "Levels/meadows_01.lvl";
        worldId = "meadows";
    }

    std::string err;
    LevelData data;
    if (!ParseLevelText(ReadAll(assetsDir_ + "/" + file), data, &err)) {
        INK_LOG_ERROR("level parse failed ({}): {}", file, err);
        return;
    }
    level_.Load(data);
    progress_.currentWorld = worldId;
    counters_.levelId = levelId;

    // Player stats: base + upgrades + equipped charms.
    PlayerStats stats;
    stats.maxHp = 3 + progress_.hpUpgrades;
    if (autotest_) {
        // QA bot robustness (test-only): survive gauntlets long enough to
        // validate the whole level flow; 1-damage hits can't be scaled down
        // (rounded + clamped to 1), so use flat extra HP.
        stats.maxHp += 9;
        stats.damageMul *= 3.0;
    }
    for (const auto& cid : progress_.charmSlots) {
        if (cid.empty())
            continue;
        const CharmDef* c = charmBook_.Get(cid);
        if (!c)
            continue;
        stats.maxHp += c->mods.maxHpAdd;
        stats.walkSpeed *= c->mods.moveSpeedMul;
        stats.runSpeed *= c->mods.moveSpeedMul;
        stats.jumpVel *= c->mods.jumpMul;
        stats.damageMul *= c->mods.damageMul;
        stats.dmgTakenMul *= c->mods.dmgTakenMul;
        stats.dashCooldown *= c->mods.dashCooldownMul;
        stats.energyGainMul *= c->mods.energyGainMul;
        stats.invulnTime *= c->mods.invulnMul;
        stats.airDashCount += c->mods.airDashAdd;
        stats.coinMul *= c->mods.coinMul;
    }
    player_.Init(1, stats);
    player_.pos = data.playerSpawn;
    player_.SetWeapon(weapons_.Get(progress_.currentWeapon) ? weapons_.Get(progress_.currentWeapon)
                                                            : &weapons_.Default());
    player_.SetReviveAvailable(progress_.OwnsCharm("echo_heart"));

    // Enemies.
    enemies_.clear();
    int eid = 1000;
    for (const auto& s : data.spawns) {
        auto en = Enemy::Create(s.type);
        if (!en)
            continue;
        const EnemyDef* def = enemyBook_.Get(s.type);
        if (!def)
            continue;
        en->Init(*def, s.pos, ++eid);
        enemies_.push_back(std::move(en));
    }
    liveEnemies_.clear();
    for (auto& en : enemies_)
        liveEnemies_.push_back(en.get());

    projectiles_.clear();
    particles_.Clear();
    boss_.reset();
    bossSpawned_ = false;
    bossDefeated_ = false;
    rankShown_ = false;
    playerDeathT_ = 0.0;
    levelTime_ = 0.0;
    damageTakenThisBoss_ = 0;
    parriesThisBoss_ = 0;
    hitStopT_ = 0.0;
    slowMoT_ = 0.0;
    hurtFlash_ = 0.0;

    ctx_.level = &level_;
    ctx_.player = &player_;
    ctx_.enemies = &liveEnemies_;
    ctx_.boss = nullptr;
    ctx_.rng = &rng_;
    ctx_.events = &events_;
    ctx_.particles = &particles_;
    ctx_.projectiles = &projectiles_;
    ctx_.time = 0.0;
    ctx_.frame = 0;
    ctx_.timescale = 1.0;

    cam_.SetBounds(level_.Data().bounds);
    cam_.ForcePos({player_.pos.x - cfg_.width * 0.5, player_.pos.y - cfg_.height * 0.5});

    bannerT_ = 3.5;
    audio_.PlayMusic(data.musicId.empty() ? "meadows" : data.musicId);
    INK_LOG_INFO("Loaded level {} ({}x{}, {} spawns)", data.id, data.w, data.h, data.spawns.size());
}

void Game::StartLevelCine() {
    cineDef_ = Cine::TextCards(
        "intro",
        {level_.Data().worldId, level_.Data().name, "The reels are torn. Save the last projection."},
        1.6);
    cine_.Start(cineDef_);
}

void Game::TriggerBoss() {
    if (bossSpawned_ || !level_.HasBoss())
        return;
    bossSpawned_ = true;
    const BossDef* def = bossBook_.Get(level_.BossId());
    if (!def) {
        INK_LOG_ERROR("boss def missing: {}", level_.BossId());
        bossDefeated_ = true; // let the exit open rather than soft-lock
        return;
    }
    boss_ = Boss::Create(def->id);
    boss_->Init(*def, level_.BossSpawn(), 9000);
    ctx_.boss = boss_.get();
    boss_->StartIntro(ctx_);
    cam_.SetZoom(1.0f);
    std::string music = def->musicId;
    audio_.StopMusic(0.25);
    if (!music.empty())
        audio_.PlayMusic(music, 0);
    hud_.AddToast("BOSS", def->name);
}

void Game::ComputeRank() {
    if (!boss_)
        return;
    rank_.bossName = boss_->def ? boss_->def->name : "???";
    rank_.time = boss_->fightTime;
    rank_.damageTaken = damageTakenThisBoss_;
    rank_.parries = parriesThisBoss_;
    rank_.maxCombo = player_.MaxCombo();
    rank_.specials = counters_.supers;
    rank_.noHit = damageTakenThisBoss_ == 0;
    long long score = 1000 + counters_.coins * 5 - damageTakenThisBoss_ * 50 + parriesThisBoss_ * 200;
    if (rank_.time < 60.0)
        score += 500;
    rank_.score = score;
    std::string r;
    if (rank_.noHit && rank_.time < 60.0)
        r = "S+";
    else if (rank_.noHit)
        r = "S";
    else if (damageTakenThisBoss_ < 5)
        r = "A";
    else if (damageTakenThisBoss_ < 12)
        r = "B";
    else if (damageTakenThisBoss_ < 22)
        r = "C";
    else
        r = "D";
    rank_.rank = r;

    // Persist best record.
    BossRecord rec;
    rec.bestTime = rank_.time;
    rec.bestScore = rank_.score;
    rec.noHit = rank_.noHit;
    rec.bestRank = r;
    if (boss_->def)
        progress_.bosses[boss_->def->id] = rec;
    counters_.lastBossId = boss_->def ? boss_->def->id : "";
    counters_.lastBossRank = r;
    counters_.lastBossTime = rank_.time;
    counters_.lastBossNoHit = rank_.noHit;
}

void Game::OnLevelComplete() {
    ctx_.Emit({Evt::LevelComplete, -1, 0, player_.Center(), level_.Data().id});
    counters_.lastLevelTime = levelTime_;
    counters_.lastLevelNoHit = progress_.hpUpgrades >= 0 && hurtFlash_ <= 0.0;
    counters_.levelClears++;

    // Next level in the same world, else ending.
    std::string next;
    bool last = false;
    for (const auto& w : worlds_.Worlds()) {
        if (w.id != progress_.currentWorld)
            continue;
        for (std::size_t i = 0; i < w.levels.size(); ++i) {
            if (w.levels[i].id == level_.Data().id) {
                if (i + 1 < w.levels.size())
                    next = w.levels[i + 1].id;
                else
                    last = true;
            }
        }
    }
    if (next.empty() && last) {
        endingKind_ = progress_.frames.size() >= 3 ? "true" : "normal";
        if (!progress_.stamps.empty())
            endingKind_ = "secret";
        audio_.StopMusic(0.3);
        audio_.PlayMusic("victory");
        SaveNow();
        screen_ = Screen::Ending;
        screenT_ = 0.0;
        // Ending card (3.5s) + full credit roll, then back to title (autotest).
        creditsEndT_ = 3.5 + Screens::CreditsRollSeconds(credits_, cfg_.height) + 2.0;
        INK_LOG_INFO("ending started ({} entries, roll ends ~{}s)",
                     static_cast<long>(credits_.roll.size()), creditsEndT_);
        return;
    }
    if (!next.empty())
        progress_.currentLevel = next;
    SaveNow();
    screen_ = Screen::Complete;
    screenT_ = 0.0;
}

void Game::GoTitle() {
    SaveNow();
    audio_.StopMusic(0.3);
    audio_.PlayMusic("menu");
    // Refresh the menu label now that a save may exist.
    bool anySave = false;
    for (int s = 0; s < SaveSystem::kSlots; ++s)
        anySave |= saves_.Exists(s);
    titleMenu_.SetItems(std::vector<std::string>{
        anySave ? "CONTINUE" : "CONTINUE (NO SAVE)", "NEW GAME", "OPTIONS", "QUIT"});
    screen_ = Screen::Title;
    screenT_ = 0.0;
}

void Game::SaveNow() {
    // Note: currentLevel is owned by the level flow (StartNewGame /
    // OnLevelComplete); never overwrite it here or completions would be
    // saved under the level being left instead of the next one.
    std::string err;
    if (slot_ >= 0) {
        if (!saves_.Save(slot_, progress_, &err))
            INK_LOG_ERROR("save failed: {}", err);
    } else {
        saves_.SaveSettingsOnly(progress_, &err);
    }
}

// =========================================================================
// Update
// =========================================================================
void Game::OnUpdate(double dt) {
    screenT_ += dt;
    if (errorFlashT_ > 0.0)
        errorFlashT_ -= dt;
    hud_.Update(dt);
    if (autotest_ && app_) {
        int f = app_->SimFrame();
        for (auto& [ff, path] : shots_)
            if (f == ff)
                app_->CaptureRaw(path);
    }

    switch (screen_) {
    case Screen::Title:
    case Screen::Slots:
    case Screen::Paused:
        UpdateMenus(dt);
        break;
    case Screen::Options:
        UpdateOptions(dt);
        break;
    case Screen::Playing:
        UpdatePlaying(dt);
        break;
    case Screen::Dialogue:
        UpdateDialogue(dt);
        break;
    case Screen::Complete:
        UpdateComplete(dt);
        break;
    case Screen::GameOver:
        UpdateGameOver(dt);
        break;
    case Screen::Rank:
        UpdateRank(dt);
        break;
    case Screen::Ending:
        UpdateEnding(dt);
        break;
    }
    if (quitRequested_)
        app_->RequestQuit();
}

void Game::UpdateMenus(double dt) {
    (void)dt;
    input_.BeginFrame();
    if (input_.Pressed(Action::Pause) || input_.Pressed(Action::Interact)) {
        // Back out of slots/pause.
        if (screen_ == Screen::Slots) {
            screen_ = Screen::Title;
            screenT_ = 0.0;
            return;
        }
        if (screen_ == Screen::Paused) {
            screen_ = Screen::Playing;
            audio_.SetMusicPlaying(true);
            return;
        }
    }
    Menu* m = screen_ == Screen::Title    ? &titleMenu_
              : screen_ == Screen::Slots  ? &slotsMenu_
                                          : &pauseMenu_;
    if (input_.Pressed(Action::Jump))
        m->Move(1);
    if (input_.Pressed(Action::Parry))
        m->Move(-1);
    if (input_.Pressed(Action::Attack) || input_.Pressed(Action::Interact) ||
        input_.Pressed(Action::Special))
        m->Select();
}

void Game::UpdateOptions(double dt) {
    (void)dt;
    input_.BeginFrame();
    auto& s = progress_.settings;
    if (remapActive_) {
        // Waiting for the player to press a key (handled in OnRawEvent).
        if (input_.Pressed(Action::Pause))
            remapActive_ = false;
        return;
    }
    if (input_.Pressed(Action::Pause)) {
        SaveNow();
        screen_ = Screen::Title;
        screenT_ = 0.0;
        return;
    }
    if (input_.Pressed(Action::Parry))
        optionsRow_ = (optionsRow_ + 8) % 9;
    if (input_.Pressed(Action::Jump))
        optionsRow_ = (optionsRow_ + 1) % 9;
    double step = 0.0;
    if (input_.Pressed(Action::MoveRight))
        step = 1.0;
    if (input_.Pressed(Action::MoveLeft))
        step = -1.0;
    if (step != 0.0) {
        auto& S = progress_.settings;
        switch (optionsRow_) {
        case 0: S.masterVol = static_cast<float>(Clamp(S.masterVol + step * 0.1, 0.0, 1.0)); break;
        case 1: S.musicVol = static_cast<float>(Clamp(S.musicVol + step * 0.1, 0.0, 1.0)); break;
        case 2: S.sfxVol = static_cast<float>(Clamp(S.sfxVol + step * 0.1, 0.0, 1.0)); break;
        case 3: {
            const char* diffs[] = {"Story", "Standard", "Expert", "Nightmare"};
            for (auto d : diffs)
                if (S.difficulty == d)
                    S.difficulty = diffs[(std::find(diffs, diffs + 4, d) - diffs + 1) % 4];
            break;
        }
        case 4: S.filmFx = static_cast<float>(Clamp(S.filmFx + step * 0.25, 0.0, 1.0)); break;
        case 5: S.reducedFlash = !S.reducedFlash; break;
        case 6: S.reducedShake = !S.reducedShake; break;
        case 7: S.highContrast = !S.highContrast; break;
        case 8: S.fontScale = static_cast<float>(Clamp(S.fontScale + step * 0.125, 0.75, 1.5)); break;
        }
        ApplySettings();
        audio_.PlaySfx(AudioEngine::SfxUiMove);
    }
    if (input_.Pressed(Action::Attack)) {
        remapAction_ = 0;
        remapActive_ = true;
    }
}

void Game::UpdatePlaying(double dt) {
    if (cine_.Active()) {
        cine_.Update(dt);
        if (input_.Down(Action::Jump) || input_.Down(Action::Interact))
            cine_.Skip();
    }
    input_.BeginFrame();
    if (input_.Pressed(Action::Pause)) {
        screen_ = Screen::Paused;
        audio_.SetMusicPlaying(false);
        audio_.PlaySfx(AudioEngine::SfxUiSelect);
        return;
    }

    // Timescale (hit-stop / slow-mo, §13).
    double ts = 1.0;
    if (hitStopT_ > 0.0) {
        hitStopT_ -= dt;
        ts = 0.0;
    } else if (slowMoT_ > 0.0) {
        slowMoT_ -= dt;
        ts = 0.4;
    }
    ctx_.timescale = ts;
    const double sdt = dt * ts;
    if (sdt > 0.0) {
        ctx_.time += sdt;
        ctx_.frame++;
        levelTime_ += sdt;
    }
    if (bannerT_ > 0.0)
        bannerT_ -= dt;
    if (npcTalkCd_ > 0.0)
        npcTalkCd_ -= dt;
    if (hurtFlash_ > 0.0)
        hurtFlash_ = std::max(0.0, hurtFlash_ - dt * 2.5);

    // Player.
    PlayerInput in;
    BuildPlayerInput(in);
    if (player_.alive)
        player_.Update(sdt, ctx_, in);

    // World.
    level_.Update(sdt);
    // Moving platform carry.
    if (player_.OnGround()) {
        for (auto& p : level_.Platforms()) {
            Rect pr{p.pos.x, p.pos.y, 64.0, 8.0};
            Rect feet{player_.pos.x - 2.0, player_.pos.y + player_.h - 6.0, player_.w + 4.0, 8.0};
            if (pr.Overlaps(feet)) {
                player_.pos.x += p.vel.x;
                player_.pos.y = p.pos.y - player_.h;
            }
        }
    }
    // Hazards.
    if (player_.alive && level_.HazardOverlaps(player_.Box()))
        player_.Damage(1, player_.Center(), ctx_, /*hazard=*/true);
    // Fell out of the world: hard reset to the checkpoint (no soft-locks).
    const auto& lb = level_.Data().bounds;
    if (player_.alive && player_.pos.y > lb.y + lb.h + 120.0) {
        INK_LOG_INFO("player fell out of the world — respawning at checkpoint");
        player_.Damage(999, player_.Center(), ctx_, /*hazard=*/true);
    }

    // Enemies.
    for (auto& en : enemies_)
        en->Update(sdt, ctx_);

    // Boss.
    if (!bossSpawned_ && level_.HasBoss() && level_.InBossZone(player_))
        TriggerBoss();
    if (boss_) {
        boss_->Update(sdt, ctx_);
        if (boss_->inDefeat && !rankShown_ && boss_->defeatT > 2.0) {
            rankShown_ = true;
            ComputeRank();
            audio_.StopMusic(0.4);
            audio_.PlayMusic("victory");
            screen_ = Screen::Rank;
            screenT_ = 0.0;
            rankT_ = 0.0;
        }
    }

    // Projectiles.
    for (auto& p : projectiles_)
        p.Update(sdt, ctx_);
    for (std::size_t i = 0; i < projectiles_.size();) {
        if (projectiles_[i].dead)
            std::swap(projectiles_[i], projectiles_.back()), projectiles_.pop_back();
        else
            ++i;
    }

    PickupsAndZones(sdt);
    HandleNpcTalk();

    // Death.
    if (!player_.alive) {
        playerDeathT_ += dt;
        if (playerDeathT_ > 1.5) {
            audio_.StopMusic(0.4);
            audio_.PlayMusic("defeat");
            screen_ = Screen::GameOver;
            screenT_ = 0.0;
        }
    }

    // Level exit.
    if (player_.alive && level_.InExitZone(player_) && (!level_.HasBoss() || bossDefeated_))
        OnLevelComplete();

    // Cleanup.
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
                                  [](const std::unique_ptr<Enemy>& e) {
                                      return !e->alive && e->deadT > 1.2;
                                  }),
                   enemies_.end());
    liveEnemies_.clear();
    for (auto& en : enemies_)
        liveEnemies_.push_back(en.get());

    // Camera.
    if (boss_ && !boss_->inDefeat)
        cam_.UpdateBoss(dt, Rect{std::min(player_.Center().x, boss_->Center().x) - 40.0,
                                 player_.pos.y - 120.0,
                                 std::abs(player_.Center().x - boss_->Center().x) + 200.0, 320.0});
    else
        cam_.Update(dt, player_.Center(), player_.FacingRight() ? 1.0 : -1.0, player_.vel.x);
}

void Game::BuildPlayerInput(PlayerInput& in) const {
    if (autotest_) {
        // Deterministic bot: walk right, hop from the ground, fire, dash, parry.
        double t = ctx_.time;
        float mx = 1.0f;
        const auto* boss = ctx_.boss;
        if (boss && boss->alive && !boss->inIntro) {
            // Fight the boss: keep it in front, strafe near it, never hug a wall.
            const double bx = boss->Center().x - player_.Center().x;
            if (bx < -30.0)
                mx = -1.0f;
            else if (bx > 170.0)
                mx = 1.0f;
            else
                mx = (std::fmod(t, 1.4) < 0.7) ? -1.0f : 1.0f;
        }
        in.moveX = mx;
        in.jumpPressed = player_.OnGround() && fmod(t, 0.7) < 0.02;
        in.jumpHeld = true; // hold = full jump arc (no variable-height cut)
        in.attackPressed = fmod(t, 0.5) < 0.02;
        in.attackHeld = fmod(t, 0.5) < 0.12;
        in.dashPressed = fmod(t, 1.7) < 0.02;
        in.parryPressed = fmod(t, 2.3) < 0.02;
        return;
    }
    in.moveX = input_.AxisX();
    in.jumpHeld = input_.Down(Action::Jump);
    in.jumpPressed = input_.Pressed(Action::Jump);
    in.crouch = input_.CrouchHeld();
    in.crouchPressed = input_.Pressed(Action::Crouch);
    in.dashPressed = input_.Pressed(Action::Dash);
    in.attackHeld = input_.Down(Action::Attack);
    in.attackPressed = input_.Pressed(Action::Attack) || input_.PressedWithin(Action::Attack, 6);
    in.parryPressed = input_.Pressed(Action::Parry);
    in.specialPressed = input_.Pressed(Action::Special);
    in.dropThrough = input_.DropThrough();
}

void Game::PickupsAndZones(double dt) {
    (void)dt;
    const Rect pbox = player_.Box();
    for (auto& c : level_.Collectibles()) {
        if (c.taken)
            continue;
        Rect box{c.pos.x - 8.0, c.pos.y - 8.0, 16.0, 16.0};
        if (!box.Overlaps(pbox))
            continue;
        c.taken = true;
        if (c.type == "coin") {
            int v = static_cast<int>(10.0 * player_.Stats().coinMul);
            ctx_.Emit({Evt::CoinPicked, -1, v, c.pos, "coin"});
        } else if (c.type == "film")
            ctx_.Emit({Evt::FilmPicked, -1, 0, c.pos, c.itemId});
        else if (c.type == "frame")
            ctx_.Emit({Evt::MasterFramePicked, -1, 0, c.pos, c.itemId});
        else if (c.type == "stamp")
            ctx_.Emit({Evt::StampPicked, -1, 0, c.pos, c.itemId});
        else if (c.type == "ink")
            player_.AddEnergy(30);
    }
    // Checkpoints.
    const auto& cps = level_.Checkpoints();
    for (int i = 0; i < static_cast<int>(cps.size()); ++i) {
        if (i <= level_.ActiveCheckpoint())
            continue;
        Rect box{cps[i].x - 10.0, cps[i].y - 26.0, 20.0, 34.0};
        if (box.Overlaps(pbox)) {
            level_.SetActiveCheckpoint(i);
            ctx_.Emit({Evt::CheckpointReached, i, 0, cps[i], "checkpoint"});
        }
    }
}

void Game::HandleNpcTalk() {
    if (npcTalkCd_ > 0.0 || !input_.Pressed(Action::Interact))
        return;
    for (const auto& n : level_.Data().npcs) {
        Rect box{n.pos.x - 12.0, n.pos.y - 20.0, 24.0, 26.0};
        if (!box.Overlaps(player_.Box()))
            continue;
        const auto* lines = dialogue_.Lines(n.id);
        if (!lines || lines->empty())
            continue;
        dialogSpeaker_ = n.id;
        dialog_.Start(*lines);
        npcTalkCd_ = 1.0;
        screen_ = Screen::Dialogue;
        screenT_ = 0.0;
        audio_.SetMusicPlaying(false);
        return;
    }
}

void Game::UpdateDialogue(double dt) {
    input_.BeginFrame();
    dialog_.Update(dt);
    if (autotest_ || input_.Pressed(Action::Jump) || input_.Pressed(Action::Interact))
        dialog_.Advance();
    if (dialog_.Done()) {
        screen_ = Screen::Playing;
        audio_.SetMusicPlaying(true);
    }
}

void Game::UpdateComplete(double dt) {
    (void)dt;
    input_.BeginFrame();
    if (autotest_ || input_.Pressed(Action::Jump) || input_.Pressed(Action::Interact)) {
        LoadLevel(progress_.currentLevel);
        screen_ = Screen::Playing;
        StartLevelCine();
    }
}

void Game::UpdateGameOver(double dt) {
    (void)dt;
    input_.BeginFrame();
    if (autotest_ || input_.Pressed(Action::Jump) || input_.Pressed(Action::Interact)) {
        // Retry: reload the level, respawn at the checkpoint with full HP.
        LoadLevel(progress_.currentLevel);
        player_.pos = level_.RespawnPoint();
        player_.hp = player_.Stats().maxHp;
        player_.SetReviveAvailable(progress_.OwnsCharm("echo_heart"));
        screen_ = Screen::Playing;
        StartLevelCine();
    } else if (input_.Pressed(Action::Pause)) {
        GoTitle();
    }
}

void Game::UpdateRank(double dt) {
    rankT_ += dt;
    input_.BeginFrame();
    if (rankT_ > 1.0 && (autotest_ || input_.Pressed(Action::Jump) ||
                         input_.Pressed(Action::Interact)))
        OnLevelComplete();
}

void Game::UpdateEnding(double dt) {
    (void)dt;
    input_.BeginFrame();
    if (autotest_) {
        if (screenT_ > creditsEndT_)
            GoTitle();
        return;
    }
    if (input_.Pressed(Action::Jump) || input_.Pressed(Action::Interact))
        GoTitle();
}

// =========================================================================
// Events -> feedback
// =========================================================================
void Game::OnGameEvent(const GameEvent& e) {
    // Achievements first (they only observe).
    achievements_.OnEvent(e, counters_);

    switch (e.type) {
    case Evt::PlayerHurt:
        audio_.PlaySfx(AudioEngine::SfxHurt);
        hurtFlash_ = 0.45;
        cam_.AddShake(6.0);
        hitStopT_ = std::max(hitStopT_, 0.05);
        if (boss_ && !boss_->inDefeat && boss_->inIntro == false)
            damageTakenThisBoss_ += e.amount;
        break;
    case Evt::PlayerDied:
        audio_.PlaySfx(AudioEngine::SfxDeath);
        cam_.AddShake(12.0);
        slowMoT_ = 0.6;
        counters_.deaths++;
        if (autotest_)
            INK_LOG_INFO("bot died at ({},{}) level={} cp={}",
                         static_cast<int>(e.pos.x), static_cast<int>(e.pos.y),
                         level_.Data().id, level_.ActiveCheckpoint());
        break;
    case Evt::SuperUsed:
        audio_.PlaySfx(AudioEngine::SfxSuper);
        cam_.AddShake(9.0);
        slowMoT_ = 0.5;
        counters_.supers++;
        break;
    case Evt::SuperCharged:
        audio_.PlaySfx(AudioEngine::SfxCharged);
        hud_.AddToast("INKWELL FULL", "Press Special for Inkstorm");
        break;
    case Evt::ParrySuccess:
        audio_.PlaySfx(AudioEngine::SfxParry);
        hitStopT_ = std::max(hitStopT_, 0.06);
        cam_.AddShake(2.5);
        counters_.parries++;
        if (boss_ && !boss_->inDefeat)
            parriesThisBoss_++;
        particles_.Stars(e.pos, 0xFF7FD4FF, 8);
        break;
    case Evt::PlayerHitEnemy:
        audio_.PlaySfx(AudioEngine::SfxHit);
        break;
    case Evt::EnemyKilled:
        counters_.kills++;
        audio_.PlaySfx(AudioEngine::SfxStomp);
        break;
    case Evt::CoinPicked:
        audio_.PlaySfx(AudioEngine::SfxCoin);
        counters_.coins += e.amount;
        progress_.coins += e.amount;
        break;
    case Evt::FilmPicked: {
        audio_.PlaySfx(AudioEngine::SfxCoin);
        counters_.films++;
        bool had = std::find(progress_.films.begin(), progress_.films.end(), e.name) !=
                   progress_.films.end();
        if (!had)
            progress_.films.push_back(e.name);
        hud_.AddToast("FILM RECOVERED", e.name);
        break;
    }
    case Evt::MasterFramePicked:
        audio_.PlaySfx(AudioEngine::SfxAchievement);
        counters_.frames++;
        progress_.GrantFrame(e.name);
        hud_.AddToast("MASTER FRAME", "The projector hums to life");
        break;
    case Evt::StampPicked:
        audio_.PlaySfx(AudioEngine::SfxAchievement);
        counters_.stamps++;
        progress_.stamps.push_back(e.name);
        hud_.AddToast("DIRECTOR'S STAMP", "A hidden mark is recorded");
        break;
    case Evt::PlayerHealed:
        audio_.PlaySfx(AudioEngine::SfxCoin);
        break;
    case Evt::CheckpointReached:
        audio_.PlaySfx(AudioEngine::SfxCheckpoint);
        hud_.AddToast("CHECKPOINT", "Progress saved");
        break;
    case Evt::BossIntro:
        audio_.PlaySfx(AudioEngine::SfxBossIntro);
        cam_.AddShake(4.0);
        break;
    case Evt::BossPhase: {
        audio_.PlaySfx(AudioEngine::SfxPhaseUp);
        int layer = 1;
        if (boss_ && boss_->def && e.amount >= 0 &&
            e.amount < static_cast<int>(boss_->def->phases.size()))
            layer = boss_->def->phases[e.amount].musicLayer;
        audio_.SetMusicLayer(layer);
        hud_.AddToast("PHASE " + std::to_string(e.amount + 1), e.name);
        break;
    }
    case Evt::BossDefeated:
        bossDefeated_ = true;
        break;
    case Evt::LevelComplete:
        audio_.PlaySfx(AudioEngine::SfxVictory);
        break;
    case Evt::QuestCompleted:
        audio_.PlaySfx(AudioEngine::SfxQuest);
        counters_.questsDone++;
        hud_.AddToast("QUEST COMPLETE", e.name);
        break;
    case Evt::AchievementUnlocked: {
        audio_.PlaySfx(AudioEngine::SfxAchievement);
        hud_.AddToast("ACHIEVEMENT", e.name);
        break;
    }
    case Evt::Shake:
        cam_.AddShake(e.amount * 1.5);
        break;
    case Evt::HitStop:
        hitStopT_ = std::max(hitStopT_, e.amount / 1000.0);
        break;
    case Evt::SlowMo:
        if (e.amount > 0)
            slowMoT_ = 0.5;
        break;
    case Evt::ProjectileExplode:
        audio_.PlaySfx(AudioEngine::SfxExplosion);
        break;
    default:
        break;
    }

    debug_.LogEvent(std::to_string(static_cast<int>(e.type)) + ":" + e.name);
}

void Game::OnRawEvent(const SDL_Event& e) {
    if (e.type != SDL_KEYDOWN)
        return;
    switch (e.key.keysym.sym) {
    case SDLK_F1:
        debug_.ToggleHitboxes();
        break;
    case SDLK_F2:
        ctx_.godMode = !ctx_.godMode;
        debug_.SetGodMode(ctx_.godMode);
        break;
    case SDLK_F12:
        app_->CaptureRaw("screenshot.png");
        break;
    default:
        break;
    }
    // Remap capture.
    if (remapActive_ && e.key.keysym.sym != SDLK_F1 && e.key.keysym.sym != SDLK_F12) {
        const SDL_Keycode* names = nullptr;
        (void)names;
        char buf[32];
        const SDL_Keycode sym = e.key.keysym.sym;
        switch (sym) {
        case SDLK_SPACE: std::snprintf(buf, sizeof(buf), "space"); break;
        case SDLK_LEFT: std::snprintf(buf, sizeof(buf), "left"); break;
        case SDLK_RIGHT: std::snprintf(buf, sizeof(buf), "right"); break;
        case SDLK_UP: std::snprintf(buf, sizeof(buf), "up"); break;
        case SDLK_DOWN: std::snprintf(buf, sizeof(buf), "down"); break;
        case SDLK_a: std::snprintf(buf, sizeof(buf), "a"); break;
        case SDLK_d: std::snprintf(buf, sizeof(buf), "d"); break;
        case SDLK_w: std::snprintf(buf, sizeof(buf), "w"); break;
        case SDLK_s: std::snprintf(buf, sizeof(buf), "s"); break;
        case SDLK_j: std::snprintf(buf, sizeof(buf), "j"); break;
        case SDLK_k: std::snprintf(buf, sizeof(buf), "k"); break;
        case SDLK_l: std::snprintf(buf, sizeof(buf), "l"); break;
        case SDLK_u: std::snprintf(buf, sizeof(buf), "u"); break;
        case SDLK_i: std::snprintf(buf, sizeof(buf), "i"); break;
        case SDLK_o: std::snprintf(buf, sizeof(buf), "o"); break;
        case SDLK_e: std::snprintf(buf, sizeof(buf), "e"); break;
        case SDLK_q: std::snprintf(buf, sizeof(buf), "q"); break;
        case SDLK_r: std::snprintf(buf, sizeof(buf), "r"); break;
        case SDLK_t: std::snprintf(buf, sizeof(buf), "t"); break;
        default: std::snprintf(buf, sizeof(buf), "key_%d", static_cast<int>(sym)); break;
        }
        auto action = static_cast<Action>(remapAction_);
        auto binds = input_.Bindings();
        binds[action] = buf;
        input_.SetBindings(binds);
        progress_.settings.keybinds[std::string(ActionName(action))] = buf;
        remapActive_ = false;
        audio_.PlaySfx(AudioEngine::SfxUiSelect);
        SaveNow();
    }
}

// =========================================================================
// Render
// =========================================================================
void Game::OnRender(double frameDt) {
    (void)frameDt;
    const int viewW = cfg_.width, viewH = cfg_.height;

    if (screen_ == Screen::Title) {
        DrawMeadowsBackground(renderer_, cam_, screenT_, viewW, viewH);
        screens_.DrawTitle(renderer_, font_, viewW, viewH, screenT_, titleMenu_, 0);
    } else if (screen_ == Screen::Options) {
        DrawMeadowsBackground(renderer_, cam_, screenT_, viewW, viewH);
        screens_.DrawOptions(renderer_, font_, viewW, viewH, progress_);
        // Row highlight + remap text.
        int x = (viewW - 400) / 2 + 30;
        int y = 60 + optionsRow_ * 28;
        renderer_.RectFillScreen({x - 8, y - 4, 400, 22}, 0x337FD4FF);
        if (remapActive_) {
            std::string t = "PRESS A KEY FOR " + std::string(ActionName(static_cast<Action>(remapAction_)));
            font_.DrawCenter(renderer_, {0, viewH - 90, viewW, 30}, t, 1, 0xFFFFE08A);
        }
        font_.Draw(renderer_, 20, viewH - 26, "LEFT/RIGHT ADJUST   A REMAPS   ESC BACK", 1, 0x88FBF6EC, 0, false);
    } else if (screen_ == Screen::Slots) {
        DrawMeadowsBackground(renderer_, cam_, screenT_, viewW, viewH);
        screens_.DrawSlots(renderer_, font_, viewW, viewH, "NEW GAME", slotsMenu_,
                           saves_.SlotMeta(0), saves_.SlotMeta(1), saves_.SlotMeta(2));
    } else if (screen_ == Screen::Rank) {
        renderer_.Clear(0xFF17131F);
        RankScreenData d;
        d.result = rank_;
        d.t = rankT_;
        screens_.DrawRank(renderer_, font_, viewW, viewH, d);
        if (rankT_ > 1.0)
            font_.DrawCenter(renderer_, {0, viewH - 40, viewW, 24}, "PRESS JUMP TO CONTINUE", 1, 0x88FBF6EC);
    } else if (screen_ == Screen::Ending) {
        DrawMeadowsBackground(renderer_, cam_, screenT_, viewW, viewH);
        if (screenT_ < 3.5)
            screens_.DrawEnding(renderer_, font_, viewW, viewH, endingKind_, screenT_);
        else
            screens_.DrawCredits(renderer_, font_, viewW, viewH, credits_, screenT_ - 3.5);
    } else {
        // Playing / Paused / Dialogue / Complete / GameOver: world first.
        RenderWorld();
        RenderHud();
        if (boss_ && boss_->inIntro)
            screens_.DrawBossIntro(renderer_, font_, viewW, viewH,
                                   boss_->def ? boss_->def->name : "");
        if (screen_ == Screen::Dialogue && !dialog_.Done()) {
            renderer_.RectFillScreen({0, 0, viewW, viewH}, 0x5517131F);
            hud_.DrawDialogue(renderer_, font_, dialogSpeaker_, dialog_.VisibleText(),
                              dialog_.Done(), viewW, viewH);
            font_.DrawCenter(renderer_, {0, viewH - 24, viewW, 16}, "JUMP: CONTINUE", 1, 0x88FBF6EC);
        }
        if (screen_ == Screen::Paused)
            screens_.DrawPause(renderer_, font_, viewW, viewH, pauseMenu_);
        if (screen_ == Screen::Complete) {
            const auto& d = level_.Data();
            screens_.DrawVictory(renderer_, font_, viewW, viewH, d.name, levelTime_,
                                 progress_.coins, level_.CollectibleTaken(),
                                 level_.CollectibleTotal());
            font_.DrawCenter(renderer_, {0, viewH - 40, viewW, 24}, "PRESS JUMP TO CONTINUE", 1, 0x88FBF6EC);
        }
        if (screen_ == Screen::GameOver) {
            renderer_.RectFillScreen({0, 0, viewW, viewH}, 0x6617131F);
            screens_.DrawGameOver(renderer_, font_, viewW, viewH, screenT_);
        }
    }

    RenderFilmFx();
    RenderCine();

    // Hurt flash.
    if (hurtFlash_ > 0.0) {
        double a = hurtFlash_ * 0.5;
        uint32_t c = (static_cast<uint32_t>(a * 255.0) << 24) | 0x551116;
        renderer_.RectFillScreen({0, 0, viewW, 26}, c);
        renderer_.RectFillScreen({0, viewH - 26, viewW, 26}, c);
        renderer_.RectFillScreen({0, 0, 26, viewH}, c);
        renderer_.RectFillScreen({viewW - 26, 0, 26, viewH}, c);
    }

    // Debug.
    DebugState ds;
    const auto& st = app_->Stats();
    ds.fps = st.renderFps;
    ds.frameMs = st.frameTimeMs;
    ds.simEntities = static_cast<int>(enemies_.size());
    ds.projectiles = static_cast<int>(projectiles_.size());
    ds.particles = particles_.AliveCount();
    ds.playerState = PlayerStateName(player_.State());
    ds.playerHp = player_.hp;
    ds.playerX = player_.pos.x;
    ds.playerY = player_.pos.y;
    ds.camX = cam_.Pos().x;
    ds.camY = cam_.Pos().y;
    ds.godMode = ctx_.godMode;
    ds.showHitboxes = debug_.State().showHitboxes;
    if (boss_) {
        ds.bossState = boss_->phase;
        ds.bossHp = boss_->hp;
    }
    if (ctx_.godMode || debug_.State().showHitboxes || cfg_.headless) {
        debug_.State().godMode = ds.godMode;
        ds.lastEvent = "E:" + std::to_string(static_cast<int>(enemies_.size()));
        debug_.Draw(renderer_, font_, ds, viewW, viewH);
        if (debug_.State().showHitboxes) {
            renderer_.RectOutlineWorld(player_.Box(), 0xFF7FD4FF, 1.0, 1);
            for (auto& en : enemies_)
                if (en->alive)
                    renderer_.RectOutlineWorld(en->Box(), 0xFFFFE08A, 1.0, 1);
            if (boss_ && boss_->alive)
                renderer_.RectOutlineWorld(boss_->Box(), 0xFFC8452E, 1.0, 2);
            for (auto& p : projectiles_)
                if (!p.dead)
                    renderer_.RectOutlineWorld(p.Area(), 0xFFE8A0A8, 1.0, 1);
        }
    }

    if (!errorMsg_.empty() && errorFlashT_ > 0.0)
        font_.Draw(renderer_, 20, 40, errorMsg_, 1, 0xFFFF8080, 0, false);
}

void Game::RenderWorld() {
    const int viewW = cfg_.width, viewH = cfg_.height;
    DrawMeadowsBackground(renderer_, cam_, ctx_.time, viewW, viewH);
    renderer_.SetCamera(cam_, viewW, viewH);
    const Rect view = renderer_.ViewWorldRect();

    // Tiles.
    const auto& d = level_.Data();
    const int ts = 16;
    int c0x = static_cast<int>(view.x / ts) - 1;
    int c0y = static_cast<int>(view.y / ts) - 1;
    int c1x = static_cast<int>(view.Right() / ts) + 1;
    int c1y = static_cast<int>(view.Bottom() / ts) + 1;
    for (int cy = std::max(0, c0y); cy < std::min(d.h, c1y); ++cy) {
        for (int cx = std::max(0, c0x); cx < std::min(d.w, c1x); ++cx) {
            TileType t = level_.At(cx, cy);
            if (t == TileType::Empty)
                continue;
            std::string art;
            switch (t) {
            case TileType::Solid:
                art = (cy > 0 && level_.At(cx, cy - 1) == TileType::Empty) ? "tile_grass" : "tile_solid";
                break;
            case TileType::Oneway: art = "tile_oneway"; break;
            case TileType::Hazard: art = "tile_hazard"; break;
            case TileType::Breakable: art = "tile_break"; break;
            case TileType::Door: art = "tile_door"; break;
            default: continue;
            }
            if (!bank_.Has(art))
                continue;
            renderer_.Sprite(bank_.TextureId(), bank_.Rect(art),
                             {cx * ts, cy * ts, ts, ts}, false, 0xFFFFFFFF);
        }
    }

    // Moving platforms.
    for (const auto& p : level_.Platforms())
        renderer_.Sprite(bank_.TextureId(), bank_.Rect("tile_oneway"),
                         {p.pos.x, p.pos.y, 64.0, 8.0}, false, 0xFFFFFFFF);

    // Collectibles.
    for (std::size_t i = 0; i < level_.Collectibles().size(); ++i) {
        const auto& c = level_.Collectibles()[i];
        if (c.taken)
            continue;
        double bob = std::sin(ctx_.time * 3.0 + static_cast<double>(i)) * 2.0;
        if (c.type == "coin") {
            int f = static_cast<int>(ctx_.time * 6.0) % 2;
            std::string art = f ? "coin_01" : "coin_00";
            renderer_.Sprite(bank_.TextureId(), bank_.Rect(art),
                             {c.pos.x - 4.0, c.pos.y - 4.0 + bob, 8.0, 8.0}, false, 0xFFFFFFFF);
        } else if (c.type == "film") {
            renderer_.Sprite(bank_.TextureId(), bank_.Rect("film_strip"),
                             {c.pos.x - 6.0, c.pos.y - 5.0 + bob, 12.0, 10.0}, false, 0xFFFFFFFF);
        } else if (c.type == "frame") {
            double pulse = 0.7 + 0.3 * std::sin(ctx_.time * 4.0);
            renderer_.Sprite(bank_.TextureId(), bank_.Rect("master_frame"),
                             {c.pos.x - 8.0, c.pos.y - 8.0 + bob, 16.0, 16.0}, false, 0xFFFFFFFF,
                             pulse);
        } else if (c.type == "stamp") {
            renderer_.Sprite(bank_.TextureId(), bank_.Rect("director_stamp"),
                             {c.pos.x - 5.0, c.pos.y - 5.0 + bob, 10.0, 10.0}, false, 0xFFFFFFFF);
        } else if (c.type == "ink") {
            renderer_.Sprite(bank_.TextureId(), bank_.Rect("ink_vial"),
                             {c.pos.x - 4.0, c.pos.y - 5.0 + bob, 8.0, 10.0}, false, 0xFFFFFFFF);
        }
    }

    // Checkpoints.
    for (std::size_t i = 0; i < level_.Checkpoints().size(); ++i) {
        const Vec2& cp = level_.Checkpoints()[i];
        std::string art = (i <= static_cast<std::size_t>(level_.ActiveCheckpoint())) ? "tile_cp_on"
                                                                                     : "tile_cp_off";
        renderer_.Sprite(bank_.TextureId(), bank_.Rect(art),
                         {cp.x - 8.0, cp.y - 16.0, 16.0, 16.0}, false, 0xFFFFFFFF);
    }

    // NPCs.
    for (const auto& n : d.npcs) {
        int f = static_cast<int>(ctx_.time * 1.5) % 2;
        std::string art = n.id == "owl" ? ("owl_" + Pad2(f)) : ("birdie_" + Pad2(f));
        int h = n.id == "owl" ? 22 : 14;
        renderer_.Sprite(bank_.TextureId(), bank_.Rect(art),
                         {n.pos.x - 8.0, n.pos.y - h, 16.0, static_cast<double>(h)}, false,
                         0xFFFFFFFF);
    }

    RenderPlayer();
    RenderEnemies();
    RenderBoss();
    RenderProjectilesAndParticles();

    // Exit glow when open.
    if (!level_.HasBoss() || bossDefeated_) {
        Rect ez = level_.ExitZone();
        double a = 0.4 + 0.3 * std::sin(ctx_.time * 3.0);
        renderer_.RectFillWorld({ez.x, ez.y, ez.w, ez.h},
                                (static_cast<uint32_t>(a * 255.0) << 24) | 0x557FD4FF);
    }
}

void Game::RenderPlayer() {
    const auto& p = player_;
    if (!p.alive && playerDeathT_ > 1.5)
        return;
    std::string key;
    switch (p.State()) {
    case PlayerState::Idle:
        key = (fmod(ctx_.time, 3.0) < 0.12) ? "milo_idle_01" : "milo_idle_00";
        break;
    case PlayerState::Run:
        key = "milo_run_" + Pad2(static_cast<int>(ctx_.time * 12.0) % 3);
        break;
    case PlayerState::Jump: key = "milo_jump_00"; break;
    case PlayerState::Fall: key = "milo_fall_00"; break;
    case PlayerState::Crouch: key = "milo_crouch_00"; break;
    case PlayerState::Dash:
    case PlayerState::AirDash: key = "milo_dash_00"; break;
    case PlayerState::Attack:
        key = p.StateT() < 0.11 ? "milo_attack_00" : "milo_attack_01";
        break;
    case PlayerState::Parry: key = "milo_parry_00"; break;
    case PlayerState::Hurt: key = "milo_hurt_00"; break;
    case PlayerState::Dead: key = "milo_dead_00"; break;
    case PlayerState::Super: key = "milo_super_00"; break;
    default: key = "milo_idle_00"; break;
    }
    double alpha = 1.0;
    if (p.invulnT > 0.0 && p.State() != PlayerState::Dead)
        alpha = (static_cast<int>(ctx_.time * 14.0) % 2) ? 0.45 : 1.0;
    uint32_t tint = p.State() == PlayerState::Super ? 0xFFBFF0FF : 0xFFFFFFFF;
    if (p.hitFlashT > 0.0)
        tint = 0xFFFFFFFF;
    Rect dst{p.pos.x - 1.0, p.pos.y, 16.0, 20.0};
    if (p.Crouching())
        dst = {p.pos.x - 1.0, p.pos.y + 10.0, 16.0, 10.0};
    renderer_.Sprite(bank_.TextureId(), bank_.Rect(key), dst, !p.FacingRight(), tint, alpha);
    // Parry glint.
    if (p.IsParryActive())
        renderer_.RectOutlineWorld(Rect{p.pos.x - 4.0, p.pos.y - 2.0, p.w + 8.0, p.h + 4.0},
                                   0x887FD4FF, 1.0, 2);
}

void Game::RenderEnemies() {
    for (auto& en : enemies_) {
        std::string key = ArtKey(en->ArtName(), en->ArtFrame());
        if (!bank_.Has(key))
            key = ArtKey(en->ArtName(), 0);
        double alpha = en->alive ? 1.0 : std::max(0.0, 1.0 - en->deadT);
        uint32_t tint = en->hitFlashT > 0.0 ? 0xFFFFFFFF : 0xFFFFFFFF;
        renderer_.Sprite(bank_.TextureId(), bank_.Rect(key), en->Box(), !en->facingRight, tint,
                         alpha);
    }
}

void Game::RenderBoss() {
    if (!boss_)
        return;
    std::string key = ArtKey(boss_->ArtName(), boss_->ArtFrame());
    if (!bank_.Has(key))
        key = ArtKey(boss_->ArtName(), 0);
    double alpha = boss_->inIntro ? Clamp(boss_->introT / 0.5, 0.0, 1.0) : 1.0;
    renderer_.Sprite(bank_.TextureId(), bank_.Rect(key), boss_->Box(), !boss_->facingRight,
                     0xFFFFFFFF, alpha);
    // Telegraph zones.
    for (const auto& z : boss_->TellZones())
        renderer_.RectFillWorld(z, 0x33C8452E);
}

void Game::RenderProjectilesAndParticles() {
    for (auto& p : projectiles_) {
        if (p.dead)
            continue;
        Vec2 c = p.e.Center();
        uint32_t outer = p.fromPlayer ? 0xFF26213C : 0xFF7A2E1E;
        uint32_t core = p.fromPlayer ? 0xFF7FD4FF : 0xFFE8B84B;
        double r = p.def.radius;
        renderer_.RectFillWorld({c.x - r, c.y - r, r * 2.0, r * 2.0}, outer);
        renderer_.RectFillWorld({c.x - r * 0.5, c.y - r * 0.5, r, r}, core);
    }
    for (const auto& pr : particles_.Renders())
        renderer_.RectFillWorld({pr.x, pr.y, pr.w, pr.h}, pr.color, pr.alpha);
}

void Game::RenderHud() {
    if (screen_ != Screen::Playing && screen_ != Screen::Paused &&
        screen_ != Screen::Dialogue)
        return;
    HudState s;
    s.hp = std::max(0, player_.hp);
    s.maxHp = player_.Stats().maxHp;
    s.energy = player_.Energy();
    s.superReady = player_.SuperReady();
    s.superActive = player_.SuperActive();
    const WeaponDef* w = player_.Weapon();
    s.weaponName = w ? w->name : "Ink Blaster";
    s.coins = progress_.coins;
    s.combo = player_.Combo();
    s.hurtFlash = hurtFlash_;
    if (boss_ && !boss_->inIntro) {
        s.bossActive = true;
        s.bossName = boss_->def ? boss_->def->name : "";
        s.bossHpf = boss_->Hpf();
        s.bossPhase = boss_->phase;
        s.bossPhaseCount = boss_->PhaseCount();
    }
    if (bannerT_ > 0.0) {
        int wi = 0;
        for (int i = 0; i < worlds_.Count(); ++i)
            if (worlds_.ByIndex(i) && worlds_.ByIndex(i)->id == progress_.currentWorld)
                wi = i + 1;
        s.banner = "WORLD " + std::to_string(wi) + "  " + level_.Data().name;
    }
    s.objective = level_.HasBoss() ? (bossDefeated_ ? "FIND THE EXIT" : "DEFEAT THE BOSS")
                                   : "REACH THE EXIT";
    hud_.Draw(renderer_, font_, fontScale_, s, cfg_.width, cfg_.height);
}

void Game::RenderFilmFx() {
    if (filmFx_ <= 0.0 || reducedFlash_)
        return;
    const int viewW = cfg_.width, viewH = cfg_.height;
    // Grain.
    int n = static_cast<int>(40.0 * filmFx_);
    for (int i = 0; i < n; ++i) {
        int x = uiRng_.range(0, viewW - 1);
        int y = uiRng_.range(0, viewH - 1);
        uint32_t c = uiRng_.chance(0.5) ? 0x18FFFFFF : 0x1817131F;
        renderer_.RectFillScreen({x, y, 2, 2}, c);
    }
    // Flicker.
    double fl = (0.02 + 0.03 * filmFx_) * uiRng_.unit();
    if (fl > 0.015)
        renderer_.RectFillScreen({0, 0, viewW, viewH},
                                 (static_cast<uint32_t>(fl * 255.0) << 24) | 0x000000);
    // Occasional scratch.
    if (uiRng_.chance(0.02)) {
        int x = uiRng_.range(0, viewW - 1);
        renderer_.RectFillScreen({x, 0, 1, viewH}, 0x22FFFFFF);
    }
}

void Game::RenderCine() {
    if (!cine_.Active())
        return;
    const int viewW = cfg_.width, viewH = cfg_.height;
    if (cine_.Letterbox() > 0.0)
        renderer_.RectFillScreen({0, 0, viewW, 56}, 0xFF0B0910),
            renderer_.RectFillScreen({0, viewH - 56, viewW, 56}, 0xFF0B0910);
    if (const std::string* card = cine_.Card())
        font_.DrawCenter(renderer_, {0, viewH / 2 - 20, viewW, 40}, *card, 2, 0xFFF6E7C8);
}

void Game::OnError(const std::string& msg) {
    INK_LOG_ERROR("runtime error: {}", msg);
    errorMsg_ = msg;
    errorFlashT_ = 4.0;
}

} // namespace ink
