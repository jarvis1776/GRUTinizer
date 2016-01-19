#ifndef TGRUTVARIABLE_H
#define TGRUTVARIABLE_H

#include "TNamed.h"
#include <map>

class GValue : public TNamed {
public:
  GValue();
  GValue(const char *name);
  GValue(const char *name,double value);
  GValue(const GValue &val);

  double GetValue() { return fValue; }
  const char *GetInfo()   const { return info.c_str(); } 

  void SetValue(double value) { fValue = value; }
  void SetInfo(const char *temp) { info.assign(temp); }


  static int ReadValFile(const char *filename="",Option_t *opt="replace");

  //Search fValueVector for GValue with name given by string
  static GValue* FindValue(std::string);

  //Add value into static vector fValueVector
  static bool AddValue(GValue*, Option_t *opt="");

  bool AppendValue(GValue*);
  bool ReplaceValue(GValue*);

  //virtual void Clear(Option_t *opt="");
  //virtual void Print(Option_t *opt="") const;
  virtual void Copy(TObject &obj) const;
  //virtual bool Notify();

  int Size() const { return fValueVector.size(); }

private:
  double fValue;
  std::string info;
  static std::map<std::string,GValue*> fValueVector;
  static int  ParseInputData(std::string &input,Option_t *opt);
  static void trim(std::string *, const std::string &trimChars=" \f\n\r\t\v");

  ClassDef(GValue,1);
};

#endif
