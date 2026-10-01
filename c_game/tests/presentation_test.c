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
        "STRONG_ATTACK", "STRONG_ATTACK_FURIA", "ESPECIAL", "DASH_ATTACK", "JUMP", "HURT", "HURT_FURIA"
    };
    fixture.height = 40;
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
        SprAnim *a = &fixture.anims[fixture.count++];
        snprintf(a->name, sizeof a->name, "%s", names[i]);
        a->frames = 5; a->frameTime = 0.08f; a->hold = 1; a->contact = 2;
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
        for (int i = 0; i < 180 && G.leapStage == 0; i++) fighters_update(1.0f / 120);
        REQUIRE(G.leapStage == 1 && G.bossHidden, "Karasu não virou penas no fim do recuo");
        int darkFeathers = 0, rightFeathers = 0;
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (!G.fx.p[i].alive || G.fx.p[i].kind != P_FEATHER) continue;
            darkFeathers++;
            if (G.fx.p[i].pos.x > BOSS_X + 20) rightFeathers++;
        }
        REQUIRE(darkFeathers >= 20 && rightFeathers >= 20, "o rastro escuro não saiu à direita");
        for (int i = 0; i < 180 && G.leapStage == 1; i++) fighters_update(1.0f / 120);
        REQUIRE(G.leapStage == 2 && !G.bossHidden, "Karasu não reapareceu em pose de ataque");
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

static void real_assets(void) {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(64, 64, "verificação dos sprites");
    REQUIRE(IsWindowReady(), "contexto gráfico indisponível para verificar as artes");
    if (!IsWindowReady()) return;
    spr_init();
    preload_runtime_art();
    int cachedFx = spr_fx_cache_count();
    REQUIRE(cachedFx > 0, "efeitos não foram carregados antes da luta");
    for (int i = 0; i < ROSTER_SIZE; i++) {
        REQUIRE(spr_fx(TELL[i].fx) != NULL, "mestre sem folha de aviso instalada");
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
        for (int i = 0; i < G.bossS.set->count; i++) {
            const SprAnim *a = &G.bossS.set->anims[i]; animations++;
            REQUIRE(a->tex.id && a->frames > 0 && a->frames <= SPR_MAX_FRAMES, "folha inválida");
            SprPlayer player;
            spr_play(&player, a, 0, a->frames - 1, 0);
            for (int tick = 0; tick < 300; tick++) {
                spr_update(&player, 1.0f/120);
                REQUIRE(player.frame >= 0 && player.frame < a->frames, "quadro saiu da folha");
                if (spr_done(&player)) break;
            }
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

int main(int argc, char **argv) {
    fake_sprites();
    flaming_actions(); sword_attachment(); jump_and_frame_time(); karasu_warp_reappears_before_cue();
    damage_has_no_burst(); sword_continuity_and_parry(); visual_feedback_regressions();
    gamepad_uses_frame_fallback(); hanzo_uses_walk_when_available();
    if (argc > 1 && !strcmp(argv[1], "--assets")) real_assets();
    printf("apresentação: %d verificações, %d falhas\n", checks, failures);
    return failures ? 1 : 0;
}
