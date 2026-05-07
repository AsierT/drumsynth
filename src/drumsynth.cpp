#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lv2/core/lv2.h"
#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"

enum VoiceType { KICK, SNARE, HIHAT, TOM, CLAP, SUB808 };

static const char* URIS[] = {
    "https://github.com/AsierT/drumsynth#kick",
    "https://github.com/AsierT/drumsynth#snare",
    "https://github.com/AsierT/drumsynth#hihat",
    "https://github.com/AsierT/drumsynth#tom",
    "https://github.com/AsierT/drumsynth#clap",
    "https://github.com/AsierT/drumsynth#sub808",
};

#if defined(DRUMSYNTH_SINGLE_INDEX) && DRUMSYNTH_SINGLE_INDEX != 5
#define DRUMSYNTH_HAS_GLIDE_PORT 0
#else
#define DRUMSYNTH_HAS_GLIDE_PORT 1
#endif

enum PortIndex : uint32_t {
#ifdef DRUMSYNTH_INSERT_PORTS
  IN_L = 0, IN_R, OUT_L, OUT_R, PITCH, OCTAVE, TONE, AMP_ATTACK, AMP_DECAY, AMP_RELEASE,
  FILTER_TYPE, CUTOFF, RESONANCE, DRIVE, FILT_ENV_AMT, FILT_ATTACK, FILT_DECAY,
  FILT_RELEASE, DIST_TYPE, DIST_MIX,
#if DRUMSYNTH_HAS_GLIDE_PORT
  GLIDE,
#endif
  MIDI_IN
#else
  OUT_L = 0, OUT_R, PITCH, OCTAVE, TONE, AMP_ATTACK, AMP_DECAY, AMP_RELEASE,
  FILTER_TYPE, CUTOFF, RESONANCE, DRIVE, FILT_ENV_AMT, FILT_ATTACK, FILT_DECAY,
  FILT_RELEASE, DIST_TYPE, DIST_MIX, GLIDE, MIDI_IN
#endif
};

struct Plugin {
  VoiceType type;
  float sr;
  float phase;
  float freq;
  float target_freq;
  float amp_target;
  float filt_target;
  float amp_env;
  float filt_env;
  float filt_lp;
  float filt_bp;
  float age;
  float declick;
  uint32_t noise;
  uint8_t amp_attacking;
  uint8_t filt_attacking;

  const float *in_l, *in_r;
  float *out_l, *out_r;
  const float *tone, *pitch, *octave, *amp_attack, *amp_decay, *amp_release;
  const float *filt_attack, *filt_decay, *filt_release, *cutoff, *filt_env_amt, *filter_type;
  const float *resonance, *drive, *glide, *dist_type, *dist_mix;
  const LV2_Atom_Sequence* midi_in;
};

static constexpr float kPi = 3.14159265358979323846f;
static constexpr float kTwoPi = 2.0f * kPi;
static constexpr float kRefSampleRate = 48000.0f;

static bool finite_float(float x) {
  return __builtin_isfinite(x);
}

static bool finite_double(double x) {
  return __builtin_isfinite(x);
}

static float finite_or(float x, float fallback) {
  return finite_float(x) ? x : fallback;
}

static float clamp(float x, float lo, float hi) {
  x = finite_or(x, lo);
  return x < lo ? lo : (x > hi ? hi : x);
}

static float control_value(const float* value, float fallback, float lo, float hi) {
  return clamp(finite_or(value ? *value : fallback, fallback), lo, hi);
}

static float sample_rate(const Plugin* p) {
  return (p && finite_float(p->sr) && p->sr >= 1000.0f && p->sr <= 384000.0f) ? p->sr : kRefSampleRate;
}

static int control_int(const float* value, int fallback, int lo, int hi) {
  const float v = control_value(value, static_cast<float>(fallback), static_cast<float>(lo), static_cast<float>(hi));
  int i = static_cast<int>(v >= 0.0f ? v + 0.5f : v - 0.5f);
  if (i < lo) i = lo;
  if (i > hi) i = hi;
  return i;
}

static float semitone_ratio(float semitones) {
  return finite_or(powf(2.0f, semitones / 12.0f), 1.0f);
}

static float log_hz_from_normalized(float value) {
  value = clamp(value, 0.0f, 1.0f);
  if (value <= 0.000001f) return 0.0f;
  return finite_or(20.0f * powf(1000.0f, value), 20000.0f);
}

static float normalized_from_hz(float hz) {
  hz = clamp(hz, 0.0f, 20000.0f);
  if (hz <= 20.0f) return hz <= 0.000001f ? 0.0f : 0.000001f;
  return clamp(logf(hz / 20.0f) / logf(1000.0f), 0.0f, 1.0f);
}

static float time_coef_ms(const Plugin* p, float ms) {
  const float sr = sample_rate(p);
  ms = clamp(ms, 0.1f, 5000.0f);
  return finite_or(expf(-1.0f / (0.001f * ms * sr)), 0.0f);
}

static float frand(Plugin* p) {
  p->noise = p->noise * 1664525u + 1013904223u;
  return ((p->noise >> 8) & 0xFFFF) / 32768.0f - 1.0f;
}

static float clip(float x) {
  x = finite_or(x, 0.0f);
  const float y = x / (1.0f + fabsf(x));
  return finite_or(y, 0.0f);
}

static float limit_output(float x) {
  return clamp(finite_or(x, 0.0f), -1.0f, 1.0f);
}

static float wrap_positive(float x, float period) {
  if (!finite_float(x) || period <= 0.0f) return 0.0f;
  while (x >= period) x -= period;
  while (x < 0.0f) x += period;
  return x;
}

static void reset_state(Plugin* p) {
  if (!p) return;
  p->phase = 0.0f;
  p->freq = 55.0f;
  p->target_freq = 55.0f;
  p->amp_target = 0.0f;
  p->filt_target = 0.0f;
  p->amp_env = 0.0f;
  p->filt_env = 0.0f;
  p->filt_lp = 0.0f;
  p->filt_bp = 0.0f;
  p->age = 0.0f;
  p->declick = 1.0f;
  p->noise = 0x1234ABCD;
  p->amp_attacking = 0;
  p->filt_attacking = 0;
}

static void init_plugin(Plugin* p, VoiceType type, float sr) {
  p->type = type;
  p->sr = sr;
  p->in_l = nullptr;
  p->in_r = nullptr;
  p->out_l = nullptr;
  p->out_r = nullptr;
  p->tone = nullptr;
  p->pitch = nullptr;
  p->octave = nullptr;
  p->amp_attack = nullptr;
  p->amp_decay = nullptr;
  p->amp_release = nullptr;
  p->filt_attack = nullptr;
  p->filt_decay = nullptr;
  p->filt_release = nullptr;
  p->cutoff = nullptr;
  p->filt_env_amt = nullptr;
  p->filter_type = nullptr;
  p->resonance = nullptr;
  p->drive = nullptr;
  p->glide = nullptr;
  p->dist_type = nullptr;
  p->dist_mix = nullptr;
  p->midi_in = nullptr;
  reset_state(p);
}

static void trigger(Plugin* p, float vel = 1.0f) {
  vel = clamp(vel, 0.0f, 1.0f);
  p->phase = 0.0f;
  p->age = 0.0f;
  p->declick = 0.0f;
  p->amp_target = vel;
  p->filt_target = vel;
  p->amp_attacking = 1;
  p->filt_attacking = 1;
}

static void handle_midi(Plugin* p) {
  if (!p->midi_in) return;
  if (p->midi_in->atom.size < 8) return;

  LV2_ATOM_SEQUENCE_FOREACH(p->midi_in, ev) {
    const uint8_t* m = reinterpret_cast<const uint8_t*>(ev + 1);
    if (ev->body.size < 3) continue;
    if ((m[0] & 0xF0) == 0x90 && m[2] > 0) {
      if (p->type == SUB808) {
        const float n = static_cast<float>(m[1]);
        const float max_freq = sample_rate(p) * 0.45f;
        p->target_freq = clamp(440.0f * powf(2.0f, (n - 69.0f) / 12.0f), 8.0f, max_freq);
        const float glide = control_value(p->glide, 0.0f, 0.0f, 1.0f);
        if (glide <= 0.0001f || p->amp_env <= 0.0001f) {
          p->freq = p->target_freq;
        }
      }
      trigger(p, clamp(m[2] / 127.0f, 0.0f, 1.0f));
    }
  }
}

static float osc(Plugin* p) {
  const float sr = sample_rate(p);
  const float t = control_value(p->tone, 0.5f, 0.0f, 1.0f);
  const int pitch = control_int(p->pitch, 0, -36, 36);
  const int octave = control_int(p->octave, 0, -3, 3);
  const float pitch_ratio = semitone_ratio(static_cast<float>(pitch + octave * 12));
  float f0 = 55.0f * pitch_ratio;

  if (p->type == KICK) f0 = 55.0f * pitch_ratio;
  if (p->type == SNARE) f0 = 220.0f * pitch_ratio;
  if (p->type == HIHAT) f0 = 2200.0f * pitch_ratio * (1.0f + t * 7.0f);
  if (p->type == TOM) f0 = 110.0f * pitch_ratio;
  if (p->type == CLAP) f0 = 1200.0f * pitch_ratio * (0.5f + t * 1.7f);
  if (p->type == SUB808) f0 = p->freq * pitch_ratio;

  f0 = clamp(f0, 1.0f, sr * 0.45f);
  p->phase += (kTwoPi * f0) / sr;
  if (!finite_float(p->phase)) {
    p->phase = 0.0f;
  } else if (p->phase >= kTwoPi || p->phase < 0.0f) {
    p->phase = wrap_positive(p->phase, kTwoPi);
  }

  switch (p->type) {
    case KICK: {
      const float click = (p->age < 0.004f) ? (1.0f - p->age * 250.0f) * t * frand(p) : 0.0f;
      return sinf(p->phase + 20.0f * p->amp_env) + 0.45f * t * sinf(2.0f * p->phase) + 0.55f * click;
    }
    case SNARE:
      return (0.75f - 0.65f * t) * sinf(p->phase) + (0.25f + 1.25f * t) * frand(p);
    case HIHAT:
      return (sinf(p->phase * (1.0f + t * 3.0f)) > 0.0f ? 1.0f : -1.0f) * (0.50f - 0.35f * t) + frand(p) * (0.35f + 1.15f * t);
    case TOM:
      return (1.0f - 0.35f * t) * sinf(p->phase) + 0.45f * t * sinf(2.0f * p->phase) + 0.20f * t * sinf(3.0f * p->phase);
    case CLAP: {
      const float a = p->age;
      const float spread = 0.010f + t * 0.012f;
      float burst = 0.0f;
      if (a < 0.009f) burst += 1.00f - a / 0.009f;
      if (a >= spread && a < spread + 0.010f) burst += 0.85f * (1.0f - (a - spread) / 0.010f);
      if (a >= spread * 2.1f && a < spread * 2.1f + 0.012f) burst += 0.70f * (1.0f - (a - spread * 2.1f) / 0.012f);
      if (a >= spread * 3.4f && a < spread * 3.4f + 0.018f) burst += 0.50f * (1.0f - (a - spread * 3.4f) / 0.018f);
      const float tail = (a < 0.170f) ? (0.28f + 0.45f * t) * (1.0f - a / 0.170f) : 0.0f;
      const float body = frand(p) * (burst + tail);
      const float snap = (sinf(p->phase) > 0.0f ? 1.0f : -1.0f) * burst * (0.08f + 0.14f * t);
      return body + snap;
    }
    case SUB808: return sinf(p->phase) + 0.55f * t * sinf(2.0f * p->phase);
  }
  return 0.0f;
}

static float filter(Plugin* p, float x) {
  const float sr = sample_rate(p);
  x = finite_or(x, 0.0f);
  if (!finite_float(p->filt_lp)) p->filt_lp = 0.0f;
  if (!finite_float(p->filt_bp)) p->filt_bp = 0.0f;

  const float cutoff_ctl = control_value(p->cutoff, 20000.0f, 0.0f, 20000.0f);
  const float env_amt = control_value(p->filt_env_amt, 0.0f, -1.0f, 1.0f);
  const int filter_type = control_int(p->filter_type, 0, 0, 2);
  const float res = control_value(p->resonance, 0.0f, 0.0f, 0.98f);
  float cutoff = log_hz_from_normalized(normalized_from_hz(cutoff_ctl) + p->filt_env * env_amt);
  cutoff = clamp(cutoff, 0.0f, 20000.0f);
  cutoff = clamp(cutoff, 0.0f, sr * 0.45f);

  const float f = clamp(2.0f * sinf(kPi * cutoff / sr), 0.00001f, 0.95f);
  const float damping = clamp(1.55f - res * 1.45f, 0.10f, 1.55f);

  p->filt_lp += f * p->filt_bp;
  float hp = x - p->filt_lp - damping * p->filt_bp;
  p->filt_bp += f * hp;

  p->filt_lp = finite_or(clamp(p->filt_lp, -4.0f, 4.0f), 0.0f);
  p->filt_bp = finite_or(clamp(p->filt_bp, -4.0f, 4.0f), 0.0f);
  hp = finite_or(clamp(hp, -4.0f, 4.0f), 0.0f);

  float y = p->filt_lp;
  if (filter_type == 1) y = hp;
  if (filter_type == 2) y = p->filt_bp * (1.0f + res);
  return finite_or(y, 0.0f);
}

static float distort_signal(Plugin* p, float x) {
  x = finite_or(x, 0.0f);
  const float mix = control_value(p->dist_mix, 0.0f, 0.0f, 1.0f);
  const int t = control_int(p->dist_type, 0, 0, 5);
  float wet = x;

  if (t == 0) {
    return x;
  } else if (t == 1) {
    wet = tanhf(x * 3.2f);
  } else if (t == 2) {
    wet = clip(x * 6.0f);
  } else if (t == 3) {
    const float k = 20.0f;
    wet = ((1.0f + k) * x) / (1.0f + k * fabsf(x));
  } else if (t == 4) {
    const float a = x + 0.2f * x * x;
    wet = tanhf(a * 2.8f);
  } else {
    float a = x * 4.0f;
    while (a > 1.0f) a = 2.0f - a;
    while (a < -1.0f) a = -2.0f - a;
    wet = tanhf(a * 2.2f);
  }
  return finite_or(x * (1.0f - mix) + wet * mix, 0.0f);
}

static void advance_env(float* env, float* level, uint8_t* attacking, float attack_coef, float decay_coef, float release_coef) {
  float value = clamp(env ? *env : 0.0f, 0.0f, 1.0f);
  float target = clamp(level ? *level : 0.0f, 0.0f, 1.0f);

  if (attacking && *attacking) {
    const float attack_step = clamp(1.0f - attack_coef, 0.00001f, 1.0f);
    value += (target - value) * attack_step;
    if (value >= target * 0.995f || target <= 0.0001f || value >= 0.9999f) {
      value = target;
      *attacking = 0;
    }
  } else {
    value = target;
  }

  target *= (target > 0.2f) ? decay_coef : release_coef;
  target = clamp(target, 0.0f, 1.0f);
  if (target < 0.0001f) target = 0.0f;

  value = clamp(value, 0.0f, 1.0f);
  if (value < 0.0001f && (!attacking || !*attacking)) value = 0.0f;
  if (level) *level = target;
  if (env) *env = value;
}

static LV2_Handle instantiate(const LV2_Descriptor* d, double rate, const char*, const LV2_Feature* const*) {
  Plugin* p = static_cast<Plugin*>(malloc(sizeof(Plugin)));
  if (!p) return nullptr;

  VoiceType type = KICK;
  if (d && d->URI) {
    for (int i = 0; i < 6; ++i) {
      if (strcmp(d->URI, URIS[i]) == 0) {
        type = static_cast<VoiceType>(i);
        break;
      }
    }
  }

  const float sr = (finite_double(rate) && rate >= 1000.0 && rate <= 384000.0) ? static_cast<float>(rate) : kRefSampleRate;
  init_plugin(p, type, sr);
  return p;
}

static void connect_port(LV2_Handle instance, uint32_t port, void* data) {
  Plugin* p = static_cast<Plugin*>(instance);
  if (!p) return;

  switch (port) {
#ifdef DRUMSYNTH_INSERT_PORTS
    case IN_L: p->in_l = static_cast<const float*>(data); break;
    case IN_R: p->in_r = static_cast<const float*>(data); break;
#endif
    case OUT_L: p->out_l = static_cast<float*>(data); break;
    case OUT_R: p->out_r = static_cast<float*>(data); break;
    case TONE: p->tone = static_cast<const float*>(data); break;
    case PITCH: p->pitch = static_cast<const float*>(data); break;
    case OCTAVE: p->octave = static_cast<const float*>(data); break;
    case AMP_ATTACK: p->amp_attack = static_cast<const float*>(data); break;
    case AMP_DECAY: p->amp_decay = static_cast<const float*>(data); break;
    case AMP_RELEASE: p->amp_release = static_cast<const float*>(data); break;
    case FILT_ATTACK: p->filt_attack = static_cast<const float*>(data); break;
    case FILT_DECAY: p->filt_decay = static_cast<const float*>(data); break;
    case FILT_RELEASE: p->filt_release = static_cast<const float*>(data); break;
    case CUTOFF: p->cutoff = static_cast<const float*>(data); break;
    case FILT_ENV_AMT: p->filt_env_amt = static_cast<const float*>(data); break;
    case FILTER_TYPE: p->filter_type = static_cast<const float*>(data); break;
    case RESONANCE: p->resonance = static_cast<const float*>(data); break;
    case DRIVE: p->drive = static_cast<const float*>(data); break;
#if DRUMSYNTH_HAS_GLIDE_PORT
    case GLIDE: p->glide = static_cast<const float*>(data); break;
#endif
    case DIST_TYPE: p->dist_type = static_cast<const float*>(data); break;
    case DIST_MIX: p->dist_mix = static_cast<const float*>(data); break;
    case MIDI_IN: p->midi_in = static_cast<const LV2_Atom_Sequence*>(data); break;
  }
}

static void activate(LV2_Handle instance) {
  reset_state(static_cast<Plugin*>(instance));
}

static void run(LV2_Handle instance, uint32_t n) {
  Plugin* p = static_cast<Plugin*>(instance);
  if (!p || !p->out_l || !p->out_r) {
    return;
  }

  handle_midi(p);

  const float amp_attack = control_value(p->amp_attack, 0.0f, 0.0f, 1.0f);
  const float amp_decay = control_value(p->amp_decay, 0.0f, 0.0f, 1.0f);
  const float amp_release = control_value(p->amp_release, 0.0f, 0.0f, 1.0f);
  const float filt_attack = control_value(p->filt_attack, 0.0f, 0.0f, 1.0f);
  const float filt_decay = control_value(p->filt_decay, 0.0f, 0.0f, 1.0f);
  const float filt_release = control_value(p->filt_release, 0.0f, 0.0f, 1.0f);
  const float aa = time_coef_ms(p, 0.02f + amp_attack * amp_attack * 250.0f);
  const float ad = time_coef_ms(p, 3.0f + amp_decay * amp_decay * 450.0f);
  const float ar = time_coef_ms(p, 3.0f + amp_release * amp_release * 700.0f);
  const float fa = time_coef_ms(p, 0.02f + filt_attack * filt_attack * 250.0f);
  const float fd = time_coef_ms(p, 4.0f + filt_decay * filt_decay * 500.0f);
  const float fr = time_coef_ms(p, 4.0f + filt_release * filt_release * 900.0f);
  const float drv = 1.0f + control_value(p->drive, 0.0f, 0.0f, 1.0f) * 8.0f;
  const float glide = (p->type == SUB808) ? control_value(p->glide, 0.0f, 0.0f, 1.0f) : 0.0f;
  const float glide_ms = 3.0f + glide * glide * 1800.0f;
  const float glide_amt = (glide <= 0.0001f) ? 1.0f : (1.0f - time_coef_ms(p, glide_ms));
  const float sr = sample_rate(p);
  const float declick_step = 1.0f / (sr * 0.0015f);
  p->amp_env = clamp(p->amp_env, 0.0f, 1.0f);
  p->filt_env = clamp(p->filt_env, 0.0f, 1.0f);
  p->declick = clamp(p->declick, 0.0f, 1.0f);

  for (uint32_t i = 0; i < n; ++i) {
    if (p->type == SUB808) {
      if (!finite_float(p->freq)) p->freq = 55.0f;
      p->freq += (p->target_freq - p->freq) * glide_amt;
      p->freq = clamp(p->freq, 8.0f, sample_rate(p) * 0.45f);
    }

    advance_env(&p->amp_env, &p->amp_target, &p->amp_attacking, aa, ad, ar);
    advance_env(&p->filt_env, &p->filt_target, &p->filt_attacking, fa, fd, fr);

    const float raw = osc(p);
    const float shaped = filter(p, raw);
    float y = clip(shaped * drv) * p->amp_env;
    y = distort_signal(p, y);
    p->declick = clamp(p->declick + declick_step, 0.0f, 1.0f);
    y *= p->declick;
    y = limit_output(y);

#ifdef DRUMSYNTH_INSERT_PORTS
    const float in_l = p->in_l ? finite_or(p->in_l[i], 0.0f) : 0.0f;
    const float in_r = p->in_r ? finite_or(p->in_r[i], 0.0f) : 0.0f;
    p->out_l[i] = limit_output(in_l + y);
    p->out_r[i] = limit_output(in_r + y);
#else
    p->out_l[i] = y;
    p->out_r[i] = y;
#endif
    p->age += 1.0f / sr;
    if (!finite_float(p->age) || p->age > 10.0f) p->age = 10.0f;
  }
}

static void cleanup(LV2_Handle i) {
  free(i);
}

static const LV2_Descriptor descriptors[] = {
  {URIS[0], instantiate, connect_port, activate, run, nullptr, cleanup, nullptr},
  {URIS[1], instantiate, connect_port, activate, run, nullptr, cleanup, nullptr},
  {URIS[2], instantiate, connect_port, activate, run, nullptr, cleanup, nullptr},
  {URIS[3], instantiate, connect_port, activate, run, nullptr, cleanup, nullptr},
  {URIS[4], instantiate, connect_port, activate, run, nullptr, cleanup, nullptr},
  {URIS[5], instantiate, connect_port, activate, run, nullptr, cleanup, nullptr},
};

extern "C" LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index) {
#ifdef DRUMSYNTH_SINGLE_INDEX
#if DRUMSYNTH_SINGLE_INDEX < 0 || DRUMSYNTH_SINGLE_INDEX > 5
#error "DRUMSYNTH_SINGLE_INDEX must be between 0 and 5"
#endif
  return index == 0 ? &descriptors[DRUMSYNTH_SINGLE_INDEX] : nullptr;
#else
  return index < 6 ? &descriptors[index] : nullptr;
#endif
}
