namespace Apara.Core
{
    /// <summary>
    /// A Trilha dos Sete Mestres (especificação v0.5.0). Preparações convertidas
    /// dos quadros a 60 FPS para segundos. Cada mestre tem cenário próprio
    /// (ArenaAsset) e um estilo de finta que segue a mecânica descrita no roteiro.
    /// O Boss Final ainda será definido.
    /// </summary>
    public static class BossRoster
    {
        public const string Premise =
            "O mestre de Ren foi traído pela Liga dos Mestres Dissidentes. Cada um roubou " +
            "uma das sete gemas da empunhadura sagrada. Ren desafia cada mestre em seu próprio terreno.";

        public const string FinalNote = "O Boss Final será definido na próxima fase do projeto.";

        public static BossProfile[] Create()
        {
            BossProfile gorou = Make(1, "Gorou", "Mestre do Dojo Tradicional", "Dojo de tatame e bambu",
                "Ataques rítmicos e diretos, sem fintas.",
                0.09f, 0.22f, new float[] { 66f / 60f, 58f / 60f, 62f / 60f },
                "arena_dojo", new Rgb(1f, 1f, 1f), new Rgb(1f, 0.94f, 0.84f), new Rgb(0.925f, 0.447f, 0.522f),
                "Gorou|Vejo a lâmina do seu mestre em suas mãos, rapaz. Mas sem calma de espírito, ela é apenas ferro frio. Mostre-me se você sabe esperar a hora certa.",
                "Ren|Não vim buscar conselhos, Gorou. Vim buscar o que pertence ao templo.",
                "Gorou|Incrível... o som da lâmina desviando... Você ainda se lembra da verdadeira essência. Vá em frente.");
            gorou.FeintChance = 0f;
            gorou.FalseCues = 0;

            BossProfile jax = Make(2, "Neon Jax", "Campeão da Balada", "Rave underground, luzes estroboscópicas",
                "Ataque sincopado: o ritmo quebra com a batida; ele ameaça e atrasa a descida.",
                0.08f, 0.20f, new float[] { 54f / 60f, 48f / 60f, 66f / 60f },
                "arena_balada", new Rgb(1f, 0.8f, 1.05f), new Rgb(0.9f, 0.82f, 1.2f), new Rgb(0.95f, 0.45f, 0.9f),
                "Neon Jax|Olha só quem invadiu a pista! Desliga essa postura séria, ronin. Aqui a lâmina dança no ritmo do grave!",
                "Ren|Sua música é alta demais para esconder o som do seu medo.",
                "Neon Jax|Que batida foi essa...? O cara cortou no compasso perfeito...");
            jax.FeintChance = 0.2f;
            jax.FalseCues = 1;
            jax.FeintDelayMin = 0.22f;
            jax.FeintDelayMax = 0.35f;
            jax.RhythmJitter = 0.12f;

            BossProfile cavan = Make(3, "Cavan", "Titã do Campo", "Celeiro rústico ao entardecer",
                "Finta pesada: falsa partida lenta seguida de golpe com atraso inesperado.",
                0.075f, 0.19f, new float[] { 72f / 60f, 60f / 60f, 80f / 60f },
                "arena_campo", new Rgb(0.95f, 0.88f, 0.72f), new Rgb(1.2f, 0.92f, 0.7f), new Rgb(0.85f, 0.55f, 0.3f),
                "Cavan|Gente da cidade pensa que lutar é arte de salão. Na colheita, você aprende a cortar de verdade. Prepare-se para ser ceifado!",
                "Ren|Sua foice é pesada, Cavan. Mas quanto mais pesada a lâmina, maior a queda.",
                "Cavan|Minhas mãos... calejadas por nada... Você é firme como carvalho, garoto.");
            cavan.FeintChance = 0.3f;
            cavan.FalseCues = 1;
            cavan.FeintDelayMin = 0.35f;
            cavan.FeintDelayMax = 0.55f;

            BossProfile vance = Make(4, "Vance", "Executivo S.A.", "Cobertura corporativa",
                "Finta dupla: a lâmina falsa força o parry antecipado e pune cliques afobados.",
                0.07f, 0.18f, new float[] { 50f / 60f, 68f / 60f, 52f / 60f },
                "arena_escritorio", new Rgb(0.78f, 0.86f, 1f), new Rgb(0.78f, 0.86f, 1.08f), new Rgb(0.55f, 0.7f, 0.95f),
                "Vance|Tempo é dinheiro, Ren. Sua cruzada pessoal não tem liquidez alguma. Vamos encerrar este contrato aqui mesmo.",
                "Ren|Seus lucros acabam no fio da minha espada, Vance.",
                "Vance|Uma rescisão... violenta demais para o meu gosto...");
            vance.FeintChance = 0.4f;
            vance.FalseCues = 2;
            vance.FeintDelayMin = 0.40f;
            vance.FeintDelayMax = 0.55f;

            BossProfile kaelen = Make(5, "Kaelen", "Pintor Visionário", "Galeria surrealista, telas rasgadas",
                "Cortes cegantes: manchas de tinta distorcem a silhueta da lâmina antes da estocada.",
                0.065f, 0.17f, new float[] { 46f / 60f, 58f / 60f, 44f / 60f },
                "arena_galeria", new Rgb(1.1f, 0.85f, 1.1f), new Rgb(1.12f, 1f, 1.18f), new Rgb(0.9f, 0.5f, 0.8f),
                "Kaelen|A vida é uma tela em branco entediante. O sangue que espirrar da sua guarda será o meu tom de vermelho favorito!",
                "Ren|Sua arte não passa de decadência disfarçada. Vou quebrar a sua ilusão.",
                "Kaelen|Que composição primorosa... a faísca do seu bloqueio foi... sublime...");
            kaelen.FeintChance = 0.5f;
            kaelen.FalseCues = 1;
            kaelen.FeintDelayMin = 0.20f;
            kaelen.FeintDelayMax = 0.40f;
            kaelen.CueVisibility = 0.35f;

            BossProfile eleonor = Make(6, "Eleonor", "Grã-Duquesa Esgrimista", "Salão de baile com espelhos",
                "Finta tripla e estocadas rápidas: avanço instantâneo com atraso variável.",
                0.06f, 0.16f, new float[] { 42f / 60f, 50f / 60f, 38f / 60f },
                "arena_salao", new Rgb(1.15f, 1.1f, 1f), new Rgb(1.12f, 1.06f, 0.94f), new Rgb(0.95f, 0.85f, 0.55f),
                "Eleonor|A katana é uma relíquia bárbara e sem precisão. O florete é uma agulha que costura o destino. Dê o primeiro passo, se tiver coragem.",
                "Ren|Não importa a finura do aço quando a mão que o segura hesita.",
                "Eleonor|Como minha ponta pôde ser desviada três vezes seguidas...?! Impossível!");
            eleonor.FeintChance = 0.6f;
            eleonor.FalseCues = 3;
            eleonor.FeintDelayMin = 0.54f;
            eleonor.FeintDelayMax = 0.72f;

            BossProfile sombra = Make(7, "Sombra", "O Reflexo de Ren", "Jardim de Vidro, Altar da Alma",
                "Mímica e cancelamento: finge o próprio parry e ataca no intervalo de cooldown.",
                0.05f, 0.14f, new float[] { 36f / 60f, 44f / 60f, 34f / 60f },
                "arena_jardim", new Rgb(0.3f, 0.24f, 0.36f), new Rgb(0.62f, 0.6f, 0.72f), new Rgb(0.6f, 0.5f, 0.75f),
                "Sombra|Você derrotou todos eles e acha que está purificando o caminho? Olhe para você. O mesmo casaco, a mesma frieza. Eu sou o que resta de você quando a espada não tiver mais quem cortar.",
                "Ren|Você é apenas a dúvida que deixei para trás no tatame. Vou cortar meu próprio reflexo se for preciso.",
                "Sombra|Se você hesitar contra ele... eu retornarei...");
            sombra.FeintChance = 0.75f;
            sombra.FalseCues = 1;
            sombra.FeintDelayMin = 0.30f;
            sombra.FeintDelayMax = 0.36f;
            sombra.MimicParry = true;
            sombra.MirrorHero = true;
            sombra.SheetAsset = "hero";

            return new BossProfile[] { gorou, jax, cavan, vance, kaelen, eleonor, sombra };
        }

        private static BossProfile Make(int id, string name, string title, string venue, string special,
            float perfect, float good, float[] windups, string arena,
            Rgb tint, Rgb arenaTint, Rgb hud, string intro1, string intro2, string outro)
        {
            BossProfile p = new BossProfile();
            p.Id = id;
            p.Name = name;
            p.Title = title;
            p.Venue = venue;
            p.Special = special;
            p.PerfectWindow = perfect;
            p.GoodWindow = good;
            p.Windups = windups;
            p.ArenaAsset = arena;
            p.Tint = tint;
            p.ArenaTint = arenaTint;
            p.HudColor = hud;
            p.Intro = new string[] { intro1, intro2 };
            p.Outro = new string[] { outro };
            return p;
        }
    }
}
