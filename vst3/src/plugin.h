#pragma once
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

namespace DrumSynthVST3 {

static const Steinberg::FUID ProcessorUID (0x12345678, 0x22223333, 0x44445555, 0x66667777);
static const Steinberg::FUID ControllerUID(0x87654321, 0x33332222, 0x55554444, 0x77776666);

enum ParamIds : Steinberg::Vst::ParamID {
  kVoice = 0,
  kTone,
  kPitch,
  kAmpDecay,
  kAmpRelease,
  kFilterDecay,
  kFilterRelease,
  kResonance,
  kDrive,
  kGlide,
  kDistType,
  kDistMix,
};

class Processor : public Steinberg::Vst::AudioEffect {
public:
  static Steinberg::FUnknown* createInstance(void*) { return (Steinberg::Vst::IAudioProcessor*)new Processor(); }
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns,
                                                   Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
};

class Controller : public Steinberg::Vst::EditControllerEx1 {
public:
  static Steinberg::FUnknown* createInstance(void*) { return (Steinberg::Vst::IEditController*)new Controller(); }
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
};

}
