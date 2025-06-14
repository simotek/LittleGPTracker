#include "MidiService.h"
#include "Application/Model/Config.h"
#include "Application/Player/SyncMaster.h"
#include "Services/Audio/AudioDriver.h"
#include "System/Console/Trace.h"
#include "System/Timer/Timer.h"

#ifdef SendMessage
#undef SendMessage
#endif
	
MidiService::MidiService()
    : T_SimpleList<MidiOutDevice>(true), inList_(true), inDevice_(NULL), outDevice_(NULL),
      sendSync_(true) {

    const char *delay = Config::GetInstance()->GetValue("MIDIDELAY");
	midiDelay_ = delay?atoi(delay):1;

	const char *sendSync = Config::GetInstance()->GetValue("MIDISENDSYNC");
	if (sendSync) {
		sendSync_ = (strcmp(sendSync,"YES")==0);
	}
}

MidiService::~MidiService() { Close(); }

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
}

void MidiService::Close() {
	Stop();
    inDevice_->Stop();
    inDevice_->Close();
}

I_Iterator<MidiInDevice> *MidiService::GetInIterator() {
	return inList_.GetIterator();
}

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

bool MidiService::Start() { return true; };

void MidiService::Stop() { stopOutDevice(); };

void MidiService::QueueMessage(MidiMessage &m) {
    // Trace::Log("MidiService", "QueueMessage: status=0x%X", m.status_);
	midiQueue_.enqueue(m);
}

void MidiService::Trigger() {

    if (outDevice_ && sendSync_) {
        SyncMaster *sm=SyncMaster::GetInstance();
		if (sm->MidiSlice()) {
			MidiMessage msg;
			msg.status_ = 0xF8;
			QueueMessage(msg);
		}
	}
}

void MidiService::Update(Observable &o, I_ObservableData *d) {
    AudioDriver::Event *event = (AudioDriver::Event *)d;
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
}

void MidiService::flushOutQueue() {
    if (!outDevice_)
        return;
    MidiMessage msg;
    T_SimpleList<MidiMessage> batch; // <-- use T_SimpleList
    // Drain all messages currently in the queue
    while (midiQueue_.try_dequeue(msg)) {
        batch.Insert(msg); // Assuming Add() is the method to append
        // Trace::Log("MidiService", "flushOutQueue: status=0x%X", msg.status_);
    }
    if (batch.Size() > 0) { // Assuming IsEmpty() checks if list is empty
        outDevice_->SendQueue(batch);
		// Trace::Log("MidiService", "flushOutQueue: batch=0x%X", batch);
    }
}

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
                    Trace::Log("MidiService", "midi device %s started",
                               deviceName_.c_str());
                    outDevice_ = &current;
				} else {
                    Trace::Log("MidiService", "midi device %s failed to start",
                               deviceName_.c_str());
					current.Close();
				}
			}
			break;
		}
	}
}

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
}

/*
 * queues midi stop message when player stops
 */
void MidiService::OnPlayerStop() {
	if (sendSync_) {
		MidiMessage msg ;
		msg.status_=0xFC ;
		QueueMessage(msg) ;
	}
}
