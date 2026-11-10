#pragma once
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace amskt_experiment {
inline uint64_t mix64(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27U)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31U);
}
inline int sign(size_t a, size_t k) { return (std::popcount(a & k) & 1) ? -1 : 1; }
inline double laplace(std::mt19937_64& rng, double scale) {
    double u = std::generate_canonical<double, 53>(rng) - 0.5;
    u = std::clamp(u, -0.5 + 0x1p-53, 0.5 - 0x1p-53);
    return -scale * std::copysign(1.0, u) * std::log1p(-2 * std::abs(u));
}
inline unsigned rank(uint64_t h) { return std::min(31u, unsigned(std::countr_zero(h))); }

class Inverse {
    size_t registers_;
    std::vector<double> counts_, zeros_;
public:
    static constexpr double cap = 1e8;
    static constexpr size_t points = 200001;
    explicit Inverse(size_t registers): registers_(registers), counts_(points), zeros_(points) {
        if (!registers) throw std::runtime_error("zero registers");
        for (size_t i=0; i<points; ++i) {
            const double c = std::expm1(std::log1p(cap)*i/(points-1));
            counts_[i] = c;
            zeros_[i] = expectation(c);
        }
    }
    double expectation(double c) const {
        double z=0;
        for (unsigned x=0; x<32; ++x) {
            const double q = std::ldexp(1.0, -int(std::min(x+1,31u)));
            z += std::exp(c * std::log1p(-q/registers_));
        }
        return registers_*z;
    }
    double operator()(double z) const {
        z=std::clamp(z,0.0,32.0*registers_);
        if (z>=zeros_.front()) return 0;
        if (z<=zeros_.back()) return cap;
        auto it=std::lower_bound(zeros_.begin(),zeros_.end(),z,std::greater<double>());
        size_t b=it-zeros_.begin(), a=b-1;
        return counts_[a]+(zeros_[a]-z)/(zeros_[a]-zeros_[b])*(counts_[b]-counts_[a]);
    }
};
using Inverses = std::map<size_t,std::shared_ptr<const Inverse>>;
inline double zeros(uint32_t word) { return 32-std::popcount(word); }

struct Estimates { std::vector<double> np, dp; };

// Fixed layouts are constructed using the complete public directory.
class FiveSketches {
public:
    size_t N,w,m,K,s,R,vrows;
    size_t vM=0,rM=0;
    uint64_t seed;
    std::vector<uint32_t> ap,ac,rp,rc,v,cms,ind;
    std::vector<uint16_t> vmap;
    std::vector<size_t> rrow,crow0,crow1;
    FiveSketches(size_t groups,size_t rows,size_t regs,uint64_t public_seed)
      : N(groups),w(rows),m(regs),K(std::bit_ceil((groups+rows-1)/rows)),s(regs/K),
        R((8*rows*regs)/(4*groups)),vrows(2*rows),seed(public_seed),
        ap(w*m),ac(w*m),rp(w*m),rc(w*m),v(vrows*m),cms(2*w*m),ind(N*R),
        vmap(N*m),rrow(N),crow0(N),crow1(N) {
        if (!s || m%K || N>w*K || vrows>65535) throw std::runtime_error("invalid layout");
        std::vector<unsigned> vc(vrows*m),rr(w);
        for (size_t g=0;g<N;++g) {
            rrow[g]=mix64(seed ^ 0x726f77ULL ^ g)%w;
            ++rr[rrow[g]];
            crow0[g]=mix64(seed ^ 0x636d7330ULL ^ g)%w;
            crow1[g]=mix64(seed ^ 0x636d7331ULL ^ g)%w;
            for(size_t j=0;j<m;++j) {
                size_t row=mix64(seed ^ mix64(0x76726f77ULL+j) ^ g)%vrows;
                vmap[g*m+j]=static_cast<uint16_t>(row);
                vM=std::max(vM,size_t(++vc[j*vrows+row]));
            }
        }
        rM=*std::max_element(rr.begin(),rr.end());
    }
    void update(uint64_t entity,size_t g) {
        const uint64_t salt=mix64(0x47524f555053414cULL ^ g);
        const uint64_t hr=mix64(entity ^ salt ^ mix64(seed ^ 0x524547ULL));
        const uint64_t hb=mix64(entity ^ salt ^ mix64(seed ^ 0x52414e4bULL));
        const size_t j=hr%m,k=j/s;
        const uint32_t bit=uint32_t{1}<<rank(hb);
        (sign(g/w,k)>0?ap:ac)[(g%w)*m+j]|=bit;
        const bool positive=(mix64(seed ^ 0x727369676eULL ^ (g*m+j))&1)==0;
        (positive?rp:rc)[rrow[g]*m+j]|=bit;
        v[j*vrows+vmap[g*m+j]]|=bit;
        cms[crow0[g]*m+j]|=bit;
        cms[(w+crow1[g])*m+j]|=bit;
        ind[g*R+hr%R]|=bit;
    }
    // Every NP/DP pair reuses an identical bitmap. Noise is generated once
    // for the complete published coordinate vector, including inactive groups.
    std::map<std::string,Estimates> release(const Inverses& inv, double epsilon=1.0) const {
        if (!(epsilon>0) || !std::isfinite(epsilon)) throw std::runtime_error("invalid epsilon");
        std::map<std::string,Estimates> out;
        for(auto name:{"AMSkt","vSkt","rSkt","CMSkt","Independent FMS"})
            out[name]={std::vector<double>(N),std::vector<double>(N)};
        auto rng=[&](uint64_t salt){return std::mt19937_64(mix64(seed^salt));};
        auto ar=rng(0x414d53), vr=rng(0x76536b74), rr=rng(0x72536b74), cr=rng(0x434d53), ir=rng(0x494e44);
        for(size_t row=0;row<w;++row) {
            std::vector<double> d(K),dd(K);
            for(size_t k=0;k<K;++k) {
                double p=0,c=0;
                for(size_t j=0;j<s;++j) {p+=zeros(ap[row*m+k*s+j]);c+=zeros(ac[row*m+k*s+j]);}
                d[k]=(*inv.at(s))(p)-(*inv.at(s))(c);
                dd[k]=(*inv.at(s))(p+laplace(ar,1/epsilon))-(*inv.at(s))(c+laplace(ar,1/epsilon));
            }
            for(size_t g=row;g<N;g+=w) {
                double a=0,b=0;
                for(size_t k=0;k<K;++k) {a+=sign(g/w,k)*d[k];b+=sign(g/w,k)*dd[k];}
                out["AMSkt"].np[g]=a;out["AMSkt"].dp[g]=b;
            }
        }
        double allz=0;
        for(auto word:v) allz+=zeros(word);
        const double vg=(*inv.at(vrows*m))(allz)/vrows;
        const double vgd=(*inv.at(vrows*m))(allz+laplace(vr,(vM+1)/epsilon))/vrows;
        for(size_t g=0;g<N;++g) {
            double vz=0,p=0,c=0,iz=0;
            for(size_t j=0;j<m;++j) {
                vz+=zeros(v[j*vrows+vmap[g*m+j]]);
                bool pos=(mix64(seed ^ 0x727369676eULL ^ (g*m+j))&1)==0;
                p+=zeros((pos?rp:rc)[rrow[g]*m+j]);
                c+=zeros((pos?rc:rp)[rrow[g]*m+j]);
            }
            out["vSkt"].np[g]=(*inv.at(m))(vz)-vg;
            out["vSkt"].dp[g]=(*inv.at(m))(vz+laplace(vr,(vM+1)/epsilon))-vgd;
            out["rSkt"].np[g]=(*inv.at(m))(p)-(*inv.at(m))(c);
            out["rSkt"].dp[g]=(*inv.at(m))(p+laplace(rr,rM/epsilon))-(*inv.at(m))(c+laplace(rr,rM/epsilon));
            for(size_t j=0;j<R;++j) iz+=zeros(ind[g*R+j]);
            out["Independent FMS"].np[g]=(*inv.at(R))(iz);
            out["Independent FMS"].dp[g]=(*inv.at(R))(iz+laplace(ir,1/epsilon));
        }
        std::vector<double> ce(2*w),ced(2*w);
        for(size_t bucket=0;bucket<2*w;++bucket) {
            double z=0;for(size_t j=0;j<m;++j)z+=zeros(cms[bucket*m+j]);
            ce[bucket]=(*inv.at(m))(z);ced[bucket]=(*inv.at(m))(z+laplace(cr,2/epsilon));
        }
        for(size_t g=0;g<N;++g) {
            out["CMSkt"].np[g]=std::min(ce[crow0[g]],ce[w+crow1[g]]);
            out["CMSkt"].dp[g]=std::min(ced[crow0[g]],ced[w+crow1[g]]);
        }
        return out;
    }
    void merge(const FiveSketches& other) {
        if(N!=other.N || w!=other.w || m!=other.m || seed!=other.seed)throw std::runtime_error("merge layout");
        for(auto pair:{std::pair{&ap,&other.ap},{&ac,&other.ac},{&rp,&other.rp},{&rc,&other.rc},{&v,&other.v},{&cms,&other.cms},{&ind,&other.ind}})
            for(size_t i=0;i<pair.first->size();++i)(*pair.first)[i]|=(*pair.second)[i];
    }
};
}
