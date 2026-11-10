#include "fms_experiment.hpp"
#include <iostream>
#include <set>
using namespace amskt_experiment;

void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
bool same_state(const FiveSketches& a, const FiveSketches& b) {
    return a.ap==b.ap && a.ac==b.ac && a.rp==b.rp && a.rc==b.rc
        && a.v==b.v && a.cms==b.cms && a.ind==b.ind;
}
int main() try {
    for (size_t a=0; a<32; ++a) for (size_t b=0; b<32; ++b) {
        int dot=0;
        for (size_t k=0; k<32; ++k) dot += sign(a,k)*sign(b,k);
        require(dot==(a==b ? 32 : 0), "orthogonality");
    }
    FiveSketches all(256,64,64,123), p(256,64,64,123), q(256,64,64,123);
    require(all.K==4 && all.s==16 && all.R==32, "layout");
    for (uint64_t u=0; u<10000; ++u) {
        const size_t g=u%256;
        all.update(u,g);
        (u%2 ? p : q).update(u,g);
    }
    p.merge(q);
    require(same_state(all,p), "merge equivalence");
    auto old=all;
    for (uint64_t u=0; u<10000; ++u) all.update(u,u%256);
    require(same_state(old,all), "duplicate elimination");
    old=all;
    all.update(10001,3);
    size_t changed=0;
    for (size_t i=0; i<all.ap.size(); ++i) {
        changed += std::abs(zeros(all.ap[i])-zeros(old.ap[i]));
        changed += std::abs(zeros(all.ac[i])-zeros(old.ac[i]));
    }
    require(changed<=1, "one-membership zero-statistic change");
    bool rejected=false;
    try { FiveSketches incompatible(256,64,64,124); all.merge(incompatible); }
    catch (const std::runtime_error&) { rejected=true; }
    require(rejected,"incompatible merge rejection");

    Inverses inverse;
    for (size_t n : std::set<size_t>{all.s,all.m,all.R,all.vrows*all.m,all.cm}) {
        auto inv=std::make_shared<Inverse>(n);
        require((*inv)(32.0*n)==0, "empty inverse endpoint");
        require((*inv)(-1)==Inverse::cap, "saturation endpoint");
        for (double c : {0.1,1.0,100.0,10000.0,1e6})
            require(std::abs((*inv)(inv->expectation(c))-c)<std::max(1e-5,1e-6*c), "inverse consistency");
        inverse[n]=inv;
    }
    auto r=all.release(inverse,1,999);
    auto again=all.release(inverse,1,999);
    require(r.size()==5,"five methods");
    for (const auto& [name,x] : r) {
        require(x.np.size()==256 && x.dp.size()==256,"full-directory outputs");
        require(x.np==again.at(name).np && x.dp==again.at(name).dp,"repeatable experiment noise");
    }
    rejected=false;
    try { all.release(inverse,0,999); }
    catch (const std::runtime_error&) { rejected=true; }
    require(rejected,"invalid epsilon rejection");
    std::cout << "CORE_PROPERTIES_PASSED\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
