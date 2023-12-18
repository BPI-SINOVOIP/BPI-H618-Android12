/*
 * Copyright 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <array>
#include <climits>
#include <cstdlib>
#include <random>
#include <vector>

#include <benchmark/benchmark.h>

#include <audio_utils/BiquadFilter.h>
#include <audio_utils/format.h>

static constexpr size_t DATA_SIZE = 1024;
// The coefficients is a HPF with sampling frequency as 48000, center frequency as 600,
// and Q as 0.707. As all the coefficients are not zero, they can be used to benchmark
// the non-zero optimization of BiquadFilter.
// The benchmark test will iterate the channel count from 1 to 2. The occupancy will be
// iterate from 1 to 31. In that case, it is possible to test the performance of cases
// with different coefficients as zero.
static constexpr float REF_COEFS[] = {0.9460f, -1.8919f, 0.9460f, -1.8890f, 0.8949f};

static void BM_BiquadFilter1D(benchmark::State& state) {
    using android::audio_utils::BiquadFilter;

    bool doParallel = (state.range(0) == 1);
    // const size_t channelCount = state.range(1);
    const size_t filters = 1;

    std::vector<float> input(DATA_SIZE);
    std::array<float, android::audio_utils::kBiquadNumCoefs> coefs;

    // Initialize input buffer and coefs with deterministic pseudo-random values
    constexpr std::minstd_rand::result_type SEED = 42; // arbitrary choice.
    std::minstd_rand gen(SEED);
    constexpr float amplitude = 1.0f;
    std::uniform_real_distribution<> dis(-amplitude, amplitude);
    for (size_t i = 0; i < DATA_SIZE; ++i) {
        input[i] = dis(gen);
    }

    android::audio_utils::BiquadFilter parallel(filters, coefs);
    std::vector<std::unique_ptr<BiquadFilter<float>>> biquads(filters);
    for (auto& biquad : biquads) {
        biquad.reset(new BiquadFilter<float>(1, coefs));
    }

    // Run the test
    float *data = input.data();
    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(data);
        if (doParallel) {
            parallel.process1D(data, DATA_SIZE);
        } else {
            for (auto& biquad : biquads) {
                biquad->process(data, data, DATA_SIZE);
            }
        }
        benchmark::ClobberMemory();
    }
}

static void BiquadFilter1DArgs(benchmark::internal::Benchmark* b) {
    for (int k = 0; k < 2; k++) // 0 for normal random data, 1 for subnormal random data
         b->Args({k});
}

BENCHMARK(BM_BiquadFilter1D)->Apply(BiquadFilter1DArgs);

/*******************************************************************
 * A test result running on Pixel 4 for comparison.
 * The first parameter indicates the input data is subnormal or not.
 * 0 for normal input data, 1 for subnormal input data.
 * The second parameter indicates the channel count.
 * The third parameter indicates the occupancy of the coefficients.
 * -----------------------------------------------------------------
 * Benchmark                       Time             CPU   Iterations
 * -----------------------------------------------------------------
 * BM_BiquadFilter/0/1/1         734 ns          732 ns       740671
 * BM_BiquadFilter/0/1/2         554 ns          553 ns      1266836
 * BM_BiquadFilter/0/1/3         647 ns          645 ns      1085314
 * BM_BiquadFilter/0/1/4         834 ns          832 ns       841345
 * BM_BiquadFilter/0/1/5        1068 ns         1065 ns       657343
 * BM_BiquadFilter/0/1/6         767 ns          765 ns       915223
 * BM_BiquadFilter/0/1/7         929 ns          926 ns       756405
 * BM_BiquadFilter/0/1/8        2173 ns         2168 ns       322949
 * BM_BiquadFilter/0/1/9        1831 ns         1826 ns       383304
 * BM_BiquadFilter/0/1/10       1931 ns         1927 ns       363386
 * BM_BiquadFilter/0/1/11       2536 ns         2529 ns       276783
 * BM_BiquadFilter/0/1/12       2535 ns         2529 ns       276826
 * BM_BiquadFilter/0/1/13       2174 ns         2169 ns       322755
 * BM_BiquadFilter/0/1/14       3257 ns         3250 ns       215383
 * BM_BiquadFilter/0/1/15       2175 ns         2169 ns       322711
 * BM_BiquadFilter/0/1/16       1494 ns         1491 ns       469625
 * BM_BiquadFilter/0/1/17       2176 ns         2171 ns       322423
 * BM_BiquadFilter/0/1/18       1568 ns         1564 ns       447467
 * BM_BiquadFilter/0/1/19       1325 ns         1322 ns       529762
 * BM_BiquadFilter/0/1/20       1926 ns         1921 ns       364534
 * BM_BiquadFilter/0/1/21       2630 ns         2623 ns       266027
 * BM_BiquadFilter/0/1/22       1998 ns         1993 ns       351210
 * BM_BiquadFilter/0/1/23       1882 ns         1877 ns       373028
 * BM_BiquadFilter/0/1/24       2536 ns         2529 ns       276818
 * BM_BiquadFilter/0/1/25       2176 ns         2170 ns       322627
 * BM_BiquadFilter/0/1/26       3258 ns         3250 ns       215440
 * BM_BiquadFilter/0/1/27       2175 ns         2169 ns       322724
 * BM_BiquadFilter/0/1/28       2535 ns         2529 ns       276823
 * BM_BiquadFilter/0/1/29       2175 ns         2170 ns       322560
 * BM_BiquadFilter/0/1/30       3259 ns         3250 ns       215354
 * BM_BiquadFilter/0/1/31       2176 ns         2171 ns       322492
 * BM_BiquadFilter/0/2/1        1470 ns         1466 ns       477411
 * BM_BiquadFilter/0/2/2        1109 ns         1107 ns       632666
 * BM_BiquadFilter/0/2/3        1297 ns         1294 ns       540804
 * BM_BiquadFilter/0/2/4        1681 ns         1677 ns       417675
 * BM_BiquadFilter/0/2/5        2137 ns         2132 ns       328721
 * BM_BiquadFilter/0/2/6        1572 ns         1569 ns       447114
 * BM_BiquadFilter/0/2/7        1736 ns         1732 ns       385563
 * BM_BiquadFilter/0/2/8        4331 ns         4320 ns       162048
 * BM_BiquadFilter/0/2/9        3795 ns         3785 ns       184166
 * BM_BiquadFilter/0/2/10       3832 ns         3823 ns       183015
 * BM_BiquadFilter/0/2/11       5060 ns         5047 ns       138660
 * BM_BiquadFilter/0/2/12       5046 ns         5034 ns       139033
 * BM_BiquadFilter/0/2/13       4333 ns         4322 ns       161952
 * BM_BiquadFilter/0/2/14       6500 ns         6482 ns       108022
 * BM_BiquadFilter/0/2/15       4335 ns         4324 ns       161915
 * BM_BiquadFilter/0/2/16       2965 ns         2957 ns       236771
 * BM_BiquadFilter/0/2/17       4368 ns         4358 ns       160829
 * BM_BiquadFilter/0/2/18       3193 ns         3186 ns       219766
 * BM_BiquadFilter/0/2/19       2804 ns         2798 ns       250201
 * BM_BiquadFilter/0/2/20       3839 ns         3830 ns       182731
 * BM_BiquadFilter/0/2/21       5310 ns         5296 ns       133012
 * BM_BiquadFilter/0/2/22       3995 ns         3984 ns       175672
 * BM_BiquadFilter/0/2/23       3755 ns         3745 ns       186960
 * BM_BiquadFilter/0/2/24       5060 ns         5045 ns       138733
 * BM_BiquadFilter/0/2/25       4343 ns         4330 ns       161632
 * BM_BiquadFilter/0/2/26       6505 ns         6489 ns       107871
 * BM_BiquadFilter/0/2/27       4348 ns         4336 ns       161436
 * BM_BiquadFilter/0/2/28       5068 ns         5054 ns       138515
 * BM_BiquadFilter/0/2/29       4401 ns         4389 ns       158834
 * BM_BiquadFilter/0/2/30       6514 ns         6497 ns       107752
 * BM_BiquadFilter/0/2/31       4352 ns         4341 ns       161242
 * BM_BiquadFilter/1/1/1         734 ns          732 ns       955891
 * BM_BiquadFilter/1/1/2         554 ns          552 ns      1267096
 * BM_BiquadFilter/1/1/3         647 ns          645 ns      1084919
 * BM_BiquadFilter/1/1/4         834 ns          832 ns       841505
 * BM_BiquadFilter/1/1/5        1068 ns         1065 ns       657299
 * BM_BiquadFilter/1/1/6         767 ns          765 ns       915192
 * BM_BiquadFilter/1/1/7         927 ns          924 ns       761606
 * BM_BiquadFilter/1/1/8        2174 ns         2168 ns       322888
 * BM_BiquadFilter/1/1/9        1832 ns         1826 ns       383311
 * BM_BiquadFilter/1/1/10       1932 ns         1926 ns       363384
 * BM_BiquadFilter/1/1/11       2536 ns         2529 ns       276796
 * BM_BiquadFilter/1/1/12       2536 ns         2529 ns       276843
 * BM_BiquadFilter/1/1/13       2175 ns         2169 ns       322743
 * BM_BiquadFilter/1/1/14       3259 ns         3250 ns       215420
 * BM_BiquadFilter/1/1/15       2175 ns         2169 ns       322745
 * BM_BiquadFilter/1/1/16       1495 ns         1491 ns       469661
 * BM_BiquadFilter/1/1/17       2177 ns         2171 ns       322388
 * BM_BiquadFilter/1/1/18       1569 ns         1564 ns       447468
 * BM_BiquadFilter/1/1/19       1325 ns         1322 ns       529736
 * BM_BiquadFilter/1/1/20       1927 ns         1922 ns       363962
 * BM_BiquadFilter/1/1/21       2624 ns         2617 ns       267102
 * BM_BiquadFilter/1/1/22       1999 ns         1993 ns       351169
 * BM_BiquadFilter/1/1/23       1882 ns         1877 ns       372944
 * BM_BiquadFilter/1/1/24       2536 ns         2529 ns       276751
 * BM_BiquadFilter/1/1/25       2176 ns         2170 ns       322560
 * BM_BiquadFilter/1/1/26       3259 ns         3250 ns       215389
 * BM_BiquadFilter/1/1/27       2175 ns         2169 ns       322640
 * BM_BiquadFilter/1/1/28       2536 ns         2529 ns       276786
 * BM_BiquadFilter/1/1/29       2176 ns         2170 ns       322533
 * BM_BiquadFilter/1/1/30       3260 ns         3251 ns       215325
 * BM_BiquadFilter/1/1/31       2177 ns         2171 ns       322425
 * BM_BiquadFilter/1/2/1        1471 ns         1467 ns       477222
 * BM_BiquadFilter/1/2/2        1109 ns         1107 ns       632565
 * BM_BiquadFilter/1/2/3        1298 ns         1294 ns       541051
 * BM_BiquadFilter/1/2/4        1682 ns         1678 ns       417354
 * BM_BiquadFilter/1/2/5        2136 ns         2129 ns       328967
 * BM_BiquadFilter/1/2/6        1572 ns         1568 ns       446095
 * BM_BiquadFilter/1/2/7        1792 ns         1788 ns       399598
 * BM_BiquadFilter/1/2/8        4333 ns         4320 ns       162032
 * BM_BiquadFilter/1/2/9        3807 ns         3797 ns       184097
 * BM_BiquadFilter/1/2/10       3842 ns         3831 ns       182711
 * BM_BiquadFilter/1/2/11       5062 ns         5048 ns       138636
 * BM_BiquadFilter/1/2/12       5050 ns         5036 ns       139031
 * BM_BiquadFilter/1/2/13       4335 ns         4323 ns       161943
 * BM_BiquadFilter/1/2/14       6500 ns         6483 ns       108020
 * BM_BiquadFilter/1/2/15       4336 ns         4324 ns       161873
 * BM_BiquadFilter/1/2/16       2966 ns         2957 ns       236709
 * BM_BiquadFilter/1/2/17       4359 ns         4348 ns       160581
 * BM_BiquadFilter/1/2/18       3196 ns         3187 ns       219532
 * BM_BiquadFilter/1/2/19       2805 ns         2798 ns       250157
 * BM_BiquadFilter/1/2/20       3839 ns         3829 ns       182628
 * BM_BiquadFilter/1/2/21       5291 ns         5276 ns       131153
 * BM_BiquadFilter/1/2/22       3994 ns         3984 ns       175699
 * BM_BiquadFilter/1/2/23       3757 ns         3747 ns       186864
 * BM_BiquadFilter/1/2/24       5060 ns         5046 ns       138754
 * BM_BiquadFilter/1/2/25       4343 ns         4331 ns       161614
 * BM_BiquadFilter/1/2/26       6512 ns         6491 ns       107852
 * BM_BiquadFilter/1/2/27       4348 ns         4336 ns       161419
 * BM_BiquadFilter/1/2/28       5068 ns         5055 ns       138481
 * BM_BiquadFilter/1/2/29       4386 ns         4374 ns       159635
 * BM_BiquadFilter/1/2/30       6514 ns         6498 ns       107752
 * BM_BiquadFilter/1/2/31       4353 ns         4342 ns       161227
 *******************************************************************/

template <typename F>
static void BM_BiquadFilter(benchmark::State& state, bool optimized) {
    bool isSubnormal = (state.range(0) == 1);
    const size_t channelCount = state.range(1);
    const size_t occupancy = state.range(2);

    std::vector<F> input(DATA_SIZE * channelCount);
    std::vector<F> output(DATA_SIZE * channelCount);
    std::array<F, android::audio_utils::kBiquadNumCoefs> coefs;

    // Initialize input buffer and coefs with deterministic pseudo-random values
    std::minstd_rand gen(occupancy);
    const F amplitude = isSubnormal ? std::numeric_limits<F>::min() * 0.1 : 1.;
    std::uniform_real_distribution<> dis(-amplitude, amplitude);
    for (size_t i = 0; i < DATA_SIZE * channelCount; ++i) {
        input[i] = dis(gen);
    }
    for (size_t i = 0; i < coefs.size(); ++i) {
        coefs[i] = (occupancy >> i & 1) * REF_COEFS[i];
    }

    android::audio_utils::BiquadFilter<F> biquadFilter(channelCount, coefs, optimized);

    // Run the test
    while (state.KeepRunning()) {
        benchmark::DoNotOptimize(input.data());
        benchmark::DoNotOptimize(output.data());
        biquadFilter.process(output.data(), input.data(), DATA_SIZE);
        benchmark::ClobberMemory();
    }
    state.SetComplexityN(state.range(1));  // O(channelCount)
}

static void BM_BiquadFilterFloatOptimized(benchmark::State& state) {
    BM_BiquadFilter<float>(state, true /* optimized */);
}

static void BM_BiquadFilterFloatNonOptimized(benchmark::State& state) {
    BM_BiquadFilter<float>(state, false /* optimized */);
}

static void BM_BiquadFilterDoubleOptimized(benchmark::State& state) {
    BM_BiquadFilter<double>(state, true /* optimized */);
}

static void BM_BiquadFilterDoubleNonOptimized(benchmark::State& state) {
    BM_BiquadFilter<double>(state, false /* optimized */);
}

static void BiquadFilterQuickArgs(benchmark::internal::Benchmark* b) {
    constexpr int CHANNEL_COUNT_BEGIN = 1;
    constexpr int CHANNEL_COUNT_END = 24;
    for (int k = 0; k < 1; k++) { // 0 for normal random data, 1 for subnormal random data
        for (int i = CHANNEL_COUNT_BEGIN; i <= CHANNEL_COUNT_END; ++i) {
            int j = (1 << android::audio_utils::kBiquadNumCoefs) - 1; // Full
            b->Args({k, i, j});
        }
    }
}

static void BiquadFilterFullArgs(benchmark::internal::Benchmark* b) {
    constexpr int CHANNEL_COUNT_BEGIN = 1;
    constexpr int CHANNEL_COUNT_END = 4;
    for (int k = 0; k < 2; k++) { // 0 for normal random data, 1 for subnormal random data
        for (int i = CHANNEL_COUNT_BEGIN; i <= CHANNEL_COUNT_END; ++i) {
            for (int j = 1; j < (1 << android::audio_utils::kBiquadNumCoefs); ++j) { // Occupancy
                b->Args({k, i, j});
            }
        }
    }
}

static void BiquadFilterDoubleArgs(benchmark::internal::Benchmark* b) {
    constexpr int CHANNEL_COUNT_BEGIN = 1;
    constexpr int CHANNEL_COUNT_END = 4;
    for (int k = 0; k < 1; k++) { // 0 for normal random data, 1 for subnormal random data
        for (int i = CHANNEL_COUNT_BEGIN; i <= CHANNEL_COUNT_END; ++i) {
            int j = (1 << android::audio_utils::kBiquadNumCoefs) - 1; // Full
            b->Args({k, i, j});
        }
    }
}

BENCHMARK(BM_BiquadFilterDoubleOptimized)->Apply(BiquadFilterDoubleArgs);
BENCHMARK(BM_BiquadFilterDoubleNonOptimized)->Apply(BiquadFilterDoubleArgs);
BENCHMARK(BM_BiquadFilterFloatOptimized)->Apply(BiquadFilterQuickArgs);
BENCHMARK(BM_BiquadFilterFloatNonOptimized)->Apply(BiquadFilterQuickArgs);
BENCHMARK(BM_BiquadFilterFloatOptimized)->Apply(BiquadFilterFullArgs);

BENCHMARK_MAIN();
