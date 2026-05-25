#include "RTMidiService.h"
#include "Application/Model/Config.h"
#include "RTMidiInDevice.h"
#include "RTMidiOutDevice.h"
#include "System/Console/Trace.h"

#include <string> // For RtMidiError::getMessage()
// static void CALLBACK MidiFlushProc(UINT uiID, UINT uiMsg, DWORD
//                                   dwUser, DWORD dw1, DWORD dw2) {
//
//	W32MidiService *msvc=(W32MidiService *)dwUser;
//	msvc->Flush();
// };

RTMidiService::RTMidiService() {
    const char *delay = Config::GetInstance()->GetValue("MIDIDELAY");
    if (delay) {
        midiDelay_ = atoi(delay);
    } else {
        midiDelay_ = 0;
    }

    // Log the APIs actually compiled into RtMidi to ensure JACK
    // (-D__UNIX_JACK__) is present
    std::vector<RtMidi::Api> apis;
    RtMidi::getCompiledApi(apis);
    for (size_t i = 0; i < apis.size(); i++) {
        Trace::Log("RtMidiService",
                   "RtMidi Compiled with API ID: %d (ALSA = 2, JACK = 3)",
                   (int)apis[i]);
    }

    // RtMidiIn constructor
    // If we have Jack support attempt to use it with higher priority
#ifdef __UNIX_JACK__
    try {
        Trace::Log("RtMidiService", "Trying Jack");
        rtMidiIn_ = new RtMidiIn(RtMidi::UNIX_JACK, "LittleGPTracker");
    } catch (RtMidiError &error) {
        Trace::Log("RTMidiService",
                   "Failed to create RtMidiIn with Jack API: %s. Falling back "
                   "to default API.",
                   error.getMessage().c_str());
#endif
        Trace::Log("RtMidiService", "Couldn't Load Jack");
        try {
            rtMidiIn_ = new RtMidiIn(RtMidi::UNSPECIFIED, "LittleGPTracker");
        } catch (RtMidiError &error) {
            Trace::Log("RtMidiService", "Couldn't get RtMidiIn object");
            rtMidiIn_ = 0;
            Trace::Log("RtMidiService", error.getMessage().c_str());
        }
#ifdef __UNIX_JACK__
    }
#endif
    // RtMidiOut constructor
    // If we have Jack support attempt to use it with higher priority
#ifdef __UNIX_JACK__
    try {
        rtMidiOut_ = new RtMidiOut(RtMidi::UNIX_JACK, "LittleGPTracker");
    } catch (RtMidiError &error) {
        Trace::Log("RTMidiService",
                   "Failed to create RtMidiOut with Jack API: %s. Falling back "
                   "to default API.",
                   error.getMessage().c_str());
#endif
        try {
            rtMidiOut_ = new RtMidiOut(RtMidi::UNSPECIFIED, "LittleGPTracker");
        } catch (RtMidiError &error) {
            Trace::Log("RtMidiService", "Couldn't get RtMidiOut object");
            rtMidiOut_ = 0;
            Trace::Log("RtMidiService", error.getMessage().c_str());
        }
#ifdef __UNIX_JACK__
    }
#endif
};

RTMidiService::~RTMidiService() {};

/*
 * here we just loop over existing midi out and create a midi device for each of
 * them
 */
void RTMidiService::buildDriverList() {
    // check inputs
    Trace::Log(
        "RTMidiService", "Listing input ports for API: %s",
        rtMidiIn_
            ? RtMidi::getApiDisplayName(rtMidiIn_->getCurrentApi()).c_str()
            : "None");
    unsigned int nPorts = (rtMidiIn_) ? rtMidiIn_->getPortCount() : 0;
    Trace::Log("RTMidiService", "%d input port(s)", nPorts);
    for (uint i = 0; i < nPorts; i++) {
        try {
            std::string portName = rtMidiIn_->getPortName(i);
            RTMidiInDevice *in =
                new RTMidiInDevice(*rtMidiIn_, i, portName.c_str());
            Trace::Log("RTMidiService", " %s", portName.c_str());
            inList_.Insert(in);
        } catch (RtMidiError &error) {
            Trace::Log("RTMidiService", error.getMessage().c_str());
        }
    }

    // check outputs
    Trace::Log(
        "RTMidiService", "Listing output ports for API: %s",
        rtMidiOut_
            ? RtMidi::getApiDisplayName(rtMidiOut_->getCurrentApi()).c_str()
            : "None");
    nPorts = (rtMidiOut_) ? rtMidiOut_->getPortCount() : 0;
    Trace::Log("RTMidiService", "%d output port(s)", nPorts);
    for (uint i = 0; i < nPorts; i++) {
        try {
            std::string portName = rtMidiOut_->getPortName(i);
            RTMidiOutDevice *out =
                new RTMidiOutDevice(*rtMidiOut_, i, portName.c_str());
            Trace::Log("RTMidiService", " %s", portName.c_str());
            Insert(out);
        } catch (RtMidiError &error) {
            Trace::Log("RTMidiService", error.getMessage().c_str());
        }
    }
}
