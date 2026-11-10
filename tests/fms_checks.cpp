#define amskt_experiment original_engine
#include "fms_original.hpp"
#undef amskt_experiment
#include "fms_experiment.hpp"
#include <iostream>
int main(){
 const size_t N=200,w=64,m=64;const uint64_t seed=456;
 original_engine::FiveSketches a(N,w,m,seed);
 amskt_experiment::FiveSketches b(N,w,m,seed),tuned(N,w,m,seed,128,32);
 std::vector<uint32_t> expected(tuned.cms.size());
 for(size_t u=0;u<20000;++u){size_t g=u%N;a.update(u,g);b.update(u,g);tuned.update(u,g);
  using namespace amskt_experiment;uint64_t salt=mix64(0x47524f555053414cULL^g),hr=mix64(u^salt^mix64(seed^0x524547ULL)),hb=mix64(u^salt^mix64(seed^0x52414e4bULL));
  uint32_t bit=uint32_t(1)<<rank(hb);size_t r0=mix64(seed^0x636d7330ULL^g)%128,r1=mix64(seed^0x636d7331ULL^g)%128;
  expected[r0*32+hr%32]|=bit;expected[(128+r1)*32+hr%32]|=bit;
 }
 if(a.ap!=b.ap||a.ac!=b.ac||a.rp!=b.rp||a.rc!=b.rc||a.v!=b.v||a.cms!=b.cms||a.ind!=b.ind)throw std::runtime_error("original state mismatch");
 if(tuned.ap!=b.ap||tuned.ac!=b.ac||tuned.rp!=b.rp||tuned.rc!=b.rc||tuned.v!=b.v||tuned.ind!=b.ind||tuned.cms!=expected)throw std::runtime_error("tuning changed non-CMS state");
 original_engine::Inverses ia;amskt_experiment::Inverses ib;
 for(size_t r:{size_t(16),size_t(64),size_t(8192),size_t(40)}){ia[r]=std::make_shared<original_engine::Inverse>(r);ib[r]=std::make_shared<amskt_experiment::Inverse>(r);}
 const auto ra=a.release(ia);const auto rb=b.release(ib);for(const auto&[name,x]:ra)if(x.np!=rb.at(name).np||x.dp!=rb.at(name).dp)throw std::runtime_error("default release mismatch");
 // The ablation's explicitly paired noise arrays reproduce the main release.
 const auto paired=b.release(ib,1,678);
 std::mt19937_64 noise(amskt_experiment::mix64(678^0x414d53));
 for(size_t row=0;row<w;++row){
  std::vector<double> d(b.K);
  for(size_t k=0;k<b.K;++k){double p=0,c=0;for(size_t j=0;j<b.s;++j){p+=amskt_experiment::zeros(b.ap[row*m+k*b.s+j]);c+=amskt_experiment::zeros(b.ac[row*m+k*b.s+j]);}
   const double eta_p=amskt_experiment::laplace(noise,1),eta_c=amskt_experiment::laplace(noise,1);
   d[k]=(*ib.at(b.s))(std::clamp(p+eta_p,0.,32.*b.s))-(*ib.at(b.s))(std::clamp(c+eta_c,0.,32.*b.s));
  }
  for(size_t g=row;g<N;g+=w){double estimate=0;for(size_t k=0;k<b.K;++k)estimate+=amskt_experiment::sign(g/w,k)*d[k];if(estimate!=paired.at("AMSkt").dp[g])throw std::runtime_error("ablation/main noise or decode mismatch");}
 }
 std::cout<<"ABLATION_MAIN_DECODING_AND_NOISE_EXACT_MATCH; ";
 std::cout<<"FMS_ORIGINAL_STATE_AND_RELEASES_EXACT_MATCH; CMS_LAYOUT_ISOLATION_PASSED\n";
}
