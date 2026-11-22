/* Offline checks of the Nepenthe port.
 *   g++ -std=gnu++17 -O2 -ffp-contract=off -Isrc/shim -Iupstream/Source -I../mpc-vst-plugins/wrapper \
 *       tests/ref_check.cpp tests/ref_tu.cpp src/engine.cpp src/Parameters.cpp src/opt/Reverberator.cpp \
 *       src/opt/Echoes.cpp upstream/Source/{Delay,Equalizer,Mixer,Timer}.cpp -o tests/out/ref_check && tests/out/ref_check
 * 1. Bit match: the port's Reverberator (src/opt) against upstream's, same echoes, same input, several Time settings,
 *    Time changed while running, 6 s (the circular buffer wraps several times): every output sample equal,
 *    in 128-, 100- and 64-frame blocks.
 * 2. Engine: T60 of the impulse response follows TIME; MIX 0 passes the input; state round-trips; value texts.
 * 3. CPU: microseconds per 128-frame block on this machine (relative only). */
#include "Echoes.h"
#include "Parameters.h"
#include "Reverberator.h"
extern "C" {
#include "engine.h"
}

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

void* ref_new(Parameters& p, std::vector<Echo>& e);
void ref_init(void* r);
void ref_process(void* r, const std::vector<float>& in, std::vector<float>& out);
void ref_free(void* r);

static int fails;
#define CHECK(c, ...) do { if (!(c)) { fails++; std::printf("FAIL: " __VA_ARGS__); std::printf("\n"); } } while (0)

static unsigned rng = 12345;
static float noise() { rng = rng * 1664525u + 1013904223u; return ((int)(rng >> 9) - (1 << 22)) / (float)(1 << 22) * 0.5f; }

static void bit_match(float t0, float t1, int bs = 128) {
    Parameters p;
    p.initialize(44100);
    p.values.time = t0;
    p.update();
    Echoes e(p);
    e.initialize();
    Reverberator opt(p, e.left);
    void* ref = ref_new(p, e.left);
    opt.initialize();
    ref_init(ref);
    std::vector<float> in(bs), a(bs), b(bs);
    long diff = 0;
    const int blocks = 6 * 44100 / bs;
    for (int k = 0; k < blocks; k++) {
        if (k == blocks / 2) { p.values.time = t1; p.update(); e.updateAmplitudes(); }
        for (int i = 0; i < bs; i++) in[i] = k < blocks / 3 || k % 50 == 0 ? noise() : 0.0f;
        opt.process(in, a);
        ref_process(ref, in, b);
        for (int i = 0; i < bs; i++) diff += std::memcmp(&a[i], &b[i], sizeof(float)) != 0;
    }
    std::printf("bit match time %.2f -> %.2f s, %d-frame blocks: %ld differing samples of %d\n", t0, t1, bs, diff, blocks * bs);
    CHECK(diff == 0, "reverberator differs from upstream");
    ref_free(ref);
}

static void setp(const mpc_engine_t* e, void* s, const char* k, double v) {
    char b[32];
    std::snprintf(b, sizeof b, "%.9g", v);
    e->set_param(s, k, b);
}

static double t60(const mpc_engine_t* e, double pos) {
    void* s = e->create(nullptr);
    setp(e, s, "time", pos), setp(e, s, "delay", 0), setp(e, s, "mix", 100);
    const int n = 12 * 44100 / 128 * 128;
    std::vector<int16_t> in(2 * n), out(2 * n);
    in[0] = in[1] = 32000;
    for (int i = 0; i < n; i += 128) e->process(s, &in[2 * i], &out[2 * i], 128);
    std::vector<double> sch(n + 1);
    for (int i = n - 1; i >= 0; i--) sch[i] = sch[i + 1] + (double)out[2 * i] * out[2 * i] + (double)out[2 * i + 1] * out[2 * i + 1];
    int i5 = -1, i35 = -1;
    for (int i = 0; i < n; i++) {
        double db = 10 * std::log10(sch[i] / sch[0] + 1e-30);
        if (i5 < 0 && db <= -5) i5 = i;
        if (i35 < 0 && db <= -35) { i35 = i; break; }
    }
    e->destroy(s);
    return i35 < 0 ? -1 : 2.0 * (i35 - i5) / 44100.0;
}

int main() {
    bit_match(2.2f, 0.5f);
    bit_match(8.0f, 3.3f);
    bit_match(0.5f, 8.0f);
    bit_match(2.2f, 6.0f, 100);
    bit_match(1.0f, 0.7f, 64);

    const mpc_engine_t* e = mpc_engine();
    char buf[64];
    struct { double pos; double secs; } tt[] = {{0, 0.5}, {354, 2.2}, {600, 4.3}};
    for (auto& t : tt) {
        void* s = e->create(nullptr);
        setp(e, s, "time", t.pos);
        e->get_param(s, "time_display", buf, sizeof buf);
        e->destroy(s);
        double m = t60(e, t.pos);
        std::printf("time %4.0f -> %s s: measured T60 %.2f s\n", t.pos, buf, m);
        CHECK(std::fabs(m - std::atof(buf)) < 0.15 * std::atof(buf) + 0.1, "T60 %.2f vs %s", m, buf);
    }

    void* s = e->create(nullptr);
    e->get_param(s, "time_display", buf, sizeof buf);
    CHECK(!std::strcmp(buf, "2.20"), "default time text %s", buf);
    setp(e, s, "delay", 65.4);
    e->get_param(s, "delay_display", buf, sizeof buf);
    CHECK(!std::strcmp(buf, "65"), "delay text %s", buf);
    setp(e, s, "mix", 0);
    std::vector<int16_t> in(2 * 44100), out(2 * 44100);
    for (auto& x : in) x = (int16_t)(noise() * 30000);
    for (int i = 0; i < 44100 - 128; i += 128) e->process(s, &in[2 * i], &out[2 * i], 128);
    /* mix is smoothed (30 Hz): after 0.5 s it is 0 to float precision */
    long d = 0;
    for (int i = 22050; i < 44100 - 128; i++) d += in[2 * i] != out[2 * i] || in[2 * i + 1] != out[2 * i + 1];
    CHECK(d == 0, "mix 0 is not dry (%ld)", d);
    char st[256];
    setp(e, s, "time", 777), setp(e, s, "bass", 12);
    e->get_param(s, "state", st, sizeof st);
    void* s2 = e->create(nullptr);
    e->set_param(s2, "state", st);
    char st2[256];
    e->get_param(s2, "state", st2, sizeof st2);
    CHECK(!std::strcmp(st, st2), "state round trip: %s / %s", st, st2);
    std::printf("state: %s\n", st);

    setp(e, s2, "mix", 100);
    auto t0 = std::chrono::steady_clock::now();
    const int blocks = 2000;
    for (int k = 0; k < blocks; k++) e->process(s2, &in[(k % 300) * 256], &out[0], 128);
    double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count() / blocks;
    std::printf("CPU: %.1f us per 128-frame block (%.1f %% of real time here)\n", us, us / 2902.5 * 100);
    e->destroy(s), e->destroy(s2);

    std::printf(fails ? "FAILED (%d)\n" : "ALL PASS\n", fails);
    return fails != 0;
}
