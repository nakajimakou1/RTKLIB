#include "rtklib.h"

//#define USE_RINEX_INPUT

#ifdef USE_RINEX_INPUT
#define rcsid rcsid_rinex
#define obscodes obscodes_rinex
#include "../../src/rinex.c"
#undef rcsid
#undef obscodes
#endif

#ifdef WITHOUT_FILE
void rtkprintstat(int level, const char *str, ...){
  if(level > 1){return;}
  va_list arg;
  va_start(arg, str);
  vfprintf(stderr, str, arg);
  va_end(arg);
}

int trace_vprintf(const char *format, va_list ap){
  int res;
  res = vfprintf(stderr, format, ap);
  fflush(stderr);
  return res;
}

#ifdef USE_RINEX_INPUT
int uncompress(const char *file, char *uncfile){
  const char *p;
  trace(3, "uncompress: file=%s\n", file);
  if(!(p = strrchr(file, '.'))){return 0;}
  if(!strcmp(p, ".z")
      || !strcmp(p, ".Z")
      || !strcmp(p,".gz")
      || !strcmp(p,".GZ")
      || !strcmp(p,".zip")
      || !strcmp(p,".ZIP")
      || !strcmp(p,".tar")
      || ((strlen(p) > 3) && ((p[3] == 'd') || (p[3] == 'D')))){
    return -1;
  }
  return 0;
}
#endif
#endif

#ifdef WITHOUT_SYSTIME
#include <time.h>
extern gtime_t timeget(){
  time_t timer;
  struct tm *tt;
  time(&timer);
  tt = gmtime(&timer);
  double ep[] = {
      tt->tm_year + 1900, tt->tm_mon + 1, tt->tm_mday,
      tt->tm_hour, tt->tm_min, tt->tm_sec};
  return epoch2time(ep);
}
unsigned int tickget(){
  return clock() * 1000 / CLOCKS_PER_SEC;
}
#endif

int main(){

  tracelevel(3); //0xF);

  nav_t nav;

  ///< @see init_raw() of rcvraw.c
  memset(&nav, 0, sizeof(nav_t));

  if(nav.eph = (eph_t *)calloc(MAXSAT * 2, sizeof(eph_t))){
    nav.nmax = MAXSAT * 2;
  }
  if(nav.seph = (seph_t *)calloc(NSATSBS * 2, sizeof(seph_t))){
    nav.ns = nav.nsmax = NSATSBS * 2;
  }

  {
    const double lam_gnss[] = {
        CLIGHT / FREQ1, CLIGHT / FREQ2, CLIGHT / FREQ5};
    const double lam_glo[] = {
        CLIGHT / FREQ1_GLO, CLIGHT / FREQ2_GLO};
    int i;
    for(i = 0; i < sizeof(nav.lam) / sizeof(nav.lam[0]); ++i){
      switch(satsys(i + 1, NULL)){
        case SYS_NONE: continue;
        case SYS_GLO: memcpy(nav.lam[i], lam_glo, sizeof(nav.lam[i])); break;
        default: memcpy(nav.lam[i], lam_gnss, sizeof(nav.lam[i])); break;
      }
    }
  }

  rtk_t rtk;
  prcopt_t opt = prcopt_default;
  opt.mode = PMODE_PPP_KINEMA; // PMODE_SINGLE;
  opt.dynamics = 1;
  opt.modear = ARMODE_PPPAR;

  rtkinit(&rtk, &opt);

  obsd_t obs_data[MAXOBS * 2];
  memset(obs_data, 0, sizeof(obs_data));
  obs_t obs;
  obs.n = 0;
  obs.nmax = sizeof(obs_data) / sizeof(obs_data[0]);
  obs.data = obs_data;

#ifdef USE_RINEX_INPUT
  char *t_str[] = {
      "05  4  2  2  0  0.0", "05  4  2  2  0 29.0",
      "05  4  2  0  0  0.0000000", "05  4  2  0  0 29.0000000",};
  gtime_t t[sizeof(t_str) / sizeof(t_str[0])];
  {
    int i;
    for(i = 0; i < sizeof(t) / sizeof(t[0]); i++){
      str2time(t_str[i], 0, strlen(t_str[i]), &t[i]);
    }
  }
  if(readrnxt("../../../../test/data/rinex/07590920.05n", 1, t[0], t[1], 0.0, "", NULL, &nav, NULL) == 0){
    exit(-1);
  }
  uniqnav(&nav);
  if(readrnxt("../../../../test/data/rinex/07590920.05o", 1, t[2], t[3], 0.0, "", &obs, NULL, NULL) == 0){
    exit(-1);
  }
  sortobs(&obs);
#else
	// RINEX NAV Header manual-extraction
  {
    double ion_gps[] = {
        1.1180E-08,  1.4900E-08, -5.9600E-08, -5.9600E-08,
        8.8060E+04,  1.6380E+04, -1.9660E+05, -1.3110E+05};
    int i;
    for(i = 0; i < sizeof(ion_gps) / sizeof(ion_gps[0]); i++){
      nav.ion_gps[i] = ion_gps[i];
    }
  }
  {
    char *delta_utc[] = {
        "-2.793967723850D-09", "-5.329070518200D-15", "61440", "1061"};
    int i;
    for(i = 0; i < sizeof(delta_utc) / sizeof(delta_utc[0]); i++){
      nav.utc_gps[i] = str2num(delta_utc[i], 0, strlen(delta_utc[i]));
    }
  }
  nav.leaps = 13;

  double nav_raw[][35] = { // ../../../../test/data/rinex/07590920.05n Line 13..204
      {1, 05, 4, 2, 2, 0, 0.0, 3.966595977540E-04, 1.705302565820E-12, 0.000000000000E+00, 1.400000000000E+02, -5.218750000000E+01, 4.026596389650E-09, 2.871534990340E+00, -2.676621079440E-06, 5.957618006510E-03, 4.174187779430E-06, 5.153636478420E+03, 5.256000000000E+05, 1.061707735060E-07, -2.493184817740E+00, -9.313225746150E-08, 9.833919144490E-01, 3.093750000000E+02, -1.650496813270E+00, -7.889971342930E-09, -8.571785642400E-12, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 1.000000000000E+00, 0.000000000000E+00, -3.259629011150E-09, 3.960000000000E+02, 5.195760000000E+05},
      {3, 05, 4, 2, 0, 0, 0.0, 9.673088788990E-05, 3.069544618480E-12, 0.000000000000E+00, 8.300000000000E+01, 1.968750000000E+01, 5.376652456590E-09, 2.471116819930E+00, 1.018866896630E-06, 6.735791102980E-03, 7.564201951030E-06, 5.153730749130E+03, 5.184000000000E+05, -1.005828380580E-07, 5.354931929380E-01, -6.519258022310E-08, 9.274337998890E-01, 2.158750000000E+02, 6.038989687590E-01, -8.278916219240E-09, -1.525063547670E-10, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 0.000000000000E+00, 0.000000000000E+00, -4.190951585770E-09, 5.950000000000E+02, 5.112180000000E+05},
      {3, 05, 4, 2, 2, 0, 0.0, 9.675230830910E-05, 3.069544618480E-12, 0.000000000000E+00, 8.400000000000E+01, 1.865625000000E+01, 5.365223376690E-09, -2.761934047060E+00, 9.462237358090E-07, 6.735803675840E-03, 7.532536983490E-06, 5.153730754850E+03, 5.256000000000E+05, -1.490116119380E-08, 5.354335995070E-01, -9.499490261080E-08, 9.274327714580E-01, 2.173437500000E+02, 6.039165179240E-01, -8.276773044710E-09, -1.303625679630E-10, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 0.000000000000E+00, 0.000000000000E+00, -4.190951585770E-09, 3.400000000000E+02, 5.184180000000E+05},
      {4, 05, 4, 2, 2, 0, 0.0, 3.068340010940E-04, -2.273736754430E-11, 0.000000000000E+00, 1.490000000000E+02, 8.515625000000E+01, 4.453756918820E-09, 5.712637943330E-01, 4.492700099950E-06, 7.039358024490E-03, 8.033588528630E-06, 5.153595203400E+03, 5.256000000000E+05, 1.192092895510E-07, 1.686277835030E+00, 3.911554813390E-08, 9.549858547880E-01, 2.200625000000E+02, 3.854291015260E-02, -8.025691222710E-09, -2.275094834750E-10, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 0.000000000000E+00, 0.000000000000E+00, -6.053596735000E-09, 1.490000000000E+02, 5.208780000000E+05},
      {7, 05, 4, 2, 0, 0, 0.0, -1.360527239740E-04, -3.387867764100E-11, 0.000000000000E+00, 7.300000000000E+01, 2.190625000000E+01, 5.031281169470E-09, 2.666824890220E+00, 1.093372702600E-06, 1.308864122260E-02, 7.616356015210E-06, 5.153696329120E+03, 5.184000000000E+05, 1.303851604460E-07, 5.635898717570E-01, -1.024454832080E-07, 9.365227080330E-01, 2.165000000000E+02, -1.804738833410E+00, -7.899615184210E-09, -1.746501276930E-10, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 0.000000000000E+00, 0.000000000000E+00, -2.328306436540E-09, 7.300000000000E+01, 5.161620000000E+05},
      {7, 05, 4, 2, 2, 0, 0.0, -1.362971961500E-04, -3.399236447880E-11, 0.000000000000E+00, 7.400000000000E+01, 2.246875000000E+01, 5.084854759470E-09, -2.566205020470E+00, 1.151114702220E-06, 1.308932981920E-02, 7.428228855130E-06, 5.153695041660E+03, 5.256000000000E+05, 1.695007085800E-07, 5.635327038450E-01, -4.284083843230E-08, 9.365215962150E-01, 2.222187500000E+02, -1.804722965130E+00, -7.910686328220E-09, -1.271481531170E-10, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 0.000000000000E+00, 0.000000000000E+00, -2.328306436540E-09, 7.400000000000E+01, 5.184180000000E+05},
      {8, 05, 4, 2, 0, 0, 0.0, -2.513127401470E-05, -1.023181539490E-12, 0.000000000000E+00, 1.760000000000E+02, -9.487500000000E+01, 4.230533257040E-09, 5.913789369410E-01, -5.152076482770E-06, 9.153424296530E-03, 8.422881364820E-06, 5.153750442500E+03, 5.184000000000E+05, 7.264316082000E-08, -1.439554843050E+00, -1.098960638050E-07, 9.676472556950E-01, 2.197187500000E+02, 2.536251333920E+00, -7.946402647010E-09, 1.392915227600E-10, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 0.000000000000E+00, 0.000000000000E+00, -3.725290298460E-09, 6.880000000000E+02, 5.112180000000E+05},
      {8, 05, 4, 2, 2, 0, 0.0, -2.513825893400E-05, -1.023181539490E-12, 0.000000000000E+00, 1.770000000000E+02, -9.103125000000E+01, 4.246248241910E-09, 1.641484722950E+00, -4.582107067110E-06, 9.152995422480E-03, 8.529052138330E-06, 5.153750507350E+03, 5.256000000000E+05, 7.450580596920E-08, -1.439610918170E+00, 6.705522537230E-08, 9.676486410790E-01, 2.135000000000E+02, 2.536277812730E+00, -7.788181655140E-09, 6.571702210190E-11, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 1.000000000000E+00, 0.000000000000E+00, -3.725290298460E-09, 1.770000000000E+02, 5.184180000000E+05},
      {11, 05, 4, 2, 0, 0, 0.0, 2.101357094940E-04, 3.979039320260E-12, 0.000000000000E+00, 2.240000000000E+02, 7.043750000000E+01, 5.822385240610E-09, 1.063119868670E+00, 3.591179847720E-06, 4.108081571760E-03, 7.575377821920E-06, 5.153675613400E+03, 5.184000000000E+05, 4.470348358150E-08, 1.543293829710E+00, 1.303851604460E-08, 9.022531351960E-01, 2.021875000000E+02, 2.362906364190E-01, -8.617144331420E-09, -3.150131266950E-10, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 0.000000000000E+00, 0.000000000000E+00, -1.210719347000E-08, 4.800000000000E+02, 5.112180000000E+05},
      {11, 05, 4, 2, 2, 0, 0.0, 2.101645804940E-04, 3.979039320260E-12, 0.000000000000E+00, 2.250000000000E+02, 6.296875000000E+01, 5.762025523380E-09, 2.113136624420E+00, 3.255903720860E-06, 4.107686574570E-03, 7.590278983120E-06, 5.153676961900E+03, 5.256000000000E+05, -3.352761268620E-08, 1.543231477210E+00, 5.774199962620E-08, 9.022511544050E-01, 2.005625000000E+02, 2.364631203120E-01, -8.586786393040E-09, -2.471531590500E-10, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 0.000000000000E+00, 0.000000000000E+00, -1.210719347000E-08, 4.810000000000E+02, 5.184180000000E+05},
      {15, 05, 4, 2, 0, 0, 0.0, 4.110522568230E-04, 5.343281372920E-12, 0.000000000000E+00, 1.590000000000E+02, 6.434375000000E+01, 4.409826726000E-09, 4.683908069060E-01, 3.276392817500E-06, 9.006852167660E-03, 7.579103112220E-06, 5.153564750670E+03, 5.184000000000E+05, 5.029141902920E-08, 1.740895471900E+00, -1.098960638050E-07, 9.620577178170E-01, 2.342812500000E+02, 2.369334550580E+00, -8.109980242920E-09, -1.621496131810E-10, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 1.000000000000E+00, 0.000000000000E+00, -2.328306436540E-09, 1.590000000000E+02, 5.112180000000E+05},
      {16, 05, 4, 2, 0, 0, 0.0, 1.807697117330E-06, 1.136868377220E-13, 0.000000000000E+00, 6.400000000000E+01, -8.156250000000E+00, 4.586976576350E-09, -7.656040961980E-01, -4.302710294720E-07, 2.747361431830E-03, 8.367002010350E-06, 5.153725072860E+03, 5.184000000000E+05, -7.264316082000E-08, -4.258463579130E-01, 6.891787052150E-08, 9.616409207530E-01, 2.218125000000E+02, -1.171536430550E+00, -7.927115852620E-09, 2.664396758780E-10, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 1.000000000000E+00, 0.000000000000E+00, -9.778887033460E-09, 5.760000000000E+02, 5.112180000000E+05},
      {19, 05, 4, 2, 0, 0, 0.0, -1.746229827400E-05, -9.094947017730E-13, 0.000000000000E+00, 1.420000000000E+02, 2.984375000000E+01, 4.638407435920E-09, -1.980245010040E+00, 1.588836312290E-06, 3.163279267030E-03, 7.713213562970E-06, 5.153663715360E+03, 5.184000000000E+05, 9.872019290920E-08, 6.736887601720E-01, 5.029141902920E-08, 9.595385289970E-01, 2.278750000000E+02, -1.712122044250E+00, -7.898186105140E-09, -1.821504475030E-10, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 0.000000000000E+00, 0.000000000000E+00, -1.443549990650E-08, 3.980000000000E+02, 5.112180000000E+05},
      {19, 05, 4, 2, 2, 0, 0.0, -1.746835187080E-05, -9.094947017730E-13, 0.000000000000E+00, 1.430000000000E+02, 2.784375000000E+01, 4.644836515410E-09, -9.299754254840E-01, 1.532956957820E-06, 3.163182060230E-03, 7.888302206990E-06, 5.153664176940E+03, 5.256000000000E+05, -3.911554813390E-08, 6.736314298760E-01, 8.009374141690E-08, 9.595365569830E-01, 2.295625000000E+02, -1.712204080310E+00, -8.035692111720E-09, -1.896507534350E-10, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 0.000000000000E+00, 0.000000000000E+00, -1.443549990650E-08, 3.990000000000E+02, 5.184180000000E+05},
      {20, 05, 4, 1, 23, 59, 44.0, -7.536308839920E-05, 2.273736754430E-12, 0.000000000000E+00, 7.300000000000E+01, 6.643750000000E+01, 4.570547496030E-09, -1.360403297570E+00, 3.520399332050E-06, 2.565596834760E-03, 3.626570105550E-06, 5.153752218250E+03, 5.183840000000E+05, 3.911554813390E-08, 2.705221236470E+00, 5.587935447690E-09, 9.619379867500E-01, 3.139375000000E+02, 1.398322896370E+00, -8.266415996160E-09, 6.785996764510E-11, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 0.000000000000E+00, 0.000000000000E+00, -6.984919309620E-09, 7.300000000000E+01, 5.132580000000E+05},
      {20, 05, 4, 2, 2, 0, 0.0, -7.534679025410E-05, 2.273736754430E-12, 0.000000000000E+00, 7.400000000000E+01, 5.706250000000E+01, 4.576262035980E-09, -3.085393151690E-01, 2.902001142500E-06, 2.564652008000E-03, 4.146248102190E-06, 5.153748853680E+03, 5.256000000000E+05, 1.676380634310E-08, 2.705161779090E+00, 3.911554813390E-08, 9.619385207150E-01, 3.038125000000E+02, 1.398927295120E+00, -8.240343518650E-09, 5.000208233570E-11, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 0.000000000000E+00, 0.000000000000E+00, -6.984919309620E-09, 7.400000000000E+01, 5.184180000000E+05},
      {22, 05, 4, 2, 0, 0, 0.0, 1.929607242350E-05, 7.958078640510E-13, 0.000000000000E+00, 1.390000000000E+02, 5.112500000000E+01, 4.503044603870E-09, -2.872409464250E+00, 2.697110176090E-06, 4.795031039980E-03, 4.388391971590E-06, 5.153689493180E+03, 5.184000000000E+05, 8.381903171540E-08, 2.766826397680E+00, 9.499490261080E-08, 9.596223439620E-01, 2.931250000000E+02, -1.482831898010E+00, -8.136767704060E-09, 4.928776831110E-11, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 1.000000000000E+00, 0.000000000000E+00, -1.816079020500E-08, 3.950000000000E+02, 5.112180000000E+05},
      {23, 05, 4, 2, 2, 0, 0.0, 2.059829421340E-04, -2.046363078990E-12, 0.000000000000E+00, 2.170000000000E+02, -5.781250000000E+01, 4.597334513080E-09, -2.243261010750E+00, -2.888962626460E-06, 3.750981763010E-03, 4.192814230920E-06, 5.153699323650E+03, 5.256000000000E+05, -5.215406417850E-08, -2.526289084160E+00, 4.470348358150E-08, 9.641021429070E-01, 3.020000000000E+02, 2.231894652220E+00, -8.177840626900E-09, 2.321525263690E-11, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 2.000000000000E+00, 0.000000000000E+00, -2.142041921620E-08, 4.730000000000E+02, 5.215680000000E+05},
      {24, 05, 4, 1, 23, 59, 44.0, 5.968846380710E-06, 2.955857780760E-12, 0.000000000000E+00, 4.900000000000E+01, 8.450000000000E+01, 4.263748909490E-09, 1.380203778200E+00, 4.235655069350E-06, 8.682943764140E-03, 8.061528205870E-06, 5.153602882390E+03, 5.183840000000E+05, 1.341104507450E-07, 1.716019215590E+00, -2.160668373110E-07, 9.652668319690E-01, 2.235000000000E+02, -1.177364492840E+00, -7.859256356820E-09, -2.200091636650E-10, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 0.000000000000E+00, 0.000000000000E+00, -1.396983861920E-09, 4.900000000000E+01, 5.146680000000E+05},
      {24, 05, 4, 2, 2, 0, 0.0, 5.990266799930E-06, 2.955857780760E-12, 0.000000000000E+00, 5.000000000000E+01, 8.150000000000E+01, 4.243033924210E-09, 2.432778399400E+00, 4.226341843610E-06, 8.683477994050E-03, 7.724389433860E-06, 5.153602262500E+03, 5.256000000000E+05, 1.452863216400E-07, 1.715963500360E+00, -3.911554813390E-08, 9.652654070870E-01, 2.299375000000E+02, -1.177383722900E+00, -7.744251462330E-09, -2.696540768450E-10, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 0.000000000000E+00, 0.000000000000E+00, -1.396983861920E-09, 3.060000000000E+02, 5.184180000000E+05},
      {27, 05, 4, 2, 0, 0, 0.0, 3.523472696540E-05, 7.503331289630E-12, 0.000000000000E+00, 5.000000000000E+01, -9.068750000000E+01, 4.723410995670E-09, -6.577204095410E-01, -4.580244421960E-06, 1.906045328360E-02, 8.469447493550E-06, 5.153626186370E+03, 5.184000000000E+05, -1.322478055950E-07, -1.534919492680E+00, 3.129243850710E-07, 9.525869631850E-01, 2.122187500000E+02, -2.053527948470E+00, -8.391420891480E-09, 1.289339468520E-10, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 0.000000000000E+00, 0.000000000000E+00, -4.190951585770E-09, 5.000000000000E+01, 5.112180000000E+05},
      {28, 05, 4, 2, 0, 0, 0.0, 4.686601459980E-05, -1.136868377220E-13, 0.000000000000E+00, 1.110000000000E+02, -2.118750000000E+01, 4.513045048780E-09, -1.942447522480E+00, -1.190230250360E-06, 9.983274503610E-03, 8.240342140200E-06, 5.153637123110E+03, 5.184000000000E+05, 1.061707735060E-07, -4.156684433110E-01, -1.341104507450E-07, 9.596524230210E-01, 2.193125000000E+02, -2.336744732470E+00, -7.693177650480E-09, 3.071556620160E-10, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 0.000000000000E+00, 0.000000000000E+00, -1.024454832080E-08, 1.110000000000E+02, 5.117520000000E+05},
      {28, 05, 4, 2, 2, 0, 0.0, 4.686508327720E-05, -1.136868377220E-13, 0.000000000000E+00, 1.120000000000E+02, -1.531250000000E+01, 4.687338073240E-09, -8.923215057780E-01, -7.990747690200E-07, 9.983312920670E-03, 8.249655365940E-06, 5.153635662080E+03, 5.256000000000E+05, 1.639127731320E-07, -4.157236070250E-01, 2.272427082060E-07, 9.596528838400E-01, 2.199062500000E+02, -2.336667025180E+00, -7.999975792930E-09, 1.442917313410E-10, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 0.000000000000E+00, 0.000000000000E+00, -1.024454832080E-08, 1.120000000000E+02, 5.184180000000E+05},
      {13, 05, 4, 2, 2, 0, 0.0, -7.071997970340E-06, 1.250555214940E-12, 0.000000000000E+00, 2.030000000000E+02, -6.265625000000E+01, 4.081241566920E-09, -1.395829108190E+00, -3.190711140630E-06, 2.501061768270E-03, 4.086643457410E-06, 5.153751661300E+03, 5.256000000000E+05, 5.587935447690E-09, -2.509149236050E+00, 1.490116119380E-08, 9.875983291150E-01, 3.161875000000E+02, 9.046799773120E-01, -7.977475569020E-09, 2.178662111830E-11, 1.000000000000E+00, 1.316000000000E+03, 0.000000000000E+00, 0.000000000000E+00, 0.000000000000E+00, -1.164153218270E-08, 7.150000000000E+02, 5.244360000000E+05},};
  for(nav.n = 0; nav.n < sizeof(nav_raw) / sizeof(nav_raw[0]); ++nav.n){
    eph_t eph = {0}; ///< @see decode_eph() of rinex.c
    eph.sat    = (int)nav_raw[nav.n][0];
    if(nav_raw[nav.n][1] < 100){nav_raw[nav.n][1] += (nav_raw[nav.n][1] < 80 ? 2000 : 1900);}
    eph.toc    = epoch2time(&nav_raw[nav.n][1]);
    eph.f0     = nav_raw[nav.n][7 +  0];
    eph.f1     = nav_raw[nav.n][7 +  1];
    eph.f2     = nav_raw[nav.n][7 +  2];
    eph.A      = nav_raw[nav.n][7 + 10] * nav_raw[nav.n][7 + 10];
    eph.e      = nav_raw[nav.n][7 +  8];
    eph.i0     = nav_raw[nav.n][7 + 15];
    eph.OMG0   = nav_raw[nav.n][7 + 13];
    eph.omg    = nav_raw[nav.n][7 + 17];
    eph.M0     = nav_raw[nav.n][7 +  6];
    eph.deln   = nav_raw[nav.n][7 +  5];
    eph.OMGd   = nav_raw[nav.n][7 + 18];
    eph.idot   = nav_raw[nav.n][7 + 19];
    eph.crc    = nav_raw[nav.n][7 + 16];
    eph.crs    = nav_raw[nav.n][7 +  4];
    eph.cuc    = nav_raw[nav.n][7 +  7];
    eph.cus    = nav_raw[nav.n][7 +  9];
    eph.cic    = nav_raw[nav.n][7 + 12];
    eph.cis    = nav_raw[nav.n][7 + 14];
    eph.iode   = (int)nav_raw[nav.n][7 + 3];
    eph.iodc   = (int)nav_raw[nav.n][7 + 26];
    eph.toes   = nav_raw[nav.n][7 + 11];
    eph.week   = (int)nav_raw[nav.n][7 + 21];
    eph.toe    = gpst2time(eph.week, nav_raw[nav.n][7 + 11]); //adjweek(gpst2time(eph.week, nav_raw[nav.n][7 + 11]), eph.toc);
    eph.ttr    = gpst2time(eph.week, nav_raw[nav.n][7 + 27]); //adjweek(gpst2time(eph.week, nav_raw[nav.n][7 + 27]), eph.toc);
    gtime_t *t[] = {&eph.toe, &eph.ttr};
    int i;
    for(i = 0; i < sizeof(t) / sizeof(t[0]); ++i){
      double tt = timediff(*t[i], eph.toc);
      if(tt < -302400.0){*t[i] = timeadd(*t[i],  604800.0);}
      if(tt >  302400.0){*t[i] = timeadd(*t[i], -604800.0);}
    }
    eph.code   = (int)nav_raw[nav.n][7 + 20];
    eph.svh    = (int)nav_raw[nav.n][7 + 24];

    //eph.sva    = uraindex(nav_raw[nav.n][7 + 23]);
    const double ura_eph[] = {
        2.4, 3.4, 4.85, 6.85, 9.65, 13.65, 24.0, 48.0,
        96.0, 192.0, 384.0, 768.0, 1536.0, 3072.0, 6144.0, 0.0};
    for(i = 0; i < sizeof(ura_eph) / sizeof(ura_eph[0]); i++){
      if (ura_eph[i] >= nav_raw[nav.n][7 + 23]){
        eph.sva = i;
        break;
      }
    }

    eph.flag   = (int)nav_raw[nav.n][7 + 22];
    eph.tgd[0] = nav_raw[nav.n][7 + 25];
    eph.fit    = nav_raw[nav.n][7 + 28];
    nav.eph[nav.n] = eph;
  }

  double obs_raw[][12] = { // ../../../../test/data/rinex/07590920.05n Line 18..26
      {05, 4, 2, 0, 0, 0.0000000, 0, 3, 55923622.160, 24767686.375, 43647388.2424, 24767684.8224},
      {05, 4, 2, 0, 0, 0.0000000, 0, 7, -691177.898, 24361933.475, -537007.1404, 24361930.5994},
      {05, 4, 2, 0, 0, 0.0000000, 0, 8, 17984490.035, 23407378.219, 14018464.8094, 23407374.3204},
      {05, 4, 2, 0, 0, 0.0000000, 0, 11, 7712103.227, 20311445.258, 6019854.6424, 20311439.4424},
      {05, 4, 2, 0, 0, 0.0000000, 0, 19, 36724126.590, 22613015.950, 28621450.8274, 22613010.1104},
      {05, 4, 2, 0, 0, 0.0000000, 0, 20, -5764048.758, 21565852.190, -4479034.4614, 21565847.2294},
      {05, 4, 2, 0, 0, 0.0000000, 0, 24, -2292750.457, 22276378.821, -1749426.2014, 22276375.7484},
      {05, 4, 2, 0, 0, 0.0000000, 0, 28, -5448227.324, 21543408.487, -4238014.2094, 21543403.0464},};
  for(obs.n = 0; obs.n < sizeof(obs_raw) / sizeof(obs_raw[0]); ++obs.n){
    if(obs_raw[obs.n][0] < 100){obs_raw[obs.n][0] += (obs_raw[obs.n][0] < 80 ? 2000 : 1900);}
    obs.data[obs.n].time = epoch2time(&obs_raw[obs.n][0]);
    obs.data[obs.n].rcv = 1;
    obs.data[obs.n].sat = (unsigned char)obs_raw[obs.n][7];
    obs.data[obs.n].code[0] = CODE_L1C;
    obs.data[obs.n].code[1] = CODE_L2W;
    obs.data[obs.n].L[0] = obs_raw[obs.n][8];
    obs.data[obs.n].P[0] = obs_raw[obs.n][9];
    obs.data[obs.n].L[1] = obs_raw[obs.n][10];
    obs.data[obs.n].P[1] = obs_raw[obs.n][11];
  }
#endif

  char *pos_mode = NULL;
  double *x = NULL;

  if(0){
    // single point positioning
    char buf[0x100];
    if(pntpos(obs.data, obs.n, &nav, &opt, &(rtk.sol), NULL, NULL, buf)){
      pos_mode = "pntpos";
      x = rtk.sol.rr;
    }
  }else{
    // precise point positioning
    if(rtkpos(&rtk, obs.data, obs.n, &nav)){
      pos_mode = (rtk.sol.stat == SOLQ_FIX) ? "rtkpos(fix)" : "rtkpos";
      x = rtk.x;
    }
  }

  if(pos_mode){
    double pos[3];
    ecef2pos(x, pos);
    trace(3, "%s (llh): %f, %f, %f\n", pos_mode, pos[0] * R2D, pos[1] * R2D, pos[2]);
  }

  rtkfree(&rtk);

  free(nav.eph);
  free(nav.seph);

  return 0;
}
