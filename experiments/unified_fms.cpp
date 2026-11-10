#include "fms_experiment.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
using namespace amskt_experiment;
namespace fs=std::filesystem;
struct Config{int trial;uint64_t seed,noise;size_t m;double activity;std::string suite;};
struct Edge{uint32_t group,entity;};
struct Metric{double aare=0,empty_raw=0,empty_final=0,wape=0;size_t active=0,empty=0;};
Metric metric(const std::vector<double>&out,const std::vector<uint32_t>&truth){
 Metric v;double total=0;for(size_t g=0;g<truth.size();++g){const double x=std::max(0.,out[g]);if(truth[g]){v.aare+=std::abs(x-truth[g])/truth[g];++v.active;}else{v.empty_raw+=std::abs(out[g]);v.empty_final+=x;++v.empty;}v.wape+=std::abs(x-truth[g]);total+=truth[g];}
 if(v.active)v.aare/=v.active;if(v.empty){v.empty_raw/=v.empty;v.empty_final/=v.empty;}v.wape/=total;return v;
}
void row(std::ofstream&file,const Config&cfg,const std::string&dataset,const std::string&method,double eps,const std::vector<double>&out,const std::vector<uint32_t>&truth,size_t w,size_t m,size_t K){
 auto v=metric(out,truth);file<<cfg.suite<<','<<dataset<<','<<cfg.trial<<','<<cfg.seed<<','<<cfg.noise<<','<<cfg.activity<<','<<8*w*m<<','<<eps<<','<<method<<','<<w<<','<<m<<','<<K<<','<<v.aare<<','<<v.empty_raw<<','<<v.empty_final<<','<<v.wape<<','<<v.active<<','<<v.empty<<'\n';
}
std::vector<Config>configs(const fs::path&path,int reps){std::ifstream f(path);if(!f)throw std::runtime_error("missing configuration");std::string line;std::getline(f,line);std::vector<Config>out;while(std::getline(f,line)){std::istringstream s(line);std::string v;std::vector<std::string>fields;while(std::getline(s,v,','))fields.push_back(v);if(fields.size()!=6)throw std::runtime_error("bad config");Config c{std::stoi(fields[0]),std::stoull(fields[1]),std::stoull(fields[2]),std::stoull(fields[3]),std::stod(fields[4]),fields[5]};if(c.trial<reps)out.push_back(c);}return out;}
const char*header="suite,dataset,trial,seed,noise_seed,activity_probability,budget_bytes,epsilon,method,w,m,K,aare,empty_raw_mae,empty_final_mae,full_wape,active_groups,empty_groups\n";
int main(int argc,char**argv)try{
 if(argc<6)throw std::runtime_error("unified_fms compare|ablation CONFIG OUT REPS THREADS [KASANDR_EDGES]");
 const bool ablation=std::string(argv[1])=="ablation";fs::path root=argv[3];fs::create_directories(root);int reps=std::stoi(argv[4]),threads=std::stoi(argv[5]);auto cfgs=configs(argv[2],reps);
 std::vector<Edge>real;if(ablation){if(argc<7)throw std::runtime_error("KASANDR edges needed");std::ifstream in(argv[6],std::ios::binary);if(!in)throw std::runtime_error("missing real input");real.resize(fs::file_size(argv[6])/8);in.read(reinterpret_cast<char*>(real.data()),real.size()*8);}
 Inverses inv;auto add=[&](size_t r){if(!inv.count(r))inv[r]=std::make_shared<Inverse>(r);};for(auto c:cfgs){add(c.m/32);add(c.m);add(1024*c.m);add(8*512*c.m/(4*10000));}add(64);add(128);add(256);add(512);
 std::atomic<size_t>next{0},done{0};std::mutex mu;auto worker=[&]{try{for(;;){size_t task=next++;if(task>=cfgs.size())break;auto cfg=cfgs[task];fs::path final=root/("run_"+std::to_string(task)+".csv");if(fs::exists(final)){++done;continue;}auto start=std::chrono::steady_clock::now();std::ofstream out(final.string()+".partial");out<<std::setprecision(17)<<header;
 if(!ablation){const size_t N=10000,w=512,m=cfg.m;const size_t cw=512*m/128;FiveSketches sketches(N,w,m,cfg.seed,cw,128);std::mt19937_64 rng(mix64(cfg.seed^0x4143544956495459ULL));std::bernoulli_distribution active(cfg.activity);std::vector<uint32_t>truth(N);for(auto&v:truth)v=active(rng)?10000:0;
  for(size_t g=0;g<N;++g)for(uint32_t u=0;u<truth[g];++u)sketches.update(g*10000+u,g);
  auto result=sketches.release(inv,1,cfg.noise);auto half=sketches.release(inv,.5,cfg.noise);
  const bool groups=cfg.suite=="activity"||m==512;std::ofstream group;if(groups){group.open(root/("groups_"+std::to_string(task)+".csv"));group<<std::setprecision(17)<<"method,group,truth,np,dp1,dp05\n";}
  for(const auto&[method,v]:result){row(out,cfg,"Synthetic-10K",method,0,v.np,truth,w,m,sketches.K);row(out,cfg,"Synthetic-10K",method,1,v.dp,truth,w,m,sketches.K);if(cfg.suite=="memory")row(out,cfg,"Synthetic-10K",method,.5,half.at(method).dp,truth,w,m,sketches.K);if(groups)for(size_t g=0;g<N;++g)group<<method<<','<<g<<','<<truth[g]<<','<<v.np[g]<<','<<v.dp[g]<<','<<half.at(method).dp[g]<<'\n';}
 }else{const bool isreal=cfg.suite=="ablation_real";const size_t N=isreal?3000:10000,w=isreal?1024:512,m=isreal?256:512;FiveSketches state(N,w,m,cfg.seed);std::vector<uint32_t>rp(w*m),rc(w*m),truth(N);std::vector<int8_t>codes(N*state.K);std::mt19937_64 code_rng(mix64(cfg.seed^0x434f4445424f4f4bULL));for(auto&v:codes)v=(code_rng()&1)?1:-1;
  auto update=[&](size_t g,uint64_t entity){uint64_t salt=mix64(0x47524f555053414cULL^g),hr=mix64(entity^salt^mix64(cfg.seed^0x524547ULL)),hb=mix64(entity^salt^mix64(cfg.seed^0x52414e4bULL));size_t j=hr%m,k=j/state.s;uint32_t bit=uint32_t(1)<<rank(hb);(sign(g/w,k)>0?state.ap:state.ac)[(g%w)*m+j]|=bit;(codes[g*state.K+k]>0?rp:rc)[(g%w)*m+j]|=bit;};
  if(isreal)for(auto e:real){update(e.group,e.entity);++truth[e.group];}else for(size_t g=0;g<N;++g){truth[g]=10000;for(size_t u=0;u<10000;++u)update(g,g*10000+u);}
  std::vector<double>noise_p(w*state.K),noise_c(w*state.K);std::mt19937_64 nr(mix64(cfg.noise^0x414d53));for(size_t i=0;i<noise_p.size();++i){noise_p[i]=laplace(nr,1);noise_c[i]=laplace(nr,1);}
  std::ofstream group(root/("groups_"+std::to_string(task)+".csv"));group<<std::setprecision(17)<<"method,epsilon,group,truth,estimate\n";
  for(bool dp:{false,true})for(int variant=0;variant<3;++variant){std::vector<double>est(N);const auto&plus=variant==1?rp:state.ap;const auto&minus=variant==1?rc:state.ac;
   for(size_t r=0;r<w;++r){std::vector<double>p(state.K),c(state.K),difference(state.K);for(size_t k=0;k<state.K;++k){for(size_t j=0;j<state.s;++j){p[k]+=zeros(plus[r*m+k*state.s+j]);c[k]+=zeros(minus[r*m+k*state.s+j]);}if(dp){p[k]+=noise_p[r*state.K+k];c[k]+=noise_c[r*state.K+k];}p[k]=std::clamp(p[k],0.,32.*state.s);c[k]=std::clamp(c[k],0.,32.*state.s);difference[k]=(*inv.at(state.s))(p[k])-(*inv.at(state.s))(c[k]);}
    for(size_t g=r;g<N;g+=w){double v=0,zp=0,zc=0;for(size_t k=0;k<state.K;++k){const int h=variant==1?codes[g*state.K+k]:sign(g/w,k);v+=h*difference[k];zp+=h>0?p[k]:c[k];zc+=h>0?c[k]:p[k];}est[g]=variant==2?(*inv.at(m))(zp)-(*inv.at(m))(zc):v;}
   }
   const std::string name=variant==0?"AMSkt":variant==1?"Random assignment":"Aggregate-first";row(out,cfg,isreal?"KASANDR":"Synthetic-10K",name,dp?1:0,est,truth,w,m,state.K);for(size_t g=0;g<N;++g)group<<name<<','<<dp<<','<<g<<','<<truth[g]<<','<<est[g]<<'\n';
  }
 }
 out.close();fs::rename(final.string()+".partial",final);std::lock_guard lock(mu);std::cout<<"DONE "<<++done<<'/'<<cfgs.size()<<" task="<<task<<" suite="<<cfg.suite<<" trial="<<cfg.trial<<" seconds="<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<std::endl;
 }}catch(const std::exception&e){std::lock_guard lock(mu);std::cerr<<"WORKER_ERROR "<<e.what()<<std::endl;std::exit(2);}};std::vector<std::thread>pool;for(int t=0;t<threads;++t)pool.emplace_back(worker);for(auto&t:pool)t.join();return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
