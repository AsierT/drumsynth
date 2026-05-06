#include <cmath>
#include <cstdint>
#include <cstring>

#include "lv2/core/lv2.h"

#define DRUM_URI "https://example.org/plugins/drumsynth"

static constexpr uint32_t SAMPLE_RATE_MIN = 1;

enum PortIndex : uint32_t {
  PORT_OUT_L = 0,
  PORT_OUT_R,
  PORT_GATE_KICK,
  PORT_GATE_SNARE,
  PORT_GATE_HIHAT,
  PORT_GATE_TOM,
  PORT_GATE_CLAP,
  PORT_GATE_SUB,
  PORT_KICK_TUNE,
  PORT_SNARE_TONE,
  PORT_HIHAT_TONE,
  PORT_TOM_TUNE,
  PORT_CLAP_TONE,
  PORT_SUB_FREQ,
  PORT_SUB_DRIVE,
};

struct Voice {
  bool active = false;
  float phase = 0.0f;
  float env = 0.0f;
  float pitch = 0.0f;
  uint32_t noise = 0x12345678u;
};

struct DrumSynth {
  float sample_rate = 48000.0f;

  float* out_l = nullptr;
  float* out_r = nullptr;

  const float* gate_kick = nullptr;
  const float* gate_snare = nullptr;
  const float* gate_hihat = nullptr;
  const float* gate_tom = nullptr;
  const float* gate_clap = nullptr;
  const float* gate_sub = nullptr;

  const float* kick_tune = nullptr;
  const float* snare_tone = nullptr;
  const float* hihat_tone = nullptr;
  const float* tom_tune = nullptr;
  const float* clap_tone = nullptr;
  const float* sub_freq = nullptr;
  const float* sub_drive = nullptr;

  Voice kick;
  Voice snare;
  Voice hihat;
  Voice tom;
  Voice clap;
  Voice sub;

  float prev_gate[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
};

static inline float fast_rand(Voice& voice) {
  voice.noise = voice.noise * 1664525u + 1013904223u;
  return (static_cast<float>(voice.noise & 0x00FFFFFFu) / 8388608.0f) - 1.0f;
}

static inline float clamp01(float x) {
  if (x < 0.0f) return 0.0f;
  if (x > 1.0f) return 1.0f;
  return x;
}

static void trigger_voice(Voice& voice, float pitch, float env) {
  voice.active = true;
  voice.phase = 0.0f;
  voice.pitch = pitch;
  voice.env = env;
}

static float render_kick(DrumSynth* self) {
  auto& v = self->kick;
  if (!v.active) return 0.0f;
  const float decay = 0.9992f;
  const float sweep = 40.0f * v.env;
  const float freq = 35.0f + self->kick_tune[0] * 90.0f + sweep;
  v.phase += (2.0f * static_cast<float>(M_PI) * freq) / self->sample_rate;
  const float body = std::sin(v.phase);
  const float click = (v.env > 0.8f) ? (v.env - 0.8f) * 2.5f : 0.0f;
  float out = (body + click) * v.env;
  v.env *= decay;
  if (v.env < 0.0003f) v.active = false;
  return out * 1.2f;
}

static float render_snare(DrumSynth* self) {
  auto& v = self->snare;
  if (!v.active) return 0.0f;
  const float tone = 120.0f + self->snare_tone[0] * 260.0f;
  v.phase += (2.0f * static_cast<float>(M_PI) * tone) / self->sample_rate;
  const float shell = std::sin(v.phase) * 0.35f;
  const float noise = fast_rand(v) * 0.85f;
  const float out = (shell + noise) * v.env;
  v.env *= 0.995f;
  if (v.env < 0.0003f) v.active = false;
  return out;
}

static float render_hihat(DrumSynth* self) {
  auto& v = self->hihat;
  if (!v.active) return 0.0f;
  const float tone = 2500.0f + self->hihat_tone[0] * 7500.0f;
  v.phase += tone / self->sample_rate;
  if (v.phase >= 1.0f) v.phase -= 1.0f;
  const float metallic = (v.phase < 0.5f ? 1.0f : -1.0f) * 0.35f;
  const float noise = fast_rand(v) * 0.9f;
  const float out = (noise + metallic) * v.env;
  v.env *= 0.988f;
  if (v.env < 0.0003f) v.active = false;
  return out;
}

static float render_tom(DrumSynth* self) {
  auto& v = self->tom;
  if (!v.active) return 0.0f;
  const float freq = 70.0f + self->tom_tune[0] * 220.0f + v.env * 18.0f;
  v.phase += (2.0f * static_cast<float>(M_PI) * freq) / self->sample_rate;
  const float out = std::sin(v.phase) * v.env;
  v.env *= 0.997f;
  if (v.env < 0.0003f) v.active = false;
  return out * 1.1f;
}

static float render_clap(DrumSynth* self) {
  auto& v = self->clap;
  if (!v.active) return 0.0f;
  const float tone = 0.2f + self->clap_tone[0] * 0.6f;
  const float burst = (std::fmod(v.phase, tone) < 0.03f) ? 1.0f : 0.25f;
  const float out = fast_rand(v) * burst * v.env;
  v.phase += 1.0f / self->sample_rate;
  v.env *= 0.992f;
  if (v.env < 0.0003f) v.active = false;
  return out;
}

static float soft_clip(float x) {
  return x / (1.0f + std::fabs(x));
}

static float render_sub(DrumSynth* self) {
  auto& v = self->sub;
  if (!v.active) return 0.0f;
  const float base = 25.0f + self->sub_freq[0] * 85.0f;
  v.phase += (2.0f * static_cast<float>(M_PI) * base) / self->sample_rate;
  const float raw = std::sin(v.phase) * v.env;
  const float drive = 1.0f + (self->sub_drive[0] * 9.0f);
  const float out = soft_clip(raw * drive);
  v.env *= 0.9994f;
  if (v.env < 0.0003f) v.active = false;
  return out * 1.4f;
}

static LV2_Handle instantiate(const LV2_Descriptor*, double rate, const char*, const LV2_Feature* const*) {
  if (rate < SAMPLE_RATE_MIN) {
    return nullptr;
  }
  auto* self = new DrumSynth();
  self->sample_rate = static_cast<float>(rate);
  return self;
}

static void connect_port(LV2_Handle instance, uint32_t port, void* data) {
  auto* self = static_cast<DrumSynth*>(instance);
  switch (port) {
    case PORT_OUT_L: self->out_l = static_cast<float*>(data); break;
    case PORT_OUT_R: self->out_r = static_cast<float*>(data); break;
    case PORT_GATE_KICK: self->gate_kick = static_cast<const float*>(data); break;
    case PORT_GATE_SNARE: self->gate_snare = static_cast<const float*>(data); break;
    case PORT_GATE_HIHAT: self->gate_hihat = static_cast<const float*>(data); break;
    case PORT_GATE_TOM: self->gate_tom = static_cast<const float*>(data); break;
    case PORT_GATE_CLAP: self->gate_clap = static_cast<const float*>(data); break;
    case PORT_GATE_SUB: self->gate_sub = static_cast<const float*>(data); break;
    case PORT_KICK_TUNE: self->kick_tune = static_cast<const float*>(data); break;
    case PORT_SNARE_TONE: self->snare_tone = static_cast<const float*>(data); break;
    case PORT_HIHAT_TONE: self->hihat_tone = static_cast<const float*>(data); break;
    case PORT_TOM_TUNE: self->tom_tune = static_cast<const float*>(data); break;
    case PORT_CLAP_TONE: self->clap_tone = static_cast<const float*>(data); break;
    case PORT_SUB_FREQ: self->sub_freq = static_cast<const float*>(data); break;
    case PORT_SUB_DRIVE: self->sub_drive = static_cast<const float*>(data); break;
    default: break;
  }
}

static void run(LV2_Handle instance, uint32_t n_samples) {
  auto* self = static_cast<DrumSynth*>(instance);

  const float gates[6] = {
      self->gate_kick ? self->gate_kick[0] : 0.0f,
      self->gate_snare ? self->gate_snare[0] : 0.0f,
      self->gate_hihat ? self->gate_hihat[0] : 0.0f,
      self->gate_tom ? self->gate_tom[0] : 0.0f,
      self->gate_clap ? self->gate_clap[0] : 0.0f,
      self->gate_sub ? self->gate_sub[0] : 0.0f,
  };

  if (gates[0] > 0.5f && self->prev_gate[0] <= 0.5f) trigger_voice(self->kick, 1.0f, 1.0f);
  if (gates[1] > 0.5f && self->prev_gate[1] <= 0.5f) trigger_voice(self->snare, 1.0f, 1.0f);
  if (gates[2] > 0.5f && self->prev_gate[2] <= 0.5f) trigger_voice(self->hihat, 1.0f, 0.9f);
  if (gates[3] > 0.5f && self->prev_gate[3] <= 0.5f) trigger_voice(self->tom, 1.0f, 1.0f);
  if (gates[4] > 0.5f && self->prev_gate[4] <= 0.5f) trigger_voice(self->clap, 1.0f, 1.0f);
  if (gates[5] > 0.5f && self->prev_gate[5] <= 0.5f) trigger_voice(self->sub, 1.0f, 1.0f);

  std::memcpy(self->prev_gate, gates, sizeof(gates));

  for (uint32_t i = 0; i < n_samples; ++i) {
    float mix = 0.0f;
    mix += render_kick(self);
    mix += render_snare(self);
    mix += render_hihat(self);
    mix += render_tom(self);
    mix += render_clap(self);
    mix += render_sub(self);
    mix *= 0.25f;
    if (self->out_l) self->out_l[i] = mix;
    if (self->out_r) self->out_r[i] = mix;
  }
}

static void cleanup(LV2_Handle instance) {
  delete static_cast<DrumSynth*>(instance);
}

static const LV2_Descriptor descriptor = {
    DRUM_URI,
    instantiate,
    connect_port,
    nullptr,
    run,
    nullptr,
    cleanup,
    nullptr,
};

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index) {
  return (index == 0) ? &descriptor : nullptr;
}
