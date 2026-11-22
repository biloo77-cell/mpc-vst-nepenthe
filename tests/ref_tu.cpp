/* Upstream's Reverberator.cpp, unmodified, compiled as RefReverberator next to the port's src/opt version. */
#define Reverberator RefReverberator
#include "../upstream/Source/Reverberator.cpp"
#undef Reverberator

void* ref_new(Parameters& p, std::vector<Echo>& e) { return new RefReverberator(p, e); }
void ref_init(void* r) { static_cast<RefReverberator*>(r)->initialize(); }
void ref_process(void* r, const std::vector<float>& in, std::vector<float>& out) { static_cast<RefReverberator*>(r)->process(in, out); }
void ref_free(void* r) { delete static_cast<RefReverberator*>(r); }
