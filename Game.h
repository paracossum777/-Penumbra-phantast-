#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <vector>
#include "Battle.h"

using namespace sf;
using namespace std;

// ─── Коды состояний игры (gameState) ─────────────────────────────────────────
// 0  — главное меню
// 1  — карта перемещений
// 2  — обычный бой
// 3  — магазин
// 4  — экран наград после боя (выбор золота / карты)
// 5  — выбор карты-награды (анимация)
// 6  — бой с боссом
// 7  — экран финальной победы
// 8  — костёр (отдых)
// 9  — экран поражения в обычном бою
// 10 — экран поражения от босса

class Game
{
private:

    // ── Узел карты мира ───────────────────────────────────────────────────────
    // type: 0=враг, 1=магазин, 2=костёр, 3=босс
    struct Node {
        CircleShape circle;
        int         type;
        bool        visited;   // узел уже пройден
        bool        available; // узел доступен для выбора прямо сейчас
    };

    // ── Связь между узлами ────────────────────────────────────────────────────
    struct Link { int from; int to; };

    // ── Карта в магазине ──────────────────────────────────────────────────────
    struct ShopCard
    {
        CardData       data;
        int            price;
        bool           sold;
        RectangleShape body;
        Text* nameText;
        Text* descText;
        Text* costText;
        Text* priceText;
        RectangleShape priceBg;
    };

    // ── Прогресс по карте мира ────────────────────────────────────────────────
    int  currentNodeIndex;         // -1 = путь ещё не начат
    void updateAvailableNodes();   // пересчитывает доступность узлов
    void resetMapProgress();       // сброс при рестарте игры

    // ── Колода игрока (хранится между боями) ─────────────────────────────────
    vector<CardData> playerDeck;

    // ── Карты-награды после боя (3 варианта на выбор) ────────────────────────
    CardData rewardCardChoices[3];
    void generateRewardCards();

    // ── Просмотрщик колоды ───────────────────────────────────────────────────
    bool           showDeckViewer;
    RectangleShape deckViewerBg;
    RectangleShape deckViewerPanel;
    RectangleShape deckViewerButton;  // кнопка открытия на экране карты мира
    Text* deckViewerTitle;
    Text* deckViewerCloseText;
    RectangleShape deckViewerCloseBtn;
    int            deckViewerScroll;  // строка прокрутки (0 = начало)

    // ── Окно ─────────────────────────────────────────────────────────────────
    RenderWindow* window;
    VideoMode      videoMode;

    // ── Боевая система ───────────────────────────────────────────────────────
    Battle battleSystem;

    // ── HP героя (сохраняются между боями) ───────────────────────────────────
    int heroCurrentHP = 80;
    int heroMaxHP = 80;

    // ── Звуки и музыка ───────────────────────────────────────────────────────
    SoundBuffer bgMusicBuffer;
    SoundBuffer selectionSoundBuffer;
    SoundBuffer selectionCardAudioBuffer;
    SoundBuffer defeatSoundBuffer;
    SoundBuffer victoryMusicBuffer;
    SoundBuffer shopMusicBuffer;
    SoundBuffer battleMusicBuffer;
    SoundBuffer bossMusicBuffer;

    Sound* sound;          // фоновая музыка меню (A/B)
    Sound* selectionAudio; // звук наведения на кнопку
    Sound* cardAudio;      // звук выбора карты
    Sound* defeatSound;    // звук поражения (экраны 9/10)
    Sound* victorySound;   // музыка победы (экран 7)
    Sound* shopMusic;      // музыка магазина (looping, пока state==3)
    Sound* battleMusic;    // музыка обычного боя (looping, пока state==2)
    Sound* bossMusic;      // музыка боя с боссом (looping, пока state==6)

    int previousGameState; // для определения входа/выхода из состояний

    // ── Таймер ───────────────────────────────────────────────────────────────
    Clock clock;

    // ── Шрифты и тексты UI ───────────────────────────────────────────────────
    Font  mainfont;  // декоративный шрифт для заголовков
    Font  font;      // Arial для всего остального

    Text* menuText;           // "Выберите вашу карту" (экран выбора карты-награды)
    Text* mapText;            // подпись на карте мира
    Text* legendTitle;
    Text* legendEnemyText;
    Text* legendShopText;
    Text* legendBossText;
    Text* legendCampfireText;

    // ── Главное меню ─────────────────────────────────────────────────────────
    RectangleShape backgraund;
    Texture        bkgraundImg;
    Text* mainMenu;
    Text* startButtonText;
    Text* exsitButtonText;
    Text* helpButtonText;

    RectangleShape startButton;   Texture startIMG;
    RectangleShape exsitButton;   Texture exsitIMG;
    RectangleShape helpButton;    Texture helpIMG;

    // ── Легенда карты мира ────────────────────────────────────────────────────
    RectangleShape legendBg;
    CircleShape    legendEnemyCircle;
    CircleShape    legendShopCircle;
    CircleShape    legendBossCircle;
    CircleShape    legendCampfireCircle;

    // ── Состояние игры ────────────────────────────────────────────────────────
    int gameState;

    // ── Граф карты мира ───────────────────────────────────────────────────────
    vector<Node> nodes;
    vector<Link> links;

    // ── Костёр (тип узла 2) ──────────────────────────────────────────────────
    RectangleShape campfireBg;
    RectangleShape campfireRestButton;
    RectangleShape campfireContinueButton;
    Text* campfireTitleText;
    Text* campfireHintText;
    Text* campfireRestText;
    Text* campfireContinueText;
    bool           campfireRested; // отдых уже использован на этом костре

    void updateCampfire(Vector2f mousePos, bool clicked);
    void renderCampfire();

    // ── Экраны конца забега (победа/поражение) ────────────────────────────────
    RectangleShape endRunMenuButton;
    Text* endRunMenuButtonText;
    Text* endRunTitleText;
    Text* endRunSubText;
    Text* endRunStatsText;
    bool           endRunButtonReady; // кнопка «В меню» появляется после звука

    void initEndRunScreen();
    void updateEndRunScreen(Vector2f mousePos, bool clicked);
    void renderEndRunScreen(int outcome); // 0=бой, 1=победа, 2=босс
    void returnToMainMenu();              // сбрасывает прогресс и возвращает в меню

    // ── Анимированные карты-награды (3 штуки) ───────────────────────────────
    // shape/shape2/shape3 — визуальные прямоугольники для анимации выбора
    RectangleShape shape, shape2, shape3;

    // ── UI окна наград ─────────────────────────────────────────────────────
    RectangleShape rewardBlock;
    RectangleShape rewardBlockBg;
    RectangleShape goldBlock;
    RectangleShape cardsBlock;
    RectangleShape continueButton;
    Text* rewardText;
    Text* rewardGoldText;
    Text* rewardCardsText;
    Text* continueButtonText;

    // ── Деньги ────────────────────────────────────────────────────────────────
    Text* moneyText;
    int   moneyCount = 0;
    bool  goldSelected; // золото уже взято на экране наград

    // ── Спрайты персонажей ────────────────────────────────────────────────────
    RectangleShape enemy;
    Texture enemyTex_Tutorial, enemyTex_Brute, enemyTex_Shaman;
    Texture enemyTex_Duelist, enemyTex_Assassin, enemyTex_Boss;

    RectangleShape hero;      Texture heroIMG;
    RectangleShape shopKeeper; Texture shopkeeperIMG;

    void applyEnemySprite(); // применяет текстуру/размер в зависимости от типа врага

    // ── Флаги состояния ───────────────────────────────────────────────────────
    bool endGame; // флаг "игра завершена" (передаётся в Battle::render)

    // ── Анимация выбора карты-награды ────────────────────────────────────────
    // active1/2/3 — карта анимируется; stage — фаза (1=сжатие, 2=вращение, 3=падение)
    bool  active1, active2, active3, selected;
    bool  blockInput;
    int   stage1, stage2, stage3;
    float rotationAngle1, rotationAngle2, rotationAngle3;
    float center1X, center2X, center3X, centerY; // опорные позиции трёх карт

    // ── Магазин ──────────────────────────────────────────────────────────────
    vector<ShopCard> shopCards;
    RectangleShape   shopBg;
    RectangleShape   shopPanel;
    Text* shopTitleText;
    RectangleShape   shopExitButton;
    Text* shopExitText;
    bool             shopMouseWasDown; // предыдущий стейт ЛКМ для edge-detect

    // Услуги магазина (каждая доступна один раз за посещение)
    static const int sellCardPrice = 75;
    static const int healPrice = 100;
    bool cardSoldThisVisit;
    bool healedThisVisit;

    RectangleShape sellCardButton; Text* sellCardButtonText;
    RectangleShape healButton;     Text* healButtonText;

    // Экран выбора карты для продажи (модальное окно поверх магазина)
    bool           showSellCardScreen;
    RectangleShape sellScreenBg;
    RectangleShape sellScreenPanel;
    Text* sellScreenTitle;
    RectangleShape sellScreenCloseBtn;
    Text* sellScreenCloseText;
    int            sellScreenScroll;

    void renderSellCardScreen();
    void updateSellCardScreen(Vector2f mousePos, bool clicked);

    void generateShopCards();
    void clearShopCards();
    void updateShop(Vector2f mousePos, bool clicked);
    void renderShop();
    void renderDeckViewer();

    // ── Инициализация (вызываются в конструкторе) ─────────────────────────────
    void initVariables();
    void initWindow();
    void initSound();
    void initMainMenu();
    void initFont();
    void initHero();
    void initEnemy();
    void initBattleReward();
    void initShape();
    void initShop();
    void initMoneyBar();
    void initMap();
    void initDeckViewer();
    void initCampfire();

public:
    Game();
    virtual ~Game();

    const bool getWindowisOpen() const;

    void update(); // обрабатывает события и игровую логику (вызывается каждый кадр)
    void render(); // рисует текущий экран (вызывается каждый кадр)
};
