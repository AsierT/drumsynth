#include <cmath>
#include <cstdint>
#include <cstring>

#include "lv2/core/lv2.h"
#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"

enum VoiceType { KICK, SNARE, HIHAT, TOM, CLAP, SUB808 };

static const char* URIS[] = {
    "https://example.org/plugins/drumsynth/kick",
    "https://example.org/plugins/drumsynth/snare",
    "https://example.org/plugins/drumsynth/hihat",
    "https://example.org/plugins/drumsynth/tom",
    "https://example.org/plugins/drumsynth/clap",
    "https://example.org/plugins/drumsynth/sub808",
};

enum PortIndex : uint32_t {
  OUT_L = 0, OUT_R, GATE, TONE, PITCH, AMP_DECAY, AMP_RELEASE,
  FILT_DECAY, FILT_RELEASE, RESONANCE, DRIVE, GLIDE, DIST_TYPE, DIST_MIX, MIDI_IN
};

struct Plugin {
  VoiceType type;
  float sr;
  float phase = 0.0f;
  float freq = 55.0f;
  float target_freq = 55.0f;
  float amp_env = 0.0f;
  float filt_env = 0.0f;
  float filt_state = 0.0f;
  float prev_gate = 0.0f;
  uint32_t noise = 0x1234ABCD;

  float *out_l=nullptr,*out_r=nullptr;
  const float *gate=nullptr,*tone=nullptr,*pitch=nullptr,*amp_decay=nullptr,*amp_release=nullptr;
  const float *filt_decay=nullptr,*filt_release=nullptr,*resonance=nullptr,*drive=nullptr,*glide=nullptr,*dist_type=nullptr,*dist_mix=nullptr;
  const LV2_Atom_Sequence* midi_in=nullptr;
};

static float frand(Plugin* p){ p->noise = p->noise*1664525u + 1013904223u; return ((p->noise>>8)&0xFFFF)/32768.0f-1.0f; }
static float clip(float x){ return x/(1.0f+std::fabs(x)); }
static float clamp(float x,float lo,float hi){ return x<lo?lo:(x>hi?hi:x); }

static void trigger(Plugin* p, float vel=1.0f){ p->amp_env = vel; p->filt_env = vel; }

static void handle_midi(Plugin* p){
  if(!p->midi_in) return;
  LV2_ATOM_SEQUENCE_FOREACH(p->midi_in, ev){
    const uint8_t* m = reinterpret_cast<const uint8_t*>(ev + 1);
    if(ev->body.size < 3) continue;
    if((m[0]&0xF0)==0x90 && m[2]>0){
      if (p->type == SUB808) {
        const float n = static_cast<float>(m[1]);
        p->target_freq = 440.0f * std::pow(2.0f, (n - 69.0f) / 12.0f);
      }
      trigger(p, m[2]/127.0f);
    }
  }
}

static float osc(Plugin* p){
  float t= p->tone?*p->tone:0.5f, pi = p->pitch?*p->pitch:0.5f;
  float f0=40.0f+pi*120.0f;
  if(p->type==HIHAT) f0 = 3000.0f + t*7000.0f;
  if(p->type==SNARE) f0 = 150.0f + t*250.0f;
  if(p->type==CLAP) f0 = 900.0f + t*2500.0f;
  if(p->type==SUB808) f0 = p->freq;
  p->phase += (2.0f*float(M_PI)*f0)/p->sr;

  switch(p->type){
    case KICK: return std::sin(p->phase + 20.0f*p->amp_env);
    case SNARE: return 0.35f*std::sin(p->phase)+0.8f*frand(p);
    case HIHAT: return (std::sin(p->phase*1.37f)>0?1.0f:-1.0f)*0.3f + frand(p)*0.9f;
    case TOM: return std::sin(p->phase);
    case CLAP: return ((std::fmod(p->phase,0.9f)<0.2f)?1.0f:0.2f)*frand(p);
    case SUB808: return std::sin(p->phase);
  }
  return 0.0f;
}

static float filter(Plugin* p,float x){
  float t=p->tone?*p->tone:0.5f;
  float cutoff = 80.0f + t*9000.0f + p->filt_env*4000.0f;
  cutoff = clamp(cutoff, 40.0f, p->sr*0.45f);
  float a = 1.0f - std::exp(-2.0f*float(M_PI)*cutoff/p->sr);
  float res = clamp(p->resonance?*p->resonance:0.1f,0.0f,0.98f);
  float in = x - res*p->filt_state;
  p->filt_state += a*(in - p->filt_state);
  return p->filt_state;
}


static float distort_sub808(Plugin* p, float x) {
  float mix = clamp(p->dist_mix ? *p->dist_mix : 0.0f, 0.0f, 1.0f);
  float type = clamp(p->dist_type ? *p->dist_type : 0.0f, 0.0f, 5.999f);
  int t = static_cast<int>(type);
  float wet = x;
  if (t == 0) {
    wet = std::tanh(x * 3.2f);
  } else if (t == 1) {
    wet = clip(x * 6.0f);
  } else if (t == 2) {
    float k = 20.0f;
    wet = ((1.0f + k) * x) / (1.0f + k * std::fabs(x));
  } else if (t == 3) {
    // valve-style asymmetric saturation
    float a = x + 0.2f * x * x;
    wet = std::tanh(a * 2.8f);
  } else if (t == 4) {
    // tube-style smoother odd/even harmonics
    float a = x * (1.0f + 0.5f * std::fabs(x));
    wet = std::tanh(a * 2.2f) + 0.08f * std::sin(3.0f * a);
  } else {
    // tape saturation approximation with soft knee + mild compression
    float a = std::tanh(x * 1.8f);
    wet = a * (1.0f - 0.15f * std::fabs(a));
  }
  return x * (1.0f - mix) + wet * mix;
}

static LV2_Handle instantiate(const LV2_Descriptor* d,double rate,const char*,const LV2_Feature* const*){
  auto* p = new Plugin();
  p->type = KICK;
  p->sr = float(rate);
  for(int i=0;i<6;i++) if(std::strcmp(d->URI,URIS[i])==0) p->type = VoiceType(i);
  return p;
}

static void connect_port(LV2_Handle instance,uint32_t port,void* data){
  auto* p=(Plugin*)instance;
  switch(port){
    case OUT_L:p->out_l=(float*)data;break; case OUT_R:p->out_r=(float*)data;break;
    case GATE:p->gate=(const float*)data;break; case TONE:p->tone=(const float*)data;break;
    case PITCH:p->pitch=(const float*)data;break; case AMP_DECAY:p->amp_decay=(const float*)data;break;
    case AMP_RELEASE:p->amp_release=(const float*)data;break; case FILT_DECAY:p->filt_decay=(const float*)data;break;
    case FILT_RELEASE:p->filt_release=(const float*)data;break; case RESONANCE:p->resonance=(const float*)data;break;
    case DRIVE:p->drive=(const float*)data;break; case GLIDE:p->glide=(const float*)data;break; case DIST_TYPE:p->dist_type=(const float*)data;break; case DIST_MIX:p->dist_mix=(const float*)data;break; case MIDI_IN:p->midi_in=(const LV2_Atom_Sequence*)data;break;
  }
}

static void run(LV2_Handle instance,uint32_t n){
  auto* p=(Plugin*)instance;
  handle_midi(p);
  float g = p->gate?*p->gate:0.0f;
  if(g>0.5f && p->prev_gate<=0.5f) trigger(p);
  p->prev_gate = g;

  float ad=0.990f + (p->amp_decay?*p->amp_decay:0.5f)*0.0099f;
  float ar=0.990f + (p->amp_release?*p->amp_release:0.5f)*0.0099f;
  float fd=0.985f + (p->filt_decay?*p->filt_decay:0.5f)*0.014f;
  float fr=0.985f + (p->filt_release?*p->filt_release:0.5f)*0.014f;
  float drv=1.0f + (p->drive?*p->drive:0.2f)*8.0f;
  float glide_amt = 0.0005f + (p->glide?*p->glide:0.0f)*0.02f;

  for(uint32_t i=0;i<n;i++){
    if (p->type == SUB808) { p->freq += (p->target_freq - p->freq) * glide_amt; }
    float raw=osc(p);
    float shaped=filter(p,raw);
    float y=clip(shaped*drv)*p->amp_env;
    if (p->type == SUB808) y = distort_sub808(p, y);
    p->amp_env*= (p->amp_env>0.2f)?ad:ar;
    p->filt_env*= (p->filt_env>0.2f)?fd:fr;
    if(p->amp_env<0.0001f) p->amp_env=0.0f;
    if(p->filt_env<0.0001f) p->filt_env=0.0f;
    p->out_l[i]=y; p->out_r[i]=y;
  }
}

static void cleanup(LV2_Handle i){ delete (Plugin*)i; }

static const LV2_Descriptor descriptors[] = {
  {URIS[0],instantiate,connect_port,nullptr,run,nullptr,cleanup,nullptr},
  {URIS[1],instantiate,connect_port,nullptr,run,nullptr,cleanup,nullptr},
  {URIS[2],instantiate,connect_port,nullptr,run,nullptr,cleanup,nullptr},
  {URIS[3],instantiate,connect_port,nullptr,run,nullptr,cleanup,nullptr},
  {URIS[4],instantiate,connect_port,nullptr,run,nullptr,cleanup,nullptr},
  {URIS[5],instantiate,connect_port,nullptr,run,nullptr,cleanup,nullptr},
};

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index){
  return index < 6 ? &descriptors[index] : nullptr;
}
