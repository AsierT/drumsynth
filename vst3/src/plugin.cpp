#include "plugin.h"
#include "public.sdk/source/main/pluginfactory.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace DrumSynthVST3 {

static float clamp(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }

static inline float soft(float x) { return x / (1.0f + std::abs(x)); }

struct VoiceState {
  float phase = 0.0f, env = 0.0f, filt = 0.0f, freq = 55.0f, target = 55.0f;
  uint32 noise = 0x12345678;
};

tresult PLUGIN_API Processor::initialize(FUnknown* context) {
  auto r = AudioEffect::initialize(context);
  if (r != kResultOk) return r;
  addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
  return kResultOk;
}

tresult PLUGIN_API Processor::setBusArrangements(SpeakerArrangement*, int32, SpeakerArrangement* outputs, int32 numOuts) {
  if (numOuts != 1 || outputs[0] != SpeakerArr::kStereo) return kResultFalse;
  return kResultOk;
}

tresult PLUGIN_API Processor::process(ProcessData& data) {
  if (data.numOutputs == 0 || !data.outputs[0].channelBuffers32) return kResultOk;
  auto** out = data.outputs[0].channelBuffers32;
  for (int32 i = 0; i < data.numSamples; ++i) {
    out[0][i] = 0.0f;
    out[1][i] = 0.0f;
  }
  return kResultOk;
}

tresult PLUGIN_API Controller::initialize(FUnknown* context) {
  auto r = EditControllerEx1::initialize(context);
  if (r != kResultOk) return r;

  parameters.addParameter(STR16("Voice"), nullptr, 5, 0, ParameterInfo::kCanAutomate, kVoice);
  parameters.addParameter(STR16("Tone"), nullptr, 0, 0.5, ParameterInfo::kCanAutomate, kTone);
  parameters.addParameter(STR16("Pitch"), nullptr, 0, 0.5, ParameterInfo::kCanAutomate, kPitch);
  parameters.addParameter(STR16("Amp Decay"), nullptr, 0, 0.5, ParameterInfo::kCanAutomate, kAmpDecay);
  parameters.addParameter(STR16("Amp Release"), nullptr, 0, 0.5, ParameterInfo::kCanAutomate, kAmpRelease);
  parameters.addParameter(STR16("Filter Decay"), nullptr, 0, 0.5, ParameterInfo::kCanAutomate, kFilterDecay);
  parameters.addParameter(STR16("Filter Release"), nullptr, 0, 0.5, ParameterInfo::kCanAutomate, kFilterRelease);
  parameters.addParameter(STR16("Resonance"), nullptr, 0, 0.1, ParameterInfo::kCanAutomate, kResonance);
  parameters.addParameter(STR16("Drive"), nullptr, 0, 0.2, ParameterInfo::kCanAutomate, kDrive);
  parameters.addParameter(STR16("Glide"), nullptr, 0, 0.0, ParameterInfo::kCanAutomate, kGlide);
  parameters.addParameter(STR16("Dist Type"), nullptr, 5, 0, ParameterInfo::kCanAutomate, kDistType);
  parameters.addParameter(STR16("Dist Mix"), nullptr, 0, 0, ParameterInfo::kCanAutomate, kDistMix);
  return kResultOk;
}

}

BEGIN_FACTORY_DEF("DrumSynth", "https://example.org", "mailto:dev@example.org")

DEF_CLASS2(INLINE_UID_FROM_FUID(DrumSynthVST3::ProcessorUID),
  PClassInfo::kManyInstances,
  kVstAudioEffectClass,
  "DrumSynth",
  Vst::kDistributable,
  "Fx",
  FULL_VERSION_STR,
  kVstVersionString,
  DrumSynthVST3::Processor::createInstance)

DEF_CLASS2(INLINE_UID_FROM_FUID(DrumSynthVST3::ControllerUID),
  PClassInfo::kManyInstances,
  kVstComponentControllerClass,
  "DrumSynthController",
  0,
  "",
  FULL_VERSION_STR,
  kVstVersionString,
  DrumSynthVST3::Controller::createInstance)

END_FACTORY
