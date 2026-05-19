#include "MidiService.h"
#include "Application/Player/SyncMaster.h"
#include "System/Console/Trace.h"
#include "System/Timer/Timer.h"
#include "Application/Model/Config.h"
#include "Services/Audio/AudioDriver.h"
#include "System/Console/Trace.h"

#ifdef SendMessage
#undef SendMessage
#endif

MidiService::MidiService()
    : T_SimpleList<MidiOutDevice>(true), inList_(true), device_(0),
      sendSync_(true) {
    for (int i = 0; i < MIDI_MAX_BUFFERS; i++) {
        queues_[i] = new T_SimpleList<MidiMessage>(true);
    }

    const char *delay = Config::GetInstance()->GetValue("MIDIDELAY");
    midiDelay_ = delay ? atoi(delay) : 1;

    const char *sendSync = Config::GetInstance()->GetValue("MIDISENDSYNC");
    if (sendSync) {
        sendSync_ = (strcmp(sendSync, "YES") == 0);
    }
};

MidiService::~MidiService() { Close(); };

bool MidiService::Init() {
    Empty();
    inList_.Empty();
    buildDriverList();
    // Add a merger for the input
    merger_ = new MidiInMerger();
    IteratorPtr<MidiInDevice> it(inList_.GetIterator());
    for (it->Begin(); !it->IsDone(); it->Next()) {
        MidiInDevice &current = it->CurrentItem();
        merger_->Insert(current);
    }

    return true;
};

void MidiService::Close() { Stop(); };

I_Iterator<MidiInDevice> *MidiService::GetInIterator() {
    return inList_.GetIterator();
};

void MidiService::SelectDevice(const std::string &name) {
    deviceName_ = name;

    // MidiIn is always running for the current device.
    // If the selected device is the same as the current device
    // then we don't need to stop and start it again
    bool skipStart = false;
    if (inDevice_) {
        if (deviceName_ != std::string(inDevice_->GetName())) {
            inDevice_->Stop();
            inDevice_->Close();
            inDevice_ = NULL;
        } else {
            skipStart = true;
        }
    }
    if (!skipStart) {
        // Start new inDevice, the outDevice starts when play starts
        IteratorPtr<MidiInDevice> it(GetInIterator());
        for (it->Begin(); !it->IsDone(); it->Next()) {
            MidiInDevice &in = it->CurrentItem();
            if (deviceName_ == std::string(in.GetName())) {
                if (in.Init()) {
                    inDevice_ = &in;
                    if (inDevice_->Start()) {
                        Trace::Log(
                            "MIDI",
                            "Controlling activated for MIDI interface %s",
                            in.GetName());
                    } else {
                        Trace::Log(
                            "MIDI",
                            "Failed to start input for MIDI interface %s",
                            in.GetName());
                        inDevice_->Close();
                    }
                }
            }
        }
    }
    if (inDevice_ == NULL) {
        Trace::Log("MIDI", "MIDI interface %s not found", deviceName_.c_str());
    }
}

bool MidiService::Start() {
    currentPlayQueue_ = 0;
    currentOutQueue_ = 0;

    return true;
}

void MidiService::Stop() { stopDevice(); }

#ifdef _FEAT_MIDI_MULTITHREAD
// For multi-threaded systems we use a concurrentqueue
void MidiService::QueueMessage(MidiMessage &m) {
    if (device_) {
#ifdef _FEAT_MIDI_LOCK
        SysMutexLocker locker(queueMutex_);
#endif
        T_SimpleList<MidiMessage> *queue = queues_[currentPlayQueue_];
        MidiMessage *ms = new MidiMessage(m.status_, m.data1_, m.data2_);
        queue->Insert(ms);
    }
};

void MidiService::Trigger() {
    AdvancePlayQueue();
    if (device_ && sendSync_) {
        SyncMaster *sm=SyncMaster::GetInstance();
		if (sm->MidiSlice()) {
            MidiMessage msg;
            msg.status_ = 0xF8;
            QueueMessage(msg);
        }
    }
}

void MidiService::AdvancePlayQueue() {
#ifdef _FEAT_MIDI_LOCK
    SysMutexLocker locker(queueMutex_);
#endif
    int next = (currentPlayQueue_ + 1) % MIDI_MAX_BUFFERS;
    if (queueMutex_.TryLock()) {
        // Only clear AFTER successful lock — avoids data loss
        queues_[next]->Empty();
        queueMutex_.Unlock();
        currentPlayQueue_ = next;
    }
}

void MidiService::Update(Observable &o, I_ObservableData *d) {
    AudioDriver::Event *event = (AudioDriver::Event *)d;
    if (event->type_ == AudioDriver::Event::ADET_DRIVERTICK) {
        onAudioTick();
    }
};

void MidiService::onAudioTick() {
    if (tickToFlush_ > 0) {
        if (--tickToFlush_ == 0) {
            flushOutQueue();
        }
    }
}

void MidiService::Flush() {
    tickToFlush_ = midiDelay_;
    if (tickToFlush_ == 0) {
        flushOutQueue();
    }
};

void MidiService::flushOutQueue() {
#ifdef _FEAT_MIDI_LOCK
    SysMutexLocker locker(queueMutex_);
#endif
    int next = (currentOutQueue_ + 1) % MIDI_MAX_BUFFERS;

    if (queueMutex_.TryLock()) {
        T_SimpleList<MidiMessage> *flushQueue = queues_[next];

        if (device_) {
            device_->SendQueue(*flushQueue);
        }

        flushQueue->Empty();
        currentOutQueue_ = next; // Advance only after safe flush
        queueMutex_.Unlock();
    }
}

/*
 * starts midi device
 */
void MidiService::startDevice() {
    IteratorPtr<MidiOutDevice> it(GetIterator());

    for (it->Begin(); !it->IsDone(); it->Next()) {
        MidiOutDevice &current = it->CurrentItem();
        if (!strcmp(deviceName_.c_str(), current.GetName())) {
            if (current.Init()) {
                if (current.Start()) {
                    Trace::Log("MidiService", "midi device %s started",
                               deviceName_.c_str());
                    device_ = &current;
                } else {
                    Trace::Log("MidiService", "midi device %s failed to start",
                               deviceName_.c_str());
                    current.Close();
                }
            }
            break;
        }
    }
};

/*
 * closes midi device
 */
void MidiService::stopDevice() {
    if (device_) {
        device_->Stop();
        device_->Close();
    }
    device_ = 0;
};

/*
 * starts midi device when playback starts
 */
void MidiService::OnPlayerStart() {
    if (deviceName_.size() != 0) {
        stopDevice();
        startDevice();
        deviceName_ = "";
    } else {
        startDevice();
    }

    if (sendSync_) {
        MidiMessage msg;
        msg.status_ = 0xFA;
        QueueMessage(msg);
    }
};

/*
 * queues midi stop message when player stops
 */
void MidiService::OnPlayerStop() {
    if (sendSync_) {
        MidiMessage msg;
        msg.status_ = 0xFC;
        QueueMessage(msg);
    }
};
