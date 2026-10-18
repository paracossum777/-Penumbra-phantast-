#include "Game.h"
#include <iostream>
#include <ctime>

// Открытие .chm-справки без подключения <windows.h> (избегаем конфликт sf::byte / std::byte)
// ShellExecuteW не требует дополнительных библиотек — Windows открывает .chm сам
#ifdef _WIN32
extern "C" __declspec(dllimport) void* __stdcall ShellExecuteW(
    void* hwnd, const wchar_t* lpOperation, const wchar_t* lpFile,
    const wchar_t* lpParameters, const wchar_t* lpDirectory, int nShowCmd);
#define SW_SHOWNORMAL 1
#endif

using namespace sf;
using namespace std;


// ИНИЦИАЛИЗАЦИЯ

// Обнуляем все указатели и поля до безопасных значений 
void Game::initVariables()
{
    endGame = false;
    window = nullptr;

    // Звуковые объекты
    sound = selectionAudio = cardAudio = defeatSound = nullptr;
    victorySound = shopMusic = battleMusic = bossMusic = nullptr;
    previousGameState = -1;

    // Тексты меню
    menuText = mapText = legendTitle = nullptr;
    legendEnemyText = legendShopText = legendCampfireText = legendBossText = nullptr;
    mainMenu = startButtonText = exsitButtonText = helpButtonText = nullptr;

    // Тексты костра
    campfireTitleText = campfireHintText = campfireRestText = campfireContinueText = nullptr;

    // Тексты экрана конца забега
    endRunMenuButtonText = endRunTitleText = endRunSubText = endRunStatsText = nullptr;
    endRunButtonReady = false;

    // Тексты наград и магазина
    rewardText = continueButtonText = rewardCardsText = rewardGoldText = nullptr;
    moneyText = nullptr;
    moneyCount = 50; // стартовое золото

    shopTitleText = shopExitText = nullptr;
    shopMouseWasDown = false;
    cardSoldThisVisit = healedThisVisit = false;
    sellCardButtonText = healButtonText = nullptr;

    // Тексты экрана продажи карт
    showSellCardScreen = false;
    sellScreenTitle = sellScreenCloseText = nullptr;
    sellScreenScroll = 0;

    // Просмотрщик колоды
    showDeckViewer = false;
    deckViewerTitle = deckViewerCloseText = nullptr;
    deckViewerScroll = 0;

    // Состояние
    gameState = 0;
    goldSelected = false;
    currentNodeIndex = -1;
    campfireRested = false;

    // Анимация выбора карты-награды
    active1 = active2 = active3 = selected = blockInput = false;
    stage1 = stage2 = stage3 = 0;
    rotationAngle1 = rotationAngle2 = rotationAngle3 = 0.f;
}

// Окно 
void Game::initWindow()
{
    window = new RenderWindow(VideoMode({ 800, 600 }), "Penumbra phantast",
        Style::Titlebar | Style::Resize | Style::Close);
    window->setFramerateLimit(30);
    Image icon;
    if (icon.loadFromFile("image/icon.png"))
        window->setIcon(icon.getSize(), icon.getPixelsPtr());
}

// ── Загрузка звуков и музыки 
// Для каждого файла: пытаемся загрузить, при ошибке — оставляем nullptr.
void Game::initSound()
{
    // Вспомогательная лямбда: загружает буфер и создаёт Sound; сообщает об ошибке
    auto loadSound = [&](SoundBuffer& buf, Sound*& snd, const char* file, bool looping = false)
        {
            if (!buf.loadFromFile(file))
            {
                cout << "Alarm! Could not load " << file << endl;
                snd = nullptr;
            }
            else
            {
                snd = new Sound(buf);
                if (looping) snd->setLooping(true);
            }
        };

    loadSound(bgMusicBuffer, sound, "sound/Oldestdaughter.ogg");
    loadSound(selectionSoundBuffer, selectionAudio, "sound/selection.ogg");
    loadSound(selectionCardAudioBuffer, cardAudio, "sound/Selecting_card_sound.ogg");
    loadSound(defeatSoundBuffer, defeatSound, "sound/Death_of_Hero.ogg");
    loadSound(victoryMusicBuffer, victorySound, "sound/WICTORY.ogg");
    loadSound(shopMusicBuffer, shopMusic, "sound/Shop-Theame.ogg", true);
    loadSound(battleMusicBuffer, battleMusic, "sound/Yeah-let_s-fight.ogg", true);
    loadSound(bossMusicBuffer, bossMusic, "sound/Final-fight.ogg", true);
}

// ── Шрифты и первичные тексты 
void Game::initFont()
{
    if (!mainfont.openFromFile("Fonts/Adventure Indiana.ttf")) return;
    if (!font.openFromFile("Fonts/Arial.ttf")) return;

    // Заголовок экрана выбора карты-награды
    menuText = new Text(mainfont, L"Выберите вашу карту", 40);
    menuText->setFillColor(Color::Yellow);
    FloatRect tb = menuText->getLocalBounds();
    menuText->setOrigin({ tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f });
    menuText->setPosition({ 400.f, 50.f });

    // Инициализируем визуал боевой системы (один раз на весь запуск)
    battleSystem.initBattle(&font);
}

// ── Главное меню ─────────────────────────────────────────────────────────────
void Game::initMainMenu()
{
    backgraund.setSize({ 800.f, 600.f });
    if (!bkgraundImg.loadFromFile("image/BACKgraundIMG.jpg")) return;
    backgraund.setTexture(&bkgraundImg);

    mainMenu = new Text(mainfont, L"Penumbra phantast", 55);
    mainMenu->setFillColor(Color::Yellow);
    FloatRect tb = mainMenu->getLocalBounds();
    mainMenu->setOrigin({ tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f });
    mainMenu->setPosition({ 430.f, 180.f });

    const float btnW = 180.f, btnH = 85.f;

    // Вспомогательная лямбда для инициализации кнопки меню
    auto initMenuBtn = [&](RectangleShape& btn, Texture& tex, const char* file,
        Vector2f pos, float h)
        {
            btn.setSize({ btnW, h });
            btn.setOrigin({ btnW / 2.f, btnH / 2.f });
            btn.setPosition(pos);
            if (tex.loadFromFile(file))
                btn.setTexture(&tex);
            btn.setFillColor({ 212, 175, 55 }); // золотой по умолчанию
        };

    initMenuBtn(startButton, startIMG, "image/StartButton.png", { 160.f, 370.f }, btnH);
    initMenuBtn(exsitButton, exsitIMG, "image/ExsitButton.png", { 160.f, 450.f }, btnH - 20.f);
    initMenuBtn(helpButton, helpIMG, "image/Sprawka.png", { 160.f, 525.f }, btnH - 20.f);
}

// ── Спрайт героя ────────────────────────────────────────────────────────────
void Game::initHero()
{
    hero.setSize({ 150.f, 350.f });
    hero.setOrigin({ 75.f, 175.f });
    hero.setPosition({ 100.f, 440.f });
    if (heroIMG.loadFromFile("image/New Piskel.png"))
        hero.setTexture(&heroIMG);
}

// ── Спрайты врагов ───────────────────────────────────────────────────────────
// Загружаем все текстуры заранее; нужная применяется через applyEnemySprite().
void Game::initEnemy()
{
    enemy.setPosition({ 660.f, 300.f });
    enemy.setSize({ 150.f, 350.f });
    enemy.setOrigin({ 75.f, 175.f });

    // Загружаем все текстуры; ошибки молча игнорируются (покажется цветной прямоугольник)
    enemyTex_Tutorial.loadFromFile("image/Baltasar.png");
    enemyTex_Brute.loadFromFile("image/enemy_brute.png");
    enemyTex_Shaman.loadFromFile("image/enemy_shaman.png");
    enemyTex_Duelist.loadFromFile("image/enemy_duelist.png");
    enemyTex_Assassin.loadFromFile("image/enemy_assassin.png");
    enemyTex_Boss.loadFromFile("image/enemy_boss.png");

    if (enemyTex_Tutorial.getSize().x > 0)
        enemy.setTexture(&enemyTex_Tutorial);
}

// ── Применяет размер и текстуру врага в зависимости от его типа ──────────────
void Game::applyEnemySprite()
{
    // Конфигурация спрайта для каждого типа врага
    struct Cfg { float w, h, y; Texture* tex; };
    Cfg cfg;

    switch (battleSystem.getEnemyType())
    {
    case EnemyType::Tutorial: cfg = { 150.f, 350.f, 300.f, &enemyTex_Tutorial }; break;
    case EnemyType::Brute:    cfg = { 200.f, 400.f, 350.f, &enemyTex_Brute }; break;
    case EnemyType::Shaman:   cfg = { 130.f, 300.f, 340.f, &enemyTex_Shaman }; break;
    case EnemyType::Duelist:  cfg = { 150.f, 340.f, 300.f, &enemyTex_Duelist }; break;
    case EnemyType::Assassin: cfg = { 120.f, 300.f, 315.f, &enemyTex_Assassin }; break;
    case EnemyType::Boss:     cfg = { 220.f, 430.f, 270.f, &enemyTex_Boss }; break;
    default:                  cfg = { 150.f, 350.f, 300.f, &enemyTex_Tutorial }; break;
    }

    enemy.setSize({ cfg.w, cfg.h });
    enemy.setOrigin({ cfg.w / 2.f, cfg.h / 2.f });
    enemy.setPosition({ 660.f, cfg.y });
    enemy.setTexture((cfg.tex && cfg.tex->getSize().x > 0) ? cfg.tex : nullptr);
}

// ── Окно наград после боя ─────────────────────────────────────────────────────
void Game::initBattleReward()
{
    rewardBlockBg.setSize({ 800.f, 600.f });
    rewardBlockBg.setFillColor(Color(0, 0, 0, 180));

    rewardBlock.setSize({ 420.f, 320.f });
    rewardBlock.setFillColor(Color(30, 25, 20));
    rewardBlock.setOutlineThickness(3.f);
    rewardBlock.setOutlineColor(Color(180, 140, 50));
    rewardBlock.setPosition({ 190.f, 130.f });

    rewardText = new Text(mainfont, L"ПОБЕДА!", 42);
    rewardText->setFillColor(Color(255, 210, 50));
    FloatRect rb = rewardText->getLocalBounds();
    rewardText->setOrigin({ rb.size.x / 2.f, rb.size.y / 2.f });
    rewardText->setPosition({ 400.f, 165.f });

    // Блок золота
    goldBlock.setSize({ 380.f, 55.f });
    goldBlock.setFillColor(Color(60, 50, 10));
    goldBlock.setOutlineThickness(2.f);
    goldBlock.setOutlineColor(Color(180, 140, 50));
    goldBlock.setPosition({ 210.f, 230.f });

    rewardGoldText = new Text(font, L"+ 25 золота", 20);
    rewardGoldText->setFillColor(Color(240, 200, 60));
    rewardGoldText->setPosition({ 225.f, 245.f });

    // Блок карт
    cardsBlock.setSize({ 380.f, 55.f });
    cardsBlock.setFillColor(Color(10, 35, 60));
    cardsBlock.setOutlineThickness(2.f);
    cardsBlock.setOutlineColor(Color(80, 140, 200));
    cardsBlock.setPosition({ 210.f, 300.f });

    rewardCardsText = new Text(font, L"Добавить карту в колоду", 20);
    rewardCardsText->setFillColor(Color(140, 200, 255));
    rewardCardsText->setPosition({ 225.f, 315.f });

    // Кнопка «Продолжить»
    continueButton.setSize({ 200.f, 45.f });
    continueButton.setFillColor(Color(70, 130, 70));
    continueButton.setOutlineThickness(2.f);
    continueButton.setOutlineColor(Color(120, 220, 120));
    continueButton.setPosition({ 300.f, 380.f });

    continueButtonText = new Text(font, L"Продолжить", 20);
    continueButtonText->setFillColor(Color::White);
    FloatRect cb = continueButtonText->getLocalBounds();
    continueButtonText->setOrigin({ cb.size.x / 2.f, cb.size.y / 2.f });
    continueButtonText->setPosition({ 400.f, 402.f });
}

// ── Три анимированных прямоугольника для выбора карты-награды ─────────────────
void Game::initShape()
{
    centerY = 300.f; // вертикальный центр экрана
    center1X = 200.f;
    center2X = 400.f;
    center3X = 600.f;

    // Все три карты одинакового размера и по центру своей колонки
    for (auto* s : { &shape, &shape2, &shape3 })
    {
        s->setSize({ 100.f, 200.f });
        s->setOrigin({ 50.f, 100.f });
    }
    shape.setPosition({ center1X, centerY });
    shape2.setPosition({ center2X, centerY });
    shape3.setPosition({ center3X, centerY });
}

// ── Просмотрщик колоды ───────────────────────────────────────────────────────
void Game::initDeckViewer()
{
    deckViewerBg.setSize({ 800.f, 600.f });
    deckViewerBg.setFillColor(Color(0, 0, 0, 210));

    deckViewerPanel.setSize({ 720.f, 480.f });
    deckViewerPanel.setFillColor(Color(35, 28, 18));
    deckViewerPanel.setOutlineThickness(3.f);
    deckViewerPanel.setOutlineColor(Color(180, 140, 50));
    deckViewerPanel.setPosition({ 40.f, 60.f });

    deckViewerTitle = new Text(mainfont, L"Ваша колода", 30);
    deckViewerTitle->setFillColor(Color(255, 215, 60));
    FloatRect tb = deckViewerTitle->getLocalBounds();
    deckViewerTitle->setOrigin({ tb.size.x / 2.f, tb.size.y / 2.f });
    deckViewerTitle->setPosition({ 400.f, 85.f });

    // Кнопка открытия (на экране карты мира)
    deckViewerButton.setSize({ 130.f, 35.f });
    deckViewerButton.setFillColor(Color(50, 70, 110));
    deckViewerButton.setOutlineThickness(2.f);
    deckViewerButton.setOutlineColor(Color(100, 150, 220));
    deckViewerButton.setPosition({ 20.f, 555.f });

    // Кнопка закрытия (внутри панели)
    deckViewerCloseBtn.setSize({ 120.f, 35.f });
    deckViewerCloseBtn.setFillColor(Color(110, 40, 40));
    deckViewerCloseBtn.setOutlineThickness(2.f);
    deckViewerCloseBtn.setOutlineColor(Color(200, 70, 70));
    deckViewerCloseBtn.setPosition({ 340.f, 518.f });

    deckViewerCloseText = new Text(font, L"Закрыть", 18);
    deckViewerCloseText->setFillColor(Color::White);
    FloatRect cb = deckViewerCloseText->getLocalBounds();
    deckViewerCloseText->setOrigin({ cb.size.x / 2.f, cb.size.y / 2.f });
    deckViewerCloseText->setPosition({ 400.f, 535.f });
}

// ── Экран костра (отдых перед боссом) ────────────────────────────────────────
void Game::initCampfire()
{
    campfireBg.setSize({ 800.f, 600.f });
    campfireBg.setFillColor(Color(25, 18, 15));

    // Вспомогательная лямбда для центрированного текста
    auto makeTitle = [&](Text*& ptr, Font& f, const wstring& str, unsigned sz,
        Color col, Vector2f pos)
        {
            ptr = new Text(f, str, sz);
            ptr->setFillColor(col);
            FloatRect b = ptr->getLocalBounds();
            ptr->setOrigin({ b.size.x / 2.f, b.size.y / 2.f });
            ptr->setPosition(pos);
        };

    makeTitle(campfireTitleText, mainfont, L"Костёр", 38, Color(255, 150, 50), { 400.f, 130.f });
    makeTitle(campfireHintText, font, L"Тёплый огонь даёт возможность отдохнуть и восстановить силы.", 18,
        Color(200, 170, 140), { 400.f, 210.f });

    // Кнопка «Отдохнуть»
    campfireRestButton.setSize({ 220.f, 56.f });
    campfireRestButton.setFillColor(Color(150, 70, 30));
    campfireRestButton.setOutlineThickness(2.f);
    campfireRestButton.setOutlineColor(Color(255, 150, 50));
    campfireRestButton.setOrigin({ 110.f, 28.f });
    campfireRestButton.setPosition({ 400.f, 320.f });
    makeTitle(campfireRestText, font, L"Отдохнуть (+30% HP)", 20, Color::White, { 400.f, 320.f });

    // Кнопка «Продолжить путь»
    campfireContinueButton.setSize({ 220.f, 50.f });
    campfireContinueButton.setFillColor(Color(60, 60, 60));
    campfireContinueButton.setOutlineThickness(2.f);
    campfireContinueButton.setOutlineColor(Color(150, 150, 150));
    campfireContinueButton.setOrigin({ 110.f, 25.f });
    campfireContinueButton.setPosition({ 400.f, 420.f });
    makeTitle(campfireContinueText, font, L"Продолжить путь", 18, Color(220, 220, 220), { 400.f, 420.f });
}

// ── Логика экрана костра ────────────────────────────────────────────────────
void Game::updateCampfire(Vector2f mousePos, bool clicked)
{
    // Кнопка «Отдохнуть» — только если ещё не использована
    if (!campfireRested && campfireRestButton.getGlobalBounds().contains(mousePos))
    {
        campfireRestButton.setFillColor(Color(190, 95, 40)); // подсветка при наведении
        if (clicked)
        {
            // Восстанавливаем 30% HP (не больше максимума)
            heroCurrentHP = min(heroCurrentHP + (int)(heroMaxHP * 0.30f), heroMaxHP);
            campfireRested = true;
            if (selectionAudio) { selectionAudio->setLooping(false); selectionAudio->play(); }
        }
    }
    else if (!campfireRested)
    {
        campfireRestButton.setFillColor(Color(150, 70, 30));
    }

    // Кнопка «Продолжить путь»
    if (campfireContinueButton.getGlobalBounds().contains(mousePos))
    {
        campfireContinueButton.setFillColor(Color(90, 90, 90));
        if (clicked) { gameState = 1; shopMouseWasDown = true; }
    }
    else
    {
        campfireContinueButton.setFillColor(Color(60, 60, 60));
    }
}

// ── Отрисовка экрана костра ──────────────────────────────────────────────────
void Game::renderCampfire()
{
    window->clear(Color(25, 18, 15));
    window->draw(campfireBg);
    if (campfireTitleText) window->draw(*campfireTitleText);
    if (campfireHintText)  window->draw(*campfireHintText);

    // Кнопка «Отдохнуть» — затемняем, если уже использована
    RectangleShape restBtn = campfireRestButton;
    Text restLabel = *campfireRestText;
    if (campfireRested)
    {
        restBtn.setFillColor(Color(70, 50, 40));
        restBtn.setOutlineColor(Color(110, 90, 70));
        restLabel.setFillColor(Color(140, 130, 120));
    }
    window->draw(restBtn);
    window->draw(restLabel);

    window->draw(campfireContinueButton);
    if (campfireContinueText) window->draw(*campfireContinueText);

    // Показываем HP и подсказку «уже отдохнули»
    Text hpText(font, L"HP: " + to_wstring(heroCurrentHP) + L" / " + to_wstring(heroMaxHP), 20);
    hpText.setFillColor(Color(220, 180, 140));
    FloatRect hpb = hpText.getLocalBounds();
    hpText.setOrigin({ hpb.size.x / 2.f, hpb.size.y / 2.f });
    hpText.setPosition({ 400.f, 480.f });
    window->draw(hpText);

    if (campfireRested)
    {
        Text restedHint(font, L"Вы уже отдохнули у этого костра", 16);
        restedHint.setFillColor(Color(150, 130, 110));
        FloatRect rhb = restedHint.getLocalBounds();
        restedHint.setOrigin({ rhb.size.x / 2.f, rhb.size.y / 2.f });
        restedHint.setPosition({ 400.f, 365.f });
        window->draw(restedHint);
    }
}

// ── Сброс прогресса и возврат в главное меню ─────────────────────────────────
void Game::returnToMainMenu()
{
    gameState = 0;
    heroCurrentHP = 80;
    moneyCount = 50; // стартовое золото нового забега
    playerDeck.clear();
    resetMapProgress();
    initMoneyBar();
}

// ── Общий UI экранов конца забега (создаётся один раз) ───────────────────────
void Game::initEndRunScreen()
{
    endRunMenuButton.setSize({ 240.f, 50.f });
    endRunMenuButton.setFillColor(Color(60, 60, 60));
    endRunMenuButton.setOutlineThickness(2.f);
    endRunMenuButton.setOutlineColor(Color(150, 150, 150));
    endRunMenuButton.setOrigin({ 120.f, 25.f });
    endRunMenuButton.setPosition({ 400.f, 450.f });

    endRunMenuButtonText = new Text(font, L"В главное меню", 20);
    endRunMenuButtonText->setFillColor(Color::White);
    FloatRect mb = endRunMenuButtonText->getLocalBounds();
    endRunMenuButtonText->setOrigin({ mb.size.x / 2.f, mb.size.y / 2.f });
    endRunMenuButtonText->setPosition({ 400.f, 450.f });

    // Тексты — строки заполняются при каждом показе экрана в renderEndRunScreen
    endRunTitleText = new Text(mainfont, L"", 56);
    endRunTitleText->setPosition({ 400.f, 200.f });

    endRunSubText = new Text(font, L"", 22);
    endRunSubText->setPosition({ 400.f, 300.f });

    endRunStatsText = new Text(font, L"", 18);
    endRunStatsText->setPosition({ 400.f, 360.f });
}

// ── Обработка ввода на экранах победы/поражения ───────────────────────────────
void Game::updateEndRunScreen(Vector2f mousePos, bool clicked)
{
    // Кнопка «В меню» активируется только после окончания звука
    if (!endRunButtonReady)
    {
        bool defeatPlaying = defeatSound && defeatSound->getStatus() == Sound::Status::Playing;
        bool victoryPlaying = victorySound && victorySound->getStatus() == Sound::Status::Playing;
        if (!defeatPlaying && !victoryPlaying)
            endRunButtonReady = true;
        else
            return;
    }

    bool over = endRunMenuButton.getGlobalBounds().contains(mousePos);
    endRunMenuButton.setFillColor(over ? Color(90, 90, 90) : Color(60, 60, 60));
    if (over && clicked) returnToMainMenu();
}

// ── Отрисовка экрана конца забега ────────────────────────────────────────────
// outcome: 0 = поражение в бою, 1 = победа над боссом, 2 = поражение от босса
void Game::renderEndRunScreen(int outcome)
{
    window->clear(Color(10, 5, 20));
    RectangleShape overlay({ 800.f, 600.f });
    overlay.setFillColor(Color(0, 0, 0, 180));
    window->draw(overlay);

    if (outcome == 1) // ПОБЕДА
    {
        endRunTitleText->setString(L"ПОБЕДА!");
        endRunTitleText->setFillColor(Color(255, 215, 50));
        endRunSubText->setString(L"Священный граль был возвращён. Отличная работа!");
        endRunSubText->setFillColor(Color(200, 180, 140));
        endRunStatsText->setString(L"HP оставшееся: " + to_wstring(heroCurrentHP) + L" / " + to_wstring(heroMaxHP));
        endRunStatsText->setFillColor(Color(160, 220, 160));
    }
    else // ПОРАЖЕНИЕ (бой или босс)
    {
        endRunTitleText->setString(L"ПОРАЖЕНИЕ");
        endRunTitleText->setFillColor(Color(200, 50, 50));
        endRunSubText->setString(outcome == 2
            ? L"Босс оказался сильнее. Граль остался не возвращён."
            : L"Путь оборвался здесь. Но впереди ещё много попыток.");
        endRunSubText->setFillColor(Color(180, 150, 150));
        endRunStatsText->setString(L"Собрано золота: " + to_wstring(moneyCount));
        endRunStatsText->setFillColor(Color(220, 180, 100));
    }

    // Центрируем тексты по горизонтали
    for (Text* t : { endRunTitleText, endRunSubText, endRunStatsText })
    {
        FloatRect b = t->getLocalBounds();
        t->setOrigin({ b.size.x / 2.f, b.size.y / 2.f });
        window->draw(*t);
    }

    // Кнопка «В меню» — затемнена, пока звук не закончился
    if (endRunButtonReady)
    {
        window->draw(endRunMenuButton);
        if (endRunMenuButtonText) window->draw(*endRunMenuButtonText);
    }
    else
    {
        RectangleShape hiddenButton = endRunMenuButton;
        hiddenButton.setFillColor(Color(40, 40, 40, 120));
        hiddenButton.setOutlineColor(Color(80, 80, 80, 120));
        window->draw(hiddenButton);
    }
}

// ── Магазин ──────────────────────────────────────────────────────────────────
void Game::initShop()
{
    shopKeeper.setSize({ 150.f, 350.f });
    shopKeeper.setOrigin({ 75.f, 175.f });
    shopKeeper.setPosition({ 720.f, 430.f });
    if (shopkeeperIMG.loadFromFile("image/ShopKPimg.png"))
        shopKeeper.setTexture(&shopkeeperIMG);

    shopBg.setSize({ 800.f, 600.f });
    shopBg.setFillColor(Color(20, 35, 40));

    shopPanel.setSize({ 760.f, 420.f });
    shopPanel.setFillColor(Color(28, 52, 56));
    shopPanel.setOutlineThickness(3.f);
    shopPanel.setOutlineColor(Color(70, 170, 150));
    shopPanel.setPosition({ 20.f, 80.f });

    shopTitleText = new Text(mainfont, L"Магазин", 38);
    shopTitleText->setFillColor(Color(120, 220, 200));
    FloatRect tb = shopTitleText->getLocalBounds();
    shopTitleText->setOrigin({ tb.size.x / 2.f, tb.size.y / 2.f });
    shopTitleText->setPosition({ 400.f, 40.f });

    shopExitButton.setSize({ 160.f, 40.f });
    shopExitButton.setFillColor(Color(40, 70, 110));
    shopExitButton.setOutlineThickness(2.f);
    shopExitButton.setOutlineColor(Color(80, 140, 220));
    shopExitButton.setPosition({ 320.f, 535.f });

    shopExitText = new Text(font, L"Покинуть магазин", 17);
    shopExitText->setFillColor(Color::White);
    FloatRect eb = shopExitText->getLocalBounds();
    shopExitText->setOrigin({ eb.size.x / 2.f, eb.size.y / 2.f });
    shopExitText->setPosition({ 400.f, 555.f });

    // Услуга: избавиться от карты
    sellCardButton.setSize({ 280.f, 60.f });
    sellCardButton.setFillColor(Color(35, 90, 60));
    sellCardButton.setOutlineThickness(2.f);
    sellCardButton.setOutlineColor(Color(90, 200, 130));
    sellCardButton.setOrigin({ 140.f, 30.f });
    sellCardButton.setPosition({ 165.f, 335.f });

    sellCardButtonText = new Text(font, L"Избавиться от карты (75 зол.)", 16);
    sellCardButtonText->setFillColor(Color::White);
    FloatRect scb = sellCardButtonText->getLocalBounds();
    sellCardButtonText->setOrigin({ scb.size.x / 2.f, scb.size.y / 2.f });
    sellCardButtonText->setPosition({ 165.f, 335.f });

    // Услуга: восстановить HP
    healButton.setSize({ 280.f, 60.f });
    healButton.setFillColor(Color(35, 70, 100));
    healButton.setOutlineThickness(2.f);
    healButton.setOutlineColor(Color(80, 160, 230));
    healButton.setOrigin({ 140.f, 30.f });
    healButton.setPosition({ 165.f, 410.f });

    healButtonText = new Text(font, L"Восстановить 20% HP (100 зол.)", 16);
    healButtonText->setFillColor(Color::White);
    FloatRect hlb = healButtonText->getLocalBounds();
    healButtonText->setOrigin({ hlb.size.x / 2.f, hlb.size.y / 2.f });
    healButtonText->setPosition({ 165.f, 410.f });

    // Модальный экран выбора карты для продажи
    sellScreenBg.setSize({ 800.f, 600.f });
    sellScreenBg.setFillColor(Color(0, 0, 0, 210));

    sellScreenPanel.setSize({ 720.f, 480.f });
    sellScreenPanel.setFillColor(Color(24, 48, 42));
    sellScreenPanel.setOutlineThickness(3.f);
    sellScreenPanel.setOutlineColor(Color(90, 200, 130));
    sellScreenPanel.setPosition({ 40.f, 60.f });

    sellScreenTitle = new Text(mainfont, L"Выберите карту, от которой избавиться", 24);
    sellScreenTitle->setFillColor(Color(140, 230, 180));
    FloatRect sst = sellScreenTitle->getLocalBounds();
    sellScreenTitle->setOrigin({ sst.size.x / 2.f, sst.size.y / 2.f });
    sellScreenTitle->setPosition({ 400.f, 85.f });

    sellScreenCloseBtn.setSize({ 120.f, 35.f });
    sellScreenCloseBtn.setFillColor(Color(110, 40, 40));
    sellScreenCloseBtn.setOutlineThickness(2.f);
    sellScreenCloseBtn.setOutlineColor(Color(200, 70, 70));
    sellScreenCloseBtn.setPosition({ 340.f, 518.f });

    sellScreenCloseText = new Text(font, L"Отмена", 18);
    sellScreenCloseText->setFillColor(Color::White);
    FloatRect ssc = sellScreenCloseText->getLocalBounds();
    sellScreenCloseText->setOrigin({ ssc.size.x / 2.f, ssc.size.y / 2.f });
    sellScreenCloseText->setPosition({ 400.f, 535.f });
}

// ── Индикатор золота (обновляется при изменении moneyCount) ───────────────────
void Game::initMoneyBar()
{
    delete moneyText;
    moneyText = new Text(font, to_wstring(moneyCount) + L" золото", 24);
    moneyText->setFillColor(Color::Yellow);
    moneyText->setPosition({ 620.f, 20.f });
}

// ── Карта мира (8 ярусов, снизу вверх: старт → костры → босс) ────────────────
void Game::initMap()
{
    mapText = new Text(font, L"Карта: Выберите свой путь", 24);
    mapText->setFillColor(Color(50, 50, 50));
    FloatRect b = mapText->getLocalBounds();
    mapText->setOrigin({ b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f });
    mapText->setPosition({ 400.f, 40.f });

    // Легенда
    legendBg.setSize({ 160.f, 170.f });
    legendBg.setFillColor(Color(245, 240, 230, 240));
    legendBg.setOutlineThickness(2.f);
    legendBg.setOutlineColor(Color(130, 120, 110));
    legendBg.setPosition({ 610.f, 100.f });

    legendTitle = new Text(font, L"Легенда", 18);
    legendTitle->setFillColor(Color(50, 50, 50));
    legendTitle->setPosition({ 625.f, 110.f });

    // Вспомогательная лямбда для пары «кружок + текст» в легенде
    auto addLegendItem = [&](CircleShape& circle, Color col, Vector2f cPos,
        Text*& txt, const wstring& label, Vector2f tPos)
        {
            circle.setRadius(8.f);
            circle.setFillColor(col);
            circle.setPosition(cPos);
            txt = new Text(font, label, 15);
            txt->setFillColor(Color(50, 50, 50));
            txt->setPosition(tPos);
        };

    addLegendItem(legendEnemyCircle, Color(220, 60, 60), { 625.f,145.f }, legendEnemyText, L"Враг", { 655.f,142.f });
    addLegendItem(legendShopCircle, Color(230, 180, 40), { 625.f,180.f }, legendShopText, L"Магазин", { 655.f,177.f });
    addLegendItem(legendCampfireCircle, Color(240, 130, 30), { 625.f,215.f }, legendCampfireText, L"Костёр", { 655.f,212.f });
    addLegendItem(legendBossCircle, Color(255, 37, 234), { 625.f,250.f }, legendBossText, L"Босс", { 655.f,248.f });

    // ── Узлы карты (тип: 0=враг, 1=магазин, 2=костёр, 3=босс) ───────────────
    struct NodeDef { float x, y; int type; };
    NodeDef nodeDefs[] = {
        // Ярус 0 (старт)
        { 280.f,565.f,0 }, { 480.f,565.f,0 },
        // Ярус 1
        { 230.f,495.f,0 }, { 380.f,495.f,1 }, { 530.f,495.f,0 },
        // Ярус 2
        { 280.f,425.f,0 }, { 480.f,425.f,0 },
        // Ярус 3
        { 230.f,355.f,1 }, { 380.f,355.f,0 }, { 530.f,355.f,1 },
        // Ярус 4
        { 280.f,285.f,0 }, { 480.f,285.f,0 },
        // Ярус 5
        { 230.f,215.f,0 }, { 380.f,215.f,1 }, { 530.f,215.f,0 },
        // Ярус 6 (два костра перед боссом)
        { 280.f,145.f,2 }, { 480.f,145.f,2 },
        // Ярус 7 (босс)
        { 380.f,75.f, 3 },
    };

    // Цвет узла по типу
    Color typeColors[] = { Color(220,60,60), Color(230,180,40), Color(240,130,30), Color(255,37,234) };

    for (auto& nd : nodeDefs)
    {
        Node node;
        node.circle.setRadius(10.f);
        node.circle.setOrigin({ 12.f, 12.f });
        node.circle.setPosition({ nd.x, nd.y });
        node.circle.setFillColor(typeColors[nd.type]);
        node.type = nd.type;
        node.visited = node.available = false;
        nodes.push_back(node);
    }

    // ── Связи между узлами ────────────────────────────────────────────────────
    Link linkDefs[] = {
        {0,2},{0,3},{1,3},{1,4},   // ярус 0→1
        {2,5},{3,5},{3,6},{4,6},   // ярус 1→2
        {5,7},{5,8},{6,8},{6,9},   // ярус 2→3
        {7,10},{8,10},{8,11},{9,11},// ярус 3→4
        {10,12},{10,13},{11,13},{11,14},// ярус 4→5
        {12,15},{13,15},{13,16},{14,16},// ярус 5→6
        {15,17},{16,17},            // ярус 6→7 (босс)
    };
    for (auto& l : linkDefs) links.push_back(l);

    currentNodeIndex = -1;
    updateAvailableNodes();
}

// ── Пересчёт доступных узлов (логика Slay the Spire) ────────────────────────
// Доступны только узлы, куда ведёт прямая связь от текущего.
void Game::updateAvailableNodes()
{
    for (auto& n : nodes) n.available = false;

    if (currentNodeIndex == -1)
    {
        // Старт: доступны узлы без входящих связей
        vector<bool> hasIncoming(nodes.size(), false);
        for (auto& l : links) hasIncoming[l.to] = true;
        for (int i = 0; i < (int)nodes.size(); i++)
            if (!hasIncoming[i] && !nodes[i].visited) nodes[i].available = true;
    }
    else
    {
        // Доступны соседи текущего узла (только вперёд)
        for (auto& l : links)
            if (l.from == currentNodeIndex && !nodes[l.to].visited)
                nodes[l.to].available = true;
    }
}

// ── Сброс прогресса карты (при рестарте) ─────────────────────────────────────
void Game::resetMapProgress()
{
    currentNodeIndex = -1;
    for (auto& n : nodes) n.visited = false;
    updateAvailableNodes();
}

// ── Генерация 3 случайных карт-наград после боя ───────────────────────────────
void Game::generateRewardCards()
{
    // Пул всех возможных карт-наград
    struct Entry { wstring name, desc; CardType type; int cost, val, effect; };
    Entry pool[] = {
        { L"Двойной удар",   L"Наносит 8 урона дважды.",              CardType::Attack,  2, 16, 0 },
        { L"Рывок",          L"Наносит 12 урона.",                    CardType::Attack,  1, 12, 0 },
        { L"Шквал",          L"Наносит 5 урона трижды.",              CardType::Attack,  2, 15, 0 },
        { L"Резня",          L"Наносит 20 урона.",                    CardType::Attack,  3, 20, 0 },
        { L"Укол",           L"Наносит 4 урона.",                     CardType::Attack,  0,  4, 0 },
        { L"Щит",            L"Даёт 10 блока.",                       CardType::Defend,  1, 10, 0 },
        { L"Бастион",        L"Даёт 16 блока.",                       CardType::Defend,  2, 16, 0 },
        { L"Крепость",       L"Даёт 6 блока дважды.",                 CardType::Defend,  2, 12, 0 },
        { L"Оплот",          L"Даёт 25 блока.",                       CardType::Defend,  3, 25, 0 },
        { L"Стойкость",      L"Даёт 4 блока.",                        CardType::Defend,  0,  4, 0 },
        { L"Боевой клич",    L"Следующая атака +5 урона.",            CardType::Buff,    1,  5, 5 },
        { L"Ярость",         L"Следующая атака +10 урона.",           CardType::Buff,    2, 10, 5 },
        { L"Ослабление",     L"Враг получает -3 атаки.",              CardType::Debuff,  1,  3, 0 },
        { L"Ционид",          L"Накладывает 8 яда на врага.",         CardType::Debuff,  1,  8,  4 },
        { L"Яд",             L"Враг теряет 6 HP в свой ход.",         CardType::Debuff,  1,  6, 4 },
        { L"Броня",          L"Даёт 8 блока постоянно.",              CardType::Defend,  2,  8, 0 },
        { L"Предсказуемость",L"Меняет намерение врага на этот ход.",  CardType::Debuff,  3,  0, 7 },
        { L"Бессилие",       L"Ты и враг получаете -12 атаки.",      CardType::Debuff,  1, 12, 1 },
    };

    const int poolSize = 18;
    bool used[poolSize] = {};

    for (int i = 0; i < 3; i++)
    {
        int r;
        do { r = rand() % poolSize; } while (used[r]);
        used[r] = true;
        rewardCardChoices[i] = { pool[r].name, pool[r].desc, pool[r].type,
                                  pool[r].cost, pool[r].val, pool[r].effect };
    }
}

// ── Освобождение текстовых объектов карт магазина 
void Game::clearShopCards()
{
    for (auto& sc : shopCards)
    {
        delete sc.nameText; delete sc.descText;
        delete sc.costText; delete sc.priceText;
    }
    shopCards.clear();
}

// ── Генерация 5 случайных карт для магазина 
void Game::generateShopCards()
{
    clearShopCards();

    struct ShopEntry { wstring name, desc; CardType type; int cost, val, price, effect; };
    ShopEntry pool[] = {
        { L"Двойной удар",   L"Наносит 8 урона дважды.",             CardType::Attack,  2, 16,  75, 0 },
        { L"Рывок",          L"Наносит 12 урона.",                   CardType::Attack,  1, 12,  60, 0 },
        { L"Шквал",          L"Наносит 5 урона трижды.",             CardType::Attack,  2, 15,  90, 0 },
        { L"Резня",          L"Наносит 20 урона.",                   CardType::Attack,  3, 20, 120, 0 },
        { L"Укол",           L"Наносит 4 урона.",                    CardType::Attack,  0,  4,  35, 0 },
        { L"Щит",            L"Даёт 10 блока.",                      CardType::Defend,  1, 10,  60, 0 },
        { L"Бастион",        L"Даёт 16 блока.",                      CardType::Defend,  2, 16,  90, 0 },
        { L"Крепость",       L"Даёт 6 блока дважды.",                CardType::Defend,  2, 12,  80, 0 },
        { L"Оплот",          L"Даёт 25 блока.",                      CardType::Defend,  3, 25, 130, 0 },
        { L"Стойкость",      L"Даёт 4 блока.",                       CardType::Defend,  0,  4,  30, 0 },
        { L"Боевой клич",    L"Следующая атака +5 урона.",           CardType::Buff,    1,  5,  70, 5 },
        { L"Ярость",         L"Следующая атака +10 урона.",          CardType::Buff,    2, 10, 100, 5 },
        { L"Ослабление",     L"Враг получает -3 атаки.",             CardType::Debuff,  1,  3, 120, 0 },
        { L"Яд",             L"Накладывает 4 яда на врага",          CardType::Debuff,  1,  4,  65, 4 },
        { L"Опиум",          L"Накладывает 6 яда на врага.",         CardType::Debuff,  1,  6,  95, 4 },
        { L"Католизатор",    L"Удваивает эффект яда.",               CardType::Debuff,  2,  2, 100, 5 },
        { L"Броня",          L"Даёт 8 блока постоянно.",             CardType::Defend,  2,  8, 110, 0 },
        { L"Предсказуемость",L"Меняет намерение врага.",             CardType::Debuff,  3,  0, 105, 7 },
    };

    const int poolSize = 18;
    bool used[poolSize] = {};
    int count = 0;

    while (count < 5)
    {
        int r = rand() % poolSize;
        if (used[r]) continue;
        used[r] = true;

        ShopEntry& e = pool[r];
        ShopCard sc;
        sc.data = { e.name, e.desc, e.type, e.cost, e.val, e.effect };
        sc.price = e.price;
        sc.sold = false;

        const float cardW = 130.f, cardH = 180.f;
        const float startX = 35.f, gap = 148.f;
        float cardX = startX + count * gap;

        sc.body.setSize({ cardW, cardH });
        sc.body.setPosition({ cardX, 110.f });
        sc.body.setOutlineThickness(2.f);

        // Цвет карты по типу
        switch (e.type)
        {
        case CardType::Attack: sc.body.setFillColor(Color(110, 35, 35));  sc.body.setOutlineColor(Color(220, 80, 80));  break;
        case CardType::Defend: sc.body.setFillColor(Color(30, 60, 110));  sc.body.setOutlineColor(Color(80, 140, 220)); break;
        case CardType::Buff:   sc.body.setFillColor(Color(60, 100, 35));  sc.body.setOutlineColor(Color(120, 210, 80)); break;
        default:               sc.body.setFillColor(Color(90, 30, 90));   sc.body.setOutlineColor(Color(200, 80, 200)); break;
        }

        wstring typeLabel;
        switch (e.type)
        {
        case CardType::Attack: typeLabel = L"Атака";  break;
        case CardType::Defend: typeLabel = L"Защита"; break;
        case CardType::Buff:   typeLabel = L"Баф";    break;
        default:               typeLabel = L"Дебаф";  break;
        }

        sc.nameText = new Text(font, e.name, 14);
        sc.nameText->setFillColor(Color::White);
        sc.nameText->setPosition({ cardX + 6.f, 116.f });

        sc.descText = new Text(font, e.desc + L"\n[" + typeLabel + L"]", 11);
        sc.descText->setFillColor(Color(220, 220, 220));
        sc.descText->setPosition({ cardX + 6.f, 190.f });

        sc.costText = new Text(font, to_wstring(e.cost), 20);
        sc.costText->setFillColor(Color(100, 210, 255));
        sc.costText->setPosition({ cardX + 108.f, 116.f });

        sc.priceBg.setSize({ cardW, 28.f });
        sc.priceBg.setFillColor(Color(20, 45, 42));
        sc.priceBg.setOutlineThickness(1.f);
        sc.priceBg.setOutlineColor(Color(80, 170, 140));
        sc.priceBg.setPosition({ cardX, 110.f + cardH - 28.f });

        sc.priceText = new Text(font, to_wstring(e.price) + L" зол.", 14);
        sc.priceText->setFillColor(Color(140, 230, 180));
        FloatRect pb = sc.priceText->getLocalBounds();
        sc.priceText->setOrigin({ pb.size.x / 2.f, pb.size.y / 2.f });
        sc.priceText->setPosition({ cardX + cardW / 2.f, 110.f + cardH - 14.f });

        shopCards.push_back(sc);
        count++;
    }
}

// ── Логика магазина ───────────────────────────────────────────────────────────
void Game::updateShop(Vector2f mousePos, bool clicked)
{
    // Если открыт экран продажи карты — обрабатываем только его
    if (showSellCardScreen) { updateSellCardScreen(mousePos, clicked); return; }

    // Кнопка выхода из магазина
    bool overExit = shopExitButton.getGlobalBounds().contains(mousePos);
    shopExitButton.setFillColor(overExit ? Color(60, 100, 150) : Color(40, 70, 110));
    if (overExit && clicked) { gameState = 1; shopMouseWasDown = true; return; }

    // Услуга: избавиться от карты (75 зол., только раз за посещение)
    bool canSell = !cardSoldThisVisit && moneyCount >= sellCardPrice;
    bool overSell = sellCardButton.getGlobalBounds().contains(mousePos);

    if (canSell && overSell)
    {
        sellCardButton.setFillColor(Color(50, 120, 80));
        if (clicked) { showSellCardScreen = true; sellScreenScroll = 0; return; }
    }
    else
    {
        sellCardButton.setFillColor(cardSoldThisVisit ? Color(40, 50, 45) :
            moneyCount < sellCardPrice ? Color(45, 55, 50) : Color(35, 90, 60));
    }

    // Услуга: восстановить 20% HP (100 зол., только раз за посещение)
    bool canHeal = !healedThisVisit && moneyCount >= healPrice;
    bool overHeal = healButton.getGlobalBounds().contains(mousePos);

    if (canHeal && overHeal)
    {
        healButton.setFillColor(Color(50, 100, 140));
        if (clicked)
        {
            moneyCount -= healPrice;
            initMoneyBar();
            heroCurrentHP = min(heroCurrentHP + (int)(heroMaxHP * 0.20f), heroMaxHP);
            healedThisVisit = true;
            if (selectionAudio) { selectionAudio->setLooping(false); selectionAudio->play(); }
        }
    }
    else
    {
        healButton.setFillColor(healedThisVisit ? Color(40, 50, 45) : Color(35, 70, 100));
    }

    // Карточки магазина
    for (auto& sc : shopCards)
    {
        if (sc.sold) continue;

        bool overCard = sc.body.getGlobalBounds().contains(mousePos);
        bool affordable = moneyCount >= sc.price;

        if (overCard)
        {
            sc.body.setOutlineColor(affordable ? Color(140, 230, 180) : Color(180, 50, 50));
            sc.body.setOutlineThickness(3.f);

            if (clicked && affordable)
            {
                moneyCount -= sc.price;
                initMoneyBar();
                playerDeck.push_back(sc.data);
                sc.sold = true;
                if (cardAudio) { cardAudio->setLooping(false); cardAudio->play(); }
            }
        }
        else
        {
            sc.body.setOutlineThickness(2.f);
            switch (sc.data.type)
            {
            case CardType::Attack: sc.body.setOutlineColor(Color(220, 80, 80));  break;
            case CardType::Defend: sc.body.setOutlineColor(Color(80, 140, 220)); break;
            case CardType::Buff:   sc.body.setOutlineColor(Color(120, 210, 80)); break;
            default:               sc.body.setOutlineColor(Color(200, 80, 200)); break;
            }
        }
    }
}

// ── Отрисовка магазина ────────────────────────────────────────────────────────
void Game::renderShop()
{
    window->draw(shopBg);
    window->draw(shopPanel);
    if (shopTitleText) window->draw(*shopTitleText);
    if (moneyText)     window->draw(*moneyText);

    // Карточки магазина
    for (auto& sc : shopCards)
    {
        if (sc.sold)
        {
            // Проданная карта — затемнённый прямоугольник с надписью «Куплено»
            RectangleShape sold = sc.body;
            sold.setFillColor(Color(20, 35, 32, 200));
            sold.setOutlineColor(Color(70, 110, 95));
            window->draw(sold);

            Text soldText(font, L"Куплено", 16);
            soldText.setFillColor(Color(150, 180, 165));
            FloatRect sb = soldText.getLocalBounds();
            soldText.setOrigin({ sb.size.x / 2.f, sb.size.y / 2.f });
            Vector2f pos = sc.body.getPosition(), sz = sc.body.getSize();
            soldText.setPosition({ pos.x + sz.x / 2.f, pos.y + sz.y / 2.f });
            window->draw(soldText);
            continue;
        }

        // Недоступные (нет золота) карты затемняем
        if (moneyCount < sc.price)
        {
            RectangleShape dim = sc.body;
            dim.setFillColor(Color(35, 40, 38));
            window->draw(dim);
        }
        else window->draw(sc.body);

        window->draw(sc.priceBg);
        if (sc.nameText)  window->draw(*sc.nameText);
        if (sc.descText)  window->draw(*sc.descText);
        if (sc.costText)  window->draw(*sc.costText);
        if (sc.priceText) window->draw(*sc.priceText);
    }

    window->draw(shopKeeper);

    // Кнопка «Избавиться от карты»
    RectangleShape sellBtn = sellCardButton;
    Text sellLabel = sellCardButtonText ? *sellCardButtonText : Text(font);
    if (cardSoldThisVisit)
    {
        sellBtn.setFillColor(Color(40, 50, 45)); sellBtn.setOutlineColor(Color(70, 90, 80));
        sellLabel.setString(L"Услуга уже использована");
        sellLabel.setFillColor(Color(130, 150, 140));
        FloatRect lb = sellLabel.getLocalBounds();
        sellLabel.setOrigin({ lb.size.x / 2.f, lb.size.y / 2.f });
        sellLabel.setPosition(sellCardButton.getPosition());
    }
    window->draw(sellBtn);
    window->draw(sellLabel);

    // Кнопка «Восстановить HP»
    RectangleShape healBtn = healButton;
    Text healLabel = healButtonText ? *healButtonText : Text(font);
    if (healedThisVisit)
    {
        healBtn.setFillColor(Color(40, 50, 45)); healBtn.setOutlineColor(Color(70, 90, 80));
        healLabel.setString(L"Вы уже отдохнули здесь");
        healLabel.setFillColor(Color(130, 150, 140));
        FloatRect lb = healLabel.getLocalBounds();
        healLabel.setOrigin({ lb.size.x / 2.f, lb.size.y / 2.f });
        healLabel.setPosition(healButton.getPosition());
    }
    else if (moneyCount < healPrice)
    {
        healBtn.setFillColor(Color(30, 45, 55)); healBtn.setOutlineColor(Color(50, 90, 120));
    }
    window->draw(healBtn);
    window->draw(healLabel);

    Text hint(font, L"Нажмите на карту чтобы купить", 15);
    hint.setFillColor(Color(150, 200, 185));
    FloatRect hb = hint.getLocalBounds();
    hint.setOrigin({ hb.size.x / 2.f, 0.f });
    hint.setPosition({ 400.f, 465.f });
    window->draw(hint);

    window->draw(shopExitButton);
    if (shopExitText) window->draw(*shopExitText);

    if (showSellCardScreen) renderSellCardScreen();
}

// ── Вспомогательная лямбда для цвета карты (используется в нескольких местах) ─
// Вынести в отдельную функцию нельзя без изменения архитектуры,
// поэтому дублируется как inline-код в нужных местах.

// ── Отрисовка просмотрщика колоды ─────────────────────────────────────────────
void Game::renderDeckViewer()
{
    window->draw(deckViewerBg);
    window->draw(deckViewerPanel);
    if (deckViewerTitle) window->draw(*deckViewerTitle);

    // Всего карт = 17 базовых + купленные
    const int baseDeckSize = 17;
    int totalCards = baseDeckSize + (int)playerDeck.size();

    Text countText(font, L"Всего карт: " + to_wstring(totalCards), 16);
    countText.setFillColor(Color(200, 200, 160));
    countText.setPosition({ 60.f, 110.f });
    window->draw(countText);

    // Собираем полный список карт для отображения
    struct ViewCard { wstring name, typeStr; int cost; CardType type; };
    vector<ViewCard> allCards;

    // 8 пар «Удар + Блок» + 1 «Выпад»
    for (int i = 0; i < 8; i++)
    {
        allCards.push_back({ L"Удар", L"Атака",  1, CardType::Attack });
        allCards.push_back({ L"Блок", L"Защита", 1, CardType::Defend });
    }
    allCards.push_back({ L"Выпад", L"Атака", 2, CardType::Attack });

    // Купленные/полученные карты
    for (auto& cd : playerDeck)
    {
        wstring tStr;
        switch (cd.type)
        {
        case CardType::Attack: tStr = L"Атака";  break;
        case CardType::Defend: tStr = L"Защита"; break;
        case CardType::Buff:   tStr = L"Баф";    break;
        default:               tStr = L"Дебаф";  break;
        }
        allCards.push_back({ cd.name, tStr, cd.energyCost, cd.type });
    }

    // Отображаем в сетке 5×3
    const int cols = 5, visRows = 3;
    const float cardW = 120.f, cardH = 90.f;
    const float startX = 55.f, startY = 140.f, gapX = 136.f, gapY = 108.f;
    int startIndex = deckViewerScroll * cols;

    for (int i = 0; i < cols * visRows; i++)
    {
        int idx = startIndex + i;
        if (idx >= (int)allCards.size()) break;

        ViewCard& vc = allCards[idx];
        float cx = startX + (i % cols) * gapX;
        float cy = startY + (i / cols) * gapY;

        RectangleShape cardBg({ cardW, cardH });
        cardBg.setPosition({ cx, cy });
        cardBg.setOutlineThickness(2.f);
        switch (vc.type)
        {
        case CardType::Attack: cardBg.setFillColor(Color(100, 30, 30));  cardBg.setOutlineColor(Color(200, 70, 70));  break;
        case CardType::Defend: cardBg.setFillColor(Color(25, 50, 100));  cardBg.setOutlineColor(Color(70, 130, 210)); break;
        case CardType::Buff:   cardBg.setFillColor(Color(45, 85, 30));   cardBg.setOutlineColor(Color(100, 190, 60)); break;
        default:               cardBg.setFillColor(Color(80, 25, 80));   cardBg.setOutlineColor(Color(180, 60, 180)); break;
        }
        window->draw(cardBg);

        Text nameT(font, vc.name, 13);  nameT.setFillColor(Color::White);        nameT.setPosition({ cx + 5.f, cy + 5.f });  window->draw(nameT);
        Text typeT(font, vc.typeStr, 11); typeT.setFillColor(Color(200, 200, 160)); typeT.setPosition({ cx + 5.f, cy + 26.f }); window->draw(typeT);
        Text costT(font, to_wstring(vc.cost), 20); costT.setFillColor(Color(100, 210, 255)); costT.setPosition({ cx + 98.f, cy + 4.f }); window->draw(costT);
    }

    // Кнопки прокрутки
    int totalRows = ((int)allCards.size() + cols - 1) / cols;
    int maxScroll = max(0, totalRows - visRows);

    if (deckViewerScroll > 0)
    {
        RectangleShape upBtn({ 80.f, 28.f });
        upBtn.setFillColor(Color(70, 70, 100)); upBtn.setOutlineThickness(1.f); upBtn.setOutlineColor(Color(140, 140, 200)); upBtn.setPosition({ 60.f,484.f });
        window->draw(upBtn);
        Text upT(font, L"< Вверх", 14); upT.setFillColor(Color::White); upT.setPosition({ 66.f,490.f }); window->draw(upT);
    }
    if (deckViewerScroll < maxScroll)
    {
        RectangleShape downBtn({ 80.f, 28.f });
        downBtn.setFillColor(Color(70, 70, 100)); downBtn.setOutlineThickness(1.f); downBtn.setOutlineColor(Color(140, 140, 200)); downBtn.setPosition({ 620.f,484.f });
        window->draw(downBtn);
        Text downT(font, L"Вниз >", 14); downT.setFillColor(Color::White); downT.setPosition({ 628.f,490.f }); window->draw(downT);
    }

    window->draw(deckViewerCloseBtn);
    if (deckViewerCloseText) window->draw(*deckViewerCloseText);
}

// ── Экран выбора карты для продажи ───────────────────────────────────────────
void Game::renderSellCardScreen()
{
    window->draw(sellScreenBg);
    window->draw(sellScreenPanel);
    if (sellScreenTitle) window->draw(*sellScreenTitle);

    if (playerDeck.empty())
    {
        Text empty(font, L"В колоде нет купленных карт, от которых можно избавиться.", 18);
        empty.setFillColor(Color(180, 200, 190));
        FloatRect eb = empty.getLocalBounds();
        empty.setOrigin({ eb.size.x / 2.f, eb.size.y / 2.f });
        empty.setPosition({ 400.f, 280.f });
        window->draw(empty);
    }
    else
    {
        Text priceHint(font, L"Стоимость услуги: " + to_wstring(sellCardPrice) + L" зол.", 16);
        priceHint.setFillColor(moneyCount >= sellCardPrice ? Color(140, 230, 180) : Color(220, 120, 110));
        FloatRect ph = priceHint.getLocalBounds();
        priceHint.setOrigin({ ph.size.x / 2.f, ph.size.y / 2.f });
        priceHint.setPosition({ 400.f, 115.f });
        window->draw(priceHint);
    }

    const int cols = 5, visRows = 3;
    const float cardW = 120.f, cardH = 90.f;
    const float startX = 55.f, startY = 140.f, gapX = 136.f, gapY = 108.f;
    int startIndex = sellScreenScroll * cols;
    Vector2f mousePos = window->mapPixelToCoords(Mouse::getPosition(*window));

    for (int i = 0; i < cols * visRows; i++)
    {
        int idx = startIndex + i;
        if (idx >= (int)playerDeck.size()) break;

        CardData& cd = playerDeck[idx];
        float cx = startX + (i % cols) * gapX;
        float cy = startY + (i / cols) * gapY;

        RectangleShape cardBg({ cardW, cardH });
        cardBg.setPosition({ cx, cy });
        cardBg.setOutlineThickness(2.f);
        switch (cd.type)
        {
        case CardType::Attack: cardBg.setFillColor(Color(100, 30, 30));  cardBg.setOutlineColor(Color(200, 70, 70));  break;
        case CardType::Defend: cardBg.setFillColor(Color(25, 50, 100));  cardBg.setOutlineColor(Color(70, 130, 210)); break;
        case CardType::Buff:   cardBg.setFillColor(Color(45, 85, 30));   cardBg.setOutlineColor(Color(100, 190, 60)); break;
        default:               cardBg.setFillColor(Color(80, 25, 80));   cardBg.setOutlineColor(Color(180, 60, 180)); break;
        }
        if (cardBg.getGlobalBounds().contains(mousePos))
        {
            cardBg.setOutlineThickness(3.f); cardBg.setOutlineColor(Color(140, 230, 180));
        }
        window->draw(cardBg);

        wstring tStr;
        switch (cd.type)
        {
        case CardType::Attack: tStr = L"Атака";  break;
        case CardType::Defend: tStr = L"Защита"; break;
        case CardType::Buff:   tStr = L"Баф";    break;
        default:               tStr = L"Дебаф";  break;
        }
        Text nameT(font, cd.name, 13); nameT.setFillColor(Color::White); nameT.setPosition({ cx + 5.f,cy + 5.f }); window->draw(nameT);
        Text typeT(font, tStr, 11); typeT.setFillColor(Color(200, 220, 210)); typeT.setPosition({ cx + 5.f,cy + 26.f }); window->draw(typeT);
        Text costT(font, to_wstring(cd.energyCost), 20); costT.setFillColor(Color(100, 210, 255)); costT.setPosition({ cx + 98.f,cy + 4.f }); window->draw(costT);
    }

    // Кнопки прокрутки
    int totalRows = ((int)playerDeck.size() + cols - 1) / cols;
    int maxScroll = max(0, totalRows - visRows);
    if (sellScreenScroll > 0)
    {
        RectangleShape upBtn({ 80.f,28.f }); upBtn.setFillColor(Color(35, 70, 65)); upBtn.setOutlineThickness(1.f); upBtn.setOutlineColor(Color(90, 200, 130)); upBtn.setPosition({ 60.f,484.f }); window->draw(upBtn);
        Text upT(font, L"< Вверх", 14); upT.setFillColor(Color::White); upT.setPosition({ 66.f,490.f }); window->draw(upT);
    }
    if (sellScreenScroll < maxScroll)
    {
        RectangleShape downBtn({ 80.f,28.f }); downBtn.setFillColor(Color(35, 70, 65)); downBtn.setOutlineThickness(1.f); downBtn.setOutlineColor(Color(90, 200, 130)); downBtn.setPosition({ 620.f,484.f }); window->draw(downBtn);
        Text downT(font, L"Вниз >", 14); downT.setFillColor(Color::White); downT.setPosition({ 628.f,490.f }); window->draw(downT);
    }
    window->draw(sellScreenCloseBtn);
    if (sellScreenCloseText) window->draw(*sellScreenCloseText);
}

// ── Обработка ввода на экране продажи карты ───────────────────────────────────
void Game::updateSellCardScreen(Vector2f mousePos, bool clicked)
{
    // Кнопка «Отмена»
    bool overClose = sellScreenCloseBtn.getGlobalBounds().contains(mousePos);
    sellScreenCloseBtn.setFillColor(overClose ? Color(160, 55, 55) : Color(110, 40, 40));
    if (overClose && clicked) { showSellCardScreen = false; return; }

    const int cols = 5, visRows = 3;
    const float cardW = 120.f, cardH = 90.f;
    const float startX = 55.f, startY = 140.f, gapX = 136.f, gapY = 108.f;

    // Прокрутка вверх/вниз
    RectangleShape upBtn({ 80.f,28.f });   upBtn.setPosition({ 60.f,484.f });
    RectangleShape downBtn({ 80.f,28.f }); downBtn.setPosition({ 620.f,484.f });

    if (upBtn.getGlobalBounds().contains(mousePos) && clicked)
    {
        if (sellScreenScroll > 0) sellScreenScroll--; return;
    }

    int totalRows = ((int)playerDeck.size() + cols - 1) / cols;
    if (downBtn.getGlobalBounds().contains(mousePos) && clicked)
    {
        if (sellScreenScroll < totalRows - visRows) sellScreenScroll++; return;
    }

    if (!clicked || cardSoldThisVisit || moneyCount < sellCardPrice) return;

    int startIndex = sellScreenScroll * cols;
    for (int i = 0; i < cols * visRows; i++)
    {
        int idx = startIndex + i;
        if (idx >= (int)playerDeck.size()) break;

        float cx = startX + (i % cols) * gapX;
        float cy = startY + (i / cols) * gapY;

        RectangleShape cardBg({ cardW, cardH });
        cardBg.setPosition({ cx, cy });
        if (cardBg.getGlobalBounds().contains(mousePos))
        {
            playerDeck.erase(playerDeck.begin() + idx);
            moneyCount -= sellCardPrice;
            initMoneyBar();
            cardSoldThisVisit = true;
            showSellCardScreen = false;
            if (cardAudio) { cardAudio->setLooping(false); cardAudio->play(); }
            return;
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// КОНСТРУКТОР И ДЕСТРУКТОР
// ═════════════════════════════════════════════════════════════════════════════

Game::Game()
{
    srand(static_cast<unsigned int>(time(nullptr))); // инициализируем генератор случайных чисел
    initVariables();
    initWindow();
    initSound();
    initFont();
    initMainMenu();
    initHero();
    initEnemy();
    initBattleReward();
    initShape();
    initShop();
    initMoneyBar();
    initMap();
    initDeckViewer();
    initCampfire();
    initEndRunScreen();
}

Game::~Game()
{
    delete window;
    delete sound; delete selectionAudio; delete cardAudio;
    delete defeatSound; delete victorySound; delete shopMusic;
    delete battleMusic; delete bossMusic;

    delete menuText; delete mapText; delete legendTitle;
    delete legendEnemyText; delete legendShopText;
    delete legendCampfireText; delete legendBossText;
    delete mainMenu; delete startButtonText; delete exsitButtonText; delete helpButtonText;
    delete rewardText; delete rewardCardsText; delete continueButtonText; delete rewardGoldText;
    delete moneyText;
    delete shopTitleText; delete shopExitText; delete sellCardButtonText; delete healButtonText;
    delete sellScreenTitle; delete sellScreenCloseText;
    delete deckViewerTitle; delete deckViewerCloseText;
    delete campfireTitleText; delete campfireHintText;
    delete campfireRestText; delete campfireContinueText;
    delete endRunMenuButtonText; delete endRunTitleText;
    delete endRunSubText; delete endRunStatsText;

    clearShopCards();
}

const bool Game::getWindowisOpen() const { return window->isOpen(); }

// ═════════════════════════════════════════════════════════════════════════════
// UPDATE — обработка событий и логика каждого кадра
// ═════════════════════════════════════════════════════════════════════════════
void Game::update()
{
    // Флаги для определения момента наведения (edge-detect звука кнопок)
    static bool wasOverStart = false, wasOverExit = false, wasOverHelp = false;
    static bool ButtonIsOver = false, ButtonIsOver2 = false, ButtonIsOver3 = false;
    static bool wasOverCard1 = false, wasOverCard2 = false, wasOverCard3 = false;

    Vector2f mouse_position = Vector2f(Mouse::getPosition(*window));

    // ── Обработка системных событий ──────────────────────────────────────────
    while (optional<Event> event = window->pollEvent())
    {
        if (event->is<Event::Closed>()) { window->close(); return; }

        if (auto* kp = event->getIf<Event::KeyPressed>())
        {
            switch (kp->code)
            {
            case Keyboard::Key::Escape:
                if (showDeckViewer) showDeckViewer = false;
                else if (gameState == 0) window->close();
                else                     gameState = 0;
                break;
            case Keyboard::Key::A: if (sound) sound->play();  break;
            case Keyboard::Key::B: if (sound) sound->stop();  break;
            case Keyboard::Key::M: gameState = 1;             break;
            default: break;
            }
        }
    }

    // ── Управление музыкой при смене состояния ────────────────────────────────
    // Магазин
    if (previousGameState == 3 && gameState != 3 && shopMusic)  shopMusic->stop();
    // Обычный бой
    if (previousGameState != 2 && gameState == 2 && battleMusic) battleMusic->play();
    if (previousGameState == 2 && gameState != 2 && battleMusic) battleMusic->stop();
    // Бой с боссом
    if (previousGameState != 6 && gameState == 6 && bossMusic)  bossMusic->play();
    if (previousGameState == 6 && gameState != 6 && bossMusic)  bossMusic->stop();

    previousGameState = gameState;

    bool mouseDown = Mouse::isButtonPressed(Mouse::Button::Left);

    // ── Просмотрщик колоды (работает поверх любого экрана) ────────────────────
    if (showDeckViewer)
    {
        static bool dvMouseWasDown = false;
        bool dvClicked = mouseDown && !dvMouseWasDown;

        bool overClose = deckViewerCloseBtn.getGlobalBounds().contains(mouse_position);
        deckViewerCloseBtn.setFillColor(overClose ? Color(160, 55, 55) : Color(110, 40, 40));
        if (overClose && dvClicked) showDeckViewer = false;

        RectangleShape upBtn({ 80.f,28.f });   upBtn.setPosition({ 60.f,484.f });
        RectangleShape downBtn({ 80.f,28.f }); downBtn.setPosition({ 620.f,484.f });

        if (upBtn.getGlobalBounds().contains(mouse_position) && dvClicked)
            if (deckViewerScroll > 0) deckViewerScroll--;

        int total = 17 + (int)playerDeck.size();
        int totalRows = (total + 4) / 5;
        if (downBtn.getGlobalBounds().contains(mouse_position) && dvClicked)
            if (deckViewerScroll < totalRows - 3) deckViewerScroll++;

        dvMouseWasDown = mouseDown;
        return;
    }

    // ── Главное меню ─────────────────────────────────────────────────────────
    if (gameState == 0)
    {
        // Вспомогательная лямбда для кнопок с звуком наведения
        auto handleMenuBtn = [&](RectangleShape& btn, bool& wasOver,
            Vector2f posHover, Vector2f posNorm,
            Color colHover, Color colNorm) -> bool
            {
                bool over = btn.getGlobalBounds().contains(mouse_position);
                if (over)
                {
                    if (!wasOver && selectionAudio) { selectionAudio->setLooping(false); selectionAudio->play(); }
                    btn.setPosition(posHover); btn.setFillColor(colHover);
                }
                else
                {
                    btn.setPosition(posNorm); btn.setFillColor(colNorm);
                }
                wasOver = over;
                return over && mouseDown;
            };

        if (handleMenuBtn(startButton, wasOverStart, { 185.f,370.f }, { 160.f,370.f }, { 239,253,95 }, { 212,175,55 }))
            gameState = 1;

        if (handleMenuBtn(exsitButton, wasOverExit, { 185.f,450.f }, { 160.f,450.f }, { 239,253,95 }, { 212,175,55 }))
            window->close();

        if (handleMenuBtn(helpButton, wasOverHelp, { 185.f,525.f }, { 160.f,525.f }, { 239,253,95 }, { 212,175,55 }))
        {
#ifdef _WIN32
            // (void) подавляет C4834 — предупреждение об игнорировании возвращаемого значения
            (void)ShellExecuteW(nullptr, L"open", L"Справка.chm", nullptr, nullptr, SW_SHOWNORMAL);
#endif
        }
    }
    // ── Экран выбора карты-награды (анимация) ─────────────────────────────────
    else if (gameState == 5)
    {
        if (blockInput) { if (!mouseDown) blockInput = false; return; }

        // Обработка одной из трёх карт (одинаковая логика, разные переменные)
        auto handleRewardCard = [&](RectangleShape& s, bool& active, bool& wasOver,
            bool& bOver, int& stage, float angle,
            float cx, CardData& choice)
            {
                bool over = s.getGlobalBounds().contains(mouse_position);
                if (!selected && over)
                {
                    s.setFillColor(Color(0, 175, 255));
                    if (!wasOver && cardAudio) { cardAudio->setLooping(false); cardAudio->play(); }
                    if (!bOver) { s.setSize({ 200.f,400.f }); s.setOrigin({ 100.f,200.f }); s.setPosition({ cx,centerY }); bOver = true; }
                    if (mouseDown) { active = true; selected = true; stage = 1; s.setFillColor(Color::Yellow); playerDeck.push_back(choice); }
                }
                else
                {
                    if (!selected) s.setFillColor(Color::White);
                    if (bOver && !selected) { s.setSize({ 100.f,200.f }); s.setOrigin({ 50.f,100.f }); s.setPosition({ cx,centerY }); bOver = false; }
                }
                wasOver = over;
            };

        handleRewardCard(shape, active1, wasOverCard1, ButtonIsOver, stage1, rotationAngle1, center1X, rewardCardChoices[0]);
        handleRewardCard(shape2, active2, wasOverCard2, ButtonIsOver2, stage2, rotationAngle2, center2X, rewardCardChoices[1]);
        handleRewardCard(shape3, active3, wasOverCard3, ButtonIsOver3, stage3, rotationAngle3, center3X, rewardCardChoices[2]);
    }
    // ── Карта перемещений ─────────────────────────────────────────────────────
    else if (gameState == 1)
    {
        bool overDeckBtn = deckViewerButton.getGlobalBounds().contains(mouse_position);
        deckViewerButton.setFillColor(overDeckBtn ? Color(70, 100, 160) : Color(50, 70, 110));
        if (overDeckBtn && mouseDown) { showDeckViewer = true; deckViewerScroll = 0; }

        for (int i = 0; i < (int)nodes.size(); i++)
        {
            if (!nodes[i].available || nodes[i].visited)
            {
                nodes[i].circle.setOutlineThickness(0.f); continue;
            }

            bool overNode = nodes[i].circle.getGlobalBounds().contains(mouse_position);
            nodes[i].circle.setOutlineThickness(overNode ? 3.f : 0.f);
            if (overNode) nodes[i].circle.setOutlineColor(Color::White);

            if (overNode && mouseDown)
            {
                nodes[i].visited = true;
                currentNodeIndex = i;
                updateAvailableNodes();

                switch (nodes[i].type)
                {
                case 0: // Враг
                    battleSystem.startBattle(heroCurrentHP, heroMaxHP, i, playerDeck);
                    applyEnemySprite();
                    gameState = 2;
                    break;
                case 1: // Магазин
                    generateShopCards();
                    gameState = 3;
                    cardSoldThisVisit = healedThisVisit = false;
                    showSellCardScreen = false;
                    if (shopMusic) shopMusic->play();
                    break;
                case 2: // Костёр
                    campfireRested = false;
                    gameState = 8;
                    break;
                case 3: // Босс
                    battleSystem.startBattle(heroCurrentHP, heroMaxHP, -1, playerDeck);
                    applyEnemySprite();
                    gameState = 6;
                    break;
                }
            }
        }
    }
    // ── Обычный бой ──────────────────────────────────────────────────────────
    else if (gameState == 2)
    {
        float dt = clock.restart().asSeconds();
        battleSystem.update(window->mapPixelToCoords(Mouse::getPosition(*window)), mouseDown, dt);

        if (battleSystem.battleOver && !battleSystem.playerWon && gameState != 9)
        {
            gameState = 9;
            endRunButtonReady = false;
            if (defeatSound) { defeatSound->setLooping(false); defeatSound->play(); }
        }
        if (battleSystem.battleOver && battleSystem.playerWon)
        {
            heroCurrentHP = battleSystem.heroHP;
            goldSelected = false;
            gameState = 4;
        }
    }
    // ── Магазин ──────────────────────────────────────────────────────────────
    else if (gameState == 3)
    {
        bool clicked = mouseDown && !shopMouseWasDown;
        updateShop(mouse_position, clicked);
        shopMouseWasDown = mouseDown;
    }
    // ── Костёр ───────────────────────────────────────────────────────────────
    else if (gameState == 8)
    {
        bool clicked = mouseDown && !shopMouseWasDown;
        updateCampfire(mouse_position, clicked);
        shopMouseWasDown = mouseDown;
    }
    // ── Бой с боссом ─────────────────────────────────────────────────────────
    else if (gameState == 6)
    {
        float dt = clock.restart().asSeconds();
        battleSystem.update(window->mapPixelToCoords(Mouse::getPosition(*window)), mouseDown, dt);

        if (battleSystem.battleOver && !battleSystem.playerWon && gameState != 10)
        {
            gameState = 10;
            endRunButtonReady = false;
            if (defeatSound) { defeatSound->setLooping(false); defeatSound->play(); }
        }
        if (battleSystem.battleOver && battleSystem.playerWon && gameState != 7)
        {
            heroCurrentHP = battleSystem.heroHP;
            gameState = 7;
            endRunButtonReady = false;
            if (victorySound) { victorySound->setLooping(false); victorySound->play(); }
        }
    }
    // ── Экраны конца забега (победа/поражение) ────────────────────────────────
    else if (gameState == 7 || gameState == 9 || gameState == 10)
    {
        bool clicked = mouseDown && !shopMouseWasDown;
        updateEndRunScreen(mouse_position, clicked);
        shopMouseWasDown = mouseDown;
    }
    // ── Экран наград после боя ────────────────────────────────────────────────
    else if (gameState == 4)
    {
        if (goldBlock.getGlobalBounds().contains(mouse_position) && mouseDown && !goldSelected)
        {
            moneyCount += 25;
            goldSelected = true;
            initMoneyBar();
        }
        if (cardsBlock.getGlobalBounds().contains(mouse_position) && mouseDown)
        {
            // Сбрасываем состояние анимации и переходим к выбору карты
            selected = active1 = active2 = active3 = false;
            stage1 = stage2 = stage3 = 0;
            ButtonIsOver = ButtonIsOver2 = ButtonIsOver3 = false;
            rotationAngle1 = rotationAngle2 = rotationAngle3 = 0.f;
            for (auto* s : { &shape, &shape2, &shape3 })
            {
                s->setSize({ 100.f,200.f }); s->setOrigin({ 50.f,100.f });
            }
            shape.setPosition({ center1X,centerY }); shape2.setPosition({ center2X,centerY }); shape3.setPosition({ center3X,centerY });
            generateRewardCards();
            gameState = 5;
            blockInput = true;
        }
        if (continueButton.getGlobalBounds().contains(mouse_position) && mouseDown)
            gameState = 1;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// RENDER — отрисовка текущего экрана
// ═════════════════════════════════════════════════════════════════════════════

// Вспомогательная функция: цвет и обводка карты-награды по типу
static void applyCardColor(RectangleShape& s, CardType t)
{
    switch (t)
    {
    case CardType::Attack: s.setFillColor(Color(130, 35, 35));  s.setOutlineColor(Color(230, 80, 80));  break;
    case CardType::Defend: s.setFillColor(Color(30, 60, 120));  s.setOutlineColor(Color(80, 150, 230)); break;
    case CardType::Buff:   s.setFillColor(Color(50, 100, 35));  s.setOutlineColor(Color(110, 210, 80)); break;
    default:               s.setFillColor(Color(90, 30, 90));   s.setOutlineColor(Color(200, 80, 200)); break;
    }
    s.setOutlineThickness(3.f);
}

// Вспомогательная функция: одна фаза анимации «улетающей» карты-награды
static bool animateCard(RectangleShape& s, int& stage, float& angle)
{
    if (stage == 1) // сжатие + вращение
    {
        Vector2f sz = s.getSize();
        if (sz.x > 20.f) { s.setSize({ sz.x - 8.f, sz.y - 16.f }); angle += 15.f; s.setRotation(degrees(angle)); s.setOrigin({ sz.x / 2.f,sz.y / 2.f }); }
        else { stage = 2; angle = 0.f; }
    }
    else if (stage == 2) // полный оборот
    {
        angle += 20.f; s.setRotation(degrees(angle));
        if (angle >= 360.f) { stage = 3; angle = 0.f; s.setRotation(degrees(0.f)); }
    }
    else if (stage == 3) // падение вниз
    {
        s.move({ 0.f, 15.f });
        if (s.getPosition().y > 700.f) { stage = 0; return true; } // анимация завершена
    }
    return false;
}

void Game::render()
{
    // ── Анимация карт-наград (выполняется на экране 5, но обновляется здесь) ──
    if (active1 && animateCard(shape, stage1, rotationAngle1)) { active1 = false; gameState = 1; }
    if (active2 && animateCard(shape2, stage2, rotationAngle2)) { active2 = false; gameState = 1; }
    if (active3 && animateCard(shape3, stage3, rotationAngle3)) { active3 = false; gameState = 1; }

    // ── Главное меню ──────────────────────────────────────────────────────────
    if (gameState == 0)
    {
        window->draw(backgraund);
        window->draw(startButton); window->draw(exsitButton); window->draw(helpButton);
        if (mainMenu) window->draw(*mainMenu);
    }
    // ── Выбор карты-награды ───────────────────────────────────────────────────
    else if (gameState == 5)
    {
        window->clear(Color(30, 20, 40));

        RectangleShape* shapes[3] = { &shape, &shape2, &shape3 };
        for (int i = 0; i < 3; i++)
        {
            bool animating = (i == 0 && active1) || (i == 1 && active2) || (i == 2 && active3);
            // Если карта не выбрана — пропускаем остальные
            if (selected && !animating) continue;

            applyCardColor(*shapes[i], rewardCardChoices[i].type);
            window->draw(*shapes[i]);

            // Текст на карте — только пока карта не анимируется
            if (!animating)
            {
                Vector2f pos = shapes[i]->getPosition();
                Vector2f sz = shapes[i]->getSize();
                Vector2f origin = shapes[i]->getOrigin();
                float left = pos.x - origin.x, top = pos.y - origin.y;

                Text nameT(font, rewardCardChoices[i].name, 14);
                nameT.setFillColor(Color::White);
                FloatRect nb = nameT.getLocalBounds();
                nameT.setOrigin({ nb.size.x / 2.f, 0.f });
                nameT.setPosition({ left + sz.x / 2.f, top + 10.f });
                window->draw(nameT);

                Text descT(font, rewardCardChoices[i].description, 11);
                descT.setFillColor(Color(220, 220, 200));
                descT.setPosition({ left + 8.f, top + sz.y * 0.38f });
                window->draw(descT);

                Text costT(font, to_wstring(rewardCardChoices[i].energyCost), 22);
                costT.setFillColor(Color(100, 210, 255));
                costT.setPosition({ left + sz.x - 26.f, top + 6.f });
                window->draw(costT);

                wstring typeStr;
                switch (rewardCardChoices[i].type)
                {
                case CardType::Attack: typeStr = L"Атака";  break;
                case CardType::Defend: typeStr = L"Защита"; break;
                case CardType::Buff:   typeStr = L"Баф";    break;
                default:               typeStr = L"Дебаф";  break;
                }
                Text typeT(font, typeStr, 12);
                typeT.setFillColor(Color(200, 200, 160));
                FloatRect tb = typeT.getLocalBounds();
                typeT.setOrigin({ tb.size.x / 2.f, 0.f });
                typeT.setPosition({ left + sz.x / 2.f, top + sz.y - 24.f });
                window->draw(typeT);
            }
        }
        if (menuText) window->draw(*menuText);
    }
    // ── Карта перемещений ─────────────────────────────────────────────────────
    else if (gameState == 1)
    {
        window->clear(Color(225, 220, 210));
        if (moneyText) window->draw(*moneyText);

        // Связи
        for (auto& l : links)
        {
            VertexArray line(PrimitiveType::Lines, 2);
            bool traveled = nodes[l.from].visited && nodes[l.to].visited;
            Color col = traveled ? Color(90, 160, 90) : Color(110, 105, 100);
            line[0].position = nodes[l.from].circle.getPosition(); line[0].color = col;
            line[1].position = nodes[l.to].circle.getPosition();   line[1].color = col;
            window->draw(line);
        }

        // Узлы
        for (int i = 0; i < (int)nodes.size(); i++)
        {
            CircleShape nd = nodes[i].circle;
            if (nodes[i].visited) { Color c = nd.getFillColor(); nd.setFillColor(Color(c.r / 2, c.g / 2, c.b / 2, 200)); }
            else if (!nodes[i].available) nd.setFillColor(Color(150, 145, 140, 140));
            window->draw(nd);

            // Маркер текущего узла
            if (currentNodeIndex == i)
            {
                CircleShape marker = nd;
                marker.setRadius(nd.getRadius() + 4.f);
                marker.setOrigin({ nd.getOrigin().x + 4.f, nd.getOrigin().y + 4.f });
                marker.setFillColor(Color::Transparent);
                marker.setOutlineThickness(2.f);
                marker.setOutlineColor(Color(255, 255, 255, 220));
                window->draw(marker);
            }
        }

        // Легенда
        window->draw(legendBg);
        window->draw(legendEnemyCircle); window->draw(legendShopCircle);
        window->draw(legendCampfireCircle); window->draw(legendBossCircle);
        if (legendTitle)        window->draw(*legendTitle);
        if (legendEnemyText)    window->draw(*legendEnemyText);
        if (legendShopText)     window->draw(*legendShopText);
        if (legendCampfireText) window->draw(*legendCampfireText);
        if (legendBossText)     window->draw(*legendBossText);
        if (mapText)            window->draw(*mapText);

        // Кнопка просмотра колоды
        window->draw(deckViewerButton);
        Text dvBtn(font, L"Моя колода (" + to_wstring(17 + (int)playerDeck.size()) + L")", 13);
        dvBtn.setFillColor(Color(200, 220, 255));
        dvBtn.setPosition({ 26.f, 562.f });
        window->draw(dvBtn);
    }
    // ── Бой (обычный и с боссом — одинаковая отрисовка) 
    else if (gameState == 2 || gameState == 6)
    {
        battleSystem.battleBGrender(window);
        window->draw(hero);
        window->draw(enemy);
        if (moneyText) window->draw(*moneyText);
        battleSystem.render(window, endGame);
    }
    // ── Магазин
    else if (gameState == 3) renderShop();
    // ── Костёр
    else if (gameState == 8) renderCampfire();
    // ── Экраны конца забега 
    else if (gameState == 7)  renderEndRunScreen(1);
    else if (gameState == 9)  renderEndRunScreen(0);
    else if (gameState == 10) renderEndRunScreen(2);

    // ── Окно наград после боя (gameState == 4, рисуется поверх фона боя) 
    if (gameState == 4)
    {
        battleSystem.battleBGrender(window);
        window->draw(rewardBlockBg);
        window->draw(rewardBlock);
        if (moneyText) window->draw(*moneyText);

        Vector2f mp = Vector2f(Mouse::getPosition(*window));
        goldBlock.setFillColor(goldBlock.getGlobalBounds().contains(mp) ? Color(90, 75, 15) : Color(60, 50, 10));
        cardsBlock.setFillColor(cardsBlock.getGlobalBounds().contains(mp) ? Color(15, 55, 95) : Color(10, 35, 60));
        continueButton.setFillColor(continueButton.getGlobalBounds().contains(mp) ? Color(90, 170, 90) : Color(70, 130, 70));

        if (!goldSelected) { window->draw(goldBlock);    if (rewardGoldText)  window->draw(*rewardGoldText); }
        window->draw(cardsBlock);
        window->draw(continueButton);
        if (rewardText)         window->draw(*rewardText);
        if (rewardCardsText)    window->draw(*rewardCardsText);
        if (continueButtonText) window->draw(*continueButtonText);
    }

    // ── Просмотрщик колоды (поверх всего остального) 
    if (showDeckViewer) renderDeckViewer();

    window->display();
}
