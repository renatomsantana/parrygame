using System.Collections.Generic;
using Apara.Core;
using UnityEngine;
#if ENABLE_INPUT_SYSTEM
using UnityEngine.InputSystem;
#endif

namespace Apara
{
    /// <summary>
    /// Ponte entre o núcleo puro e o Unity: monta a cena, lê o input, avança o
    /// relógio real em subpassos e liga os eventos do combate às animações,
    /// sons e HUD. Trilha de duelos de um botão: sete mestres, fintas e falas.
    /// </summary>
    public class AparaCombatBridge : MonoBehaviour
    {
        private const float Width = 640f;
        private const float Height = 360f;

        public CombatSettings Settings = new CombatSettings();
        public Campaign Campaign;
        public CombatCore Combat;
        public string Screen = "intro";
        public bool Paused;
        public bool Victory;
        public bool ShakeEnabled = true;

        private Transform world;
        private SpriteRenderer background;
        private ActorView hero;
        private ActorView boss;
        private FeedbackView fx;
        private HudView hud;
        private Texture2D heroTexture;
        private Texture2D bossTexture;
        private Texture2D sharedArena;
        private Dictionary<string, SheetData> sheets;

        private float hitstop, grace, finishAge, messageLife, shake;
        private string message = "", detail = "";
        private Color messageColor = Color.white;
        private Color flashColor = new Color(1, 1, 1, 0);
        private float flashLife, flashTotal = 1f;
        private string[] dialogue = new string[0];
        private int dialogueIndex;
        private string dialogueNext = "play";
        private double lastTime;
        private readonly HudState state = new HudState();

        /// <summary>Converte as coordenadas do desenho (Y para baixo) para o mundo (Y para cima).</summary>
        private static Vector2 V(float x, float y)
        {
            return new Vector2(x, Height - y);
        }

        private void Awake()
        {
            BuildScene();
            Campaign = new Campaign(BossRoster.Create());
            Combat = new CombatCore(Settings, Campaign.Current, null);
            Combat.OnWindupStarted += OnWindup;
            Combat.OnFeintStarted += OnFeint;
            Combat.OnAttackStarted += OnAttack;
            Combat.OnCue += OnCue;
            Combat.OnParryPressed += OnPress;
            Combat.OnImpact += OnImpact;
            Combat.OnFinished += OnFinished;
            ApplyBoss();
            lastTime = Time.realtimeSinceStartupAsDouble;
            PaintHud();
        }

        private void BuildScene()
        {
            Camera camera = Camera.main;
            if (camera == null)
            {
                camera = new GameObject("Main Camera").AddComponent<Camera>();
                camera.tag = "MainCamera";
                camera.gameObject.AddComponent<AudioListener>();
            }
            camera.orthographic = true;
            camera.orthographicSize = Height / 2f;
            camera.transform.position = new Vector3(Width / 2f, Height / 2f, -10f);
            camera.clearFlags = CameraClearFlags.SolidColor;
            camera.backgroundColor = new Color(0.031f, 0.024f, 0.086f);

            world = new GameObject("World").transform;
            world.SetParent(transform, false);

            Texture2D arena = Resources.Load<Texture2D>("Art/arena");
            sharedArena = arena;
            heroTexture = Resources.Load<Texture2D>("Art/hero_orange");
            bossTexture = Resources.Load<Texture2D>("Art/boss");
            TextAsset frames = Resources.Load<TextAsset>("Art/frames");
            sheets = SpriteSheetData.Parse(frames.text);

            GameObject backgroundObject = new GameObject("Background");
            backgroundObject.transform.SetParent(world, false);
            background = backgroundObject.AddComponent<SpriteRenderer>();
            background.sprite = Sprite.Create(arena, new Rect(0, 0, arena.width, arena.height), Vector2.zero, 1f, 0, SpriteMeshType.FullRect);
            background.sortingOrder = -10;
            backgroundObject.transform.localScale = new Vector3(Width / arena.width, Height / arena.height, 1f);

            hero = ActorView.Create("Hero", world, V(258f, 266f), new Vector2(0.43f, 0.43f), 0);
            boss = ActorView.Create("Boss", world, V(384f, 266f), new Vector2(-0.45f, 0.45f), 2);
            hero.Load(heroTexture, sheets["hero"], false);
            fx = FeedbackView.Create(world);
            hud = HudView.Create();
        }

        private void Update()
        {
            AdvanceToNow();
            ReadInput();
            PaintHud();
        }

        private void OnApplicationFocus(bool focus)
        {
            if (!focus && (Screen == "play" || Screen == "result"))
            {
                SetPaused(true);
            }
        }

        private void AdvanceToNow()
        {
            double now = Time.realtimeSinceStartupAsDouble;
            float delta = (float)(now - lastTime);
            lastTime = now;
            if (Paused || Screen == "intro" || Screen == "end")
            {
                return;
            }
            if (Screen == "dialogue")
            {
                // Durante as falas só a respiração dos dois continua.
                float slow = Mathf.Min(delta, 0.1f);
                hero.Tick(slow);
                boss.Tick(slow);
                fx.Tick(slow);
                flashLife = Mathf.Max(0f, flashLife - delta);
                return;
            }
            // Um travamento longo não pode produzir um golpe inevitável ao voltar.
            if (delta > 0.20f)
            {
                SetPaused(true);
                return;
            }
            if (grace > 0f)
            {
                grace = Mathf.Max(0f, grace - delta);
                return;
            }
            if (hitstop > 0f)
            {
                hitstop = Mathf.Max(0f, hitstop - delta);
                return;
            }
            // Subpassos mantêm o contato e a animação próximos mesmo a 30 FPS.
            float remaining = delta;
            while (remaining > 0f)
            {
                float step = Mathf.Min(remaining, 1f / 240f);
                hero.Tick(step);
                boss.Tick(step);
                fx.Tick(step);
                messageLife = Mathf.Max(0f, messageLife - step);
                flashLife = Mathf.Max(0f, flashLife - step);
                shake = Mathf.MoveTowards(shake, 0f, step * 22f);
                if (Screen == "play") Combat.Tick(step);
                else finishAge += step;
                remaining -= step;
                if (hitstop > 0f) break;
            }
            world.localPosition = Vector3.zero;
            if (ShakeEnabled && shake > 0f)
            {
                world.localPosition = new Vector3(Mathf.Round(Random.Range(-shake, shake)), Mathf.Round(Random.Range(-shake, shake)), 0f);
            }
        }

        private void ReadInput()
        {
            if (KeyDown("m")) fx.SetMuted(!fx.Muted);
            if (KeyDown("f"))
            {
                ShakeEnabled = !ShakeEnabled;
                world.localPosition = Vector3.zero;
            }
            if (KeyDown("r"))
            {
                if (Screen == "end") RestartCampaign();
                else if (Screen != "intro") StartDuel();
            }
            if (KeyDown("escape") && (Screen == "play" || Screen == "result"))
            {
                SetPaused(!Paused);
            }
            if (KeyDown("p"))
            {
                ScreenCapture.CaptureScreenshot("apara_" + System.DateTime.Now.ToString("HHmmss") + ".png");
            }
            if (!Pressed())
            {
                return;
            }
            if (Paused)
            {
                SetPaused(false);
                return;
            }
            switch (Screen)
            {
                case "intro":
                    OpenDialogue(Campaign.Current.Intro, "play");
                    break;
                case "dialogue":
                    AdvanceDialogue();
                    break;
                case "end":
                    RestartCampaign();
                    break;
                case "result":
                    if (finishAge < 0.8f) return;
                    if (Victory) OpenDialogue(Campaign.Current.Outro, "next");
                    else StartDuel();
                    break;
                case "play":
                    // O relógio já foi avançado até este frame antes de ler o input.
                    if (grace <= 0f && hitstop <= 0f) Combat.Press();
                    break;
            }
        }

        private static bool Pressed()
        {
#if ENABLE_INPUT_SYSTEM
            bool mouse = Mouse.current != null && Mouse.current.leftButton.wasPressedThisFrame;
            bool space = Keyboard.current != null && Keyboard.current.spaceKey.wasPressedThisFrame;
            return mouse || space;
#else
            return Input.GetMouseButtonDown(0) || Input.GetKeyDown(KeyCode.Space);
#endif
        }

        private static bool ButtonHeld()
        {
#if ENABLE_INPUT_SYSTEM
            bool mouse = Mouse.current != null && Mouse.current.leftButton.isPressed;
            bool space = Keyboard.current != null && Keyboard.current.spaceKey.isPressed;
            return mouse || space;
#else
            return Input.GetMouseButton(0) || Input.GetKey(KeyCode.Space);
#endif
        }

        private static bool KeyDown(string key)
        {
#if ENABLE_INPUT_SYSTEM
            if (Keyboard.current == null) return false;
            switch (key)
            {
                case "m": return Keyboard.current.mKey.wasPressedThisFrame;
                case "f": return Keyboard.current.fKey.wasPressedThisFrame;
                case "r": return Keyboard.current.rKey.wasPressedThisFrame;
                case "p": return Keyboard.current.pKey.wasPressedThisFrame;
                case "escape": return Keyboard.current.escapeKey.wasPressedThisFrame;
            }
            return false;
#else
            switch (key)
            {
                case "m": return Input.GetKeyDown(KeyCode.M);
                case "f": return Input.GetKeyDown(KeyCode.F);
                case "r": return Input.GetKeyDown(KeyCode.R);
                case "p": return Input.GetKeyDown(KeyCode.P);
                case "escape": return Input.GetKeyDown(KeyCode.Escape);
            }
            return false;
#endif
        }

        private static Color ToColor(Rgb rgb)
        {
            return new Color(rgb.R, rgb.G, rgb.B, 1f);
        }

        /// <summary>
        /// Cada mestre tem cenário e prancha próprios por nome de recurso. Enquanto a
        /// arte não existe, a arena comum recebe a tonalidade do mestre e a prancha
        /// cai no padrão ("boss", ou "hero" para a Sombra).
        /// </summary>
        private void ApplyBoss()
        {
            BossProfile profile = Campaign.Current;
            Combat.SetBoss(profile);

            SheetData sheet = sheets.ContainsKey(profile.SheetAsset) ? sheets[profile.SheetAsset] : sheets["boss"];
            string sheetImage = sheet.Image.EndsWith(".png") ? sheet.Image.Substring(0, sheet.Image.Length - 4) : sheet.Image;
            Texture2D sheetTexture = Resources.Load<Texture2D>("Art/" + sheetImage);
            if (sheetTexture == null) sheetTexture = profile.MirrorHero ? heroTexture : bossTexture;
            boss.Load(sheetTexture, sheet, profile.MirrorHero);
            boss.BaseTint = ToColor(profile.Tint);

            Texture2D arena = Resources.Load<Texture2D>("Art/Arenas/" + profile.ArenaAsset);
            bool ownArena = arena != null;
            if (!ownArena) arena = sharedArena;
            background.sprite = Sprite.Create(arena, new Rect(0, 0, arena.width, arena.height), Vector2.zero, 1f, 0, SpriteMeshType.FullRect);
            background.transform.localScale = new Vector3(Width / arena.width, Height / arena.height, 1f);
            background.color = ownArena ? Color.white : ToColor(profile.ArenaTint);

            hero.Pose("idle");
            boss.Pose("idle");
            fx.ResetAll();
        }

        private void OpenDialogue(string[] lines, string after)
        {
            dialogue = lines;
            dialogueIndex = 0;
            dialogueNext = after;
            Paused = false;
            messageLife = 0f;
            world.localPosition = Vector3.zero;
            hero.Pose("idle");
            boss.Pose("idle");
            if (dialogue.Length == 0) AfterDialogue();
            else Screen = "dialogue";
        }

        private void AdvanceDialogue()
        {
            dialogueIndex += 1;
            if (dialogueIndex >= dialogue.Length) AfterDialogue();
        }

        private void AfterDialogue()
        {
            if (dialogueNext == "play")
            {
                StartDuel();
            }
            else if (Campaign.Advance())
            {
                ApplyBoss();
                OpenDialogue(Campaign.Current.Intro, "play");
            }
            else
            {
                Screen = "end";
            }
        }

        private void RestartCampaign()
        {
            Campaign.Reset();
            ApplyBoss();
            Screen = "intro";
            Paused = false;
        }

        private void StartDuel()
        {
            Combat.Reset();
            Screen = "play";
            Paused = false;
            Victory = false;
            finishAge = 0f;
            hitstop = 0f;
            shake = 0f;
            grace = 0.20f;
            messageLife = 0f;
            flashLife = 0f;
            world.localPosition = Vector3.zero;
            hero.Pose("idle");
            boss.Pose("idle");
            fx.ResetAll();
            fx.PauseAudio(false);
            lastTime = Time.realtimeSinceStartupAsDouble;
        }

        private void SetPaused(bool value)
        {
            Paused = value;
            fx.PauseAudio(value);
            lastTime = Time.realtimeSinceStartupAsDouble;
            if (!value) grace = 0.20f;
        }

        private void SetFlash(Color color, float duration)
        {
            flashColor = color;
            flashTotal = duration;
            flashLife = duration;
        }

        private void OnWindup(float duration, bool feint)
        {
            // A preparação termina no instante mostrado; na finta esse instante mente.
            boss.Windup(Mathf.Max(0.10f, duration - Settings.AttackLead));
        }

        private void OnFeint()
        {
            BossProfile profile = Campaign.Current;
            if (profile.MimicParry && boss.Has("parry"))
            {
                // A Sombra finge o parry de Ren: a sequência de defesa para no frame de contato.
                boss.Pose("parry", 3f / (28f * Settings.AttackLead), false, 0, 3);
            }
            else
            {
                boss.Launch(Settings.AttackLead, true);
            }
            fx.Sound("swing", 1.15f);
        }

        private void OnAttack()
        {
            boss.Launch(Settings.AttackLead);
            fx.Sound("swing");
        }

        private void OnCue(bool feint)
        {
            float visibility = Campaign.Current.CueVisibility;
            fx.Sound("cue", feint ? 1.3f : 1f, visibility);
            boss.Flash(visibility);
        }

        private void OnPress()
        {
            hero.Pose("parry");
            fx.Sound("swing");
        }

        private void OnImpact(string result, float lead, bool broke)
        {
            message = "TIMING " + result.ToUpperInvariant();
            messageLife = 0.70f;
            fx.Sound(result);
            Vector2 point = V(325f, 222f);
            switch (result)
            {
                case CombatCore.ResultPerfect:
                    hitstop = Settings.PerfectHitstop;
                    shake = 2.8f;
                    messageColor = new Color32(0xFF, 0xE4, 0xA0, 0xFF);
                    detail = "−" + Settings.PerfectHealthDamage + " vida · −" + Settings.PerfectPostureDamage + " postura do mestre";
                    hero.Pose("parry", 1f, true, 3);
                    boss.Pose("hurt");
                    hero.Flash();
                    SetFlash(new Color(1f, 1f, 1f, 0.85f), 0.13f);
                    if (broke)
                    {
                        message = "POSTURA QUEBRADA";
                        detail = "Parry perfeito · −" + Settings.PostureBreakDamage + " de vida extra";
                        messageLife = 1.25f;
                        hitstop = Settings.BreakHitstop;
                        fx.Sound("break");
                        hero.Pose("attack");
                    }
                    break;
                case CombatCore.ResultGood:
                    hitstop = Settings.GoodHitstop;
                    shake = 1.2f;
                    messageColor = new Color32(0x73, 0xD2, 0xDE, 0xFF);
                    detail = "Golpe bloqueado";
                    hero.Pose("parry", 1f, true, 3);
                    boss.FinishAttack();
                    SetFlash(new Color(1f, 0.78f, 0.2f, 0.35f), 0.10f);
                    break;
                default:
                    hitstop = Settings.BadHitstop;
                    shake = 2.0f;
                    messageColor = new Color32(0xEC, 0x72, 0x85, 0xFF);
                    if (lead < 0f) detail = "Golpe recebido · −" + Settings.BadDamage + " vida";
                    else if (Combat.IsFeint) detail = "Caiu na finta · −" + Settings.BadDamage + " vida";
                    else detail = "Muito cedo · −" + Settings.BadDamage + " vida";
                    point = V(274f, 225f);
                    hero.Pose("hurt");
                    boss.FinishAttack();
                    SetFlash(new Color(1f, 0f, 0f, 0.6f), 0.30f);
                    break;
            }
            fx.Burst(point, result);
        }

        private void OnFinished(bool won)
        {
            Screen = "result";
            Victory = won;
            finishAge = 0f;
            if (won)
            {
                boss.Pose("death", 1f, false);
                fx.Sound("win");
            }
            else
            {
                hero.Pose("death", 1f, false);
            }
        }

        private void PaintHud()
        {
            BossProfile profile = Campaign.Current;
            state.Hp = Combat.PlayerHp; state.MaxHp = Settings.PlayerHealth;
            state.BossHp = Combat.BossHp; state.MaxBossHp = Settings.BossHealth;
            state.Posture = Combat.BossStability; state.MaxPosture = Settings.BossPosture;
            state.Perfects = Combat.PerfectCount; state.Goods = Combat.GoodCount; state.Bads = Combat.BadCount;
            state.Feints = Combat.Feints;
            state.Screen = Screen; state.Paused = Paused; state.Victory = Victory; state.FinishAge = finishAge;
            state.Message = message; state.Detail = detail; state.MessageLife = messageLife; state.MessageColor = messageColor;
            // O relógio da tela conta até o próximo instante mostrado, falso ou real: não entrega a finta.
            state.Lead = Combat.TimeToNextInstant(); state.PerfectWindow = profile.PerfectWindow; state.CueLead = Settings.CueLead;
            state.CueVisibility = profile.CueVisibility;
            state.BossName = profile.Name; state.BossTitle = profile.Title; state.BossColor = ToColor(profile.HudColor);
            state.Special = profile.Special; state.Stage = Campaign.Stage; state.Stages = Campaign.Total; state.Venue = profile.Venue;
            state.Premise = BossRoster.Premise; state.FinalNote = BossRoster.FinalNote;
            state.Speaker = ""; state.Line = "";
            if (Screen == "dialogue" && dialogueIndex < dialogue.Length)
            {
                string[] parts = dialogue[dialogueIndex].Split(new[] { '|' }, 2);
                state.Speaker = parts[0];
                state.Line = parts.Length > 1 ? parts[1] : parts[0];
            }
            Color flash = flashColor;
            flash.a = flashLife > 0f ? flashColor.a * (flashLife / flashTotal) : 0f;
            state.Flash = flash;
            state.ButtonDown = ButtonHeld();
            hud.Paint(state);
        }
    }
}
