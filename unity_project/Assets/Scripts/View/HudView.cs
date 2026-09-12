using UnityEngine;
using UnityEngine.UI;

namespace Apara
{
    /// <summary>Tudo que o HUD precisa saber num frame. A ponte preenche, o HUD desenha.</summary>
    public class HudState
    {
        public int Hp, MaxHp, BossHp, MaxBossHp, Posture, MaxPosture;
        public int Perfects, Goods, Bads, Feints;
        public string Screen = "intro";
        public bool Paused, Victory, ButtonDown;
        public float FinishAge, MessageLife, Lead, PerfectWindow, CueLead;
        public string Message = "", Detail = "", BossName = "", BossTitle = "", Special = "";
        public string Premise = "", FinalNote = "", Speaker = "", Line = "", Venue = "";
        public Color MessageColor = Color.white, BossColor = Color.white, Flash = new Color(1, 1, 1, 0);
        public int Stage = 1, Stages = 7;
        /// <summary>Força do sinal na tela (0 a 1): Kaelen esconde parte dele.</summary>
        public float CueVisibility = 1f;
        // Tela da trilha.
        public string[] TrailNames = new string[0];
        public string[] TrailVenues = new string[0];
        public int TrailUnlocked, TrailSelected, TrailCleared;
        // Modo treino.
        public bool Training;
        public float GoodWindow;
        public string LastLead = "";
        // Gemas da empunhadura.
        public int Gems, GemCount = 7;
        public bool GemWon;
    }

    /// <summary>
    /// HUD em Canvas de 640 × 360 construído em código: barras, falas, painéis,
    /// flash de tela e resultado. As coordenadas seguem o desenho original
    /// (origem no canto superior esquerdo, Y para baixo).
    /// </summary>
    public class HudView : MonoBehaviour
    {
        private static readonly Color Ink = new Color32(0x10, 0x0D, 0x23, 0xFF);
        private static readonly Color Paper = new Color32(0xF5, 0xED, 0xDC, 0xFF);
        private static readonly Color Orange = new Color32(0xF0, 0xA0, 0x44, 0xFF);
        private static readonly Color Cyan = new Color32(0x73, 0xD2, 0xDE, 0xFF);
        private static readonly Color BarBack = new Color32(0x30, 0x2B, 0x43, 0xFF);

        private Font font;
        private RectTransform root;
        private Image flash, hpBar, bossBar, postureBar, cueDiamond, button;
        private Text hpText, bossName, bossTitle, stage, perfects, message, detail, cueText, buttonText;
        private GameObject play, panel, dialogue;
        private Text panelTitle, panelSubtitle, panelRuleA, panelRuleB, panelAction, speaker, line;
        private RectTransform panelSubtitleRect;
        private GameObject trail, training;
        private Text[] trailRows = new Text[0];
        private Text trailPremise;
        private Image goodZone, perfectZone, cursor;
        private Text leadText;
        private Text gemsText, trailGems;
        private const float BarX = 170f, BarW = 300f, BarSpan = 0.5f;

        public static HudView Create()
        {
            GameObject go = new GameObject("HUD");
            Canvas canvas = go.AddComponent<Canvas>();
            canvas.renderMode = RenderMode.ScreenSpaceOverlay;
            canvas.sortingOrder = 100;
            CanvasScaler scaler = go.AddComponent<CanvasScaler>();
            scaler.uiScaleMode = CanvasScaler.ScaleMode.ScaleWithScreenSize;
            scaler.referenceResolution = new Vector2(640f, 360f);
            scaler.screenMatchMode = CanvasScaler.ScreenMatchMode.MatchWidthOrHeight;
            scaler.matchWidthOrHeight = 0.5f;
            HudView hud = go.AddComponent<HudView>();
            hud.root = go.GetComponent<RectTransform>();
            hud.Build();
            return hud;
        }

        private void Build()
        {
            font = Resources.GetBuiltinResource<Font>("LegacyRuntime.ttf");
            if (font == null) font = Resources.GetBuiltinResource<Font>("Arial.ttf");

            flash = Box("Flash", root, 0, 0, 640, 360, new Color(1, 1, 1, 0));
            Box("Header", root, 0, 0, 640, 62, new Color(0.035f, 0.023f, 0.08f, 0.86f));
            Label("Ren", root, 20, 19, 120, "REN", Orange, 12);
            hpText = Label("Hp", root, 166, 19, 80, "", Paper, 11);
            hpBar = Bar("HpBar", root, 20, 26, 206, 7, Orange);
            bossName = Label("BossName", root, 393, 19, 160, "", Paper, 11);
            bossTitle = Label("BossTitle", root, 460, 19, 160, "", Paper, 9, TextAnchor.UpperRight);
            bossBar = Bar("BossBar", root, 393, 26, 227, 7, Paper);
            Label("PostureLabel", root, 393, 48, 60, "POSTURA", Paper, 10);
            postureBar = Bar("PostureBar", root, 452, 41, 168, 5, Cyan);
            Label("Title", root, 220, 24, 200, "APARA", Paper, 18, TextAnchor.UpperCenter);
            stage = Label("Stage", root, 170, 43, 300, "", Orange, 9, TextAnchor.UpperCenter);
            perfects = Label("Perfects", root, 20, 49, 120, "", Paper, 10);
            gemsText = Label("Gems", root, 150, 49, 120, "", Orange, 10);

            play = Group("Play", root);
            message = Label("Message", play.transform, 70, 108, 500, "", Paper, 20, TextAnchor.UpperCenter);
            detail = Label("Detail", play.transform, 70, 127, 500, "", Paper, 11, TextAnchor.UpperCenter);
            cueDiamond = Box("Cue", play.transform, 380, 154, 12, 12, Orange);
            cueDiamond.rectTransform.localRotation = Quaternion.Euler(0, 0, 45f);
            cueText = Label("CueText", play.transform, 70, 304, 500, "", Paper, 11, TextAnchor.UpperCenter);
            button = Box("Button", play.transform, 250, 315, 140, 24, Ink);
            buttonText = Label("ButtonText", play.transform, 250, 331, 140, "CLIQUE · PARRY", Paper, 12, TextAnchor.UpperCenter);
            Label("HintA", play.transform, 20, 344, 200, "ESC pausa · R reinicia", Paper, 10);
            Label("HintB", play.transform, 441, 344, 200, "M som · F tremor", Paper, 10);

            panel = Group("Panel", root);
            Box("Dim", panel.transform, 0, 62, 640, 298, new Color(0.035f, 0.023f, 0.08f, 0.62f));
            Box("Card", panel.transform, 120, 95, 400, 175, new Color(0.05f, 0.035f, 0.10f, 0.94f));
            Box("Stripe", panel.transform, 120, 95, 4, 175, Orange);
            panelTitle = Label("PanelTitle", panel.transform, 130, 126, 380, "", Orange, 21, TextAnchor.UpperCenter);
            panelSubtitle = Label("PanelSubtitle", panel.transform, 140, 138, 360, "", Paper, 11, TextAnchor.UpperLeft, true);
            panelSubtitleRect = panelSubtitle.rectTransform;
            panelRuleA = Label("RuleA", panel.transform, 130, 212, 380, "Ruim: perde vida  ·  Bom: bloqueia", Paper, 12, TextAnchor.UpperCenter);
            panelRuleB = Label("RuleB", panel.transform, 130, 231, 380, "Perfeito: tira vida e postura do mestre", Paper, 12, TextAnchor.UpperCenter);
            panelAction = Label("PanelAction", panel.transform, 130, 258, 380, "", Cyan, 11, TextAnchor.UpperCenter);

            dialogue = Group("Dialogue", root);
            Box("DialogueBox", dialogue.transform, 0, 262, 640, 98, new Color(0.035f, 0.023f, 0.08f, 0.92f));
            Box("DialogueStripe", dialogue.transform, 0, 262, 640, 2, Orange);
            speaker = Label("Speaker", dialogue.transform, 24, 284, 300, "", Orange, 12);
            line = Label("Line", dialogue.transform, 24, 296, 592, "", Paper, 11, TextAnchor.UpperLeft, true);
            Label("Prompt", dialogue.transform, 416, 350, 200, "CLIQUE PARA CONTINUAR", Cyan, 9, TextAnchor.UpperRight);

            trail = Group("Trail", root);
            Box("TrailDim", trail.transform, 0, 62, 640, 298, new Color(0.035f, 0.023f, 0.08f, 0.7f));
            Box("TrailCard", trail.transform, 60, 74, 520, 272, new Color(0.05f, 0.035f, 0.10f, 0.95f));
            Box("TrailStripe", trail.transform, 60, 74, 4, 272, Orange);
            Label("TrailTitle", trail.transform, 70, 100, 500, "A TRILHA DOS SETE MESTRES", Orange, 19, TextAnchor.UpperCenter);
            trailPremise = Label("TrailPremise", trail.transform, 80, 112, 480, "", Paper, 9, TextAnchor.UpperLeft, true);
            trailGems = Label("TrailGems", trail.transform, 70, 148, 500, "", Orange, 10, TextAnchor.UpperCenter);
            trailRows = new Text[8];
            for (int i = 0; i < trailRows.Length; i++)
            {
                trailRows[i] = Label("TrailRow" + i, trail.transform, 84, 158 + i * 20, 472, "", Paper, 11);
            }
            Label("TrailAction", trail.transform, 70, 336, 500, "← →  ESCOLHA  ·  CLIQUE OU ESPAÇO PARA COMEÇAR  ·  0 APAGA O PROGRESSO", Cyan, 9, TextAnchor.UpperCenter);

            training = Group("Training", root);
            Box("BarBack", training.transform, BarX, 280, BarW, 8, BarBack);
            goodZone = Box("GoodZone", training.transform, BarX, 280, 10, 8, new Color(0.45f, 0.82f, 0.87f, 0.9f));
            perfectZone = Box("PerfectZone", training.transform, BarX, 280, 10, 8, Paper);
            cursor = Box("Cursor", training.transform, BarX, 276, 2, 16, Orange);
            Label("BarLabel", training.transform, BarX, 275, 150, "TREINO · 500 ms", Paper, 9);
            leadText = Label("Lead", training.transform, BarX + 150, 275, 150, "", Paper, 9, TextAnchor.UpperRight);

            flash.transform.SetAsLastSibling();
        }

        public void Paint(HudState s)
        {
            flash.color = s.Flash;
            hpText.text = s.Hp + " / " + s.MaxHp;
            hpBar.fillAmount = s.Hp / (float)s.MaxHp;
            bossName.text = s.BossName.ToUpperInvariant();
            bossTitle.text = s.BossTitle;
            bossTitle.color = s.BossColor;
            bossBar.color = s.BossColor;
            bossBar.fillAmount = s.BossHp / (float)s.MaxBossHp;
            postureBar.fillAmount = s.Posture / (float)s.MaxPosture;
            stage.text = "MESTRE " + s.Stage + " / " + s.Stages + (s.Venue.Length > 0 ? " · " + s.Venue : "");
            perfects.text = "PERFEITOS  " + s.Perfects.ToString("00");
            gemsText.text = "GEMAS  " + s.Gems + " / " + s.GemCount;

            bool showPanel = false, showDialogue = false, showPlay = false, rules = false, showTrail = false;
            if (s.Screen == "intro")
            {
                showTrail = true;
                PaintTrail(s);
            }
            else if (s.Screen == "end")
            {
                showPanel = true;
                SetPanel("SETE GEMAS RECUPERADAS", s.FinalNote, "CLIQUE PARA RECOMEÇAR A TRILHA");
            }
            else if (s.Paused)
            {
                showPanel = true; rules = true;
                SetPanel("PAUSADO", "O duelo espera por você.", "CLIQUE OU ESC PARA VOLTAR");
            }
            else if (s.Screen == "dialogue")
            {
                showDialogue = true;
                speaker.text = s.Speaker.ToUpperInvariant();
                line.text = s.Line;
            }
            else if (s.Screen == "result" && s.FinishAge >= 0.65f)
            {
                showPanel = true;
                string title = s.Victory ? s.BossName.ToUpperInvariant() + " DERROTADO" : "REN DERROTADO";
                string summary = s.Perfects + " perfeitos · " + s.Goods + " bons · " + s.Bads + " ruins";
                if (s.Feints > 0) summary += " · " + s.Feints + " fintas";
                if (s.GemWon) summary += "\nGEMA RECUPERADA · " + s.Gems + " / " + s.GemCount;
                SetPanel(title, summary, s.Victory ? "CLIQUE PARA CONTINUAR" : "CLIQUE OU R PARA TENTAR DE NOVO");
            }
            else
            {
                showPlay = true;
                bool hasMessage = s.MessageLife > 0f;
                message.enabled = hasMessage;
                detail.enabled = hasMessage;
                message.text = s.Message;
                message.color = s.MessageColor;
                detail.text = s.Detail;
                bool cueOn = s.Lead >= 0f && s.Lead <= s.CueLead;
                bool perfect = cueOn && s.Lead <= s.PerfectWindow;
                Color tint = perfect ? Paper : Orange;
                tint.a = Mathf.Clamp01(s.CueVisibility);
                cueDiamond.enabled = cueOn;
                cueDiamond.color = tint;
                cueText.text = cueOn ? (perfect ? "AGORA" : "PREPARE-SE") : s.Special;
                cueText.color = cueOn ? tint : Paper;
                button.color = s.ButtonDown ? Orange : Ink;
                buttonText.color = s.ButtonDown ? Ink : Paper;
            }
            play.SetActive(showPlay);
            panel.SetActive(showPanel);
            dialogue.SetActive(showDialogue);
            trail.SetActive(showTrail);
            training.SetActive(showPlay && s.Training);
            if (showPlay && s.Training) PaintTraining(s);
            panelRuleA.enabled = rules;
            panelRuleB.enabled = rules;
            panelSubtitleRect.anchoredPosition = new Vector2(140f, rules ? -138f : -150f);
            panelSubtitle.fontSize = rules ? 11 : 12;
            flash.transform.SetAsLastSibling();
        }

        private void PaintTrail(HudState s)
        {
            trailPremise.text = s.Premise;
            string gems = "";
            for (int i = 0; i < s.GemCount; i++) gems += i < s.Gems ? "◆ " : "◇ ";
            trailGems.text = "GEMAS DA EMPUNHADURA   " + gems.TrimEnd();
            for (int i = 0; i < trailRows.Length; i++)
            {
                if (i >= s.TrailNames.Length)
                {
                    trailRows[i].text = "";
                    continue;
                }
                bool cleared = (s.TrailCleared & (1 << i)) != 0;
                bool unlocked = i <= s.TrailUnlocked;
                bool selected = i == s.TrailSelected;
                string mark = cleared ? "VENCIDO" : (unlocked ? "" : "· · ·");
                string name = unlocked ? s.TrailNames[i].ToUpperInvariant() + "  —  " + s.TrailVenues[i] : "? ? ?";
                trailRows[i].text = (selected ? "▶ " : "   ") + (i + 1) + ".  " + name + (mark.Length > 0 ? "   " + mark : "");
                trailRows[i].color = selected ? Orange : (unlocked ? Paper : new Color(0.5f, 0.47f, 0.55f));
            }
        }

        private void PaintTraining(HudState s)
        {
            float pxPerSecond = BarW / BarSpan;
            float goodW = Mathf.Min(BarW, s.GoodWindow * pxPerSecond);
            float perfectW = Mathf.Min(BarW, s.PerfectWindow * pxPerSecond);
            goodZone.rectTransform.anchoredPosition = new Vector2(BarX + BarW - goodW, -280f);
            goodZone.rectTransform.sizeDelta = new Vector2(goodW, 8f);
            perfectZone.rectTransform.anchoredPosition = new Vector2(BarX + BarW - perfectW, -280f);
            perfectZone.rectTransform.sizeDelta = new Vector2(perfectW, 8f);
            bool inRange = s.Lead >= 0f && s.Lead <= BarSpan;
            cursor.enabled = inRange;
            if (inRange) cursor.rectTransform.anchoredPosition = new Vector2(BarX + BarW - s.Lead * pxPerSecond, -276f);
            leadText.text = s.LastLead;
        }

        private void SetPanel(string title, string subtitle, string action)
        {
            panelTitle.text = title;
            panelSubtitle.text = subtitle;
            panelAction.text = action;
        }

        // Helpers de layout: (x, y) no canto superior esquerdo, Y para baixo, como o desenho original.

        private static RectTransform Place(GameObject go, Transform parent, float x, float y, float w, float h)
        {
            go.transform.SetParent(parent, false);
            RectTransform rect = go.GetComponent<RectTransform>();
            if (rect == null) rect = go.AddComponent<RectTransform>();
            rect.anchorMin = new Vector2(0f, 1f);
            rect.anchorMax = new Vector2(0f, 1f);
            rect.pivot = new Vector2(0f, 1f);
            rect.anchoredPosition = new Vector2(x, -y);
            rect.sizeDelta = new Vector2(w, h);
            return rect;
        }

        private static GameObject Group(string name, Transform parent)
        {
            GameObject go = new GameObject(name);
            Place(go, parent, 0, 0, 640, 360);
            return go;
        }

        private static Image Box(string name, Transform parent, float x, float y, float w, float h, Color color)
        {
            GameObject go = new GameObject(name);
            Image image = go.AddComponent<Image>();
            image.color = color;
            image.raycastTarget = false;
            Place(go, parent, x, y, w, h);
            return image;
        }

        private static Image Bar(string name, Transform parent, float x, float y, float w, float h, Color color)
        {
            Box(name + "Back", parent, x, y, w, h, BarBack);
            Image fill = Box(name, parent, x, y, w, h, color);
            fill.type = Image.Type.Filled;
            fill.fillMethod = Image.FillMethod.Horizontal;
            fill.fillOrigin = 0;
            fill.fillAmount = 1f;
            return fill;
        }

        private Text Label(string name, Transform parent, float x, float baseline, float w, string value, Color color, int size,
            TextAnchor anchor = TextAnchor.UpperLeft, bool wrap = false)
        {
            GameObject go = new GameObject(name);
            Text text = go.AddComponent<Text>();
            text.font = font;
            text.fontSize = size;
            text.color = color;
            text.text = value;
            text.alignment = anchor;
            text.raycastTarget = false;
            text.horizontalOverflow = wrap ? HorizontalWrapMode.Wrap : HorizontalWrapMode.Overflow;
            text.verticalOverflow = VerticalWrapMode.Overflow;
            // O desenho original posiciona pela linha de base; aqui o retângulo começa acima dela.
            Place(go, parent, x, baseline - size, w, wrap ? size * 4.2f : size * 1.4f);
            return text;
        }
    }
}
