#include "TJanus.h"

#include <cassert>
#include <iostream>

#include "TMath.h"

#include "JanusDataFormat.h"
#include "TNSCLEvent.h"

TJanus::TJanus() { }

TJanus::~TJanus(){ }

void TJanus::Copy(TObject& obj) const {
  TDetector::Copy(obj);

  TJanus& janus = (TJanus&)obj;
  janus.janus_hits = janus_hits;
}

void TJanus::Clear(Option_t* opt){
  TDetector::Clear(opt);

  janus_hits.clear();
}

int TJanus::BuildHits(std::vector<TRawEvent>& raw_data){
  for(auto& event : raw_data){
    TNSCLEvent& nscl = (TNSCLEvent&)event;
    SetTimestamp(nscl.GetTimestamp());
    Build_VMUSB_Read(nscl.GetPayloadBuffer());
  }
  return janus_hits.size();
}

TJanusHit& TJanus::GetJanusHit(int i){
  return janus_hits.at(i);
}

TDetectorHit& TJanus::GetHit(int i){
  return janus_hits.at(i);
}


void TJanus::Build_VMUSB_Read(TSmartBuffer buf){
  const char* data = buf.GetData();

  const VMUSB_Header* vmusb_header = (VMUSB_Header*)data;
  data += sizeof(VMUSB_Header);

  // This value should ALWAYS be zero, because that corresponds to the i1 trigger of the VMUSB.
  // If it is not, it is a malformed event.
  stack_triggered = vmusb_header->stack();

  // vmusb_header.size() returns the number of 16-bit words in the payload.
  // Each adc entry is a 32-bit word.
  // 6 additional 16-bit words for the timestamp (2 48-bit numbers)
  num_packets = vmusb_header->size()/2 - 3;

  const VME_Timestamp* vme_timestamp = (VME_Timestamp*)(data + num_packets*sizeof(CAEN_DataPacket));
  long timestamp = vme_timestamp->ts1() * 20;

  // std::cout << "JANUS timestamp at " << timestamp << std::endl;

  //buf.Print("all");

  std::map<unsigned int,TJanusHit> front_hits;
  std::map<unsigned int,TJanusHit> back_hits;
  for(int i=0; i<num_packets; i++){
    const CAEN_DataPacket* packet = (CAEN_DataPacket*)data;
    data += sizeof(CAEN_DataPacket);


    if(!packet->IsValid()){
      continue;
    }

    // ADCs are in slots 5-8, TDCs in slots 9-12
    bool is_tdc = packet->card_num() >= 9;
    unsigned int adc_cardnum = packet->card_num();
    if(is_tdc){
      adc_cardnum -= 4;
    }
    unsigned int address =
      (2<<24) + //system id
      (4<<16) + //crate id
      (adc_cardnum<<8) +
      packet->channel_num();

    TChannel* chan = TChannel::GetChannel(address);
    // Bad stuff, tell somebody to fix it
    static int lines_displayed = 0;
    if(!chan){
      if(lines_displayed < 1000) {
        // std::cout << "Unknown analog (slot, channel): ("
        //           << adc_cardnum << ", " << packet->channel_num()
        //           << "), address = 0x"
        //           << std::hex << address << std::dec
        //           << std::endl;
      } else if(lines_displayed==1000){
        std::cout << "I'm going to stop telling you that the channel was unknown,"
                  << " you should probably stop the program." << std::endl;
      }
      lines_displayed++;
      continue;
    }

    TJanusHit* hit = NULL;
    if(*chan->GetArraySubposition() == 'F'){
      hit = &front_hits[address];
    } else {
      hit = &back_hits[address];
    }

    hit->SetAddress(address);
    hit->SetTimestamp(timestamp);

    if(is_tdc){
      hit->SetTDCOverflowBit(packet->overflow());
      hit->SetTDCUnderflowBit(packet->underflow());
      hit->SetTime(packet->adcvalue());
    } else {
      hit->SetADCOverflowBit(packet->overflow());
      hit->SetADCUnderflowBit(packet->underflow());
      hit->SetCharge(packet->adcvalue());
    }
  }

  for(auto& elem : front_hits){
    TJanusHit& hit = elem.second;
    janus_channels.emplace_back(hit);
  }
  for(auto& elem : back_hits){
    TJanusHit& hit = elem.second;
    janus_channels.emplace_back(hit);
  }

  // Find all fronts with a reasonable TDC value
  int best_front = -1;
  int max_charge = -1;
  for(auto& elem : front_hits){
    TJanusHit& hit = elem.second;
    // if(hit.Time() > 200 && hit.Time() < 3900 &&
    //    hit.Charge() > max_charge){
    if(hit.Charge() > max_charge){
      best_front = elem.first;
      max_charge = hit.Charge();
    }
  }

  // Find all backs with a reasonable TDC value
  int best_back = -1;
  max_charge = -1;
  for(auto& elem : back_hits){
    TJanusHit& hit = elem.second;
    // if(hit.Time() > 200 && hit.Time() < 3900 &&
    //    hit.Charge() > max_charge) {
    if(hit.Charge() > max_charge) {
      best_back = elem.first;
      max_charge = hit.Charge();
    }
  }


  if(best_front != -1 && best_back != -1){
    //Copy most parameters from the front
    TJanusHit& front = front_hits[best_front];
    janus_hits.emplace_back(front);
    fSize++;
    TJanusHit& hit = janus_hits.back();

    //Copy more parameters from the back
    TJanusHit& back  = back_hits[best_back];
    hit.GetBackHit().SetAddress(back.Address());
    hit.GetBackHit().SetCharge(back.Charge());
    hit.GetBackHit().SetTime(back.Time());
    hit.GetBackHit().SetTimestamp(back.Timestamp());

  } //else {
//   static bool message_displayed = false;
//   //    if(!message_displayed){
//     std::cout << "Abnormal JANUS Event: " << good_fronts.size()
//               << ", " << good_backs.size() << std::endl;
//     for(auto good_front : good_fronts){
//       std::cout << "\tRing: " << std::hex << front_hits[good_front].Address() << std::dec<< "\tCharge: " << front_hits[good_front].Charge() << "\tTime: " << front_hits[good_front].Time() << std::dec << std::endl;
//     }
//     for(auto good_back : good_backs){
//       std::cout << "\tSector: " << std::hex << back_hits[good_back].Address() << std::dec << "\tCharge: " << back_hits[good_back].Charge() << "\tTime: " << back_hits[good_back].Time() << std::dec << std::endl;
//     }
//     message_displayed = true;
//     //    }
// }


  data += sizeof(VME_Timestamp);

  //assert(data == buf.GetData() + buf.GetSize());
  if(data != buf.GetData() + buf.GetSize()){
    std::cerr << "End of janus read not equal to size of buffer given:\n"
              << "\tBuffer Start: " << (void*)buf.GetData() << "\tBuffer Size: " << buf.GetSize()
              << "\n\tBuffer End: " << (void*)(buf.GetData() + buf.GetSize())
              << "\n\tNum ADC chan: " << num_packets
              << "\n\tPtr at end of read: " << (void*)(data)
              << "\n\tDiff: " << (buf.GetData() + buf.GetSize()) - data
              << std::endl;

    buf.Print("all");
  }
}

TVector3 TJanus::GetPosition(int detnum, int ring_num, int sector_num){
  if(detnum<0 || detnum>1 ||
     ring_num<1 || ring_num>24 ||
     sector_num<1 || sector_num>32){
    // batman vector, nan-nan-nan
    return TVector3(std::sqrt(-1),std::sqrt(-1),std::sqrt(-1));
  }

  TVector3 origin = TVector3(0,0,3);
  // Phi of sector 1 of downstream detector
  double phi_offset = 2*3.1415926535*(0.25);

  // Winding direction of sectors.
  bool clockwise = true;

  double janus_outer_radius = 3.5;
  double janus_inner_radius = 1.1;

  TVector3 position(1,0,0);  // Not (0,0,0), because otherwise SetPerp and SetPhi fail.
  double rad_slope = (janus_outer_radius - janus_inner_radius) /24;
  double rad_offset = janus_inner_radius;
  double perp_num = ring_num - 0.5; // Shift to 0.5-23.5
  position.SetPerp(perp_num*rad_slope + rad_offset);
  double phi_num = sector_num;
  double phi =
    phi_offset +
    (clockwise ? -1 : 1) * 2*3.1415926/32 * (phi_num - 1);
  position.SetPhi(phi);

  position += origin;

  if(detnum==0){
    position.RotateY(TMath::Pi());
  }

  return position;
}

void TJanus::InsertHit(const TDetectorHit& hit) {
  janus_hits.emplace_back((TJanusHit&)hit);
  fSize++;
}

void TJanus::Print(Option_t *opt) const {
  printf("TJanus @ %lu\n",Timestamp());
  printf(" Size: %i\n",Size());
  for(int i=0;i<Size();i++) {
    printf("\t"); janus_hits.at(i).Print(); printf("\n");
  }
  printf("---------------------------\n");

}

void TJanus::SetRunStart(unsigned int unix_time) {
  // Wed Jan 27 22:57:09 2016
  unsigned int previous = fRunStart==0 ? 1453953429 : fRunStart;
  int tdiff = unix_time - previous;
  long timestamp_diff = (1e9) * tdiff;

  fTimestamp += timestamp_diff;
  for(auto& hit : janus_hits) {
    hit.SetTimestamp(timestamp_diff + hit.Timestamp());
  }
}
