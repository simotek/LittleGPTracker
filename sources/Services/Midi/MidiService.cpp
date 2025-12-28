#include "MidiService.h"
#include "Application/Model/Config.h"
#include "Application/Player/SyncMaster.h"
#include "Services/Audio/AudioDriver.h"
#include "System/Console/Trace.h"
#include "System/Timer/Timer.h"
#include "System/Console/Trace.h"

#ifdef SendMessage
#undef SendMessage
#endif
	
MidiService::MidiService()
    : T_SimpleList<MidiOutDevice>(true), inList_(true), outDevice_(0),
      sendSync_(true) {
#ifndef _FEAT_MIDI_MULTITHREAD
    for (int i=0;i<MIDI_MAX_BUFFERS;i++) {
		queues_[i]=new T_SimpleList<MidiMessage>(true);
	}
#endif
	const char *delay = Config::GetInstance()->GetValue("MIDIDELAY");
	midiDelay_ = delay?atoi(delay):1;

	const char *sendSync = Config::GetInstance()->GetValue("MIDISENDSYNC");
	if (sendSync) {
		sendSync_ = (strcmp(sendSync,"YES")==0);
	}
};

MidiService::~MidiService() {
	Close();
};

bool MidiService::Init() {
	Empty();
  inList_.Empty();
	buildDriverList();
	// Add a merger for the input
	merger_=new MidiInMerger();
	IteratorPtr<MidiInDevice>it(inList_.GetIterator());
	for (it->Begin();!it->IsDone();it->Next()) {
		MidiInDevice &current=it->CurrentItem();
		merger_->Insert(current);
	}

	return true;
};

void MidiService::Close() {
	Stop();
};

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
#ifndef _FEAT_MIDI_MULTITHREAD
    currentPlayQueue_ = 0;
    currentOutQueue_ = 0;
#endif
    return true;
}

void MidiService::Stop() { stopOutDevice(); };

#ifdef _FEAT_MIDI_MULTITHREAD
// For multi-threaded systems we use a concurrentqueue
void MidiService::QueueMessage(MidiMessage &m) {
    if (!outDevice_)
        return;
    // Trace::Log("MidiService", "QueueMessage: status=0x%X", m.status_);

    midiQueue_.enqueue(m);
}
#else
// For single-threaded systems we do it the old way
void MidiService::QueueMessage(MidiMessage &m) {
	if (outDevice_) {
        SysMutexLocker locker(queueMutex_) ;
		T_SimpleList<MidiMessage> *queue=queues_[currentPlayQueue_];
		MidiMessage *ms=new MidiMessage(m.status_,m.data1_,m.data2_);
		queue->Insert(ms);
	}
};
#endif

void MidiService::Trigger() {
#ifndef _FEAT_MIDI_MULTITHREAD
	AdvancePlayQueue();
#endif
	if (outDevice_&&sendSync_) {
		SyncMaster *sm=SyncMaster::GetInstance();
		if (sm->MidiSlice()) {
			MidiMessage msg;
			msg.status_ = 0xF8;
			QueueMessage(msg);
		}
	}
}

#ifndef _FEAT_MIDI_MULTITHREAD
void MidiService::AdvancePlayQueue() {
    int next = (currentPlayQueue_ + 1) % MIDI_MAX_BUFFERS;
    queues_[next]->Empty();
    currentPlayQueue_ = next;
}
#endif

void MidiService::Update(Observable &o,I_ObservableData *d) {
  AudioDriver::Event *event=(AudioDriver::Event *)d;
  if (event->type_ == AudioDriver::Event::ADET_DRIVERTICK) {
    onAudioTick();
  }
};

void MidiService::onAudioTick() {
	if (tickToFlush_>0) {
		if (--tickToFlush_ ==0) {
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

#ifdef _FEAT_MIDI_MULTITHREAD
void MidiService::flushOutQueue() {
    if (!outDevice_)
        return;

    MidiMessage msg;
    T_SimpleList<MidiMessage> batch;
    // Drain all messages currently in the queue
    while (midiQueue_.try_dequeue(msg)) {
        batch.Insert(msg);
        // Trace::Log("MidiService", "flushOutQueue: status=0x%X", msg.status_);
    }
    if (batch.Size() > 0) {
        outDevice_->SendQueue(batch);
		// Trace::Log("MidiService", "flushOutQueue: batch=0x%X", batch);
    }
}
#else
void MidiService::flushOutQueue() {
    int next = (currentOutQueue_ + 1) % MIDI_MAX_BUFFERS;
    T_SimpleList<MidiMessage> *flushQueue = queues_[next];
    if (outDevice_) {
        outDevice_->SendQueue(*flushQueue);
    }

    flushQueue->Empty();
    currentOutQueue_ = next;  // Advance only after safe flush
}
#endif

/*
 * starts midi device
 */
void MidiService::startOutDevice() {
	IteratorPtr<MidiOutDevice>it(GetIterator()) ;

	for (it->Begin(); !it->IsDone(); it->Next()) {
		MidiOutDevice &current = it->CurrentItem();
		if (!strcmp(deviceName_.c_str(), current.GetName())) {
			if (current.Init()) {
				if (current.Start()) {
					Trace::Log("MidiService", "midi device %s started", deviceName_.c_str());
					outDevice_ = &current;
				} else {
					Trace::Log("MidiService", "midi device %s failed to start", deviceName_.c_str());
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
void MidiService::stopOutDevice() {
    if (outDevice_) {
        outDevice_->Stop();
        outDevice_->Close();
	}
    outDevice_ = 0;
}

/*
 * starts midi device when playback starts
 */
void MidiService::OnPlayerStart() {
	if (deviceName_.size()!=0) {
		stopOutDevice();
		startOutDevice();
		deviceName_="";
	} else {
        startOutDevice();
  }

	if (sendSync_) {
		MidiMessage msg ;
		msg.status_=0xFA ;
		QueueMessage(msg) ;
	}
};

/*
 * queues midi stop message when player stops
 */
void MidiService::OnPlayerStop() {
	if (sendSync_) {
		MidiMessage msg ;
		msg.status_=0xFC ;
		QueueMessage(msg) ;
	}
};
