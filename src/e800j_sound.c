/* Elwro 800 Junior emulator v0.3
 * Copyright (C) 2006 Krzysztof Komarnicki
 * Copyright (C) 2026 Artur Wojcik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See the file COPYING.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <SDL3/SDL.h>
#include "e800j_sound.h"
#include "e800j_msg.h"
#include "z80_cpu.h"

#define SND_MAX_LATENCY_MS 200 /* powyżej tego opóźnienia nadmiarowe próbki są odrzucane */
#define SND_TARGET_LATENCY_MS 50 /* docelowa ilość zakolejkowanego dźwięku, utrzymywana korektą tempa */
#define SND_RATE_ADJ 0.01 /* maksymalna korekta tempa renderowania (1% ~ 17 centów) */
#define SND_QUEUE_SMOOTH 0.2 /* współczynnik wygładzania pomiaru kolejki */
#define SND_DC_R 0.995f /* współczynnik filtru górnoprzepustowego */

void snd_free_buffers(struct sound* w) {
    if (w->stream == nullptr)
        return;
    SDL_DestroyAudioStream(w->stream);
    w->stream = nullptr;
}

/* przekazuje wyrenderowane ramki do SDL; przy zbyt dużej kolejce (emulacja szybsza od czasu rzeczywistego) odrzuca je */
static void snd_put(struct sound* w) {
    if (w->nout == 0)
        return;
    if (SDL_GetAudioStreamQueued(w->stream) < w->max_queue)
        SDL_PutAudioStreamData(w->stream, w->out, w->nout * w->chn * (int)sizeof(short));
    w->nout = 0;
}

/* wprowadza ciszę do kolejki, by urządzenie nie zaczynało od pustego bufora */
static void snd_prefill(struct sound* w) {
    short silence[SND_CHUNK * 2] = { 0 };
    for (int left = w->target_frames; left > 0; left -= SND_CHUNK) {
        const int n = left < SND_CHUNK ? left : SND_CHUNK;
        SDL_PutAudioStreamData(w->stream, silence, n * w->chn * (int)sizeof(short));
    }
}

static void snd_emit(struct sound* w, float x) {
    /* filtr górnoprzepustowy usuwa składową stałą (trzaski przy zmianie stanu bezczynności) */
    const float y = x - w->dc_x + SND_DC_R * w->dc_y;
    w->dc_x = x;
    w->dc_y = y;
    const float v = y * w->amp;
    const short s = (short)(v > 1.f ? 32767 : v < -1.f ? -32767 : v * 32767.f);
    for (int c = 0; c < w->chn; c++)
        w->out[w->nout * w->chn + c] = s;
    if (++w->nout == SND_CHUNK)
        snd_put(w);
}

/* renderuje dźwięk do taktu 'now' przy stałym dotychczasowym poziomie wyjścia */
static void snd_advance(struct sound* w, const long now) {
    double dt = (double)(now - w->last_ticks);
    w->last_ticks = now;
    if (dt <= 0)
        return;
    if (dt > w->cpu_hz) /* np. wstrzymanie emulacji w debuggerze */
        dt = w->cpu_hz;
    while (dt > 0) {
        const double room = w->step - w->pos;
        if (dt < room) {
            w->acc += w->level * dt;
            w->pos += dt;
            return;
        }
        w->acc += w->level * room;
        dt -= room;
        snd_emit(w, w->alias ? (float)(w->acc / w->step) : w->level);
        w->acc = 0;
        w->pos = 0;
    }
}

/* utrzymuje zakolejkowany dźwięk blisko wartości docelowej: gdy kolejka maleje, próbki są
   renderowane nieco gęściej (i odwrotnie), co kompensuje różnicę między zegarem emulacji i urządzenia */
static void snd_rate(struct sound* w) {
    const double queued = (double)SDL_GetAudioStreamQueued(w->stream) / (w->chn * (int)sizeof(short));
    w->avg_q += SND_QUEUE_SMOOTH * (queued - w->avg_q);
    double err = (w->target_frames - w->avg_q) / w->target_frames;
    err = err > 1.0 ? 1.0 : err < -1.0 ? -1.0 : err;
    const double step = w->step_nom * (1.0 - SND_RATE_ADJ * err);
    const double ratio = step / w->step;
    w->pos *= ratio;
    w->acc *= ratio;
    w->step = step;
}

void snd_init(struct sound* w, const struct cpu* cpu, const double cpu_hz) {
    if (w->nosound)
        return;
    msg_printf(MSG_MSG, "Sound init\n");
    w->stream = nullptr;
    if (w->chn < 1 || w->chn > 2)
        w->chn = 1;
    if (w->freq < 8000 || cpu_hz <= 0) {
        msg_printf(MSG_ERROR, "Invalid sound sample rate or CPU frequency\n");
        w->nosound = true;
        return;
    }

    const SDL_AudioSpec spec = { SDL_AUDIO_S16, w->chn, w->freq };
    char samples[16];
    snprintf(samples, sizeof(samples), "%d", w->length);
    SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, samples);

    if ((w->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr)) == nullptr) {
        msg_printf(MSG_WARN, "Couldn't open audio: %s\n", SDL_GetError());
        w->nosound = true;
        return;
    }
    msg_printf(MSG_MSG, "Using audio driver: %s\n", SDL_GetCurrentAudioDriver());

    w->cpu = cpu;
    w->cpu_hz = cpu_hz;
    w->step = w->step_nom = cpu_hz / w->freq;
    w->pos = w->acc = 0;
    w->last_ticks = cpu->ticks;
    w->level = -1.f;
    w->amp = 0.3f * w->volume / 128.f;
    w->dc_x = w->dc_y = 0;
    w->nout = 0;
    w->max_queue = w->freq * SND_MAX_LATENCY_MS / 1000 * w->chn * (int)sizeof(short);
    w->target_frames = w->freq * SND_TARGET_LATENCY_MS / 1000;
    w->avg_q = w->target_frames;
    snd_prefill(w);
    SDL_ResumeAudioStreamDevice(w->stream);
}

/* wywoływana po każdym przebiegu pętli CPU - dogania renderowanie do bieżącego taktu */
void snd_flush(struct sound* w) {
    if (w->nosound || w->stream == nullptr)
        return;
    snd_advance(w, w->cpu->ticks);
    snd_put(w);
    snd_rate(w);
}

void snd_beeper(struct sound* w, const unsigned char port_fe) {
    if (w->nosound || w->stream == nullptr)
        return;
    const float level = (port_fe & 0x10) ? 1.f : -1.f;
    if (level == w->level)
        return;
    snd_advance(w, w->cpu->ticks);
    w->level = level;
}
