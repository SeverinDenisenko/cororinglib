#pragma once

#include <chrono>
#include <cstddef>
#include <limits>

/** @file */

namespace cororing {

/**
 * @brief Tool for measuring statistical properties of operation latency
 */
class perf_sampler {
public:
    using clock      = std::chrono::high_resolution_clock;
    using duration   = clock::duration;
    using time_point = clock::time_point;

    /**
     * @brief Mark begin of the meshured section
     */
    void begin_sample();

    /**
     * @brief Mark end of the meshured section, record latency
     */
    void end_sample();

    /**
     * @brief Get the minumum latency
     * 
     * @return duration Minumum latency.
     */
    duration get_min() const;

    /**
     * @brief Get the maximum latency
     * 
     * @return duration Maximum latency.
     */
    duration get_max() const;

    /**
     * @brief Get the mean for latency
     * 
     * @return duration Latency.
     */
    duration get_mean() const;

    /**
     * @brief Get standart deviation for latency
     * 
     * @return duration Standart deviation.
     */
    duration get_std() const;

private:
    time_point last_ {};
    size_t samples_ {};
    duration::rep sum_ {};
    duration::rep sum_squares_ {};
    duration min_ { std::numeric_limits<duration>::max() };
    duration max_ { std::numeric_limits<duration>::min() };
};

}
