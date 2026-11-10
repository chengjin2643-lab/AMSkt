#include "fms_experiment.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>

using namespace amskt_experiment;

bool same_state(const FiveSketches& a, const FiveSketches& b) {
    return a.ap == b.ap && a.ac == b.ac && a.rp == b.rp && a.rc == b.rc
        && a.v == b.v && a.cms == b.cms && a.ind == b.ind;
}

int main(int argc, char** argv) try {
    if (argc > 2) throw std::runtime_error("usage: amskt_demo [summary.csv]");
    constexpr size_t N = 256, w = 64, m = 64;
    constexpr uint64_t public_seed = 2026, noise_seed = 10112026;
    constexpr double epsilon = 1.0;
    FiveSketches direct(N, w, m, public_seed);
    FiveSketches left(N, w, m, public_seed), right(N, w, m, public_seed);
    std::vector<uint32_t> truth(N);
    std::mt19937_64 input(20261110);
    std::bernoulli_distribution active(0.1);
    size_t nonempty = 0;
    for (size_t g = 0; g < N; ++g) {
        truth[g] = active(input) ? 2000 : 0;
        nonempty += truth[g] > 0;
        for (uint64_t u = 0; u < truth[g]; ++u) {
            const uint64_t entity = g * 2000 + u;
            direct.update(entity, g);
            (u % 2 ? right : left).update(entity, g);
            if (u % 7 == 0) {
                left.update(entity, g);
                right.update(entity, g);
            }
        }
    }
    if (nonempty == 0 || nonempty == N) throw std::runtime_error("invalid example input");
    left.merge(right);
    if (!same_state(left, direct)) throw std::runtime_error("merged/direct state mismatch");

    Inverses inverse;
    for (size_t count : std::set<size_t>{direct.s, direct.m, direct.vrows * direct.m,
                                        direct.R, direct.cm}) {
        inverse[count] = std::make_shared<Inverse>(count);
    }
    auto estimates = left.release(inverse, epsilon, noise_seed);
    auto reference = direct.release(inverse, epsilon, noise_seed);
    std::ofstream file;
    if (argc == 2) {
        std::filesystem::path path(argv[1]);
        if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
        file.open(path);
        if (!file) throw std::runtime_error("cannot open summary output");
    }
    const auto header = "method,active_groups,bitmap_budget_bytes,epsilon,nonprivate_aare,private_aare,inactive_mae\n";
    std::cout << header << std::setprecision(10);
    if (file) file << header << std::setprecision(10);
    for (const auto& [method, counts] : estimates) {
        if (counts.np != reference.at(method).np || counts.dp != reference.at(method).dp)
            throw std::runtime_error("merged/direct release mismatch");
        double np = 0, dp = 0, empty = 0;
        for (size_t g = 0; g < N; ++g) {
            const double a = std::max(0.0, counts.np[g]);
            const double b = std::max(0.0, counts.dp[g]);
            if (!std::isfinite(a) || !std::isfinite(b)) throw std::runtime_error("nonfinite count");
            if (truth[g]) {
                np += std::abs(a - truth[g]) / truth[g];
                dp += std::abs(b - truth[g]) / truth[g];
            } else {
                empty += b;
            }
        }
        auto write = [&](std::ostream& out) {
            out << method << ',' << nonempty << ',' << 8*w*m << ',' << epsilon << ','
                << np/nonempty << ',' << dp/nonempty << ',' << empty/(N-nonempty) << '\n';
        };
        write(std::cout);
        if (file) write(file);
    }
    std::cerr << "Merged state and matched-noise releases equal direct construction.\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
