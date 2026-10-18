#include "Battle.h"
#include <iostream>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// Конструктор: обнуляем все поля до безопасных значений.
// Реальная инициализация визуала происходит в initBattle().
// ─────────────────────────────────────────────────────────────────────────────
Battle::Battle()
{
    // Указатели на графику / звук — ставим nullptr, чтобы не было мусора
    deathOfHero = nullptr;
    battleFont = nullptr;
    heroHPText = heroBlockText = heroStrengthText = nullptr;
    enemyHPText = enemyBlockText = enemyPoisonText = enemyWeakenText = nullptr;
    energyText = enemyIntentText = endTurnText = nullptr;
    messageText = enemyNameText = nullptr;
    foresightTitleText = foresightAttackText = foresightDefendText = nullptr;
    foresightBuffText = foresightDebuffText = nullptr;

    // Стартовые значения статов
    heroHP = heroMaxHP = 80;
    enemyHP = enemyMaxHP = 50;
    heroBlock = enemyBlock = 0;
    maxEnergy = energy = 3;
    heroDefenseBonus = heroStrength = 0;

    // Флаги состояния боя
    playerTurn = true;
    battleOver = playerWon = enemyDieing = false;
    mouseWasClicked = enemyTurnPending = false;
    extraTurnGranted = false;

    // Параметры врага
    currentIntent = EnemyIntent::Attack;
    currentEnemyType = EnemyType::Tutorial;
    intentValue = 6;
    enemyDefenseBonus = enemyStrength = enemyRegeneration = enemyHealing = 0;
    buff = turnCount = 0;

    // Дебаффы / баффы
    buffStandartBonus = debuffStandart = heroWeaken = 0;
    poison = heroPoison = echo = heroIntentBonus = 0;

    // Прочие флаги
    messageTtl = 0.f;
    deathSoundPlayed = false;
    heroStunnedAttack = heroStunnedDefend = enemyAppliesStunNext = false;

    // Панель "Предсказуемость"
    showForesightPanel = false;
    foresightCardIndex = -1;
    foresightChosenIntent = EnemyIntent::Attack;
    foresightChosenBuff = 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Деструктор: освобождаем все Text*, выделенные через new
// ─────────────────────────────────────────────────────────────────────────────
Battle::~Battle()
{
    delete heroHPText;    delete enemyHPText;
    delete heroBlockText; delete enemyBlockText;
    delete heroStrengthText; delete enemyPoisonText; delete enemyWeakenText;
    delete energyText;    delete enemyIntentText;
    delete endTurnText;   delete messageText;       delete enemyNameText;
    delete foresightTitleText; delete foresightAttackText;
    delete foresightDefendText; delete foresightBuffText; delete foresightDebuffText;

    // Карты в руке тоже содержат Text*, удаляем их
    for (auto& card : hand)
    {
        delete card.nameText;
        delete card.descriptionText;
        delete card.costText;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// initBattle — создаёт все SFML-объекты (HP-бары, кнопки, тексты).
// Вызывается один раз при старте игры, font хранится в Game.
// ─────────────────────────────────────────────────────────────────────────────
void Battle::initBattle(Font* font)
{
    // Фон арены
    battleBG.setSize({ 800.f, 600.f });
    if (!battleBGimage.loadFromFile("image/BattleBG.png"))
        return;
    battleBG.setTexture(&battleBGimage);

    // Звук смерти героя
    if (heroFaile.loadFromFile("sound/Death_of_Hero.ogg"))
        deathOfHero = new Sound(heroFaile);

    battleFont = font;

    // ── HP-бары ──────────────────────────────────────────────────────────────
    auto makeHPBar = [](RectangleShape& bg, RectangleShape& bar, Vector2f pos)
        {
            bg.setSize({ 160.f, 20.f });
            bg.setFillColor(Color(60, 20, 20));   // тёмно-красный фон
            bg.setPosition(pos);

            bar.setSize({ 160.f, 20.f });
            bar.setFillColor(Color(200, 40, 40)); // красный прогресс
            bar.setPosition(pos);
        };
    makeHPBar(heroHPbarBg, heroHPbar, { 50.f,  120.f });
    makeHPBar(enemyHPbarBg, enemyHPbar, { 580.f, 120.f });

    // ── Вспомогательная лямбда: создать Text с заданными параметрами ─────────
    auto makeText = [&](Text*& ptr, const wstring& str, unsigned sz,
        Color col, Vector2f pos)
        {
            ptr = new Text(*battleFont, str, sz);
            ptr->setFillColor(col);
            ptr->setPosition(pos);
        };

    // HP цифры
    makeText(heroHPText, L"", 14, Color::White, { 55.f,  121.f });
    makeText(enemyHPText, L"", 14, Color::White, { 585.f, 121.f });

    // Блок (синий)
    makeText(heroBlockText, L"", 16, Color(80, 150, 230), { 50.f,  90.f });
    makeText(enemyBlockText, L"", 16, Color(80, 150, 230), { 580.f, 90.f });

    // Сила героя (зелёный)
    makeText(heroStrengthText, L"", 16, Color(110, 210, 80), { 50.f, 70.f });

    // Яд и ослабление врага (фиолетовый)
    makeText(enemyPoisonText, L"", 16, Color(200, 80, 200), { 580.f, 70.f });
    makeText(enemyWeakenText, L"", 16, Color(200, 80, 200), { 580.f, 50.f });

    // Имя врага
    makeText(enemyNameText, L"", 16, Color(255, 200, 100), { 580.f, 148.f });

    // Намерение врага
    makeText(enemyIntentText, L"", 15, Color(255, 160, 50), { 520.f, 60.f });

    // Всплывающее сообщение
    makeText(messageText, L"", 22, Color::Yellow, { 320.f, 220.f });

    // ── Круговой индикатор энергии ───────────────────────────────────────────
    energyBar.setRadius(25.f);
    energyBar.setFillColor(Color(0, 90, 180));
    energyBar.setOutlineThickness(3.f);
    energyBar.setOutlineColor(Color::Cyan);
    energyBar.setPosition({ 40.f, 370.f });

    energyText = new Text(*battleFont, L"", 22);
    energyText->setFillColor(Color::White);

    // ── Кнопка «Конец хода» ──────────────────────────────────────────────────
    endTurnButton.setSize({ 140.f, 40.f });
    endTurnButton.setFillColor(Color(110, 90, 80));
    endTurnButton.setOutlineThickness(2.f);
    endTurnButton.setOutlineColor(Color::Black);
    endTurnButton.setPosition({ 620.f, 370.f });

    endTurnText = new Text(*battleFont, L"Конец Хода", 16);
    endTurnText->setFillColor(Color::White);

    // ── Панель «Предсказуемость» (выбор намерения врага) ─────────────────────
    foresightPanelBg.setSize({ 400.f, 220.f });
    foresightPanelBg.setFillColor(Color(25, 20, 35, 235));
    foresightPanelBg.setOutlineThickness(3.f);
    foresightPanelBg.setOutlineColor(Color(200, 80, 200));
    foresightPanelBg.setPosition({ 200.f, 190.f });

    foresightTitleText = new Text(*battleFont,
        L"Предсказуемость: выберите намерение врага", 16);
    foresightTitleText->setFillColor(Color(200, 80, 200));
    foresightTitleText->setPosition({ 215.f, 200.f });

    // Четыре кнопки выбора намерения
    struct ForesightOption {
        RectangleShape& btn;
        Text*& txt;
        Color           fill, outline;
        Vector2f        pos;
        wstring         label;
        unsigned        labelSz;
        Vector2f        labelPos;
    };

    const float optW = 170.f, optH = 50.f;

    auto initForesightBtn = [&](RectangleShape& btn, Color fill, Color outline,
        Vector2f pos,
        Text*& txt, const wstring& label, unsigned sz,
        Color lblCol, Vector2f lblPos)
        {
            btn.setSize({ optW, optH });
            btn.setFillColor(fill);
            btn.setOutlineThickness(2.f);
            btn.setOutlineColor(outline);
            btn.setPosition(pos);
            txt = new Text(*battleFont, label, sz);
            txt->setFillColor(lblCol);
            txt->setPosition(lblPos);
        };

    initForesightBtn(foresightOptionAttack, Color(130, 35, 35), Color(230, 80, 80),
        { 215.f,240.f }, foresightAttackText, L"Атака", 16,
        Color::White, { 270.f,258.f });
    initForesightBtn(foresightOptionDefend, Color(30, 60, 120), Color(80, 150, 230),
        { 400.f,240.f }, foresightDefendText, L"Защита", 16,
        Color::White, { 450.f,258.f });
    initForesightBtn(foresightOptionBuff, Color(50, 100, 35), Color(110, 210, 80),
        { 215.f,305.f }, foresightBuffText, L"Усиление", 16,
        Color::White, { 265.f,323.f });
    initForesightBtn(foresightOptionDebuff, Color(90, 30, 90), Color(200, 80, 200),
        { 400.f,305.f }, foresightDebuffText, L"Ослабление врага", 14,
        Color::White, { 410.f,325.f });
}

// ─────────────────────────────────────────────────────────────────────────────
// startBattle — сбрасывает все переменные и запускает новый бой.
// nodeIndex определяет тип врага (-1 = босс, 0-1 = туториал, остальные = рандом).
// extraCards — купленные/полученные как награда карты добавляются в колоду.
// ─────────────────────────────────────────────────────────────────────────────
void Battle::startBattle(int hHP, int hMaxHP, int nodeIndex, vector<CardData>& extraCards)
{
    // ── Сброс состояния врага ─────────────────────────────────────────────────
    enemyDieing = false;
    buff = enemyHealing = enemyDefenseBonus = enemyStrength = enemyRegeneration = 0;
    turnCount = 0;

    // ── Сброс состояния героя ─────────────────────────────────────────────────
    heroHP = hHP;
    heroMaxHP = hMaxHP;
    heroBlock = enemyBlock = 0;
    maxEnergy = energy = 3;
    heroDefenseBonus = heroStrength = 0;

    // ── Сброс боя ─────────────────────────────────────────────────────────────
    playerTurn = true;
    battleOver = playerWon = enemyTurnPending = false;
    messageTtl = 0.f;
    mouseWasClicked = false;
    deathSoundPlayed = false;

    // ── Сброс дебаффов ────────────────────────────────────────────────────────
    buffStandartBonus = debuffStandart = heroWeaken = poison = heroPoison = echo = 0;

    // ── Сброс механик стана и Предсказуемости ────────────────────────────────
    heroStunnedAttack = heroStunnedDefend = enemyAppliesStunNext = false;
    showForesightPanel = false;
    foresightCardIndex = -1;
    extraTurnGranted = false;

    // ── Очистка руки (освобождаем Text*) ──────────────────────────────────────
    for (auto& card : hand)
    {
        delete card.nameText;
        delete card.descriptionText;
        delete card.costText;
    }
    hand.clear();
    discardPile.clear();

    // ── Выбор врага по узлу карты ────────────────────────────────────────────
    // Тип врага определяет его HP, имя и поведение в generateNextEnemyIntent().
    if (nodeIndex == 0 || nodeIndex == 1)
    {
        currentEnemyType = EnemyType::Tutorial;
        enemyHP = enemyMaxHP = 50;
        if (enemyNameText) enemyNameText->setString(L"Бальтазар");
    }
    else if (nodeIndex == -1)
    {
        currentEnemyType = EnemyType::Boss;
        enemyHP = enemyMaxHP = 200;
        if (enemyNameText) enemyNameText->setString(L"Злой маг Альдеборан");
    }
    else
    {
        // Случайный враг (25% на каждого)
        struct EnemyConfig { EnemyType type; int hp; wstring name; };
        EnemyConfig pool[] = {
            { EnemyType::Brute,    140, L"Громила"  },
            { EnemyType::Shaman,    90, L"Шаман"    },
            { EnemyType::Duelist,  100, L"Дуэлянт"  },
            { EnemyType::Assassin,  65, L"Ассасин"  },
        };
        auto& cfg = pool[rand() % 4];
        currentEnemyType = cfg.type;
        enemyHP = enemyMaxHP = cfg.hp;
        if (enemyNameText) enemyNameText->setString(cfg.name);
    }

    // ── Сборка колоды ────────────────────────────────────────────────────────
    buildDeck();

    // Добавляем доп. карты (из магазина и наград)
    for (auto& card : extraCards)
        deck.push_back(card);

    // Перемешиваем всю колоду
    random_device rd;
    mt19937 g(rd());
    shuffle(deck.begin(), deck.end(), g);

    // Берём 5 карт в руку и получаем первое намерение врага
    drawCards(5);
    generateNextEnemyIntent();
}

// ─────────────────────────────────────────────────────────────────────────────
// buildDeck — создаёт базовую стартовую колоду (одинакова для всех боёв).
// Доп. карты добавляются снаружи, в startBattle().
// ─────────────────────────────────────────────────────────────────────────────
void Battle::buildDeck()
{
    deck.clear();

    // 8 пар "Удар + Блок"
    for (int i = 0; i < 8; i++)
    {
        deck.push_back({ L"Удар", L"Наносит 15 урона.",  CardType::Attack, 1, 15, 0 });
        deck.push_back({ L"Блок", L"Даёт 18 блока.",     CardType::Defend, 1, 18, 0 });
    }

    // Особые стартовые карты
    deck.push_back({ L"Выпад",      L"Наносит 10 урона, бафф.",             CardType::Attack, 2, 10, 1 });
    deck.push_back({ L"Боевой клич",L"Даёт 3 силы на 3 хода.",              CardType::Buff,   1,  3, 0 });
    deck.push_back({ L"Ослабление", L"Снижает урон врага на 5.",             CardType::Debuff, 1,  5, 0 });
    deck.push_back({ L"Яд",         L"Накладывает 4 яда на врага",           CardType::Debuff, 1,  4, 4 });
    deck.push_back({ L"Адреналин",  L"Доп. ход. Забирает 10 HP.",            CardType::Buff,   1,  0, 6 });
    deck.push_back({ L"Бастион",        L"Даёт 16 блока.",                   CardType::Defend,  2, 16, 0 });

    // Перемешиваем колоду
    random_device rd;
    mt19937 g(rd());
    shuffle(deck.begin(), deck.end(), g);
}

// drawCards — берёт count карт из колоды в руку (максимум 7 карт в руке).
// Если колода опустела, сброс перетасовывается обратно.
void Battle::drawCards(int count)
{
    // Цвет карты зависит от её типа
    auto cardColor = [](CardType t) -> Color {
        switch (t)
        {
        case CardType::Attack: return Color(130, 35, 35);
        case CardType::Defend: return Color(30, 60, 120);
        case CardType::Buff:   return Color(50, 100, 35);
        default:               return Color(90, 30, 90);
        }
        };

    for (int i = 0; i < count; i++)
    {
        // Пересобираем колоду из сброса, если она пустая
        if (deck.empty())
        {
            if (discardPile.empty()) break;
            deck = discardPile;
            discardPile.clear();
            random_device rd;
            mt19937 g(rd());
            shuffle(deck.begin(), deck.end(), g);
        }

        if (hand.size() >= 7) break; // рука не вмещает больше 7 карт

        CardData data = deck.back();
        deck.pop_back();

        HandCard card;
        card.nameText = card.descriptionText = card.costText = nullptr;
        card.dragging = card.mouse_hovered = false;
        card.data = data;

        // Визуальное тело карты
        card.body.setSize({ 110.f, 150.f });
        card.body.setOutlineThickness(2.f);
        card.body.setOutlineColor(Color::Black);
        card.body.setFillColor(cardColor(data.type));

        // Текстуры карт (если загружены)
        if (data.type == CardType::Attack && attackCardTex.getSize().x > 0)
            card.body.setTexture(&attackCardTex);
        else if (data.type == CardType::Defend && defendCardTex.getSize().x > 0)
            card.body.setTexture(&defendCardTex);

        // Тексты на карте
        card.nameText = new Text(*battleFont, data.name, 14);
        card.nameText->setFillColor(Color::White);

        card.descriptionText = new Text(*battleFont, data.description, 11);
        card.descriptionText->setFillColor(Color(240, 240, 240));

        card.costText = new Text(*battleFont, to_wstring(data.energyCost), 18);
        card.costText->setFillColor(Color::Yellow);

        hand.push_back(card);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// generateNextEnemyIntent — выбирает следующее намерение врага.
// Каждый тип врага имеет свой уникальный паттерн поведения.
// ─────────────────────────────────────────────────────────────────────────────
void Battle::generateNextEnemyIntent()
{
    turnCount++;

    switch (currentEnemyType)
    {
        // ── Туториальный враг: лёгкий и предсказуемый ────────────────────────────
    case EnemyType::Tutorial:
        if (enemyHP <= enemyMaxHP / 3)
        {
            // При низком HP — лечится
            currentIntent = EnemyIntent::Buff;
            buff = 1;
        }
        else
        {
            int r = rand() % 10;
            if (r < 5) { currentIntent = EnemyIntent::Attack;  intentValue = 6 + (rand() % 5); }
            else if (r < 8) { currentIntent = EnemyIntent::Defend;  intentValue = 6 + (rand() % 7); }
            else { currentIntent = EnemyIntent::Buff;     buff = rand() % 3; intentValue = 2; }
        }
        break;

        // ── Громила: тяжёлые удары, каждые 3 хода — удар ярости ─────────────────
    case EnemyType::Brute:
        if (turnCount % 4 == 0) { currentIntent = EnemyIntent::Buff;   buff = 4; }           // Оковы
        else if (turnCount % 3 == 0) { currentIntent = EnemyIntent::Attack;  intentValue = 22 + (rand() % 7); } // ярость
        else
        {
            if (rand() % 10 < 7) { currentIntent = EnemyIntent::Attack;  intentValue = 12 + (rand() % 6); }
            else { currentIntent = EnemyIntent::Defend;  intentValue = 10 + (rand() % 6); }
        }
        break;

        // ── Шаман: усиливается и ослабляет игрока, иногда регенерирует ───────────
    case EnemyType::Shaman:
        if (turnCount % 3 == 0) { currentIntent = EnemyIntent::Buff;   buff = 1; enemyHealing = 0; }
        else if (turnCount % 5 == 0) { currentIntent = EnemyIntent::Buff;   buff = 2; intentValue = 6; }
        else if (turnCount % 2 == 1) { currentIntent = EnemyIntent::Buff;   buff = 0; intentValue = 4 + (rand() % 3); }
        else { currentIntent = EnemyIntent::Attack;  intentValue = 10 + (rand() % 6) + enemyStrength; }
        break;

        // ── Дуэлянт: чётные ходы — защита, нечётные — контратака ────────────────
    case EnemyType::Duelist:
        if (turnCount % 2 == 1)
        {
            currentIntent = EnemyIntent::Defend;  intentValue = 15 + (rand() % 6);
        }
        else
        {
            currentIntent = EnemyIntent::Attack;  intentValue = 10 + (rand() % 6) + (enemyBlock / 2);
        }
        break;

        // ── Ассасин: 3 хода копит силу, потом один мощный удар ───────────────────
    case EnemyType::Assassin:
        if (turnCount % 6 == 0) { currentIntent = EnemyIntent::Buff;   buff = 4; }           // Оковы
        else if (turnCount % 4 != 0) { currentIntent = EnemyIntent::Buff;   buff = 0; intentValue = 5 + (rand() % 4); }
        else { currentIntent = EnemyIntent::Attack;  intentValue = 8 + enemyStrength; enemyStrength = 0; }
        break;

        // ── Босс: фаза 1 (>50% HP) и фаза 2 (берсерк) ───────────────────────────
    case EnemyType::Boss:
        if (enemyHP > enemyMaxHP / 2)
        {
            // Фаза 1: паттерн из атак, защиты, усилений и специальных действий
            if (turnCount % 7 == 0) { currentIntent = EnemyIntent::Buff;   buff = 5; intentValue = 3; } // яд игрока
            else if (turnCount % 4 == 0) { currentIntent = EnemyIntent::Buff;   buff = 4; }                   // Оковы
            else if (turnCount % 3 == 0) { currentIntent = EnemyIntent::Buff;   buff = 0; intentValue = 6; }  // усиление
            else if (turnCount % 2 == 0) { currentIntent = EnemyIntent::Defend; intentValue = 18 + (rand() % 6); }
            else { currentIntent = EnemyIntent::Attack;  intentValue = 16 + (rand() % 8); }
        }
        else
        {
            // Фаза 2 (берсерк): 33% — двойной удар, остальное — мощная атака
            if (rand() % 3 == 0) { currentIntent = EnemyIntent::Buff;   buff = 3; intentValue = 17 + enemyStrength; }
            else { currentIntent = EnemyIntent::Attack;  intentValue = 11 + (rand() % 9) + enemyStrength; }
        }
        break;

    default:
        currentIntent = EnemyIntent::Attack;
        intentValue = 6;
        break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// showMessage — показывает всплывающее сообщение на 1.5 секунды
// ─────────────────────────────────────────────────────────────────────────────
void Battle::showMessage(const wstring& msg)
{
    if (messageText)
    {
        messageText->setString(msg);
        messageTtl = 1.5f;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// enemyTurnAction — выполняет действие врага.
// Вызывается один раз за переход хода (из update() через флаг enemyTurnPending).
// ─────────────────────────────────────────────────────────────────────────────
void Battle::enemyTurnAction()
{
    // ── Яд на враге: тикает в начале его хода ────────────────────────────────
    if (poison > 0)
    {
        enemyHP -= poison;
        showMessage(L"Яд наносит " + to_wstring(poison) + L" урона врагу!");
        poison--;

        if (enemyHP <= 0)
        {
            enemyHP = 0;
            battleOver = true;
            playerWon = true;
            return;
        }
    }

    // ── Регенерация врага ─────────────────────────────────────────────────────
    if (enemyRegeneration > 0)
    {
        enemyHP = min(enemyHP + enemyRegeneration, enemyMaxHP);
        showMessage(L"Враг регенерирует " + to_wstring(enemyRegeneration) + L" HP");
        enemyRegeneration--;
    }

    // ── Выполнение намерения врага ────────────────────────────────────────────
    if (currentIntent == EnemyIntent::Attack)
    {
        int damage = intentValue + enemyStrength;

        // Ослабление снижает урон врага, но не ниже 0
        if (debuffStandart > 0) damage = max(0, damage - debuffStandart);

        // Ассасин сбрасывает накопленный бонус после удара
        if (currentEnemyType == EnemyType::Assassin)
            enemyStrength = 0;

        // Блок поглощает урон; остаток идёт в HP героя
        if (heroBlock >= damage)
        {
            heroBlock -= damage;
            showMessage(L"Враг атакует: блок поглотил " + to_wstring(damage));
        }
        else
        {
            damage -= heroBlock;
            heroBlock = 0;
            heroHP = max(0, heroHP - damage);
            showMessage(L"Враг атакует: -" + to_wstring(damage) + L" HP");
        }
    }
    else if (currentIntent == EnemyIntent::Defend)
    {
        enemyBlock += intentValue + enemyDefenseBonus;
        showMessage(L"Враг защищается: +" + to_wstring(intentValue) + L" блока");
    }
    else if (currentIntent == EnemyIntent::Buff)
    {
        switch (buff)
        {
        case 0: // Усиление атаки (Шаман ещё и ослабляет игрока)
            enemyStrength += intentValue;
            if (currentEnemyType == EnemyType::Shaman)
            {
                heroWeaken += 2;
                showMessage(L"Шаман усиливается и ослабляет вас! (Бессилие +2)");
            }
            else
                showMessage(L"Враг усиливает атаку! (+" + to_wstring(intentValue) + L")");
            break;

        case 1: // Лечение
        {
            int heal = (currentEnemyType == EnemyType::Shaman)
                ? (rand() % 15) + 12   // Шаман лечится сильнее: 12..26
                : (rand() % 9) + 3;   // остальные: 3..11
            enemyHP = min(enemyHP + heal, enemyMaxHP);
            showMessage(L"Враг восстанавливает " + to_wstring(heal) + L" HP");
            break;
        }

        case 2: // Усиление обороны (Шаман — регенерация вместо блока)
            if (currentEnemyType == EnemyType::Shaman)
            {
                enemyRegeneration += intentValue;
                showMessage(L"Враг получает регенерацию +" + to_wstring(intentValue));
            }
            else
            {
                enemyDefenseBonus += intentValue;
                showMessage(L"Враг усиливает оборону! (+" + to_wstring(intentValue) + L")");
            }
            break;

        case 3: // Двойной удар босса
        {
            int totalDmg = 0;
            for (int hit = 0; hit < 2; hit++)
            {
                int dmg = intentValue;
                if (heroBlock >= dmg)
                    heroBlock -= dmg;
                else
                {
                    dmg -= heroBlock;
                    heroBlock = 0;
                    heroHP = max(0, heroHP - dmg);
                    totalDmg += dmg;
                }
            }
            showMessage(L"Альдеборан: Двойной удар! -" + to_wstring(totalDmg) + L" HP");
            break;
        }

        case 4: // Оковы — на следующем ходу игрок не сможет атаковать/защищаться
            enemyAppliesStunNext = true;
            showMessage(L"Враг накладывает Оковы! Следующий ход без атаки/защиты!");
            break;

        case 5: // Яд на игрока (только босс)
            heroPoison += intentValue;
            showMessage(L"Альдеборан отравляет вас! Яд +" + to_wstring(intentValue));
            break;
        }
    }

    // ── Проверка смерти героя ────────────────────────────────────────────────
    if (heroHP <= 0)
    {
        heroHP = 0;
        battleOver = true;
        playerWon = false;
        return;
    }

    // ── Начало нового хода игрока ─────────────────────────────────────────────
    heroBlock = 0;  // блок сбрасывается в конце каждого хода
    heroDefenseBonus = 0;

    // Сила героя спадает через echo ходов
    echo--;
    if (echo <= 0)
        heroStrength = 0;

    // Дебаффы постепенно спадают (на 1 за ход)
    if (debuffStandart > 0) debuffStandart--;
    if (heroWeaken > 0)  heroWeaken--;

    // Яд на герое тикает в начале его хода
    if (heroPoison > 0)
    {
        heroHP = max(0, heroHP - heroPoison);
        showMessage(L"Яд жжёт вас: -" + to_wstring(heroPoison) + L" HP");
        heroPoison--;

        if (heroHP <= 0)
        {
            heroHP = 0;
            battleOver = true;
            playerWon = false;
            energy = maxEnergy;
            playerTurn = true;
            enemyTurnPending = false;
            return;
        }
    }

    // Восстанавливаем энергию и передаём ход игроку
    energy = maxEnergy;
    playerTurn = true;
    enemyTurnPending = false;

    // Применяем Оковы, если враг их наложил в этот ход
    if (enemyAppliesStunNext)
    {
        heroStunnedAttack = true;
        heroStunnedDefend = true;
        enemyAppliesStunNext = false;
    }
    else
    {
        heroStunnedAttack = heroStunnedDefend = false;
    }

    // Генерируем следующее намерение и добираем карты
    generateNextEnemyIntent();
    drawCards(5);
}

// ─────────────────────────────────────────────────────────────────────────────
// update — главный игровой цикл боя.
// Вызывается каждый кадр из Game::update().
// ─────────────────────────────────────────────────────────────────────────────
void Battle::update(Vector2f mousePos, bool mouseDown, float dt)
{
    // Таймер сообщения (угасает со временем)
    if (messageTtl > 0.f)
        messageTtl -= dt;

    // Если бой завершён или ждём хода врага — не обрабатываем ввод
    if (battleOver) { mouseWasClicked = mouseDown; return; }
    if (enemyTurnPending) { enemyTurnAction(); mouseWasClicked = mouseDown; return; }

    bool clicked = mouseDown && !mouseWasClicked; // одиночный клик (edge)

    // ── Обновление визуала HP-баров ───────────────────────────────────────────
    heroHPbar.setSize({ (float)heroHP / heroMaxHP * 160.f, 20.f });
    enemyHPbar.setSize({ (float)enemyHP / enemyMaxHP * 160.f, 20.f });

    heroHPText->setString(to_wstring(heroHP) + L" / " + to_wstring(heroMaxHP));
    enemyHPText->setString(to_wstring(enemyHP) + L" / " + to_wstring(enemyMaxHP));

    // ── Обновление текстов статусов ──────────────────────────────────────────

    // Блок врага
    enemyBlockText->setString(enemyBlock > 0 ? L"Блок: " + to_wstring(enemyBlock) : L"");

    // Яд на враге
    enemyPoisonText->setString(poison > 0 ? L"Яд: " + to_wstring(poison) : L"");

    // Блок и яд героя (в одной строке)
    if (heroPoison > 0)
        heroBlockText->setString((heroBlock > 0 ? L"Блок: " + to_wstring(heroBlock) + L"  " : L"") + L"Яд: " + to_wstring(heroPoison));
    else
        heroBlockText->setString(heroBlock > 0 ? L"Блок: " + to_wstring(heroBlock) : L"");

    // Ослабление врага
    enemyWeakenText->setString(debuffStandart > 0 ? L"Слабость: -" + to_wstring(debuffStandart) : L"");

    // Сила / бессилие героя (бессилие приоритетнее)
    if (heroWeaken > 0) heroStrengthText->setString(L"Бессилие: -" + to_wstring(heroWeaken));
    else if (heroStrength > 0) heroStrengthText->setString(L"Сила: +" + to_wstring(heroStrength));
    else                       heroStrengthText->setString(L"");

    // Энергия (центрируем цифру в кружке)
    energyText->setString(to_wstring(energy));
    FloatRect etBounds = energyText->getLocalBounds();
    energyText->setPosition({ 40.f + 25.f - etBounds.size.x / 2.f, 370.f + 25.f - etBounds.size.y / 1.3f });

    // ── Текст намерения врага ────────────────────────────────────────────────
    if (currentIntent == EnemyIntent::Attack)
        enemyIntentText->setString(L"Намерение: Атака (" + to_wstring(intentValue + enemyStrength) + L")");
    else if (currentIntent == EnemyIntent::Defend)
        enemyIntentText->setString(L"Намерение: Защита (" + to_wstring(intentValue) + L")");
    else // Buff — расшифровываем код buff
    {
        switch (buff)
        {
        case 0: enemyIntentText->setString(
            (currentEnemyType == EnemyType::Shaman)
            ? L"Намерение: Усиление + Бессилие на вас"
            : L"Намерение: Усиление атаки (+" + to_wstring(intentValue) + L")");
            break;
        case 1: enemyIntentText->setString(L"Намерение: Восстановление HP");        break;
        case 2: enemyIntentText->setString(
            (currentEnemyType == EnemyType::Shaman)
            ? L"Намерение: Регенерация (+" + to_wstring(intentValue) + L")"
            : L"Намерение: Усиление обороны (+" + to_wstring(intentValue) + L")");
            break;
        case 3: enemyIntentText->setString(L"Намерение: Двойной удар! (" + to_wstring(intentValue) + L" x2)"); break;
        case 4: enemyIntentText->setString(L"Намерение: Оковы (блок атаки/защиты)");                           break;
        case 5: enemyIntentText->setString(L"Намерение: Отравить вас! (Яд +" + to_wstring(intentValue) + L")"); break;
        }
    }

    // ── Кнопка «Конец хода» ──────────────────────────────────────────────────
    FloatRect etb = endTurnText->getLocalBounds();
    endTurnText->setOrigin({ etb.size.x / 2.f, etb.size.y / 2.f });
    endTurnText->setPosition({ 620.f + 70.f, 370.f + 16.f });

    bool overEndTurn = endTurnButton.getGlobalBounds().contains(mousePos);
    endTurnButton.setFillColor(overEndTurn ? Color(150, 130, 120) : Color(110, 90, 80));

    if (overEndTurn && clicked)
    {
        // Сбрасываем руку в сброс
        for (auto& card : hand)
        {
            discardPile.push_back(card.data);
            delete card.nameText;
            delete card.descriptionText;
            delete card.costText;
        }
        hand.clear();

        // Карта "Адреналин" даёт дополнительный ход без хода врага
        if (extraTurnGranted)
        {
            extraTurnGranted = false;
            energy = maxEnergy;
            drawCards(5);
            mouseWasClicked = mouseDown;
            return;
        }

        playerTurn = false;
        enemyTurnPending = true;
        mouseWasClicked = mouseDown;
        return;
    }

    // ── Панель «Предсказуемость» ──────────────────────────────────────────────
    // Пока открыта — блокируем розыгрыш карт; только клик по кнопке закрывает
    if (showForesightPanel)
    {
        if (clicked)
        {
            bool choiceMade = false;

            if (foresightOptionAttack.getGlobalBounds().contains(mousePos))
            {
                currentIntent = EnemyIntent::Attack;
                intentValue = 6 + (rand() % 5);
                choiceMade = true;
                showMessage(L"Предсказуемость: намерение изменено на Атаку!");
            }
            else if (foresightOptionDefend.getGlobalBounds().contains(mousePos))
            {
                currentIntent = EnemyIntent::Defend;
                intentValue = 6 + (rand() % 7);
                choiceMade = true;
                showMessage(L"Предсказуемость: намерение изменено на Защиту!");
            }
            else if (foresightOptionBuff.getGlobalBounds().contains(mousePos))
            {
                currentIntent = EnemyIntent::Buff;
                buff = 0;
                intentValue = 3;
                choiceMade = true;
                showMessage(L"Предсказуемость: намерение изменено на Усиление!");
            }
            else if (foresightOptionDebuff.getGlobalBounds().contains(mousePos))
            {
                currentIntent = EnemyIntent::Buff;
                buff = 4; // Оковы
                choiceMade = true;
                showMessage(L"Предсказуемость: враг применит Оковы!");
            }
            else if (!foresightPanelBg.getGlobalBounds().contains(mousePos))
            {
                // Клик вне панели = отмена; карта остаётся в руке
                showForesightPanel = false;
                foresightCardIndex = -1;
            }

            if (choiceMade && foresightCardIndex >= 0 && foresightCardIndex < (int)hand.size())
            {
                // Списываем энергию и убираем карту из руки
                energy -= hand[foresightCardIndex].data.energyCost;
                discardPile.push_back(hand[foresightCardIndex].data);
                delete hand[foresightCardIndex].nameText;
                delete hand[foresightCardIndex].descriptionText;
                delete hand[foresightCardIndex].costText;
                hand.erase(hand.begin() + foresightCardIndex);
                foresightCardIndex = -1;
                showForesightPanel = false;
            }
        }
        mouseWasClicked = mouseDown;
        return;
    }

    // ── Карты в руке ─────────────────────────────────────────────────────────
    float startX = 400.f - (hand.size() * 120.f) / 2.f;
    int   playedCardIndex = -1; // индекс розыгранной карты (-1 = не разыграна)

    // Вспомогательная лямбда: цвет карты (дубликат из drawCards, но нужен здесь)
    auto cardBaseColor = [](CardType t) -> Color {
        switch (t)
        {
        case CardType::Attack: return Color(130, 35, 35);
        case CardType::Defend: return Color(30, 60, 120);
        case CardType::Buff:   return Color(50, 100, 35);
        default:               return Color(90, 30, 90);
        }
        };

    for (int i = 0; i < (int)hand.size(); i++)
    {
        Vector2f basePos(startX + i * 120.f, 430.f);
        hand[i].originalPosition = basePos;

        bool hovered = hand[i].body.getGlobalBounds().contains(mousePos);
        hand[i].mouse_hovered = hovered;

        // Карта недоступна: нет энергии или Оковы
        bool blockedByEnergy = hand[i].data.energyCost > energy;
        bool blockedByStun = (hand[i].data.type == CardType::Attack && heroStunnedAttack)
            || (hand[i].data.type == CardType::Defend && heroStunnedDefend);

        if (hovered)
        {
            // Поднимаем карту при наведении
            hand[i].body.setPosition({ basePos.x, basePos.y - 30.f });
            hand[i].body.setOutlineColor(Color::Yellow);

            if (clicked && !blockedByEnergy)
            {
                if (blockedByStun)
                    showMessage(L"Вы в Оковах! Эта карта недоступна.");
                else if (hand[i].data.type == CardType::Debuff && hand[i].data.effectType == 7)
                {
                    // "Предсказуемость" — открываем панель выбора
                    foresightCardIndex = i;
                    showForesightPanel = true;
                }
                else
                    playedCardIndex = i; // обычная карта — разыгрываем
            }
        }
        else
        {
            hand[i].body.setPosition(basePos);

            // Затемняем недоступные карты
            if (blockedByEnergy || blockedByStun)
            {
                hand[i].body.setFillColor(Color(60, 60, 60));
                hand[i].body.setOutlineColor(Color(80, 80, 80));
                hand[i].body.setOutlineThickness(3.f);
            }
            else
            {
                hand[i].body.setFillColor(cardBaseColor(hand[i].data.type));
                hand[i].body.setOutlineColor(Color::Black);
                hand[i].body.setOutlineThickness(2.f);
            }
        }

        // Позиционируем тексты на карте
        Vector2f cPos = hand[i].body.getPosition();
        hand[i].nameText->setPosition({ cPos.x + 8.f, cPos.y + 10.f });
        hand[i].descriptionText->setPosition({ cPos.x + 8.f, cPos.y + 55.f });
        hand[i].costText->setPosition({ cPos.x + 88.f, cPos.y + 5.f });
    }

    // ── Розыгрыш карты ────────────────────────────────────────────────────────
    if (playedCardIndex != -1)
    {
        CardData card = hand[playedCardIndex].data;
        energy -= card.energyCost;

        // Специальный эффект effectType == 1 (Выпад): даёт силу
        if (card.effectType == 1)
        {
            heroStrength += 4;
            echo = 3;
        }

        if (card.type == CardType::Attack)
        {
            int dmg = card.value + heroStrength + heroIntentBonus;
            if (heroWeaken > 0) dmg = max(0, dmg - heroWeaken);
            heroIntentBonus = 0; // одноразовый бонус сгорает

            if (enemyBlock >= dmg)
            {
                enemyBlock -= dmg;
                showMessage(L"Блок поглотил урон!");
            }
            else
            {
                dmg -= enemyBlock;
                enemyBlock = 0;
                enemyHP = max(0, enemyHP - dmg);
                showMessage(L"-" + to_wstring(dmg) + L" HP врагу");
            }
        }
        else if (card.type == CardType::Defend)
        {
            int totalBlock = card.value + heroDefenseBonus;
            heroBlock += totalBlock;
            showMessage(L"+" + to_wstring(totalBlock) + L" блока");
        }
        else if (card.type == CardType::Buff)
        {
            switch (card.effectType)
            {
            case 2: // Усиление блока на несколько ходов
                heroDefenseBonus += card.value;
                showMessage(L"Защита усилена на +" + to_wstring(card.value));
                break;
            case 3: // Мгновенный щит
                buffStandartBonus += card.value;
                heroBlock += card.value;
                showMessage(L"+" + to_wstring(card.value) + L" защитного бафа");
                break;
            case 5: // Усиление следующей атаки (одноразово)
                heroIntentBonus += card.value;
                showMessage(L"Следующая атака усилена на +" + to_wstring(card.value));
                break;
            case 6: // Адреналин: доп. ход, -10 HP
                heroHP = max(0, heroHP - 10);
                extraTurnGranted = true;
                showMessage(L"Адреналин! Доп. ход, -10 HP");
                if (heroHP <= 0) { battleOver = true; playerWon = false; }
                break;
            default: // Сила атаки на 3 хода
                heroStrength += card.value;
                echo = 3;
                showMessage(L"Сила увеличена на +" + to_wstring(card.value));
                break;
            }
        }
        else if (card.type == CardType::Debuff)
        {
            switch (card.effectType)
            {
            case 4: // Яд: урон за каждый ход врага
                poison += card.value;
                showMessage(L"Враг отравлен на " + to_wstring(card.value) + L" урона/ход");
                break;
            case 5: // Умножение яда
                poison *= card.value;
                showMessage(L"Враг отравлен на " + to_wstring(card.value) + L" урона/ход");
                break;
            case 1: // Бессилие: снижает и урон врага, и урон героя
                debuffStandart += card.value;
                heroWeaken += card.value;
                showMessage(L"Бессилие! Урон врага и ваш снижен на " + to_wstring(card.value));
                break;
            default: // Ослабление врага (только его урон)
                debuffStandart += card.value;
                showMessage(L"Враг ослаблен на " + to_wstring(card.value));
                break;
            }
        }

        // Убираем карту из руки в сброс
        discardPile.push_back(card);
        delete hand[playedCardIndex].nameText;
        delete hand[playedCardIndex].descriptionText;
        delete hand[playedCardIndex].costText;
        hand.erase(hand.begin() + playedCardIndex);

        // Проверяем победу
        if (enemyHP <= 0)
        {
            enemyHP = 0;
            battleOver = true;
            playerWon = true;
        }
    }

    mouseWasClicked = mouseDown;
}

// ─────────────────────────────────────────────────────────────────────────────
// battleBGrender — рисует только фон арены.
// Вынесено отдельно, чтобы Game мог рисовать поверх него спрайты персонажей.
// ─────────────────────────────────────────────────────────────────────────────
void Battle::battleBGrender(RenderWindow* window)
{
    window->draw(battleBG);
}

// ─────────────────────────────────────────────────────────────────────────────
// render — рисует весь UI боя: HP-бары, карты, кнопки, статусы, сообщения.
// ─────────────────────────────────────────────────────────────────────────────
void Battle::render(RenderWindow* window, bool eGame)
{
    // HP-бары
    window->draw(heroHPbarBg);  window->draw(heroHPbar);
    window->draw(enemyHPbarBg); window->draw(enemyHPbar);

    // Тексты статусов (проверяем на nullptr перед отрисовкой)
    auto drawIfNotNull = [&](Text* t) { if (t) window->draw(*t); };
    drawIfNotNull(heroHPText);     drawIfNotNull(enemyHPText);
    drawIfNotNull(heroBlockText);  drawIfNotNull(enemyBlockText);
    drawIfNotNull(heroStrengthText);
    drawIfNotNull(enemyPoisonText); drawIfNotNull(enemyWeakenText);
    drawIfNotNull(enemyIntentText); drawIfNotNull(enemyNameText);

    // Энергия и кнопка конца хода
    window->draw(energyBar);
    drawIfNotNull(energyText);
    window->draw(endTurnButton);
    drawIfNotNull(endTurnText);

    // Индикатор Оков
    if ((heroStunnedAttack || heroStunnedDefend) && !battleOver)
    {
        Text stunText(*battleFont, L"ОКОВЫ: атака и защита недоступны!", 16);
        stunText.setFillColor(Color(220, 60, 60));
        stunText.setPosition({ 250.f, 150.f });
        window->draw(stunText);
    }

    // Карты в руке
    for (auto& card : hand)
    {
        window->draw(card.body);
        drawIfNotNull(card.nameText);
        drawIfNotNull(card.descriptionText);
        drawIfNotNull(card.costText);
    }

    // Панель «Предсказуемость»
    if (showForesightPanel)
    {
        window->draw(foresightPanelBg);
        drawIfNotNull(foresightTitleText);
        window->draw(foresightOptionAttack);
        window->draw(foresightOptionDefend);
        window->draw(foresightOptionBuff);
        window->draw(foresightOptionDebuff);
        drawIfNotNull(foresightAttackText);
        drawIfNotNull(foresightDefendText);
        drawIfNotNull(foresightBuffText);
        drawIfNotNull(foresightDebuffText);
    }

    // Всплывающее сообщение (угасает по alpha)
    if (messageTtl > 0.f && messageText)
    {
        uint8_t alpha = static_cast<uint8_t>(min(255.f, messageTtl * 200.f));
        messageText->setFillColor(Color(255, 220, 50, alpha));
        window->draw(*messageText);
    }

    // Экран конца боя (победа / поражение)
    if (battleOver)
    {
        // Полупрозрачное затемнение
        RectangleShape filter({ 800.f, 600.f });
        filter.setFillColor(Color(0, 0, 0, 200));
        window->draw(filter);

        if (!playerWon)
        {
            resultStr = L"ВЫ ПОГИБЛИ";
            if (!deathSoundPlayed && deathOfHero)
            {
                deathOfHero->play();
                deathSoundPlayed = true;
            }
        }

        Text endText(*battleFont, resultStr, 36);
        if (!playerWon) endText.setFillColor(Color::Red);
        FloatRect bounds = endText.getLocalBounds();
        endText.setOrigin({ bounds.size.x / 2.f, bounds.size.y / 2.f });
        endText.setPosition({ 400.f, 300.f });
        window->draw(endText);
    }
}
