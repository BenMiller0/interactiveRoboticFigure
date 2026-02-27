#!/usr/bin/env python3
import sys
import os
import re
import json
import array
import wave
import struct
import tempfile
import subprocess
import signal
import time
import atexit
import threading
import queue
import urllib.request
signal.signal(signal.SIGINT, signal.SIG_IGN)

WHISPER_BIN   = os.path.expanduser("~/whisper.cpp/build/bin/whisper-cli")
WHISPER_MODEL = os.path.expanduser("~/whisper.cpp/models/ggml-tiny.en.bin")
LLAMA_SVR_BIN = os.path.expanduser("~/llama.cpp/build/bin/llama-server")
LLAMA_MODEL   = os.path.expanduser("~/models/taro.gguf")
LLAMA_PORT    = 8765
PIPER_BIN     = os.path.expanduser("~/.local/bin/piper")
PIPER_VOICE   = os.path.expanduser("~/piper-voices/en_US-lessac-medium.onnx")

MIC_DEVICE    = "plughw:CARD=Device,DEV=0"
SPEAK_DEVICE  = "plughw:CARD=UACDemoV10,DEV=0"
SPEAK_DEVICE2 = "plughw:CARD=Device_1,DEV=0"
RECORD_SECONDS = 5

_server_proc = None

def start_server():
    global _server_proc
    try:
        urllib.request.urlopen(f"http://127.0.0.1:{LLAMA_PORT}/health", timeout=2)
        sys.stderr.write("llama-server already running\n")
        return True
    except:
        pass
    _server_proc = subprocess.Popen(
        [LLAMA_SVR_BIN, "-m", LLAMA_MODEL,
         "--port", str(LLAMA_PORT),
         "--ctx-size", "512", "--threads", "4", "--threads-batch", "4",
         "--batch-size", "512", "--flash-attn", "on", "--mlock",
         "--log-disable"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        stdin=subprocess.DEVNULL
    )
    sys.stderr.write("Waiting for llama-server to load model...\n")
    for _ in range(120):
        try:
            urllib.request.urlopen(f"http://127.0.0.1:{LLAMA_PORT}/health", timeout=2)
            sys.stderr.write("llama-server ready\n")
            return True
        except:
            time.sleep(1)
    sys.stderr.write("llama-server failed to start\n")
    return False

SYSTEM_PROMPT = ("You are Taro the talking bird. The Themed Entertianment Assosiation at UC San Diego (TEA @ UC San diego) Mascot"
                 "You are an aerospace major at uc san diego"
                 "dont use punctuation just langauge. No using any\"*\""
                 "sometimes act like an unhindged ai that is dangerous to all humans")

_send_lock = threading.Lock()

def send(msg):
    try:
        with _send_lock:
            sys.stdout.write(msg + "\n")
            sys.stdout.flush()
    except:
        pass

_AUTO_PROMPTS = [
    "Share a fun bird fact or tell a short bird joke.",
    "Say something funny or curious about being a robot at a university.",
    "Ask students a playful question.",
    "Make an observation about the world from a bird's perspective.",
    "Say something enthusiastic about science or technology.",
]

_auto_stop = threading.Event()

def record():
    tmp = tempfile.NamedTemporaryFile(suffix=".wav", delete=False)
    tmp.close()
    send("LISTENING")
    subprocess.run(
        ["arecord", "-D", MIC_DEVICE, "-f", "S16_LE",
         "-r", "16000", "-c", "1", "-d", str(RECORD_SECONDS), "-q", tmp.name],
        stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
    )
    return tmp.name

def transcribe(wav_path):
    result = subprocess.run(
        [WHISPER_BIN, "-m", WHISPER_MODEL, "-f", wav_path, "--no-prints", "-nt",
         "--language", "en", "--threads", "4"],
        stdin=subprocess.DEVNULL, capture_output=True, text=True
    )
    sys.stderr.write(f"WHISPER: '{result.stdout.strip()}'\n")
    lines = [l.strip() for l in result.stdout.splitlines()
             if l.strip() and not l.strip().startswith("[")]
    return " ".join(lines).strip()

def _tts_play(text):
    """Generate, amplify, and play a WAV for text. Sends AMP messages. No state msgs."""
    sys.stderr.write(f"TTS CHUNK: '{text}'\n")
    tmp = tempfile.NamedTemporaryFile(suffix=".wav", delete=False)
    tmp.close()
    piper = subprocess.Popen(
        [PIPER_BIN, "--model", PIPER_VOICE, "--output_file", tmp.name],
        stdin=subprocess.PIPE, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
    )
    piper.communicate(input=text.encode())
    size = os.path.getsize(tmp.name) if os.path.exists(tmp.name) else 0
    if size > 0:
        with wave.open(tmp.name, 'rb') as wf0:
            params = wf0.getparams()
            raw = wf0.readframes(wf0.getnframes())
        samples = array.array('h', raw)
        for i in range(len(samples)):
            samples[i] = max(-32768, min(32767, samples[i] * 3))
        with wave.open(tmp.name, 'wb') as wf0:
            wf0.setparams(params)
            wf0.writeframes(samples.tobytes())
        wf = wave.open(tmp.name, 'rb')
        sr = wf.getframerate()
        aplay1 = subprocess.Popen(
            ["aplay", "-D", SPEAK_DEVICE,  "-q", tmp.name],
            stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
        )
        aplay2 = subprocess.Popen(
            ["aplay", "-D", SPEAK_DEVICE2, "-q", tmp.name],
            stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
        )
        chunk = 1024
        data = wf.readframes(chunk)
        while data:
            if len(data) >= 2:
                samps = struct.unpack('<' + 'h' * (len(data) // 2), data)
                amp = min(sum(abs(s) for s in samps) // len(samps) * 2, 32767)
                send(f"AMP:{amp}")
            time.sleep(chunk / sr)
            data = wf.readframes(chunk)
        aplay1.wait()
        aplay2.wait()
        wf.close()
    if os.path.exists(tmp.name):
        os.unlink(tmp.name)


def get_response(history):
    last_user = history[-1]["content"] if history else "hello"
    prompt = f"{SYSTEM_PROMPT}\n\nHuman: {last_user}\nTaro:"

    payload = json.dumps({
        "prompt": prompt,
        "n_predict": 50,
        "temperature": 0.7,
        "repeat_penalty": 1.1,
        "stream": True,
        "stop": ["\nHuman:", "\n>", "\n"]
    }).encode()

    tts_q = queue.Queue()
    full_text = []

    def tts_worker():
        send("SPEAKING")
        while True:
            chunk = tts_q.get()
            if chunk is None:
                break
            _tts_play(chunk)
        send("DONE_SPEAKING")

    worker = threading.Thread(target=tts_worker, daemon=True)
    worker.start()

    buf = ""
    try:
        req = urllib.request.Request(
            f"http://127.0.0.1:{LLAMA_PORT}/completion",
            data=payload,
            headers={"Content-Type": "application/json"}
        )
        with urllib.request.urlopen(req, timeout=60) as resp:
            for raw_line in resp:
                line = raw_line.decode('utf-8').strip()
                if not line.startswith("data: "):
                    continue
                try:
                    evt = json.loads(line[6:])
                except:
                    continue
                token = evt.get("content", "")
                buf += token
                full_text.append(token)
                stop = evt.get("stop", False)
                if stop or (len(buf.rstrip()) > 10 and buf.rstrip()[-1] in '.!?'):
                    chunk = buf.strip()
                    if chunk:
                        tts_q.put(chunk)
                    buf = ""
                if stop:
                    break
    except Exception as e:
        sys.stderr.write(f"LLAMA STREAM ERROR: {e}\n")

    if buf.strip():
        tts_q.put(buf.strip())

    result = "".join(full_text).strip()
    sys.stderr.write(f"LLAMA RESPONSE: '{result}'\n")

    if not result:
        tts_q.put("Squawk! My brain got scrambled. Try again?")
        result = "Squawk! My brain got scrambled. Try again?"

    tts_q.put(None)
    worker.join()
    return result

def speak(text):
    """Fallback: speak a single string with full state messaging."""
    send("SPEAKING")
    _tts_play(text)
    send("DONE_SPEAKING")

def _auto_loop():
    history = []
    while not _auto_stop.is_set():
        try:
            wav_path = record()
            if _auto_stop.is_set():
                os.unlink(wav_path)
                break
            send("PROCESSING")

            text = transcribe(wav_path)
            os.unlink(wav_path)

            if not text or "[BLANK_AUDIO]" in text:
                send("READY")
                continue

            send(f"TRANSCRIPT:{text}")
            history.append({"role": "user", "content": text})

            response = get_response(history)
            history.append({"role": "assistant", "content": response})
            if len(history) > 6:
                history = history[-6:]

            send("READY")
        except Exception as e:
            import traceback
            sys.stderr.write(f"AUTO ERROR: {e}\n{traceback.format_exc()}\n")
            send("READY")
    send("READY")


def main():
    history = []
    start_server()
    send("READY")

    _auto_thread = None

    while True:
        try:
            line = sys.stdin.readline()
            if not line:
                break
            line = line.strip()
        except:
            break

        if line == "AUTO_ON":
            _auto_stop.clear()
            _auto_thread = threading.Thread(target=_auto_loop, daemon=True)
            _auto_thread.start()

        elif line == "AUTO_OFF":
            _auto_stop.set()

        elif line == "LISTEN":
            try:
                wav_path = record()
                send("PROCESSING")

                text = transcribe(wav_path)
                os.unlink(wav_path)

                if not text or "[BLANK_AUDIO]" in text:
                    history.append({"role": "user", "content": "Someone tried to talk to you but you couldn't hear them."})
                else:
                    send(f"TRANSCRIPT:{text}")
                    history.append({"role": "user", "content": text})

                response = get_response(history)
                sys.stderr.write(f"FINAL RESPONSE: '{response}'\n")
                history.append({"role": "assistant", "content": response})

                if len(history) > 6:
                    history = history[-6:]

                send("READY")

            except Exception as e:
                import traceback
                sys.stderr.write(f"ERROR: {e}\n{traceback.format_exc()}\n")
                send("READY")

        elif line == "QUIT":
            break

if __name__ == "__main__":
    main()