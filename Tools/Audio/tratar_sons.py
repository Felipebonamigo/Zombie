"""
Trata os sons de um manifesto para Audio/<id>_<nn>.wav: mono, 48 kHz, PCM 16 bits, sem silêncio nas pontas, pico em -1 dBFS e
fade curto (o som em laço fica inteiro). Uso (skill som-livre):

    python3 -I Tools/Audio/tratar_sons.py <manifesto.json> Audio

O manifesto é uma lista JSON; cada item tem id (o som de Data/Sons.csv), arquivo (o original baixado), e opcionalmente
trecho_s (lista de [início, fim] em segundos: cada trecho vira uma variação) e reserva (true = pula). Precisa do ffmpeg.
Grava ao lado do manifesto um tratados.json com a origem de cada arquivo gerado (para docs/licencas.csv).
"""
import array, json, os, re, subprocess, sys, wave

MANIFESTO = sys.argv[1]
SAIDA = sys.argv[2]
RENOMEIA = {"zombie_gemido": "zumbi_gemido", "zombie_ataque": "zumbi_ataque", "zombie_morte": "zumbi_morte"}
# duração máxima por id [s]; o motor fica inteiro (laço)
MAXIMO = {"tiro_pistola": 1.6, "tiro_escopeta": 1.8, "tiro_fuzil": 1.6, "recarga": 2.5, "arma_vazia": 0.6, "explosao": 5.0,
          "chiado": 1.8, "zumbi_gemido": 4.0, "zumbi_ataque": 2.0, "zumbi_morte": 3.0, "gritador_grito": 3.5, "impacto_carne": 0.8,
          "despedacar": 1.0, "barricada_pancada": 0.7, "barricada_quebra": 2.5, "construir": 1.2, "desmanche": 2.0,
          "carro_batida": 2.5, "doacao": 1.6, "pauta_ok": 1.6, "pauta_nova": 1.6, "clipe": 1.0, "caixa": 1.2, "chute": 0.8}
LACO = {"carro_motor"}
MAX_VARIACOES = 6
# o .22 Magnum sai com 0,16 s (espoleta) e o segundo estalo de osso vem atrasado: menos variações nesses
VARIACOES = {"tiro_pistola": 4, "despedacar": 3}

# som de impacto começa no estalo: o ruído antes dele (fundo da gravação) passa de -30 dB do pico em alguns tiros
IMPULSIVOS = ("tiro_", "arma_vazia", "explosao", "impacto_carne", "despedacar", "barricada_", "carro_batida", "chute", "caixa")

def corta_ataque(caminho, limiar):
    """Corta o começo até 5 ms antes do som passar do limiar (fração do pico), e o fim depois do último trecho acima de -50 dB."""
    with wave.open(caminho, "rb") as w:
        taxa = w.getframerate()
        dados = array.array("h", w.readframes(w.getnframes()))
    if not dados:
        return
    pico_abs = max(abs(x) for x in dados) or 1
    inicio = next(i for i, x in enumerate(dados) if abs(x) > pico_abs * limiar)
    fim = len(dados) - next(i for i, x in enumerate(reversed(dados)) if abs(x) > pico_abs * 0.00316)
    inicio = max(0, inicio - int(taxa * (0.010 if limiar >= 0.5 else 0.005)))
    fim = min(len(dados), fim + int(taxa * 0.02))
    with wave.open(caminho, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(taxa)
        w.writeframes(dados[inicio:fim].tobytes())

def roda(args):
    r = subprocess.run(args, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError(" ".join(args) + "\n" + r.stderr[-2000:])
    return r

def duracao(caminho):
    r = roda(["ffprobe", "-v", "error", "-show_entries", "format=duration", "-of", "csv=p=0", caminho])
    return float(r.stdout.strip())

def pico(caminho):
    r = subprocess.run(["ffmpeg", "-hide_banner", "-i", caminho, "-af", "volumedetect", "-f", "null", "-"], capture_output=True, text=True)
    m = re.search(r"max_volume: (-?[0-9.]+) dB", r.stderr)
    return float(m.group(1)) if m else 0.0

def trata(entrada, saida, inicio, fim, maximo, laco, limiar):
    tmp = saida + ".tmp.wav"
    filtros = []
    if inicio is not None:
        filtros.append(f"atrim={inicio}:{fim},asetpts=PTS-STARTPTS")
    if not laco:
        filtros += ["silenceremove=start_periods=1:start_threshold=-45dB", "areverse",
                    "silenceremove=start_periods=1:start_threshold=-45dB", "areverse", f"atrim=0:{maximo},asetpts=PTS-STARTPTS"]
    cadeia = ",".join(filtros) if filtros else "anull"
    roda(["ffmpeg", "-y", "-hide_banner", "-loglevel", "error", "-i", entrada, "-af", cadeia, "-ac", "1", "-ar", "48000",
          "-c:a", "pcm_s16le", tmp])
    if not laco:
        corta_ataque(tmp, limiar)
    d = duracao(tmp)
    ganho = -1.0 - pico(tmp)
    filtros = [f"volume={ganho:.2f}dB"]
    if not laco:
        filtros += ["afade=t=in:d=0.004", f"afade=t=out:st={max(0.0, d - 0.06):.3f}:d=0.06"]
    roda(["ffmpeg", "-y", "-hide_banner", "-loglevel", "error", "-i", tmp, "-af", ",".join(filtros), "-ac", "1", "-ar", "48000",
          "-c:a", "pcm_s16le", saida])
    os.remove(tmp)
    return duracao(saida)

itens = json.load(open(MANIFESTO, encoding="utf-8"))
os.makedirs(SAIDA, exist_ok=True)
contagem = {}
registro = []
for item in itens:
    if item.get("reserva"):
        continue
    ident = RENOMEIA.get(item["id"], item["id"])
    trechos = item.get("trecho_s") or [None]
    for trecho in trechos:
        n = contagem.get(ident, 0)
        if n >= VARIACOES.get(ident, MAX_VARIACOES):
            break
        nome = f"{ident}_{n + 1:02d}"
        destino = os.path.join(SAIDA, nome + ".wav")
        inicio, fim = (trecho if trecho else (None, None))
        limiar = 0.5 if ident.startswith(IMPULSIVOS) else 0.0316
        d = trata(item["arquivo"], destino, inicio, fim, MAXIMO.get(ident, 3.0), ident in LACO, limiar)
        contagem[ident] = n + 1
        registro.append({"arquivo": nome, "id": ident, "duracao_s": round(d, 2), "origem": item})
        print(f"{nome}: {d:.2f} s  <- {os.path.basename(item['arquivo'])} {trecho or ''}")
json.dump(registro, open(os.path.join(os.path.dirname(MANIFESTO), "tratados.json"), "w", encoding="utf-8"), ensure_ascii=False, indent=1)
print("variações por id:", contagem)
