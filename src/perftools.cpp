#include "cororing/perftools.hpp"

#include <cmath>

namespace cororing {

void perf_sampler::begin_sample()
{
    last_ = clock::now();
}

void perf_sampler::end_sample()
{
    duration dt = clock::now() - last_;

    min_ = std::min(dt, min_);
    max_ = std::max(dt, max_);

    samples_ += 1;
    sum_ += dt.count();
    sum_squares_ += dt.count() * dt.count();
}

perf_sampler::duration perf_sampler::get_min() const
{
    return min_;
}

perf_sampler::duration perf_sampler::get_max() const
{
    return max_;
}

perf_sampler::duration perf_sampler::get_mean() const
{
    return duration(sum_ / samples_);
}

perf_sampler::duration perf_sampler::get_std() const
{
    float a0 = static_cast<float>(samples_ * sum_squares_ - sum_ * sum_);
    float a1 = samples_ * (samples_ - 1);

    duration::rep std = static_cast<duration::rep>(std::sqrt(a0 / a1));

    return duration(std);
}

}
