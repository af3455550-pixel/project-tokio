#pragma once
// Game: the app orchestrator. Owns the renderer, camera, input, audio, UI
// and the live simulation (level, player, enemies, boss, projectiles).
// It is the ONLY place that translates sim GameEvents into SFX / VFX /
// camera / music / UI feedback (§7 seam), and drives the screen state
// machine (title -> options/slots -> playing -> rank/complete/ending).
#include "Art/Art.h"
#include "Audio/AudioEngine.h"
#include "Audio/MusicData.h"
#include "Bosses/Boss.h"
#include "Charms/CharmDef.h"
#include "Cinematics/Cine.h"
#include "Core/Event.h"
#include "Core/Rng.h"
#include "Dialogue/Dialogue.h"
#include "Enemies/EnemyBook.h"
#include "Engine/Application.h"
#include "Gameplay/GameEvent.h"
#include "Gameplay/Level.h"
#include "Gameplay/Projectile.h"
#include "Gameplay/SimContext.h"
#include "Input/InputManager.h"
#include "Player/Player.h"
#include "Progression/Achievements.h"
#include "Quests/QuestBook.h"
#include "Rendering/Camera2D.h"
#include "Rendering/Renderer2D.h"
#include "Rendering/SpriteBank.h"
#include "Save/SaveSystem.h"
#include "Tools/DebugOverlay.h"
#include "UI/BitmapFont.h"
#include "UI/Hud.h"
#include "UI/Screens.h"
#include "VFX/Particles.h"
#include "Weapons/WeaponDef.h"
#include "World/WorldCatalog.h"
#include <memory>
#include <string>
#include <vector>

union SDL_Event; // SDL declares SDL_Event as a union — keep the tag in sync

namespace ink {

class Game : public IAppCallbacks {
public:
    bool Init(Application& app, std::string* err);

    // IAppCallbacks
    void OnUpdate(double dt) override;
    void OnRender(double frameDt) override;
    void OnError(const std::string& msg) override;

private:
    enum class Screen {
        Title,
        Options,
        Slots,
        Playing,
        Paused,
        Dialogue,
        Complete,
        GameOver,
        Rank,
        Ending
    };

    // ---- setup --------------------------------------------------------
    bool LoadData(std::string* err);
    void ApplySettings();
    void StartNewGame(int slot);
    void ContinueGame(int slot);
    void LoadLevel(const std::string& levelId);
    void StartLevelCine();
    void TriggerBoss();
    void OnLevelComplete();
    void GoTitle();
    void SaveNow();

    // ---- sim ----------------------------------------------------------
    void UpdatePlaying(double dt);
    void UpdateMenus(double dt);
    void UpdateOptions(double dt);
    void UpdateDialogue(double dt);
    void UpdateComplete(double dt);
    void UpdateGameOver(double dt);
    void UpdateRank(double dt);
    void UpdateEnding(double dt);
    void BuildPlayerInput(PlayerInput& in) const;
    void OnGameEvent(const GameEvent& e);
    void OnRawEvent(const SDL_Event& e);
    void PickupsAndZones(double dt);
    void HandleNpcTalk();
    void ComputeRank();

    // ---- render -------------------------------------------------------
    void RenderWorld();
    void RenderPlayer();
    void RenderEnemies();
    void RenderBoss();
    void RenderProjectilesAndParticles();
    void RenderHud();
    void RenderFilmFx();
    void RenderCine();
    std::string PlayerArtKey() const;

    // ---- systems ------------------------------------------------------
    Application* app_ = nullptr;
    AppConfig cfg_{};
    std::string assetsDir_ = "Assets";

    Renderer2D renderer_;
    Camera2D cam_;
    SpriteBank bank_;
    InputManager input_;
    AudioEngine audio_;
    BitmapFont font_;
    Hud hud_;
    Screens screens_;
    DebugOverlay debug_;

    // sim
    Rng rng_{1337};
    Event<GameEvent> events_;
    ParticleSystem particles_;
    SimContext ctx_;
    Level level_;
    Player player_;
    std::vector<std::unique_ptr<Enemy>> enemies_;
    std::vector<Enemy*> liveEnemies_; // SimContext view (mirrors enemies_)
    std::unique_ptr<Boss> boss_;
    std::vector<Projectile> projectiles_;

    // data (the "editor" — JSON driven)
    WeaponBook weapons_;
    EnemyBook enemyBook_;
    BossBook bossBook_;
    WorldCatalog worlds_;
    CharmBook charmBook_;
    DialogueBook dialogue_;
    QuestBook quests_;
    AchievementBook achievements_;

    // progress
    SaveSystem saves_;
    GameProgress progress_;
    int slot_ = -1;
    MetaCounters counters_;

    // screen state
    Screen screen_ = Screen::Title;
    double screenT_ = 0.0;
    double levelTime_ = 0.0;
    double bannerT_ = 0.0;
    Menu titleMenu_;
    Menu slotsMenu_;
    Menu pauseMenu_;
    int optionsRow_ = 0;
    bool remapActive_ = false;
    int remapAction_ = 0; // index into Action enum
    DialoguePlayer dialog_;
    std::string dialogSpeaker_;
    CineDef cineDef_{}; // storage for cine_ (Cine holds a pointer)
    Cine cine_;
    RankResult rank_;
    double rankT_ = 0.0;
    std::string endingKind_ = "normal";
    CreditsData credits_;
    double creditsEndT_ = 90.0; // autotest: auto-advance to title

    // run state
    bool bossSpawned_ = false;
    bool bossDefeated_ = false;
    bool rankShown_ = false;
    double playerDeathT_ = 0.0;
    double hitStopT_ = 0.0;
    double slowMoT_ = 0.0;
    double hurtFlash_ = 0.0;
    double npcTalkCd_ = 0.0;
    int damageTakenThisBoss_ = 0;
    int parriesThisBoss_ = 0;
    double fontScale_ = 1.0;
    double filmFx_ = 1.0;
    bool reducedFlash_ = false;
    Rng uiRng_{777};

    bool quitRequested_ = false;
    double errorFlashT_ = 0.0;
    std::string errorMsg_;

    // Headless QA hooks (INK_AUTOTEST=1, INK_SHOTS=frame:path,frame:path).
    bool autotest_ = false;
    std::vector<std::pair<int, std::string>> shots_;
};

} // namespace ink
