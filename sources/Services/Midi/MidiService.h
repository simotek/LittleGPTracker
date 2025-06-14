
#ifndef _MIDI_SERVICE_H_
#define _MIDI_SERVICE_H_

#include "Foundation/Observable.h"
#include "Foundation/T_Factory.h"
#include "MidiInDevice.h"
#include "MidiInMerger.h"
#include "MidiOutDevice.h"
#include "System/Process/ConcurrentQueue.h"
#include "System/Timer/Timer.h"
#include <string>

#define MIDI_MAX_BUFFERS 20

class MidiService : public T_Factory<MidiService>,
                    public T_SimpleList<MidiOutDevice>,
                    public I_Observer {

  public:
    MidiService();
    virtual ~MidiService();

    bool Init();
    void Close();
    bool Start();
    void Stop();

    void SelectDevice(const std::string &name);

    I_Iterator<MidiInDevice> *GetInIterator();

    //! player notification

    void OnPlayerStart();
    void OnPlayerStop();

    //! Queues a MidiMessage to the current time chunk

    void QueueMessage(MidiMessage &);

    //! Time chunk trigger
    void Trigger();
    //! Flush current queue to the output
    void Flush();

  protected:
    T_SimpleList<MidiInDevice> inList_;

    virtual void Update(Observable &o, I_ObservableData *d);
    void onAudioTick();

    //! start the selected midi device

    void startInDevice();
    void startOutDevice();

    //! stop the selected midi device

    void stopInDevice();
    void stopOutDevice();

    //! build the list of available drivers

    virtual void buildDriverList() = 0;

  private:
    void flushOutQueue();

  private:
    std::string deviceName_;
    MidiInDevice *inDevice_;
    MidiOutDevice *outDevice_;
    moodycamel::ConcurrentQueue<MidiMessage> midiQueue_;

  MidiInMerger *merger_;
  int midiDelay_;
  int tickToFlush_;
  bool sendSync_;
};
#endif
