using System.Collections.Generic;
using System.Globalization;
using System.Text;

namespace Apara.Core
{
    /// <summary>
    /// Leitor de JSON mínimo, sem dependências, para art/frames.json.
    /// Objetos viram Dictionary&lt;string, object&gt;, listas viram List&lt;object&gt;,
    /// números viram double, e ainda há string, bool e null.
    /// </summary>
    public static class MiniJson
    {
        public static object Parse(string text)
        {
            int index = 0;
            object value = ParseValue(text, ref index);
            SkipSpace(text, ref index);
            if (index != text.Length)
            {
                throw new System.FormatException("Conteúdo extra depois do JSON na posição " + index);
            }
            return value;
        }

        private static object ParseValue(string s, ref int i)
        {
            SkipSpace(s, ref i);
            if (i >= s.Length)
            {
                throw new System.FormatException("JSON terminou cedo");
            }
            char c = s[i];
            if (c == '{') return ParseObject(s, ref i);
            if (c == '[') return ParseArray(s, ref i);
            if (c == '"') return ParseString(s, ref i);
            if (c == 't') { Expect(s, ref i, "true"); return true; }
            if (c == 'f') { Expect(s, ref i, "false"); return false; }
            if (c == 'n') { Expect(s, ref i, "null"); return null; }
            return ParseNumber(s, ref i);
        }

        private static Dictionary<string, object> ParseObject(string s, ref int i)
        {
            Dictionary<string, object> result = new Dictionary<string, object>();
            i++; // {
            SkipSpace(s, ref i);
            if (s[i] == '}') { i++; return result; }
            while (true)
            {
                SkipSpace(s, ref i);
                string key = ParseString(s, ref i);
                SkipSpace(s, ref i);
                if (s[i] != ':') throw new System.FormatException("Esperava ':' na posição " + i);
                i++;
                result[key] = ParseValue(s, ref i);
                SkipSpace(s, ref i);
                if (s[i] == ',') { i++; continue; }
                if (s[i] == '}') { i++; return result; }
                throw new System.FormatException("Esperava ',' ou '}' na posição " + i);
            }
        }

        private static List<object> ParseArray(string s, ref int i)
        {
            List<object> result = new List<object>();
            i++; // [
            SkipSpace(s, ref i);
            if (s[i] == ']') { i++; return result; }
            while (true)
            {
                result.Add(ParseValue(s, ref i));
                SkipSpace(s, ref i);
                if (s[i] == ',') { i++; continue; }
                if (s[i] == ']') { i++; return result; }
                throw new System.FormatException("Esperava ',' ou ']' na posição " + i);
            }
        }

        private static string ParseString(string s, ref int i)
        {
            if (s[i] != '"') throw new System.FormatException("Esperava string na posição " + i);
            i++;
            StringBuilder b = new StringBuilder();
            while (i < s.Length)
            {
                char c = s[i++];
                if (c == '"') return b.ToString();
                if (c != '\\') { b.Append(c); continue; }
                char e = s[i++];
                switch (e)
                {
                    case '"': b.Append('"'); break;
                    case '\\': b.Append('\\'); break;
                    case '/': b.Append('/'); break;
                    case 'b': b.Append('\b'); break;
                    case 'f': b.Append('\f'); break;
                    case 'n': b.Append('\n'); break;
                    case 'r': b.Append('\r'); break;
                    case 't': b.Append('\t'); break;
                    case 'u':
                        b.Append((char)int.Parse(s.Substring(i, 4), NumberStyles.HexNumber, CultureInfo.InvariantCulture));
                        i += 4;
                        break;
                    default: throw new System.FormatException("Escape inválido na posição " + i);
                }
            }
            throw new System.FormatException("String sem fechamento");
        }

        private static object ParseNumber(string s, ref int i)
        {
            int start = i;
            while (i < s.Length && "+-0123456789.eE".IndexOf(s[i]) >= 0) i++;
            string token = s.Substring(start, i - start);
            double value;
            if (!double.TryParse(token, NumberStyles.Float, CultureInfo.InvariantCulture, out value))
            {
                throw new System.FormatException("Número inválido: " + token);
            }
            return value;
        }

        private static void Expect(string s, ref int i, string word)
        {
            if (string.CompareOrdinal(s, i, word, 0, word.Length) != 0)
            {
                throw new System.FormatException("Esperava " + word + " na posição " + i);
            }
            i += word.Length;
        }

        private static void SkipSpace(string s, ref int i)
        {
            while (i < s.Length && char.IsWhiteSpace(s[i])) i++;
        }
    }
}
