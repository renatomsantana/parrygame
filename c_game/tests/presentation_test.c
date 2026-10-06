/* As rotinas de apresentação são exercitadas sem janela, texturas ou som.
 * O core continua sendo validado separadamente em core_test. */
#define main apara_application_main
#include "../src/main.c"
#undef main

static int checks, failures;
#define REQUIRE(c, msg) do { checks++; if (!(c)) { failures++; fprintf(stderr, "%s\n", msg); } } while (0)

static SprSet fixture;

static void fake_sprites(void) {
    memset(&fixture, 0, sizeof fixture);
    static const char *names[] = {
        "IDLE", "IDLE_FURIA", "ATTACK_1", "ATTACK_2", "ATTACK_3",
        "ATTACK_1_FURIA", "ATTACK_2_FURIA", "ATTACK_3_FURIA",
        "STRONG_ATTACK", "STRONG_ATTACK_FURIA", "ESPECIAL", "DASH_ATTACK", "JUMP", "HURT", "HURT_FURIA", "DEATH", "DEFEND"
    };
    fixture.height = 40;
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
        SprAnim *a = &fixture.anims[fixture.count++];
        snprintf(a->name, sizeof a->name, "%s", names[i]);
        a->frames = 5; a->frameTime = 0.08f; a->hold = 1; a->contact = 2; a->stop = -1;
    }
}

static void flaming_actions(void) {
    memset(&G, 0, sizeof G);
    G.m = roster_get(12);
    G.bossS.set = &fixture;
    G.bossS.furia = true;
    settings_default(&G.settings);
    settings_for_level(&G.settings, 12);
    for (unsigned seed = 1; seed <= 12; seed++) {
        duel_init(&G.duel, &G.settings, G.m, seed);
        duel_start_seal(&G.duel, 2);
        RoboMente robot;
        robo_iniciar(&robot, &ROBO_DO_DEMO, seed);
        for (int tick = 0; tick < 30000 && G.duel.phase != PH_FINISHED; tick++) {
            if (G.duel.phase == PH_WINDUP) {
                for (int look = LOOK_HIGH; look <= LOOK_WARP; look++) {
                    const SprAnim *a = boss_strike_anim((MoveLook)look);
                    REQUIRE(a && strstr(a->name, "_FURIA"), "Oboro fase 3 voltou para um ataque sem fogo");
                }
                G.special = true;
                REQUIRE(strstr(boss_strike_anim(LOOK_HIGH)->name, "_FURIA"), "especial sem fogo");
                G.special = false;
            }
            bool press = robo_quer_apertar(&robot, &G.duel, ROBO_QUADRO);
            duel_step(&G.duel, ROBO_QUADRO, press);
            duel_drain(&G.duel, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
        }
    }
    G.bossS.furia = false;
    REQUIRE(!strstr(boss_strike_anim(LOOK_HIGH)->name, "_FURIA"), "fase sem fúria escolheu a espada em chamas");
}

static void sword_attachment(void) {
    memset(&G, 0, sizeof G);
    G.bossS.set = &fixture;
    SprAnim *a = &fixture.anims[0];
    a->hasWeapon[0] = a->hasWeapon[1] = true;
    a->weapon[0] = (Vector2){10, -30};
    a->weapon[1] = (Vector2){20, -10};
    G.bossS.pl.anim = a;
    G.boss.x = 200; G.boss.y = 100; G.boss.faceLeft = true;
    G.boss.offsetX = 5; G.boss.hopY = 24; G.bossS.squat = 2;
    G.vfx[0].sword = true;
    Vector2 p = vfx_position(0);
    REQUIRE(p.x == 195 && p.y == 48, "efeito fora da lâmina no salto");
    G.boss.offsetX = 32; G.boss.hopY = 5;
    p = vfx_position(0);
    REQUIRE(p.x == 222 && p.y == 67, "efeito ficou na posição anterior");
    G.bossS.pl.frame = 1;
    p = vfx_position(0);
    REQUIRE(p.x == 212 && p.y == 87, "efeito não acompanhou o quadro da espada");
    G.vfx[1].pos = (Vector2){4, 5};
    p = vfx_position(1);
    REQUIRE(p.x == 4 && p.y == 5, "poeira estacionária passou a seguir o personagem");
}

/* Os doze ecos e os aprendizes resolvem a mesma identidade de feedback. */
static void borrowed_feedback(void) {
    memset(&G, 0, sizeof G);
    settings_default(&G.settings);
    G.m = roster_get(12);
    duel_init(&G.duel, &G.settings, G.m, 11);
    duel_start_seal(&G.duel, 1);
    bool seen[MASTER_COUNT] = {0};
    for (int tick = 0; tick < 50000; tick++) {
        int echo = echo_of(duel_move(&G.duel));
        if (echo >= 0) {
            bool first = !seen[echo];
            seen[echo] = true;
            const MasterProfile *source = roster_get(echo);
            REQUIRE(feedback_master() == source, "eco escolheu som/partículas de outro mestre");
            if (first) {
                const MasterProfile *oboro = G.m;
                fx_init(&G.fx); srand(314); tell_fx();
                Fx borrowed = G.fx;
                G.m = source;
                fx_init(&G.fx); srand(314); tell_fx();
                REQUIRE(!memcmp(borrowed.p, G.fx.p, sizeof borrowed.p), "partículas do eco diferem da postura original");
                G.m = oboro;
            }
            Color a = cor_rastro(), b = posture_color(echo);
            REQUIRE(!memcmp(&a, &b, sizeof a), "aura do eco diverge da postura original");
        }
        int count = 0;
        for (int i = 0; i < MASTER_COUNT; i++) count += seen[i];
        if (count == MASTER_COUNT) break;
        if (G.duel.phase == PH_FINISHED || G.duel.seal != 1) {
            /* Não vencer antes de visitar a primeira volta inteira. */
            duel_start_seal(&G.duel, 1);
        }
        G.duel.renPosture = G.settings.renPosture;
        G.duel.bossPosture = 0;
        duel_step(&G.duel, .01f, false);
        duel_drain(&G.duel, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
    }
    for (int i = 0; i < MASTER_COUNT; i++) REQUIRE(seen[i], "um eco não foi verificado");
    for (int i = 0; i < MASTER_COUNT; i++) {
        G.m = roster_get(i);
        REQUIRE(feedback_master() == G.m, "aprendiz não usa sua própria assinatura");
    }
}

static void fixed_element_colors(void) {
    Color ice = posture_color(3), sea = posture_color(8), storm = posture_color(9);
    REQUIRE(ice.r > 180 && ice.g > 225 && ice.b > 240, "gelo perdeu a leitura clara/quase branca");
    REQUIRE(sea.g > sea.r + 80 && sea.g > sea.b, "mar deixou de ser turquesa e voltou ao azul do gelo");
    REQUIRE(storm.r > 90 && storm.r < 190 && abs(storm.b - storm.r) < 50, "tempestade perdeu o cinza de nuvem");
}

static void bought_pack_timing_and_hands(void) {
    SprFx sheet = {.name = "slash", .cell = 96, .rows = 12, .frames = 9};
    const int rows[] = {1,3,7,10}, peaks[] = {2,2,0,1}, hz[] = {30,60,144,240};
    for (int i = 0; i < 4; i++) {
        BoughtSlash cut = {.sheet = &sheet, .row = rows[i], .peak = peaks[i], .start = .9, .contact = 1, .end = 1.12};
        REQUIRE(bought_slash_frame(&cut, .899) == -1, "pack começou antes da partida visual");
        REQUIRE(bought_slash_frame(&cut, 1) == peaks[i], "quadro principal do pack fora do contato");
        REQUIRE(bought_slash_frame(&cut, 1.12) == -1, "slash ficou preso depois da cauda");
        for (int h = 0; h < 4; h++) {
            int last = -1;
            for (double clock = .9; clock < 1.12; clock += 1.0 / hz[h]) {
                int frame = bought_slash_frame(&cut, clock);
                REQUIRE(frame >= last && frame < spr_fx_row_frames(&sheet, rows[i]), "pack voltou de quadro ou mostrou coluna vazia");
                last = frame;
            }
        }
    }
    memset(&G, 0, sizeof G);
    G.m = roster_get(9); G.bossS.set = &fixture;
    SprAnim *a = &fixture.anims[0];
    a->hasWeapon[0] = a->hasOffhand[0] = true;
    a->weapon[0] = (Vector2){10,-30}; a->offhand[0] = (Vector2){4,-15};
    G.bossS.pl.anim = a;
    G.boss.x = 200; G.boss.y = 100; G.boss.faceLeft = true;
    G.boss.offsetX = 5; G.boss.hopY = 24; G.bossS.squat = 2;
    Vector2 main, other;
    REQUIRE(bought_weapon(false, &main) && bought_weapon(true, &other), "uma espada visível ficou sem o raio do pack");
    REQUIRE(main.x != other.x && main.y != other.y, "as duas espadas duplicaram a origem do VFX");
    G.cut.sheet = &sheet;
    G.cut.second = true;
    Duel before = G.duel;
    bought_slash_contact();
    REQUIRE(G.cut.hand[0] && G.cut.hand[1] && G.cut.captured, "contato não capturou as duas lâminas");
    REQUIRE(!memcmp(&before, &G.duel, sizeof before), "capturar o slash alterou o duelo");
    G.cut.second = false;
    bought_slash_contact();
    REQUIRE(G.cut.hand[0] && !G.cut.hand[1], "garra de espera ganhou slash num golpe simples");
    Vector2 stored = G.cut.local[0];
    G.boss.offsetX += 18; G.boss.hopY -= 10; G.bossS.pl.frame = 1;
    REQUIRE(G.cut.local[0].x == stored.x && G.cut.local[0].y == stored.y, "a recuperação arrastou a cauda para outra pose");
    G.m = roster_get(12); G.bossS.pl.frame = 0;
    REQUIRE(bought_weapon(false, &main) && !bought_weapon(true, &other), "Oboro ganhou uma segunda espada ao roubar o raio");
    G.m = roster_get(9); a->hasOffhand[0] = false;
    REQUIRE(!bought_weapon(true, &other), "pack inventou uma lâmina oculta a partir de outro quadro");
    vfx_clear();
    REQUIRE(!G.cut.sheet, "reset deixou o slash ativo");
}

static void jump_and_frame_time(void) {
    memset(&G, 0, sizeof G);
    boss_hop(1, 24);
    G.hopFrom = 0; G.hopTo = -40; G.hopTravel = true;
    update_hop(0.25f);
    REQUIRE(fabsf(G.boss.hopY - 18) < 1e-5f && fabsf(G.bossStep + 6.25f) < 1e-5f, "salto não segue um arco contínuo");
    update_hop(0.25f);
    REQUIRE(G.boss.hopY == 24 && G.bossStep == -20, "ápice do salto desalinhado");
    update_hop(0.5f);
    REQUIRE(G.boss.hopY == 0 && G.bossStep == -40, "salto não pousou no alcance do golpe");

    Fighter f = {.set = &fixture};
    const SprAnim *a = &fixture.anims[2], *b = &fixture.anims[3], *c = &fixture.anims[4];
    f_clear(&f, false);
    f_add(&f, a, 0, 0, 0.1f); f_add(&f, b, 0, 0, 0.1f); f_add(&f, c, 0, 0, 0.1f);
    f_update(&f, 0.25f);
    REQUIRE(f.pl.anim == c && fabsf(f.pl.t - 0.05f) < 1e-5f, "a troca de trechos perdeu tempo de animação");
}

static void karasu_warp_reappears_before_cue(void) {
    for (int k = 0; k < 3; k++) {
        const float leads[] = {0.14f, 0.22f, 0.32f};
        memset(&G, 0, sizeof G);
        fx_init(&G.fx);
        G.m = roster_get(5);
        G.bossS.set = &fixture;
        fighter_idle(&G.bossS);
        G.boss.x = G.bossHome = BOSS_X;
        G.boss.y = GROUND_LOW;
        G.boss.faceLeft = true;
        settings_default(&G.settings);
        settings_for_level(&G.settings, 5);
        duel_init(&G.duel, &G.settings, G.m, 100 + k);
        G.duel.phase = PH_WINDUP;
        G.duel.move = 2; /* sumiço */
        G.duel.windupDuration = 0.87f;
        G.duel.strikeAt = 0.87;
        G.duel.strikeLead = leads[k];
        G.windupLen = G.duel.windupDuration - leads[k];
        G.windupSpr = G.duel.windupDuration - duel_strike_lead_base(&G.duel);
        G.bossWinding = true;
        Duel before = G.duel;
        DuelTimeline timeline = duel_timeline(&G.duel);
        sprite_windup();
        REQUIRE(G.leap == LEAP_WARP, "sumiço do Karasu não começou");
        REQUIRE(G.bossStepTo > G.bossStep, "Karasu não recua para a direita");
        REQUIRE(G.leapAt < G.leapAir &&
                G.leapAir <= timeline.cue - timeline.start - KARASU_WARP_ANTES_AVISO + 0.001f,
                "Karasu reaparece depois do aviso");
        /* o corpo apaga aos poucos (sem corte seco): inteiro no começo, só aumenta a dissolução até sumir, e some por inteiro quando vira penas */
        float anterior = 0, maisCedo = 1;
        bool sobe = true;
        int passos = 0;       /* quadros (a 120 Hz) em que o corpo está entre inteiro e sumido: a passagem é gradual */
        for (int i = 0; i < 180 && G.leapStage == 0; i++) {
            fighters_update(1.0f / 120);
            if (G.leapT < G.leapAt - KARASU_WARP_DISSOLVE - 0.011f) maisCedo = fmaxf(maisCedo - 1, G.bossDissolve);    /* ainda inteiro bem antes do fim do recuo */
            if (G.bossDissolve < anterior - 1e-6f) sobe = false;
            if (G.bossDissolve > 0.001f && G.bossDissolve < 0.999f) passos++;
            anterior = G.bossDissolve;
        }
        REQUIRE(passos >= 6, "o Karasu sumiu num corte, sem passar por meio sumido");
        REQUIRE(maisCedo <= 0.0001f, "o corpo do Karasu apagou antes da hora");
        REQUIRE(sobe, "a dissolução do Karasu recuou durante o recuo");
        REQUIRE(G.leapStage == 1 && G.bossHidden, "Karasu não virou penas no fim do recuo");
        REQUIRE(G.bossDissolve >= 0.999f, "o Karasu sumiu de uma vez (cortou em vez de dissolver)");
        int darkFeathers = 0, rightFeathers = 0;
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (!G.fx.p[i].alive || G.fx.p[i].kind != P_FEATHER) continue;
            darkFeathers++;
            if (G.fx.p[i].pos.x > BOSS_X + 20) rightFeathers++;
        }
        REQUIRE(darkFeathers >= 20 && rightFeathers >= 20, "o rastro escuro não saiu à direita");
        for (int i = 0; i < 180 && G.leapStage == 1; i++) fighters_update(1.0f / 120);
        REQUIRE(G.leapStage == 2 && !G.bossHidden, "Karasu não reapareceu em pose de ataque");
        REQUIRE(G.bossDissolve >= 0.5f && G.bossDissolve < 1, "o Karasu reapareceu de uma vez (devia se formar aos poucos, e ainda estar quase todo em penas no primeiro quadro)");
        const float noAviso = (float)(timeline.cue - timeline.start);
        for (int i = 0; i < 240 && G.leapT < noAviso; i++) fighters_update(1.0f / 120);
        REQUIRE(G.bossDissolve <= 0.0001f, "o Karasu não estava inteiro no instante do aviso");
        REQUIRE(G.bossS.pl.anim == G.bossS.strike && G.bossS.pl.frame == anim_hold(G.bossS.strike),
                "Karasu reapareceu sem a espada preparada");
        REQUIRE(memcmp(&G.duel, &before, sizeof before) == 0, "core mudou durante o sumiço visual");
    }
}

static void damage_has_no_burst(void) {
    memset(&G, 0, sizeof G);
    fx_init(&G.fx);
    G.m = roster_get(9);
    G.ren.x = 124; G.ren.y = GROUND_LOW;
    DuelEvent e = {.judgement = J_RUIM};
    on_impact(&e);
    for (int i = 0; i < MAX_PARTICLES; i++)
        REQUIRE(!G.fx.p[i].alive || G.fx.p[i].kind == P_DUST, "dano voltou a criar rajada elemental");
    for (int i = 0; i < VFX_MAX; i++) REQUIRE(!G.vfx[i].fx, "dano voltou a criar folha de explosão");
    second_blade();
    for (int i = 0; i < VFX_MAX; i++) REQUIRE(!G.vfx[i].fx, "segunda lâmina criou explosão");
    REQUIRE(G.fx.arcs[0].life > 0, "dano ficou sem sinal de contato");
}

static void fatal_hit_finishes_hitstop_before_fall(void) {
    memset(&G, 0, sizeof G);
    G.m = roster_get(0);
    G.state = ST_DEFEAT;
    G.renS.set = &fixture;
    G.ren.x = REN_X; G.ren.y = GROUND_LOW;
    G.boss.x = BOSS_X; G.boss.y = GROUND_LOW;
    G.hitstop = 0.08f;
    G.slowmo = 1;
    sprite_fall();
    const SprAnim *death = fa(&G.renS, "DEATH");
    REQUIRE(death && G.renS.pl.anim == death, "golpe fatal não iniciou a animação de morte");
    update_defeat(0.05f);
    REQUIRE(fabsf(G.hitstop - 0.03f) < 1e-5f && G.renS.pl.t == 0,
            "queda avançou durante o hitstop do golpe fatal");
    update_defeat(0.05f);
    REQUIRE(G.hitstop == 0 && fabsf(G.renS.pl.t - 0.02f) < 1e-5f,
            "o tempo restante do quadro não avançou a queda após o hitstop");
    update_defeat(0.5f);
    REQUIRE(G.renS.pl.anim == death && G.renS.pl.frame == death->frames - 1 && !G.renS.autoIdle,
            "Kojiro levantou ou trocou de animação depois de morrer");

    memset(&G, 0, sizeof G);
    G.renS.set = &fixture;
    G.duel.renPosture = G.shownRen = G.ghostRen = 42;
    ren_falls();
    REQUIRE(G.duel.renPosture == 0 && G.shownRen == 0 && G.ghostRen == 42,
            "a tela de derrota manteve vida vermelha depois do golpe fatal");
}

static void parry_and_miss_play_the_right_recovery(void) {
    struct { Judgement judgement; int flags; const char *expected; } cases[] = {
        {J_PERFEITO, 0, "DEFEND"},
        {J_BOM, 0, "DEFEND"},
        {J_RUIM, 0, "HURT"},
        {J_BOM, 2, "HURT"}, /* a segunda lâmina acertou apesar da primeira defesa */
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        memset(&G, 0, sizeof G);
        G.renS.set = &fixture;
        G.bossS.set = &fixture;
        G.renS.strike = fa(&G.renS, "DEFEND");
        G.bossS.strike = fa(&G.bossS, "ATTACK_1");
        DuelEvent e = {.judgement = cases[i].judgement, .i = cases[i].flags};
        sprite_impact(&e);
        REQUIRE(G.renS.pl.anim == fa(&G.renS, cases[i].expected),
                "parry ou erro escolheu a reação errada do Kojiro");
        f_update(&G.renS, 0.6f);
        REQUIRE(G.renS.idle && G.renS.pl.anim == fa(&G.renS, "IDLE"),
                "Kojiro não voltou à guarda após parry ou dano comum");
    }
}

static void impact_frame_stays_on_contact(void) {
    Robo robots[] = {ROBO_SEM_DEFESA, ROBO_DO_DEMO, robo_deslocado(-0.15f), robo_deslocado(0.01f)};
    Judgement expected[] = {J_RUIM, J_PERFEITO, J_BOM, J_BOM};
    const int hz[] = {30, 60, 144, 240};
    for (int rate = 0; rate < 4; rate++) for (int k = 0; k < 4; k++) {
        memset(&G, 0, sizeof G);
        fx_init(&G.fx);
        G.m = roster_get(0);
        settings_default(&G.settings);
        duel_init(&G.duel, &G.settings, G.m, 17);
        G.state = ST_DUEL;
        G.slowmo = 1;
        G.autoJogo = true;
        robo_iniciar(&G.robo, &robots[k], 17);
        G.ren.x = REN_X; G.ren.y = GROUND_LOW;
        G.boss.x = G.bossHome = BOSS_X; G.boss.y = GROUND_LOW;
        G.boss.faceLeft = true;
        G.renS.set = G.bossS.set = &fixture;
        fighter_idle(&G.renS);
        fighter_idle(&G.bossS);
        for (int i = 0; i < hz[rate] * 10 && G.hitstop <= 0; i++) update_duel(1.0f / hz[rate]);
        REQUIRE(G.hitstop > 0, "duelo de teste não chegou ao primeiro contato");
        /* O robô do demo aperta no meio do quadro; a 30 Hz pode pegar bom em
         * vez de perfeito. O clique humano com carimbo é coberto por entrada_test. */
        bool judgementOk = G.duel.lastJudgement == expected[k] ||
                           (hz[rate] == 30 && k == 1 && G.duel.lastJudgement == J_BOM);
        REQUIRE(judgementOk, "robô não produziu o resultado esperado no contato");
        REQUIRE(G.bossS.pl.anim && strstr(G.bossS.pl.anim->name, "ATTACK"),
                "contato perdeu a animação do golpe");
        REQUIRE(G.bossS.pl.t == 0, "sprite avançou depois do contato, durante o primeiro quadro de hitstop");
    }
}

static void sword_continuity_and_parry(void) {
    memset(&G, 0, sizeof G);
    G.m = roster_get(9);
    settings_default(&G.settings);
    duel_init(&G.duel, &G.settings, G.m, 1);
    G.bossS.set = &fixture;
    G.renS.set = &fixture;
    G.bossS.strike = &fixture.anims[2];
    sprite_launch();
    REQUIRE(!G.bossS.fresh && G.bossS.pl.anim == &fixture.anims[2], "lançamento deixou a espada parada");
    REQUIRE(G.bossS.pl.to == 1, "antecipação não avança até o contato");
    DuelEvent e = {.judgement = J_PERFEITO};
    sprite_impact(&e);
    REQUIRE(G.bossS.qn == 1 && G.bossS.q[0].a == &fixture.anims[2], "perfeito cortou a continuação da espada");
    G.duel.comboStrike = 1; G.windupSpr = 0.3f;
    sprite_windup();
    REQUIRE(G.bossS.pl.anim == &fixture.anims[2] && G.bossS.pl.from == 3, "sequência cortou a recuperação da espada");
    fx_init(&G.fx);
    on_impact(&e);
    REQUIRE(G.fx.flash == 0, "parry perfeito voltou a clarear a tela");
    for (int i = 0; i < MAX_RINGS; i++) REQUIRE(G.fx.rings[i].life == 0, "parry perfeito voltou a criar halo");
    for (int i = 0; i < VFX_MAX; i++) REQUIRE(!G.vfx[i].fx, "parry perfeito voltou a criar brilho");
    SprAnim *a = &fixture.anims[2];
    a->hasOffhand[0] = a->hasOffhand[1] = true;
    a->offhand[0] = (Vector2){-12,-20}; a->offhand[1] = (Vector2){5,-25};
    G.bossS.pl.anim = a; G.bossS.pl.frame = 0;
    G.boss.x = 200; G.boss.y = 100; G.boss.faceLeft = true;
    G.vfx[0].offhand = true;
    Vector2 p = vfx_position(0);
    REQUIRE(p.x == 212 && p.y == 80, "raio da segunda espada usa a primeira");
    G.bossS.pl.frame = 1; G.boss.offsetX = 10; G.boss.hopY = 15;
    p = vfx_position(0);
    REQUIRE(p.x == 205 && p.y == 60, "segunda espada não acompanha salto e animação");
    G.bossS.pl.anim = &fixture.anims[3]; G.bossS.pl.frame = 0;
    p = vfx_position(0);
    REQUIRE(p.x > 100, "efeito foi para o canto da tela quando a arma ficou encoberta");
}

/* As adagas do yoru acendem no apagão: só o aço (o pixel frio e claro, ou o violeta perto de um ponto de lâmina) abaixo da altura dos olhos. */
static void yoru_blade_rule(void) {
    const Color branco = {255, 255, 255, 255}, lilas = {232, 200, 255, 255}, violeta = {176, 112, 255, 255}, pele = {246, 202, 159, 255};
    const Color amarelo = {255, 200, 37, 255}, corpo = {74, 62, 106, 255}, solto = {255, 255, 255, 120};
    REQUIRE(spr_pixel_de_lamina(branco, 10, false), "o branco do aço fica aceso em qualquer lugar do corpo");
    REQUIRE(spr_pixel_de_lamina(lilas, 10, false), "o lilás do aço fica aceso");
    REQUIRE(!spr_pixel_de_lamina(violeta, 10, false), "o violeta longe de um ponto de lâmina fica apagado (é cabelo ou bota)");
    REQUIRE(spr_pixel_de_lamina(violeta, 10, true), "o violeta perto de um ponto de lâmina acende");
    REQUIRE(!spr_pixel_de_lamina(pele, 10, true), "a pele (quente) nunca acende, nem perto da lâmina");
    REQUIRE(!spr_pixel_de_lamina(amarelo, 10, true), "a faísca amarela da defesa não é aço");
    REQUIRE(!spr_pixel_de_lamina(corpo, 10, true), "o corpo escuro perto da lâmina continua apagado");
    REQUIRE(!spr_pixel_de_lamina(solto, 10, true), "pixel transparente não acende");
    REQUIRE(!spr_pixel_de_lamina(branco, 40, true), "acima da altura dos olhos nada acende (o olho e o brilho do cabelo)");
}

/* Cada mestre deve avisar com a matéria da sua postura, na posição certa.
 * Os IDs mudaram quando o roster cresceu; isso já fez fogo sair como água e
 * vento sair como brasa sem afetar os testes do duelo. */
static void tell_particles_match_master(void) {
    static const ParticleKind expected[ROSTER_SIZE] = {
        P_DUST, P_SHARD, P_SHARD, P_SHARD, P_SPARK, P_FEATHER, P_PETAL,
        P_EMBER, P_GEM, P_SPARK, P_SHARD, P_GEM, P_DUST
    };
    const Vector2 tip = {10, 20}, mid = {30, 40}, feet = {50, 60};
    for (int id = 1; id <= ROSTER_SIZE; id++) {
        if (id == 3) continue; /* a odachi usa raizo_tell, testada à parte */
        fx_init(&G.fx);
        tell_particles(id, tip, mid, feet);
        int count = 0;
        for (int k = 0; k < MAX_PARTICLES; k++) {
            const Particle *p = &G.fx.p[k];
            if (!p->alive) continue;
            count++;
            REQUIRE(p->kind == expected[id - 1], "aviso do mestre usa partículas de outra postura");
            Vector2 at = id == 1 || id == 2 || id == 11 ? feet :
                         id == 6 || id == 7 || id == 13 ? mid : tip;
            REQUIRE(p->pos.x == at.x && p->pos.y == at.y, "aviso se soltou do chão ou da arma errada");
            if (id == 7) REQUIRE(p->color.g > p->color.r, "vento do Hayate parece brasa");
            if (id == 8) REQUIRE(p->color.r >= 250 && p->color.b < 90, "fogo do Enjin parece água");
            if (id == 9) REQUIRE(p->color.g >= 180 && p->color.g > p->color.r && p->color.g > p->color.b, "mar da Suiren perdeu o turquesa fixo");
        }
        REQUIRE(count >= 7, "aviso do mestre perdeu as partículas");
    }
}

/* Todo golpe adicionado no roster tem um fio próprio, e os cruzados usam as
 * duas armas no mesmo contato. A geometria é validada sem janela gráfica. */
static void new_move_trails(void) {
    static const struct { int id; const char *name; } novos[] = {
        {5, "arranhão"}, {5, "duas patas"}, {6, "cruz de penas"}, {6, "corte curto"},
        {7, "gancho duplo"}, {7, "ceifada em X"}, {7, "vento partido"},
        {8, "labareda larga"}, {8, "ferro em brasa"}, {8, "chicote de chamas"},
        {9, "arpão duplo"}, {9, "varredura de maré"},
        {10, "descarga"}, {10, "cruz elétrica"},
        {11, "picada"}, {11, "tesoura"}, {11, "esquerda e direita"},
        {12, "quarto crescente"}, {12, "maré de luar"},
    };
    for (int id = 1; id <= ROSTER_SIZE; id++) {
        const MasterProfile *master = roster_get(id - 1);
        const TrailStyle base = trail_style(id - 1, id, NULL);
        for (int i = 0; i < master->moveCount; i++) {
            const Move *mv = &master->moves[i];
            const TrailStyle st = trail_style(id - 1, id, mv);
            REQUIRE(st.comprimento > 0 && st.comprimento <= 32 && st.largura >= 1 && st.largura <= 4 && st.riscos >= 1 && st.riscos <= 4,
                    "fio do golpe fora da escala dos sprites");
            if (st.cruz) REQUIRE((mv->dual & 1u) != 0, "fio cruzado sem golpe de duas lâminas");
        }
        REQUIRE(base.comprimento > 0, "mestre sem estilo base");
    }
    for (size_t n = 0; n < sizeof novos / sizeof novos[0]; n++) {
        int id = novos[n].id, found = 0;
        const MasterProfile *master = roster_get(id - 1);
        TrailStyle base = trail_style(id - 1, id, NULL);
        for (int i = 0; i < master->moveCount; i++) {
            const Move *mv = &master->moves[i];
            if (strcmp(mv->name, novos[n].name)) continue;
            found++;
            TrailStyle st = trail_style(id - 1, id, mv);
            REQUIRE(st.comprimento != base.comprimento || st.largura != base.largura || st.riscos != base.riscos ||
                    st.cruz || st.gancho || st.crescente, "golpe novo ainda usa o fio genérico");
        }
        REQUIRE(found == 1, "golpe novo ausente ou repetido no roster");
    }
}

/* Com as pranchas reais do yoru: a tira da lâmina existe em toda animação de golpe, tem aço no quadro de contato, e não acende rosto nem pele. */
static void yoru_blades_on_real_sheets(void) {
    const SprSet *yoru = spr_get("yoru");
    REQUIRE(yoru != NULL, "arte do yoru não carregou");
    if (!yoru) return;
    static const char *golpes[] = {"IDLE", "ATTACK_1", "ATTACK_2", "ATTACK_3", "ESPECIAL"};
    for (size_t n = 0; n < sizeof golpes / sizeof golpes[0]; n++) {
        const SprAnim *a = spr_anim(yoru, golpes[n]);
        REQUIRE(a != NULL, "o yoru não tem a animação");
        if (!a) continue;
        const SprAnim *lamina = spr_lamina(yoru, a);
        REQUIRE(lamina && lamina->tex.id, "a tira da lâmina não foi feita");
        if (!lamina) continue;
        REQUIRE(spr_lamina(yoru, a) == lamina, "a tira da lâmina é feita uma vez só");
        Image im = LoadImageFromTexture(lamina->tex), original = LoadImage(TextFormat("assets/sprites/yoru/%s.png", a->name));
        ImageFormat(&original, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        Color *px = (Color *)im.data, *po = (Color *)original.data;
        int total = 0, contato = 0, indevidos = 0, trocados = 0;
        const int c = a->contact >= 0 ? a->contact : a->frames / 2;
        for (int y = 0; y < im.height; y++)
            for (int x = 0; x < im.width; x++) {
                Color q = px[y * im.width + x], o = po[y * original.width + x];
                if (q.a == 0) continue;
                total++;
                if (x / yoru->cw == c) contato++;
                if (q.b < q.r || yoru->ay - y > 28) indevidos++;     /* pele, ou acima da altura dos olhos */
                if (q.r != o.r || q.g != o.g || q.b != o.b || q.a != o.a) trocados++;   /* a lâmina sai na cor dela, sem tinta e sem brilho por cima */
            }
        REQUIRE(total > 0 && indevidos == 0, "a lâmina acendeu pele ou o rosto");
        REQUIRE(trocados == 0, "a lâmina não está nas cores da própria prancha");
        if (strcmp(golpes[n], "IDLE")) REQUIRE(contato >= 8, "sem aço aceso no quadro de contato do golpe");
        UnloadImage(im); UnloadImage(original);
    }
    printf("adagas do yoru: tira da lâmina conferida em %zu animações\n", sizeof golpes / sizeof golpes[0]);
}

/* O golpe chega ao contato num quadro só, com o avanço do corpo pronto na prancha: na partida o mestre desliza esse avanço e no contato está onde a prancha o põe. */
static void golpe_desliza_ate_o_contato(void) {
    SprAnim a = {0};
    a.frames = 4; a.contact = 3; a.hold = 2;         /* o quadro de contato é o 3 */
    const float corpo[4] = {0, 3, 3, 33};            /* x do corpo por quadro: o salto de 30 px é do quadro 2 para o 3 */
    for (int i = 0; i < 4; i++) { a.body[i] = corpo[i]; a.hasBody[i] = true; }
    REQUIRE(spr_salto_do_corpo(&a, 2, 3) == 30, "o salto do corpo do quadro 2 para o contato não foi medido");
    REQUIRE(spr_salto_do_corpo(&a, 2, 9) == 0 && spr_salto_do_corpo(NULL, 0, 1) == 0, "quadro sem corpo ou prancha nula deve dar 0");
    REQUIRE(deslize_do_golpe(&a, 0, false) == 0, "antes da partida o corpo não anda");
    REQUIRE(fabsf(deslize_do_golpe(&a, 1, false) - 30) < 1e-4f, "no fim da partida o corpo tem de ter andado todo o salto (o contato entra onde a prancha o põe)");
    REQUIRE(fabsf(deslize_do_golpe(&a, 1, true) + 30) < 1e-4f, "quem olha para a esquerda anda para -x");
    float antes = 0;
    for (int k = 1; k <= 20; k++) {
        const float v = deslize_do_golpe(&a, k / 20.0f, false);
        REQUIRE(v > antes, "o deslize tem de crescer sempre, sem voltar");
        REQUIRE(v - antes <= 30 * (2.0f * k / 20.0f) / 20.0f + 1e-3f, "o deslize aceleraria mais que o p ao quadrado");
        antes = v;
    }
    /* o maior passo de um quadro (a 60 Hz a partida tem uns 15 quadros) fica bem abaixo do salto de uma vez só */
    REQUIRE(deslize_do_golpe(&a, 1, false) - deslize_do_golpe(&a, 1 - 1.0f / 15, false) < 30.0f / 5, "o último passo do deslize ainda é um salto");
    a.body[3] = 3 + 4;                               /* salto de 4 px: menor que AJ_DESLIZE_MIN, fica como a prancha tem */
    REQUIRE(deslize_do_golpe(&a, 1, false) == 0, "um salto pequeno não precisa de deslize");
    a.body[3] = 3 + 90;                              /* salto enorme: o deslize respeita o máximo */
    REQUIRE(fabsf(deslize_do_golpe(&a, 1, false) - AJ_DESLIZE_MAX) < 1e-4f, "o deslize passou do máximo");
    a.body[3] = 3 - 25;                              /* o corpo recua no contato (jinshi): o deslize recua */
    REQUIRE(fabsf(deslize_do_golpe(&a, 1, false) + 25) < 1e-4f, "o recuo do corpo no contato deveria deslizar para trás");
    a.contact = 0;
    REQUIRE(deslize_do_golpe(&a, 1, false) == 0, "sem quadro antes do contato não há de onde deslizar");
}

/* Com as pranchas reais: todo golpe de todo mestre com quadro de contato tem o corpo medido nos dois quadros (o de antes e o do contato). */
static void body_measured_on_real_sheets(void) {
    int golpes = 0, grandes = 0;
    for (int master = 0; master < roster_size(); master++) {
        const SprSet *set = spr_get(roster_get(master)->name);
        REQUIRE(set != NULL, "arte do mestre não carregou");
        if (!set) continue;
        for (int i = 0; i < set->count; i++) {
            const SprAnim *a = &set->anims[i];
            if (a->contact < 1 || strncmp(a->name, "ATTACK", 6)) continue;
            golpes++;
            REQUIRE(a->hasBody[a->contact - 1] && a->hasBody[a->contact], "golpe sem o corpo medido no quadro de contato");
            const float d = spr_salto_do_corpo(a, a->contact - 1, a->contact);
            REQUIRE(fabsf(d) < (float)set->cw, "o salto do corpo saiu da célula");
            grandes += fabsf(d) >= AJ_DESLIZE_MIN;
        }
    }
    printf("deslize do golpe: %d golpes com corpo medido, %d com salto de %.0f px ou mais no contato\n", golpes, grandes, AJ_DESLIZE_MIN);
}

/* O relâmpago do arashi: raios caem em volta de quem aparou ou de kojiro, e quem leva fica meio paralisado (só desenho: o núcleo não muda). */
static void arashi_raios_e_paralisia(void) {
    const MasterProfile *arashi = roster_get(9);
    REQUIRE(arashi->id == 10, "o mestre 9 do roster não é o arashi");
    int relampago = -1, pesados = 0, outro = -1;
    for (int i = 0; i < arashi->moveCount; i++) {
        if (arashi->moves[i].look == LOOK_HEAVY) { pesados++; relampago = i; }
        else if (outro < 0) outro = i;
    }
    REQUIRE(pesados == 1 && !strcmp(arashi->moves[relampago].name, "relâmpago"), "o arashi precisa de um só golpe pesado, o relâmpago");
    static const struct { Judgement j; int i; float forca; float duracao; } CASOS[] = {
        {J_PERFEITO, 1, 0.0f, 0}, {J_BOM, 3, 0.5f, PARALISIA_METADE}, {J_RUIM, 3, 1.0f, PARALISIA_CHEIA},
    };
    for (size_t c = 0; c < sizeof CASOS / sizeof CASOS[0]; c++) {
        memset(&G, 0, sizeof G);
        fx_init(&G.fx);
        G.m = arashi;
        settings_default(&G.settings);
        duel_init(&G.duel, &G.settings, G.m, 1);
        G.duel.move = relampago;
        G.ren.x = 124; G.ren.y = GROUND_LOW;
        const Duel antes = G.duel;
        DuelEvent e = {.kind = EV_IMPACT, .judgement = CASOS[c].j, .i = CASOS[c].i};
        impacto_do_raio(&e);
        REQUIRE(memcmp(&G.duel, &antes, sizeof antes) == 0, "o relâmpago mudou o núcleo do duelo");
        const float alvo = CASOS[c].j == J_PERFEITO ? clash_point().x : G.ren.x + G.ren.offsetX;
        int ativos = 0, esperando = 0;
        for (int k = 0; k < RAIOS_MAX; k++) { ativos += G.raios[k].vida > 0; esperando += G.raios[k].espera > 0; }
        REQUIRE(ativos == 1 && esperando == 2, "o relâmpago deve soltar um raio na hora e dois logo depois");
        REQUIRE(G.raios[0].principal && G.raios[0].x == alvo, "o raio principal não caiu em quem aparou ou apanhou");
        REQUIRE(G.ctx.lightning == 1, "o relâmpago não acendeu o céu");
        REQUIRE(G.paralisiaForca == CASOS[c].forca && G.paralisia == CASOS[c].duracao,
                "a força ou a duração do choque não bate com o julgamento (perfeito nada, bom meio, erro inteiro)");
        /* o tempo passa: os raios entram, caem e somem; o choque acaba */
        for (float t = 0; t < RAIO_VIDA + 0.2f + PARALISIA_CHEIA; t += 1.0f / 60) raios_update(1.0f / 60);
        for (int k = 0; k < RAIOS_MAX; k++) REQUIRE(G.raios[k].vida <= 0 && G.raios[k].espera <= 0, "um raio ficou na tela para sempre");
        REQUIRE(G.paralisia == 0, "o choque não acabou");
    }
    /* meio paralisado: o choque só atrasa a animação de dor, e o aperto o larga */
    memset(&G, 0, sizeof G);
    SprAnim dor = {0}, guarda = {0};
    snprintf(dor.name, sizeof dor.name, "HURT");
    snprintf(guarda.name, sizeof guarda.name, "IDLE");
    G.paralisia = PARALISIA_CHEIA; G.paralisiaTotal = PARALISIA_CHEIA; G.paralisiaForca = 1;
    G.renS.pl.anim = &dor;
    REQUIRE(fabsf(paralisia_ritmo() - (1 - PARALISIA_LENTIDAO)) < 1e-6f, "com o choque cheio a queda não ficou mais lenta");
    G.paralisiaForca = 0.5f;
    REQUIRE(fabsf(paralisia_ritmo() - (1 - PARALISIA_LENTIDAO * 0.5f)) < 1e-6f, "o meio choque não atrasa pela metade");
    G.renS.pl.anim = &guarda;
    REQUIRE(paralisia_ritmo() == 1, "o choque atrasou outra animação além da dor");
    G.renS.pl.anim = &dor; G.paralisia = 0;
    REQUIRE(paralisia_ritmo() == 1, "sem choque a dor deve correr no ritmo de sempre");
    G.paralisia = PARALISIA_CHEIA; G.paralisiaForca = 1;
    sprite_press();
    REQUIRE(G.paralisia == 0, "o aperto não largou o choque");
    /* nenhum outro golpe, nem outro mestre, solta raio */
    for (int caso = 0; caso < 2; caso++) {
        memset(&G, 0, sizeof G);
        fx_init(&G.fx);
        G.m = caso ? roster_get(0) : arashi;   /* o outro: o primeiro mestre, que também tem golpe pesado */
        settings_default(&G.settings);
        duel_init(&G.duel, &G.settings, G.m, 1);
        int mv = outro;
        if (caso) for (int i = 0; i < G.m->moveCount; i++) if (G.m->moves[i].look == LOOK_HEAVY) mv = i;
        G.duel.move = mv;
        DuelEvent e = {.kind = EV_IMPACT, .judgement = J_RUIM, .i = 3};
        impacto_do_raio(&e);
        for (int k = 0; k < RAIOS_MAX; k++) REQUIRE(G.raios[k].vida <= 0 && G.raios[k].espera <= 0, "outro golpe soltou raio");
        REQUIRE(G.paralisia == 0 && G.ctx.lightning == 0, "outro golpe paralisou ou acendeu o céu");
    }
}

/* No apagão do yoru só as adagas aparecem: o aviso (do golpe simples e do duplo) só faz som, sem faísca, estrela ou folha de efeito em volta. Com as luzes acesas, solta tudo. */
static void yoru_dark_has_no_glow(void) {
    int particulas[2], estrelas[2], folhas[2];
    for (int escuro = 0; escuro < 2; escuro++) {
        memset(&G, 0, sizeof G);
        fx_init(&G.fx);
        G.m = roster_get(10);
        REQUIRE(G.m->id == 11, "o mestre 10 do roster não é o yoru");
        settings_default(&G.settings);
        duel_init(&G.duel, &G.settings, G.m, 1);
        G.duel.blackout = escuro;
        G.boss.x = 200; G.boss.y = GROUND_LOW;
        tell_fx();
        dual_tell();
        particulas[escuro] = estrelas[escuro] = folhas[escuro] = 0;
        for (int i = 0; i < MAX_PARTICLES; i++) particulas[escuro] += G.fx.p[i].alive;
        for (int i = 0; i < 4; i++) estrelas[escuro] += G.fx.stars[i].life > 0;
        for (int i = 0; i < VFX_MAX; i++) folhas[escuro] += G.vfx[i].fx != NULL;
    }
    REQUIRE(particulas[0] > 0 && estrelas[0] > 0, "com as luzes acesas o aviso do yoru deixou de soltar faísca e estrela");
    REQUIRE(particulas[1] == 0 && estrelas[1] == 0 && folhas[1] == 0, "no apagão o aviso do yoru soltou luz além das adagas");
}

static void real_assets(void) {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(256, 256, "verificação dos sprites");
    REQUIRE(IsWindowReady(), "contexto gráfico indisponível para verificar as artes");
    if (!IsWindowReady()) return;
    spr_init();
    preload_runtime_art();
    const SprFx *slashes = spr_fx("slash");
    REQUIRE(slashes && slashes->cell == 96 && slashes->frames == 9 && slashes->rows == 12,
            "pack slash ausente ou cortado como quadros de 64 px");
    if (slashes) {
        Image sheet = LoadImageFromTexture(slashes->tex);
        ImageFormat(&sheet, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        Color *pixels = sheet.data;
        for (int row = 0; row < 12; row++) {
            int frames = spr_fx_row_frames(slashes, row);
            for (int frame = 0; frame < frames; frame++) {
                bool visible = false;
                for (int y = row * 96; y < (row + 1) * 96 && !visible; y++)
                    for (int x = frame * 96; x < (frame + 1) * 96; x++)
                        if (pixels[y * sheet.width + x].a) { visible = true; break; }
                REQUIRE(visible, "slash incluiu um quadro transparente de preenchimento");
            }
        }
        UnloadImage(sheet);
    }
    if (slashes) {
        for (int lightning = 0; lightning < 2; lightning++) {
            BeginDrawing(); ClearBackground(BLACK);
            spr_fx_draw_weapon(slashes, lightning ? 1 : 3, 2, (Vector2){128,128}, false, WHITE, 1);
            rlDrawRenderBatchActive();
            Image image = LoadImageFromScreen();
            EndDrawing();
            Color *pixels = LoadImageColors(image);
            int blue = 0, orange = 0, white = 0;
            for (int i = 0; i < image.width * image.height; i++) {
                Color c = pixels[i];
                if (c.a < 128) continue;
                blue += c.b > c.r + 40 && c.b > c.g;
                orange += c.r > c.g + 40 && c.g > c.b + 40;
                white += c.r > 230 && c.g > 230 && c.b > 230;
            }
            REQUIRE(lightning ? blue > 100 && orange == 0 : orange > 100 && blue == 0,
                "shader do pack não produziu raio azul/garras laranja");
            REQUIRE(white > 5, "recoloração apagou os highlights brancos do pack");
            UnloadImageColors(pixels); UnloadImage(image);
        }
    }
    int cachedFx = spr_fx_cache_count();
    REQUIRE(cachedFx > 0, "efeitos não foram carregados antes da luta");
    for (int i = 0; i < ROSTER_SIZE; i++) {
        REQUIRE(spr_fx(runtimeFx[i % 3]) != NULL, "efeito auxiliar não foi carregado");
        REQUIRE(spr_fx_cache_count() == cachedFx, "aviso carregou textura durante a luta");
    }
    int animations = 0;
    for (int master = 0; master < roster_size(); master++) {
        memset(&G, 0, sizeof G);
        G.m = roster_get(master);
        settings_default(&G.settings);
        settings_for_level(&G.settings, master);
        duel_init(&G.duel, &G.settings, G.m, 123);
        fighter_load(&G.bossS, G.m->name);
        REQUIRE(G.bossS.set != NULL, "arte do mestre não carregou");
        if (!G.bossS.set) continue;
        if (master == 4 || master == 9 || G.m->isBigBoss) {
            Duel original = G.duel;
            /* Exercita o evento de partida, que só existe durante a preparação. */
            G.duel.phase = PH_WINDUP;
            G.duel.strikeAt = 1;
            G.duel.strikeLead = .2f;
            for (int move = 0; move < G.m->moveCount; move++) {
                G.duel.move = move;
                const MasterProfile *source = feedback_master();
                if (source->id != 5 && source->id != ARASHI_ID) continue;
                G.bossS.strike = boss_strike_anim(strike_look());
                REQUIRE(G.bossS.strike != NULL, "eco não carregou seu golpe para o efeito");
                bool claws = source->id == 5;
                REQUIRE(bought_slash_available(G.bossS.strike) == (G.packSlashes && claws), "Arashi ocultou o rastro nativo ou Garfiel perdeu suas garras");
                G.packSlashes = true;
                Duel before = G.duel;
                bought_slash_begin();
                REQUIRE(!memcmp(&before, &G.duel, sizeof before), "garras/choque alteraram julgamento ou RNG");
                if (claws) {
                    REQUIRE(G.cut.sheet == slashes && G.cut.row == 3, "Oboro não recebeu as mesmas garras laranja do Garfiel");
                    if (G.m->isBigBoss) REQUIRE(!G.cut.second, "Oboro duplicou a arma no eco");
                    spr_play(&G.bossS.pl, G.bossS.strike, G.bossS.strike->contact, G.bossS.strike->frames - 1, 0);
                    bought_slash_contact();
                    REQUIRE(G.cut.hand[0] && (G.m->isBigBoss ? !G.cut.hand[1] : true), "garras do eco não saíram da própria katana");
                } else REQUIRE(!G.cut.sheet, "Arashi/eco ainda usam o slash comprado");
            }
            G.duel = original;
        }
        duel_tick(&G.duel, 0.01);
        Duel beforeSlash = G.duel;
        (void)feedback_master();
        REQUIRE(!memcmp(&beforeSlash, &G.duel, sizeof beforeSlash), "feedback alterou relógio, regra ou RNG do núcleo");
        REQUIRE(spr_fx_cache_count() == cachedFx, "feedback carregou textura durante o golpe");
        for (int i = 0; i < G.bossS.set->count; i++) {
            const SprAnim *a = &G.bossS.set->anims[i]; animations++;
            REQUIRE(a->tex.id && a->frames > 0 && a->frames <= SPR_MAX_FRAMES, "folha inválida");
            bool layerExpected = (master == 4 || master == 9) && (strstr(a->name, "ATTACK") || !strcmp(a->name, "ESPECIAL"));
            if (G.m->isBigBoss) layerExpected = strstr(a->name, "ECO_GARFIEL") || strstr(a->name, "ECO_ARASHI") ||
                (strstr(a->name, "FURIA") && strstr(a->name, "ATTACK"));
            if (layerExpected) REQUIRE(a->cleanTex.id, "falta a camada que remove o slash antigo");
            int element = master == 3 ? 0 : master == 8 ? 1 : master == 9 ? 2 :
                strstr(a->name, "ECO_SHIZUKU") ? 0 : strstr(a->name, "ECO_SUIREN") ? 1 : strstr(a->name, "ECO_ARASHI") ? 2 : -1;
            if (element >= 0 && (strstr(a->name, "ATTACK") || !strcmp(a->name, "ESPECIAL"))) {
                const Color palettes[3][3] = {
                    {{COR_GELO_CLARA,255},{COR_GELO_MEDIA,255},{COR_GELO_ESCURA,255}},
                    {{COR_MAR_CLARA,255},{COR_MAR_MEDIA,255},{COR_MAR_ESCURA,255}},
                    {{COR_NUVEM_CLARA,255},{COR_NUVEM_MEDIA,255},{COR_NUVEM_ESCURA,255}}
                };
                Image strip = LoadImageFromTexture(a->tex);
                Color *pixels = LoadImageColors(strip);
                Color electric = {COR_RAIO_AZUL,255};
                int signature = 0, bolts = 0;
                for (int pixel = 0; pixel < strip.width * strip.height; pixel++) {
                    for (int shade = 0; shade < 3; shade++) signature += !memcmp(&pixels[pixel], &palettes[element][shade], sizeof(Color));
                    bolts += !memcmp(&pixels[pixel], &electric, sizeof(Color));
                }
                REQUIRE(signature > 8, "tira/eco não recebeu sua paleta fixa de gelo, mar ou nuvem");
                if (element == 2) REQUIRE(bolts > 0, "raios não saíram do slash cinza do Arashi/eco");
                UnloadImageColors(pixels); UnloadImage(strip);
            }
            if (a->cleanTex.id) {
                REQUIRE(a->cleanTex.width == a->tex.width && a->cleanTex.height == a->tex.height, "camada mudou tamanho/número de quadros");
                Image original = LoadImageFromTexture(a->tex), clean = LoadImageFromTexture(a->cleanTex);
                Color *old = LoadImageColors(original), *now = LoadImageColors(clean);
                int removed = 0;
                for (int pixel = 0; pixel < original.width * original.height; pixel++) {
                    if (now[pixel].a) REQUIRE(!memcmp(&old[pixel], &now[pixel], sizeof(Color)), "camada repintou corpo/arma em vez de remover só efeitos");
                    else removed += old[pixel].a > 0;
                }
                REQUIRE(removed > 0, "camada não removeu nenhum efeito antigo");
                UnloadImageColors(old); UnloadImageColors(now); UnloadImage(original); UnloadImage(clean);
            }
            SprPlayer player;
            spr_play(&player, a, 0, a->frames - 1, 0);
            for (int tick = 0; tick < 300; tick++) {
                spr_update(&player, 1.0f/120);
                REQUIRE(player.frame >= 0 && player.frame < a->frames, "quadro saiu da folha");
                if (spr_done(&player)) break;
            }
        }
        if (G.m->isBigBoss) {
            duel_start_seal(&G.duel, 1);
            bool seen[MASTER_COUNT] = {0};
            for (int tick = 0; tick < 60000; tick++) {
                int echo = echo_of(duel_move(&G.duel));
                if (echo >= 0) {
                    seen[echo] = true;
                    for (int look = LOOK_HIGH; look <= LOOK_WARP; look++) {
                        const SprAnim *a = boss_strike_anim((MoveLook)look);
                        char suffix[48];
                        snprintf(suffix, sizeof suffix, "_ECO_%s", roster_get(echo)->name);
                        for (char *u = suffix; *u; u++) if (*u >= 'a' && *u <= 'z') *u -= 32;
                        REQUIRE(a && strstr(a->name, suffix), "katana do Oboro perdeu a cor da postura roubada");
                        REQUIRE(a && !strncmp(a->name, "ATTACK_", 7), "Oboro pegou a arma de outro lutador");
                    }
                }
                int count = 0;
                for (int i = 0; i < MASTER_COUNT; i++) count += seen[i];
                if (count == MASTER_COUNT) break;
                G.duel.renPosture = G.settings.renPosture;
                G.duel.bossPosture = 0;
                duel_step(&G.duel, .01f, false);
                duel_drain(&G.duel, (DuelEvent[MAX_EVENTS]){0}, MAX_EVENTS);
            }
            for (int i = 0; i < MASTER_COUNT; i++) REQUIRE(seen[i], "variante de katana não foi verificada");
        }
        int phases = G.m->isBigBoss ? 3 : 1;
        for (int phase = 0; phase < phases; phase++) {
            duel_start_seal(&G.duel, phase);
            G.bossS.furia = phase == 2;
            for (int look = LOOK_HIGH; look <= LOOK_WARP; look++) {
                const SprAnim *a = boss_strike_anim((MoveLook)look);
                REQUIRE(a != NULL, "padrão sem animação");
                if (!a) continue;
                if (phase == 2) REQUIRE(strstr(a->name, "_FURIA") != NULL, "Oboro real atacou sem espada de fogo");
                G.bossS.strike = a;
                sprite_launch();
                for (int i = 0; i < 40; i++) f_update(&G.bossS, 1.0f/120);
                DuelEvent e = {.judgement = J_PERFEITO};
                sprite_impact(&e);
                REQUIRE(G.bossS.pl.anim == a, "impacto trocou a animação de ataque");
                if (a->contact + 1 < a->frames) REQUIRE(G.bossS.qn > 0, "golpe perdeu sua continuação");
            }
        }
    }
    const SprSet *arashi = spr_get("arashi");
    const char *attacks[] = {"ATTACK_1", "ATTACK_2", "ATTACK_3"};
    for (int i = 0; i < 3; i++) {
        const SprAnim *a = spr_anim(arashi, attacks[i]);
        REQUIRE(a != NULL, "Arashi sem folha de ataque");
        if (!a) continue;
        SprPlayer p; spr_play(&p,a,0,a->frames-1,0);
        for (int frame = 0; frame < a->frames; frame++) {
            p.frame = frame; Vector2 other;
            REQUIRE(spr_offhand_point(&p,(Vector2){200,100},true,0,&other), "Arashi sem ponto para a segunda espada");
        }
    }
    yoru_blades_on_real_sheets();
    body_measured_on_real_sheets();
    /* Percorrer os dois finais com os PNGs reais; a vitória já foi registrada. */
    for (int choice = 0; choice < 2; choice++) {
        memset(&G, 0, sizeof G);
        G.m = roster_get(12);
        G.teste = G.demo = true;
        G.cliquePeriodo = AJ_AUTO_CLIQUE_PERIODO;
        G.quadro = 1.0 / 60;
        settings_default(&G.settings);
        campaign_reset(&G.camp);
        for (int i = 0; i < ROSTER_SIZE; i++) campaign_mark_cleared(&G.camp, i);
        Campaign before = G.camp;
        fx_init(&G.fx);
        setup_actors();
        G.maskOnGround = true;
        start_scene(choice == 0 ? SCENE_SIM : SCENE_NAO);
        for (int tick = 0; tick < 12000 && G.state == ST_SCENE; tick++) {
            G.time += G.quadro;
            G.stateTime += G.quadro;
            update_scene((float)G.quadro);
        }
        REQUIRE(G.state == ST_ENDING, "um dos finais travou com as pranchas reais");
        REQUIRE(!memcmp(&before, &G.camp, sizeof before), "final repetiu o avanço da campanha");
        REQUIRE(G.bossS.pl.anim && !strcmp(G.bossS.pl.anim->name, "DEATH"), "Oboro não morreu no final");
        REQUIRE(choice == 0 ? G.hz.on && G.hz.f.set == spr_get("hanzo_mascara") : !G.hz.on,
                "Hanzo não revelou a máscara ou não saiu na perseguição");
    }
    printf("assets reais: 13 mestres, %d animações carregadas\n", animations);
    spr_shutdown(); CloseWindow();
}

static void visual_feedback_regressions(void) {
    memset(&G, 0, sizeof G);
    G.m = roster_get(9); G.bossS.set = &fixture;
    G.bossS.pl.anim = &fixture.anims[2];
    G.bossStep = 40; G.hopT = 0; G.hopLen = 1; G.hopH = 24;
    G.after[0].life = 1;
    update_after(0.03f);
    for (int i = 0; i < AFTER_MAX; i++) REQUIRE(G.after[i].life == 0, "Arashi voltou a duplicar as espadas nos rastros");
    G.m = roster_get(12); G.duel.windupDuration = 1;
    for (int echo = 0; echo < MASTER_COUNT; echo++) {
        begin_posture_aura(echo);
        REQUIRE(G.auraEcho == echo && G.auraLeft > 1, "aura não acompanha a postura imitada");
        REQUIRE(!strcmp(G.banner, roster_get(echo)->style), "postura imitada não foi anunciada");
        Color c = posture_color(echo);
        REQUIRE(c.a == 255 && (c.r || c.g || c.b), "postura sem cor de aura");
    }
}

static void gamepad_uses_frame_fallback(void) {
    memset(&G, 0, sizeof G);
    G.usaCarimbo = true;
    G.poll = 10.0;
    G.quadro = 1.0 / 60.0;
    G.carimbo = G.poll - 0.002; /* timestamp válido de outro evento no mesmo quadro */
    double t = instante_do_aperto(G.quadro, G.quadro, false);
    REQUIRE(fabs(t - G.quadro * 0.5) < 1e-12, "gamepad herdou timestamp de teclado ou mouse");
    REQUIRE(G.carimbados == 0 && G.semCarimbo == 1, "gamepad não foi contado como fallback");
}

static void menu_click_targets(void) {
    int selected = 0;
    int title_second = menu_row_at((Vector2){UI_W / 2.0f, 490}, UI_W / 2.0f - 200, 414, 400, 54, 60, 3);
    REQUIRE(title_second == 1, "segunda opção do título fora da área clicável");
    REQUIRE(menu_pick(title_second, true, false, &selected) && selected == 1,
            "clique sem movimento do mouse não selecionou a opção apontada");
    REQUIRE(!menu_pick(-1, true, false, &selected) && selected == 1,
            "clique fora do menu confirmou a opção atual");
    REQUIRE(menu_pick(-1, false, true, &selected) && selected == 1,
            "teclado ou controle perdeu a seleção atual");
    int defeat_second = menu_row_at((Vector2){UI_W / 2.0f, 444}, UI_W / 2.0f - 220, 374, 440, 52, 56, 3);
    REQUIRE(defeat_second == 1, "segunda opção de derrota fora da área clicável");
    REQUIRE(menu_row_at((Vector2){UI_W / 2.0f, 428}, UI_W / 2.0f - 220, 374, 440, 52, 56, 3) == -1,
            "espaço entre opções de derrota aceitou clique");
}

static void menu_state_routes(void) {
    const char *screens[] = {"title", "lore", "trail", "calibra"};
    for (int i = 0; i < 4; i++) {
        memset(&G, 0, sizeof G);
        int master = -1;
        bool direct = false;
        const char *state = NULL;
        char *argv[] = {"apara", "--state", (char *)screens[i]};
        parse_args(3, argv, &master, &direct, &state);
        REQUIRE(master == -1 && state && !strcmp(state, screens[i]) && G.teste,
                "--state de menu iniciou a introdução do primeiro mestre");
    }
    memset(&G, 0, sizeof G);
    int master = -1;
    bool direct = false;
    const char *state = NULL;
    char *argv[] = {"apara", "--teste"};
    parse_args(2, argv, &master, &direct, &state);
    REQUIRE(master == 0 && direct && G.teclas, "--teste deixou de começar no duelo de Daichi");
}

static void hanzo_uses_walk_when_available(void) {
    SprSet sheet = {0};
    SprAnim *run = &sheet.anims[sheet.count++];
    snprintf(run->name, sizeof run->name, "RUN");
    run->frames = 16; run->frameTime = 0.11f;
    SprAnim *walk = &sheet.anims[sheet.count++];
    snprintf(walk->name, sizeof walk->name, "WALK");
    walk->frames = 8; walk->frameTime = 0.11f;
    memset(&G, 0, sizeof G);
    G.hz.f.set = &sheet;
    hanzo_walk(100, 1.5f);
    REQUIRE(G.hz.f.pl.anim == walk && G.hz.f.pl.frame == 0 &&
            fabsf(G.hz.f.pl.dur - 8 * 0.11f * 2) < 1e-5f,
            "Hanzo não selecionou o ciclo WALK quando ele existe");
    sheet.count = 1;
    hanzo_walk(100, 1.5f);
    REQUIRE(G.hz.f.pl.anim == run && G.hz.f.pl.frame == 0 &&
            fabsf(G.hz.f.pl.dur - 16 * 0.11f * 2) < 1e-5f,
            "Hanzo perdeu o RUN atual quando WALK ainda não existe");
}

static void pupil_aftermath_preserves_progress(void) {
    for (int master = 0; master < MASTER_COUNT; master++) {
        memset(&G, 0, sizeof G);
        G.m = roster_get(master);
        campaign_reset(&G.camp);
        for (int i = 0; i <= master; i++) campaign_mark_cleared(&G.camp, i);
        Campaign before = G.camp;
        G.ren.x = REN_X; G.boss.x = BOSS_X;
        G.renS.set = G.bossS.set = &fixture;
        fx_init(&G.fx);
        start_scene(SCENE_PUPIL_AFTER);
        REQUIRE(G.beats[0].cue == CUE_LEAVE_PUPIL, "o assassino apareceu antes da saída de Kojiro");
        while (G.state == ST_SCENE && G.beatIndex == 0) update_scene(1.0f / 60);
        REQUIRE(G.ren.x + G.ren.offsetX <= -60, "Kojiro ainda visível quando o assassino entra");
        REQUIRE(G.beats[G.beatIndex].cue == CUE_ONI_AMBUSH, "saída não chegou à emboscada");
        /* Sem texturas, mas com animações falsas: o corte deve respeitar o quadro de contato. */
        /* A entrada carrega texturas no jogo real; aqui começar após essa carga. */
        G.beatTime = G.beatPrev = 1.0f / 60;
        G.hz.f.set = &fixture;
        const SprAnim *a = fa(&G.hz.f, "ATTACK_1");
        float impact = AJ_CENA_ONI_PREPARA + anim_contact(a) * a->frameTime;
        while (G.beatTime + 1.0f / 60 < impact) update_scene(1.0f / 60);
        REQUIRE(G.bossS.pl.anim != fa(&G.bossS, "DEATH"), "aprendiz morreu antes do contato");
        update_scene(1.0f / 60);
        REQUIRE(G.bossS.pl.anim == fa(&G.bossS, "DEATH"), "aprendiz não caiu no contato");
        while (G.state == ST_SCENE) update_scene(1.0f / 60);
        REQUIRE(G.state == ST_VISIT && !G.hz.on, "cena não voltou à cabana ou deixou o assassino ativo");
        REQUIRE(!memcmp(&before, &G.camp, sizeof before), "cena avançou o progresso uma segunda vez");
    }
}

int main(int argc, char **argv) {
    fake_sprites();
    pupil_aftermath_preserves_progress();
    flaming_actions(); sword_attachment(); borrowed_feedback(); fixed_element_colors(); bought_pack_timing_and_hands(); jump_and_frame_time(); karasu_warp_reappears_before_cue();
    damage_has_no_burst(); fatal_hit_finishes_hitstop_before_fall(); parry_and_miss_play_the_right_recovery(); impact_frame_stays_on_contact(); sword_continuity_and_parry(); visual_feedback_regressions();
    gamepad_uses_frame_fallback(); menu_click_targets(); menu_state_routes(); hanzo_uses_walk_when_available(); yoru_blade_rule(); yoru_dark_has_no_glow(); arashi_raios_e_paralisia(); golpe_desliza_ate_o_contato(); tell_particles_match_master(); new_move_trails();
    if (argc > 1 && !strcmp(argv[1], "--assets")) real_assets();
    printf("apresentação: %d verificações, %d falhas\n", checks, failures);
    return failures ? 1 : 0;
}
