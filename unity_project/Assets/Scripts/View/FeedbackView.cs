using System.Collections.Generic;
using UnityEngine;

namespace Apara
{
    /// <summary>
    /// Sons metálicos sintetizados na inicialização (nenhum áudio externo),
    /// faíscas em quadrados de dois pixels e o anel do parry perfeito.
    /// </summary>
    public class FeedbackView : MonoBehaviour
    {
        private const int SampleRate = 22050;
        private const int SparkPool = 64;

        public bool Muted;

        private readonly Dictionary<string, AudioSource> players = new Dictionary<string, AudioSource>();
        private readonly List<Spark> sparks = new List<Spark>();
        private readonly Stack<SpriteRenderer> pool = new Stack<SpriteRenderer>();
        private LineRenderer ring;
        private float ringLife;
        private Vector2 ringPoint;
        private Sprite pixel;

        private class Spark
        {
            public SpriteRenderer Renderer;
            public Vector2 Position;
            public Vector2 Velocity;
            public float Life;
            public Color Color;
        }

        public static FeedbackView Create(Transform parent)
        {
            GameObject go = new GameObject("Feedback");
            go.transform.SetParent(parent, false);
            FeedbackView view = go.AddComponent<FeedbackView>();
            view.Build();
            return view;
        }

        private void Build()
        {
            foreach (string kind in new[] { "swing", "cue", "bom", "perfeito", "ruim", "break", "win" })
            {
                AudioSource source = gameObject.AddComponent<AudioSource>();
                source.clip = Synthesize(kind);
                source.playOnAwake = false;
                source.volume = 0.28f;
                players[kind] = source;
            }
            Texture2D white = new Texture2D(2, 2, TextureFormat.RGBA32, false);
            white.SetPixels(new[] { Color.white, Color.white, Color.white, Color.white });
            white.Apply();
            white.filterMode = FilterMode.Point;
            pixel = Sprite.Create(white, new Rect(0, 0, 2, 2), new Vector2(0.5f, 0.5f), 1f);
            for (int i = 0; i < SparkPool; i++)
            {
                GameObject spark = new GameObject("Spark");
                spark.transform.SetParent(transform, false);
                SpriteRenderer renderer = spark.AddComponent<SpriteRenderer>();
                renderer.sprite = pixel;
                renderer.sortingOrder = 20;
                renderer.enabled = false;
                pool.Push(renderer);
            }
            GameObject ringObject = new GameObject("Ring");
            ringObject.transform.SetParent(transform, false);
            ring = ringObject.AddComponent<LineRenderer>();
            ring.useWorldSpace = false;
            ring.loop = true;
            ring.positionCount = 24;
            ring.startWidth = 1.5f;
            ring.endWidth = 1.5f;
            ring.material = new Material(Shader.Find("Sprites/Default"));
            ring.sortingOrder = 21;
            ring.enabled = false;
        }

        public void Sound(string kind, float pitch = 1f, float volume = 1f)
        {
            // O tom mais agudo marca o sinal de finta; o volume menor, o sinal
            // escondido de Kaelen. O resto toca no tom natural.
            if (Muted || !players.ContainsKey(kind))
            {
                return;
            }
            AudioSource source = players[kind];
            source.pitch = pitch;
            source.PlayOneShot(source.clip, Mathf.Clamp01(volume));
        }

        public void SetMuted(bool value)
        {
            Muted = value;
            if (Muted)
            {
                foreach (AudioSource source in players.Values) source.Stop();
            }
        }

        public void PauseAudio(bool value)
        {
            AudioListener.pause = value;
        }

        public void Burst(Vector2 point, string kind)
        {
            Color color = new Color32(0xEF, 0xB3, 0x66, 0xFF);
            int count = 14;
            if (kind == "perfeito")
            {
                color = new Color32(0xFF, 0xF2, 0xB0, 0xFF);
                count = 28;
                ringLife = 0.16f;
                ringPoint = point;
            }
            else if (kind == "ruim")
            {
                color = new Color32(0xED, 0x77, 0x84, 0xFF);
                count = 9;
            }
            for (int i = 0; i < count && pool.Count > 0; i++)
            {
                float angle = Random.Range(-Mathf.PI, Mathf.PI);
                Spark spark = new Spark
                {
                    Renderer = pool.Pop(),
                    Position = point,
                    Velocity = new Vector2(Mathf.Cos(angle), Mathf.Sin(angle)) * Random.Range(70f, 220f),
                    Life = Random.Range(0.15f, 0.35f),
                    Color = color
                };
                spark.Renderer.enabled = true;
                sparks.Add(spark);
            }
        }

        public void Tick(float delta)
        {
            ringLife = Mathf.Max(0f, ringLife - delta);
            for (int i = sparks.Count - 1; i >= 0; i--)
            {
                Spark spark = sparks[i];
                spark.Life -= delta;
                if (spark.Life <= 0f)
                {
                    spark.Renderer.enabled = false;
                    pool.Push(spark.Renderer);
                    sparks.RemoveAt(i);
                    continue;
                }
                // Gravidade para baixo: no Unity o eixo Y cresce para cima.
                spark.Velocity += new Vector2(0f, -240f) * delta;
                spark.Position += spark.Velocity * delta;
                Color color = spark.Color;
                color.a = Mathf.Min(1f, spark.Life * 8f);
                spark.Renderer.color = color;
                spark.Renderer.transform.localPosition = new Vector3(Mathf.Round(spark.Position.x), Mathf.Round(spark.Position.y), 0f);
            }
            ring.enabled = ringLife > 0f;
            if (ring.enabled)
            {
                float t = 1f - ringLife / 0.16f;
                float radius = t * 35f + 3f;
                for (int i = 0; i < ring.positionCount; i++)
                {
                    float a = i * Mathf.PI * 2f / ring.positionCount;
                    ring.SetPosition(i, new Vector3(ringPoint.x + Mathf.Cos(a) * radius, ringPoint.y + Mathf.Sin(a) * radius, 0f));
                }
                Color ringColor = new Color(1f, 0.90f, 0.64f, ringLife / 0.16f);
                ring.startColor = ringColor;
                ring.endColor = ringColor;
            }
        }

        public void ResetAll()
        {
            foreach (Spark spark in sparks)
            {
                spark.Renderer.enabled = false;
                pool.Push(spark.Renderer);
            }
            sparks.Clear();
            ringLife = 0f;
            ring.enabled = false;
            foreach (AudioSource source in players.Values) source.Stop();
        }

        private static AudioClip Synthesize(string kind)
        {
            float duration = 0.38f;
            if (kind == "perfeito" || kind == "win") duration = 0.65f;
            else if (kind == "swing" || kind == "cue") duration = 0.15f;
            int samples = (int)(SampleRate * duration);
            float[] data = new float[samples];
            System.Random noise = new System.Random(8409);
            const float tau = Mathf.PI * 2f;
            for (int i = 0; i < samples; i++)
            {
                float t = i / (float)SampleRate;
                float n = (float)(noise.NextDouble() * 2.0 - 1.0);
                float sample = 0f;
                switch (kind)
                {
                    case "bom":
                    case "perfeito":
                        sample = (Mathf.Sin(tau * 1180f * t) * 0.31f + Mathf.Sin(tau * 1867f * t) * 0.20f
                            + Mathf.Sin(tau * 2941f * t) * 0.10f) * Mathf.Exp(-t * 11f);
                        sample += n * Mathf.Exp(-t * 100f) * 0.28f;
                        sample += Mathf.Sin(tau * 95f * t) * Mathf.Exp(-t * 30f) * 0.20f;
                        if (kind == "perfeito") sample += Mathf.Sin(tau * 2380f * t) * Mathf.Exp(-t * 5.5f) * 0.23f;
                        break;
                    case "ruim":
                        sample = (Mathf.Sin(tau * 65f * t) * 0.60f + n * 0.24f) * Mathf.Exp(-t * 17f);
                        break;
                    case "swing":
                        sample = n * Mathf.Sin(Mathf.PI * t / duration) * Mathf.Exp(-t * 12f) * 0.40f;
                        break;
                    case "cue":
                        sample = (Mathf.Sin(tau * 920f * t) + Mathf.Sin(tau * 1380f * t)) * Mathf.Exp(-t * 38f) * 0.16f;
                        break;
                    case "break":
                        sample = (Mathf.Sin(tau * 140f * t) * 0.48f + n * 0.27f) * Mathf.Exp(-t * 12f);
                        break;
                    case "win":
                        sample = (Mathf.Sin(tau * 587f * t) + Mathf.Sin(tau * 880f * t)) * Mathf.Exp(-t * 5f) * 0.20f;
                        break;
                }
                sample *= Mathf.Min(t * 1000f, 1f) * Mathf.Min((duration - t) * 300f, 1f);
                data[i] = Mathf.Clamp(sample, -0.95f, 0.95f);
            }
            AudioClip clip = AudioClip.Create(kind, samples, 1, SampleRate, false);
            clip.SetData(data, 0);
            return clip;
        }
    }
}
