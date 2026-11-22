/* MPC PORT: replaces upstream/Source/Reverberator.cpp in the build (same class, upstream's Reverberator.h unchanged).
 * The algorithm is upstream's, step for step (see the upstream file for its description); only the loops are written
 * for the device:
 *  - the echo loop (1000 echoes x block, the plugin's whole cost) is NEON on armv7 / aarch64, 16 samples per step,
 *    with the echo's gain computed once per block as upstream does (a variant by 4-cell tiles, each cell loaded and
 *    stored once, measured 2.7x slower on the Gen1: its multiply-adds form a dependency chain);
 *  - the read / clear / feedback loop runs four samples at a time while neither position wraps.
 * armv7 NEON multiplies and adds without fusing, as upstream's scalar code does; it flushes denormals, which upstream
 * also asks for (ScopedNoDenormals). aarch64 uses vmlaq (not fused either). Elsewhere the plain loops stay.
 * SPDX-License-Identifier: GPL-3.0-only */
#include "Reverberator.h"

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
#define NEP_NEON 1
#endif

Reverberator::Reverberator(Parameters& parameters, std::vector<Echo>& echoes) :
parameters(parameters),
echoes(echoes) {}

void Reverberator::initialize() {
    readPosition = 0;
    feedbackPosition = parameters.loopLength;
    data.assign(parameters.loopCapacity, 0.0f);
}

static inline void echo_add(float* __restrict w, const float* __restrict in, const float gain, const int n) {
    int i = 0;
#ifdef NEP_NEON
    const float32x4_t g = vdupq_n_f32(gain);
    for (; i + 16 <= n; i += 16) {
        float32x4_t a = vld1q_f32(w + i), b = vld1q_f32(w + i + 4), c = vld1q_f32(w + i + 8), d = vld1q_f32(w + i + 12);
        a = vmlaq_f32(a, g, vld1q_f32(in + i));
        b = vmlaq_f32(b, g, vld1q_f32(in + i + 4));
        c = vmlaq_f32(c, g, vld1q_f32(in + i + 8));
        d = vmlaq_f32(d, g, vld1q_f32(in + i + 12));
        vst1q_f32(w + i, a), vst1q_f32(w + i + 4, b), vst1q_f32(w + i + 8, c), vst1q_f32(w + i + 12, d);
    }
    for (; i + 4 <= n; i += 4)
        vst1q_f32(w + i, vmlaq_f32(vld1q_f32(w + i), g, vld1q_f32(in + i)));
#endif
    for (; i < n; i++)
        w[i] += gain * in[i];
}

void Reverberator::process(const std::vector<float>& input, std::vector<float>& output) {
    const int dataSize = static_cast<int>(data.size());
    const int inputSize = static_cast<int>(input.size());
    float* const base = data.data();
    const float* const in = input.data();
    const float pgain = parameters.gain;

    for (const Echo& echo : echoes) {
        const float gain = pgain * echo.amplitude;

        int pos = readPosition + echo.position;
        if (pos >= dataSize) pos -= dataSize;

        if (pos + inputSize <= dataSize) {
            echo_add(base + pos, in, gain, inputSize);
        } else {
            const int first = dataSize - pos;   // upstream's wrapping loop, in two straight runs
            echo_add(base + pos, in, gain, first);
            echo_add(base, in + first, gain, inputSize - first);
        }
    }

    const float feedback = parameters.feedback;
    float* out = output.data();
    const int n = static_cast<int>(output.size());
    int i = 0;
    while (i < n) {
        // samples until either position wraps (they are loopLength apart and both advance by one)
        int run = n - i;
        if (dataSize - readPosition < run) run = dataSize - readPosition;
        if (dataSize - feedbackPosition < run) run = dataSize - feedbackPosition;
        float* r = base + readPosition;
        float* f = base + feedbackPosition;
        int k = 0;
#ifdef NEP_NEON
        const float32x4_t fb = vdupq_n_f32(feedback), z = vdupq_n_f32(0.0f);
        for (; k + 4 <= run; k += 4) {
            const float32x4_t v = vld1q_f32(r + k);
            vst1q_f32(out + i + k, v);
            vst1q_f32(r + k, z);
            vst1q_f32(f + k, vmlaq_f32(vld1q_f32(f + k), v, fb));
        }
#endif
        for (; k < run; k++) {
            const float v = r[k];
            out[i + k] = v;
            r[k] = 0.0f;
            f[k] += v * feedback;
        }
        i += run;
        readPosition += run;
        feedbackPosition += run;
        if (readPosition == dataSize) readPosition = 0;
        if (feedbackPosition == dataSize) feedbackPosition = 0;
    }
}
