using System.Collections.Generic;

namespace Apara.Core
{
    /// <summary>Recorte de um frame na prancha, em pixels a partir do canto superior esquerdo.</summary>
    public struct FrameRect
    {
        public int X;
        public int Y;
        public int Width;
        public int Height;
        public int AnchorX;
        public int AnchorY;
    }

    public class AnimationData
    {
        public string Name;
        public float Fps;
        public bool Loop;
        public FrameRect[] Frames;
    }

    public class SheetData
    {
        public string Image;
        public Dictionary<string, AnimationData> Animations = new Dictionary<string, AnimationData>();

        public bool Has(string animation)
        {
            return Animations.ContainsKey(animation);
        }
    }

    /// <summary>
    /// Lê art/frames.json: {"hero": {"image": "...", "animations": {"idle": [fps, loop, [[x,y,w,h,ax,ay], ...]]}}}.
    /// </summary>
    public static class SpriteSheetData
    {
        public static Dictionary<string, SheetData> Parse(string json)
        {
            Dictionary<string, SheetData> sheets = new Dictionary<string, SheetData>();
            Dictionary<string, object> root = (Dictionary<string, object>)MiniJson.Parse(json);
            foreach (KeyValuePair<string, object> entry in root)
            {
                Dictionary<string, object> sheetNode = (Dictionary<string, object>)entry.Value;
                SheetData sheet = new SheetData();
                sheet.Image = (string)sheetNode["image"];
                Dictionary<string, object> animations = (Dictionary<string, object>)sheetNode["animations"];
                foreach (KeyValuePair<string, object> animationEntry in animations)
                {
                    List<object> spec = (List<object>)animationEntry.Value;
                    AnimationData animation = new AnimationData();
                    animation.Name = animationEntry.Key;
                    animation.Fps = (float)(double)spec[0];
                    animation.Loop = (bool)spec[1];
                    List<object> frames = (List<object>)spec[2];
                    animation.Frames = new FrameRect[frames.Count];
                    for (int i = 0; i < frames.Count; i++)
                    {
                        List<object> f = (List<object>)frames[i];
                        FrameRect rect = new FrameRect();
                        rect.X = (int)(double)f[0];
                        rect.Y = (int)(double)f[1];
                        rect.Width = (int)(double)f[2];
                        rect.Height = (int)(double)f[3];
                        // No JSON a âncora está em coordenadas da prancha; aqui fica
                        // relativa ao recorte, como o pivô do sprite precisa.
                        rect.AnchorX = (int)(double)f[4] - rect.X;
                        rect.AnchorY = (int)(double)f[5] - rect.Y;
                        animation.Frames[i] = rect;
                    }
                    sheet.Animations[animation.Name] = animation;
                }
                sheets[entry.Key] = sheet;
            }
            return sheets;
        }
    }
}
