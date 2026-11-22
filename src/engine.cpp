/* Nepenthe for MPC OS: upstream's Processor (upstream/reference/Processor.cpp) as an mpc_engine() for the Open MPC
 * wrapper (wrapper/engine.h). Same objects, same order, same processing per block: collapse to mono, EQ, pre-delay,
 * the two velvet-noise reverbs, width, dry/wet. 44.1 kHz, interleaved int16 stereo, 128-frame blocks.
 * Parameters are upstream's six, with upstream's ranges, defaults and steps:
 *   time   0..1000 = the slider's travel; seconds = JUCE NormalisableRange(0.5, 8, 0.01, skew 0.7) of it
 *   delay  0..200 ms, bass / treble / width / mix 0..100 (whole numbers, as upstream's AudioParameterInt)
 * SPDX-License-Identifier: GPL-3.0-only */
extern "C" {
#include "engine.h"
}

#include "Delay.h"
#include "Echoes.h"
#include "Equalizer.h"
#include "Mixer.h"
#include "Parameters.h"
#include "Reverberator.h"
#include "Timer.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>

#if defined(__x86_64__) || defined(__i386__)
#include <xmmintrin.h>
#endif

namespace {

constexpr int SAMPLE_RATE = 44100;
constexpr int MAX_BLOCK = 128;

/* Keys in VST index order (vst/params.lock): append only. */
enum { P_TIME, P_DELAY, P_BASS, P_TREBLE, P_WIDTH, P_MIX, P_COUNT };
const char* const keys[P_COUNT] = {"time", "delay", "bass", "treble", "width", "mix"};
const double lo[P_COUNT] = {0, 0, 0, 0, 0, 0};
const double hi[P_COUNT] = {1000, 200, 100, 100, 100, 100};
/* time 354 = 2.20 s (upstream's default), delay 30 ms, the rest as upstream */
const double defaults[P_COUNT] = {354, 30, 50, 50, 100, 100};

double clampd(double x, double a, double b) { return x < a ? a : x > b ? b : x; }

/* JUCE NormalisableRange<float>(0.5, 8, 0.01, 0.7)::convertFrom0to1 then snapToLegalValue. */
float time_seconds(double pos) {
    const float p = static_cast<float>(clampd(pos, 0, 1000) / 1000.0);
    float v = 0.5f + 7.5f * (p > 0.0f ? std::exp(std::log(p) / 0.7f) : 0.0f);
    v = 0.5f + 0.01f * std::floor((v - 0.5f) / 0.01f + 0.5f);
    return v < 0.5f ? 0.5f : v > 8.0f ? 8.0f : v;
}

/* AudioParameterInt: whole numbers. */
float whole(double v, int i) { return static_cast<float>(std::floor(clampd(v, lo[i], hi[i]) + 0.5)); }

struct Inst {
    Parameters parameters;
    Timer timer;
    Equalizer equalizer{parameters};
    Delay delay{parameters};
    Echoes echoes{parameters};
    Reverberator leftReverb{parameters, echoes.left};
    Reverberator rightReverb{parameters, echoes.right};
    Mixer mixer{parameters};

    std::vector<float> input, leftOutput, rightOutput;
    float dryL[MAX_BLOCK], dryR[MAX_BLOCK];
    double v[P_COUNT];

    Inst() {
        std::memcpy(v, defaults, sizeof v);
        push();
        // upstream's prepareToPlay(44100, 128)
        input.reserve(MAX_BLOCK);
        leftOutput.reserve(MAX_BLOCK);
        rightOutput.reserve(MAX_BLOCK);
        parameters.initialize(static_cast<float>(SAMPLE_RATE));
        parameters.update();   // echoes.initialize() reads the time through getAmplitude()
        equalizer.initialize();
        delay.initialize();
        echoes.initialize();
        leftReverb.initialize();
        rightReverb.initialize();
        mixer.initialize();
        timer.reset();
    }

    void push() {
        parameters.values.time = time_seconds(v[P_TIME]);
        parameters.values.delay = whole(v[P_DELAY], P_DELAY);
        parameters.values.bass = whole(v[P_BASS], P_BASS);
        parameters.values.treble = whole(v[P_TREBLE], P_TREBLE);
        parameters.values.width = whole(v[P_WIDTH], P_WIDTH);
        parameters.values.mix = whole(v[P_MIX], P_MIX);
    }
};

int key_index(const char* k) {
    for (int i = 0; i < P_COUNT; i++)
        if (std::strcmp(k, keys[i]) == 0)
            return i;
    return -1;
}

void* create(const char*) {
    try {
        return new Inst();
    } catch (...) {
        return nullptr;
    }
}

void destroy(void* inst) { delete static_cast<Inst*>(inst); }

void midi(void*, const uint8_t*, int) {}

int get_state(const Inst* s, char* buf, int len) {
    int n = 0;
    for (int i = 0; i < P_COUNT && n < len; i++)
        n += std::snprintf(buf + n, static_cast<size_t>(len - n), "%s=%.9g;", keys[i], s->v[i]);
    return n < len ? n : 0;
}

void set_state(Inst* s, const char* val) {
    char k[32];
    const char* p = val;
    while (*p) {
        const char *eq = std::strchr(p, '='), *end = std::strchr(p, ';');
        if (!eq || !end || eq > end || eq - p >= static_cast<long>(sizeof k))
            break;
        std::memcpy(k, p, static_cast<size_t>(eq - p));
        k[eq - p] = 0;
        const int i = key_index(k);
        if (i >= 0)
            s->v[i] = std::atof(eq + 1);
        p = end + 1;
    }
}

void set_param(void* inst, const char* key, const char* val) {
    Inst* s = static_cast<Inst*>(inst);
    if (std::strcmp(key, "state") == 0) {
        set_state(s, val);
        return;
    }
    const int i = key_index(key);
    if (i >= 0)
        s->v[i] = std::atof(val);
}

int get_param(void* inst, const char* key, char* buf, int len) {
    Inst* s = static_cast<Inst*>(inst);
    if (std::strcmp(key, "state") == 0)
        return get_state(s, buf, len);
    const size_t kl = std::strlen(key);
    if (kl > 8 && std::strcmp(key + kl - 8, "_display") == 0) {
        char k[32];
        if (kl - 8 >= sizeof k)
            return 0;
        std::memcpy(k, key, kl - 8);
        k[kl - 8] = 0;
        const int i = key_index(k);
        if (i < 0)
            return 0;
        if (i == P_TIME)   // upstream's text box: the value as JUCE prints it, no unit
            return std::snprintf(buf, static_cast<size_t>(len), "%.2f", time_seconds(s->v[i]));
        return std::snprintf(buf, static_cast<size_t>(len), "%d", static_cast<int>(whole(s->v[i], i)));
    }
    const int i = key_index(key);
    if (i < 0)
        return 0;
    return std::snprintf(buf, static_cast<size_t>(len), "%.9g", s->v[i]);
}

void render(void*, int16_t* out, int frames) { std::memset(out, 0, static_cast<size_t>(frames) * 2 * sizeof *out); }

inline int16_t to_i16(float x) {
    if (!(x == x))
        return 0;
    x *= 32768.0f;
    if (x > 32767.0f)
        return 32767;
    if (x < -32768.0f)
        return -32768;
    return static_cast<int16_t>(std::lrintf(x));
}

/* juce::ScopedNoDenormals: flush-to-zero for the block, the host's mode restored afterwards. */
typedef unsigned long fpmode_t;
fpmode_t ftz_on() {
#if defined(__aarch64__)
    unsigned long r;
    __asm__ volatile("mrs %0, fpcr" : "=r"(r));
    __asm__ volatile("msr fpcr, %0" ::"r"(r | (1UL << 24)));
    return r;
#elif defined(__arm__) && defined(__ARM_FP)
    unsigned int r;
    __asm__ volatile("vmrs %0, fpscr" : "=r"(r));
    __asm__ volatile("vmsr fpscr, %0" ::"r"(r | (1U << 24)));
    return r;
#elif defined(__x86_64__) || defined(__i386__)
    unsigned int r = _mm_getcsr();
    _mm_setcsr(r | 0x8040);
    return r;
#else
    return 0;
#endif
}

void ftz_restore(fpmode_t r) {
#if defined(__aarch64__)
    __asm__ volatile("msr fpcr, %0" ::"r"(r));
#elif defined(__arm__) && defined(__ARM_FP)
    unsigned int v = static_cast<unsigned int>(r);
    __asm__ volatile("vmsr fpscr, %0" ::"r"(v));
#elif defined(__x86_64__) || defined(__i386__)
    _mm_setcsr(static_cast<unsigned int>(r));
#else
    (void)r;
#endif
}

/* upstream's processBlock, block by block of at most 128 frames */
void process_block(Inst* s, const int16_t* in, int16_t* out, const int frames) {
    if (s->timer.boop()) {   // upstream: realtime and more than 100 ms since the last block -> clear the tails
        s->equalizer.initialize();
        s->delay.initialize();
        s->leftReverb.initialize();
        s->rightReverb.initialize();
        s->mixer.initialize();
    }

    s->push();
    s->parameters.update();
    s->delay.updateLength();
    s->echoes.updateAmplitudes();

    s->input.resize(frames);
    s->leftOutput.resize(frames);
    s->rightOutput.resize(frames);

    constexpr float k = 1.0f / 32768.0f;
    for (int i = 0; i < frames; i++) {
        const float left = in[2 * i] * k, right = in[2 * i + 1] * k;
        s->dryL[i] = left;
        s->dryR[i] = right;
        float mono = (left + right) * 0.5f;
        mono = s->equalizer.process(mono);
        mono = s->delay.process(mono);
        s->input[i] = mono;
    }

    s->leftReverb.process(s->input, s->leftOutput);
    s->rightReverb.process(s->input, s->rightOutput);

    Mixer& m = s->mixer;
    for (int i = 0; i < frames; i++) {
        m.advanceSample();

        float left = s->leftOutput[i];
        float right = s->rightOutput[i];

        left = m.stereoSelf * left + m.stereoCross * right;
        right = m.stereoSelf * right + m.stereoCross * left;   // upstream uses the new left here; kept

        left = m.mixWet * left + m.mixDry * s->dryL[i];
        right = m.mixWet * right + m.mixDry * s->dryR[i];

        out[2 * i] = to_i16(left);
        out[2 * i + 1] = to_i16(right);
    }
}

void process(void* inst, const int16_t* in, int16_t* out, int frames) {
    Inst* s = static_cast<Inst*>(inst);
    const fpmode_t fp = ftz_on();
    for (int i = 0; i < frames; i += MAX_BLOCK) {
        const int n = frames - i < MAX_BLOCK ? frames - i : MAX_BLOCK;
        process_block(s, in + 2 * i, out + 2 * i, n);
    }
    ftz_restore(fp);
}

const mpc_engine_t engine = {create, destroy, midi, set_param, get_param, render, process, nullptr};

}  // namespace

extern "C" const mpc_engine_t* mpc_engine(void) { return &engine; }
