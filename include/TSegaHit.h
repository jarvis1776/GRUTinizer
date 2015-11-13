#ifndef _TSEGAHIT_H_
#define _TSEGAHIT_H_

#include "TDetectorHit.h"
#include "TSegaSegmentHit.h"

#define MAX_TRACE_LENGTH 100

class TSegaHit : public TDetectorHit {
public:
  TSegaHit();

  virtual void Copy(TObject&) const;
  virtual void Clear(Option_t *opt = "");
  virtual void Print(Option_t *opt = "") const;

  virtual int Charge() const { return fCharge;}

  int GetDetnum();
  unsigned int GetNumSegments() const { return fSegments.size(); }
  TSegaSegmentHit& GetSegment(int i) { return fSegments.at(i); }

  void SetCharge(int chg)   { fCharge  = chg;   }

  std::vector<unsigned short>& GetTrace() { return fTrace; }

  void SetTrace(unsigned int trace_length, const unsigned short* trace);

  long GetTimestamp() const { return fTimestamp; }
  void SetTimestamp(long ts) { fTimestamp = ts; }

  TSegaSegmentHit& MakeSegmentByAddress(unsigned int address);

  int GetSlot() const;
  int GetCrate() const;
  int GetChannel() const;

private:
  long fTimestamp;
  int fCharge;
  int fCfd;

  std::vector<unsigned short> fTrace;
  std::vector<TSegaSegmentHit> fSegments;


  ClassDef(TSegaHit,3);
};

#endif /* _TSEGAHIT_H_ */
