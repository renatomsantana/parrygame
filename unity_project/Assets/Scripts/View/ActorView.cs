using System.Collections.Generic;
using Apara.Core;
using UnityEngine;

namespace Apara
{
    /// <summary>
    /// Reproduz os frames da prancha com o relógio do combate: pausa e hitstop
    /// congelam. O mesmo ator serve ao herói, ao boss e à Sombra (prancha do
    /// herói espelhada). O pivô de cada sprite é o pé, então a posição do
    /// objeto é a linha do chão.
    /// </summary>
    public class ActorView : MonoBehaviour
    {
        public SpriteRenderer Renderer;
        public SpriteRenderer FlashOverlay;
        public Color BaseTint = Color.white;
        public string WindupAnimation = "windup";
        public int WindupFrames = 6;
        public string AttackAnimation = "attack";
        public int AttackStart = 0;
        public int ContactFrame = 3;
        public string Current = "idle";
        public int Frame;

        private readonly Dictionary<string, Sprite[]> sprites = new Dictionary<string, Sprite[]>();
        private SheetData sheet;
        private float animationTime;
        private float rate = 1f;
        private bool returnToIdle = true;
        private int holdFrame = -1;
        private float flashTime;
        private float hopTime, hopDuration, hopHeight;
        private Vector3 basePosition;
        private bool baseCaptured;

        public static ActorView Create(string name, Transform parent, Vector2 position, Vector2 scale, int sortingOrder)
        {
            GameObject go = new GameObject(name);
            go.transform.SetParent(parent, false);
            go.transform.localPosition = position;
            go.transform.localScale = new Vector3(scale.x, scale.y, 1f);
            ActorView view = go.AddComponent<ActorView>();
            view.Renderer = MakeRenderer("Sprite", go.transform, sortingOrder);
            view.FlashOverlay = MakeRenderer("Flash", go.transform, sortingOrder + 1);
            view.FlashOverlay.color = new Color(1f, 1f, 0.85f, 0.55f);
            view.FlashOverlay.enabled = false;
            return view;
        }

        private static SpriteRenderer MakeRenderer(string name, Transform parent, int sortingOrder)
        {
            GameObject child = new GameObject(name);
            child.transform.SetParent(parent, false);
            SpriteRenderer renderer = child.AddComponent<SpriteRenderer>();
            renderer.sortingOrder = sortingOrder;
            return renderer;
        }

        public void Load(Texture2D texture, SheetData data, bool mirrorHero)
        {
            sheet = data;
            sprites.Clear();
            foreach (AnimationData animation in data.Animations.Values)
            {
                Sprite[] list = new Sprite[animation.Frames.Length];
                for (int i = 0; i < list.Length; i++)
                {
                    FrameRect f = animation.Frames[i];
                    // O JSON conta a partir do topo; o Unity, a partir da base.
                    Rect rect = new Rect(f.X, texture.height - f.Y - f.Height, f.Width, f.Height);
                    Vector2 pivot = new Vector2(f.AnchorX / (float)f.Width, 1f - f.AnchorY / (float)f.Height);
                    list[i] = Sprite.Create(texture, rect, pivot, 1f, 0, SpriteMeshType.FullRect);
                    list[i].name = animation.Name + "_" + i;
                }
                sprites[animation.Name] = list;
            }
            if (mirrorHero)
            {
                // A prancha do herói não tem preparação própria: os dois primeiros
                // frames do corte fazem esse papel e o contato continua no quarto.
                WindupAnimation = "attack";
                WindupFrames = 2;
                AttackStart = 2;
            }
            else
            {
                WindupAnimation = "windup";
                WindupFrames = 6;
                AttackStart = 0;
            }
            Pose("idle");
        }

        public bool Has(string animation)
        {
            return sprites.ContainsKey(animation);
        }

        public void Pose(string animation, float speed = 1f, bool autoIdle = true, int firstFrame = 0, int hold = -1)
        {
            if (!Has(animation))
            {
                return;
            }
            Current = animation;
            rate = speed;
            returnToIdle = autoIdle;
            holdFrame = hold;
            animationTime = firstFrame / Mathf.Max(sheet.Animations[animation].Fps, 0.01f);
            Show(firstFrame);
        }

        /// <summary>A preparação termina junto do instante mostrado; o último frame segura.</summary>
        public void Windup(float duration)
        {
            float fps = sheet.Animations[WindupAnimation].Fps;
            Pose(WindupAnimation, WindupFrames / (fps * Mathf.Max(duration, 0.05f)), false, 0, WindupFrames - 1);
        }

        /// <summary>
        /// O frame de contato coincide com o golpe após <paramref name="duration"/> segundos.
        /// Na finta, a sequência para um frame antes e espera o corte real.
        /// </summary>
        public void Launch(float duration, bool holdBeforeContact = false)
        {
            float fps = sheet.Animations[AttackAnimation].Fps;
            float span = ContactFrame - AttackStart;
            int hold = holdBeforeContact ? ContactFrame - 1 : -1;
            Pose(AttackAnimation, span / (fps * Mathf.Max(duration, 0.05f)), false, AttackStart, hold);
        }

        public void FinishAttack()
        {
            Pose(AttackAnimation, 1f, true, ContactFrame);
        }

        /// <summary>Salto curto no lugar: a ameaça de pular do Neon Jax. Não muda a linha do chão.</summary>
        public void Hop(float height, float duration)
        {
            hopHeight = height;
            hopDuration = Mathf.Max(0.05f, duration);
            hopTime = hopDuration;
        }

        /// <summary>Clarão sobre o sprite; a força (0 a 1) segue a visibilidade do sinal do mestre.</summary>
        public void Flash(float strength = 1f)
        {
            flashTime = 0.10f;
            Color color = FlashOverlay.color;
            color.a = 0.55f * Mathf.Clamp01(strength);
            FlashOverlay.color = color;
        }

        public void Tick(float delta)
        {
            flashTime = Mathf.Max(0f, flashTime - delta);
            FlashOverlay.enabled = flashTime > 0f;
            Renderer.color = BaseTint;
            if (!baseCaptured)
            {
                basePosition = transform.localPosition;
                baseCaptured = true;
            }
            if (hopTime > 0f)
            {
                hopTime = Mathf.Max(0f, hopTime - delta);
                float t = 1f - hopTime / hopDuration;
                transform.localPosition = basePosition + new Vector3(0f, Mathf.Sin(t * Mathf.PI) * hopHeight, 0f);
            }
            else
            {
                transform.localPosition = basePosition;
            }
            AnimationData animation = sheet.Animations[Current];
            animationTime += delta * rate;
            int count = animation.Frames.Length;
            float cursor = animationTime * animation.Fps;
            int index = (int)cursor;
            if (holdFrame >= 0 && index > holdFrame)
            {
                index = holdFrame;
            }
            if (index >= count)
            {
                if (animation.Loop)
                {
                    index %= count;
                }
                else if (returnToIdle)
                {
                    Pose("idle");
                    return;
                }
                else
                {
                    index = count - 1;
                }
            }
            Show(index);
        }

        private void Show(int index)
        {
            Frame = index;
            Sprite sprite = sprites[Current][Mathf.Clamp(index, 0, sprites[Current].Length - 1)];
            Renderer.sprite = sprite;
            FlashOverlay.sprite = sprite;
        }
    }
}
