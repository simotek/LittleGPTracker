#ifndef _RT_MIDIIN_DEVICE_H_
#define _RT_MIDIIN_DEVICE_H_

#include "Externals/RtMidi/RtMidi.h"
#include "Services/Midi/MidiInDevice.h"

class RTMidiInDevice : public MidiInDevice {
  public:
    RTMidiInDevice(RtMidiIn &in, int index, const char *name);
    ~RTMidiInDevice();

    void SendDriverMessage(MidiMessage &message);

  private:
    // Driver specific initialisation

    virtual bool initDriver();
    virtual void closeDriver();
    virtual bool startDriver();
    virtual void stopDriver();

    RtMidiIn rtMidiIn_;
    int index_;
};
#endif
