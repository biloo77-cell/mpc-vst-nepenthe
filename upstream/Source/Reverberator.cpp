#include "Reverberator.h"

/*

Reverberator implements the core of the Nepenthe reverb algorithm. It uses velvet noise
(see: Echoes.cpp) to create echoes of each input sample, sustained by a simple feedback loop.

The data container is a single circular buffer. Two positions are tracked: the read position and
the feedback position. The feedback position is always one second ahead of the read position.

The algorithm is as follows:

1. Loop through each of the defined echoes. There are 1,000 defined echoes per stereo channel,
   spanning one second. The outer loop is echoes rather than samples, for CPU cache reasons: an
   input block is much smaller than the circular buffer, so the algorithm works in one small data
   "neighborhood" at a time.

2. For each echo, loop through each sample in the input block, and echo that sample. The sample
   gets echoed somewhere between the read position and the feedback position, with an exponential
   decay function determining the amplitude.

3. After writing the echoes, go back to the read position and read N samples of output, matching
   the number of input samples.

4. While reading output, write each sample at the feedback position, reduced in amplitude by the
   same function as the defined echoes. This creates a seamless handoff between the last defined
   echo and the feedback from the first defined echo.

To speed up computation, there are two versions of the inner loop: one with a conditional for bounds
checking, and one without.

The non-checking loop is used when the end of the circular buffer won't be reached by the end of the
input block. In that case, all of the operations are parallelizable, and the loop can be vectorized.

*/

Reverberator::Reverberator(Parameters& parameters, std::vector<Echo>& echoes) :
parameters(parameters),
echoes(echoes) {}

void Reverberator::initialize() {
    readPosition = 0;
    feedbackPosition = parameters.loopLength;
    data.assign(parameters.loopCapacity, 0.0f);
}

void Reverberator::process(const std::vector<float>& input, std::vector<float>& output) {
    int dataSize = static_cast<int>(data.size());
    int inputSize = static_cast<int>(input.size());

    for (const Echo& echo : echoes) {
        const float gain = parameters.gain * echo.amplitude;

        int pos = readPosition + echo.position;
        if (pos >= dataSize) pos -= dataSize;

        if (pos + inputSize <= dataSize) {
            // Visual Studio needs a loop to be in the textbook "shape" to vectorize it.
            // Any pointer math more complicated than subscript has to happen outside the loop.
            float* writePointer = data.data() + pos;

            for (int sample = 0; sample < inputSize; sample++) {
                writePointer[sample] += gain * input[sample];
            }
        } else {
            for (int sample = 0; sample < inputSize; sample++) {
                data[pos] += gain * input[sample];
                if (++pos == dataSize) pos = 0;
            }
        }
    }

    for (float& value : output) {
        value = data[readPosition];
        data[readPosition] = 0.0f;
        data[feedbackPosition] += value * parameters.feedback;

        if (++readPosition == dataSize) readPosition = 0;
        if (++feedbackPosition == dataSize) feedbackPosition = 0;
    }
}
