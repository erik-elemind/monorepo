#ifndef _HISTORY_VAR_H_
#define _HISTORY_VAR_H_

template<class T> 
class HistoryVar{
private:
protected:
  T curr_;
  T prev_;
public:
  HistoryVar() : HistoryVar(0,0){
  }
  HistoryVar(T curr, T prev): curr_(curr), prev_(prev){
  }
  virtual inline void init(T curr, T prev){
    curr_ = curr;
    prev_ = prev;
  }
  virtual inline void set(T curr){
    curr_ = curr;
  }
  virtual inline T get(){
    return curr_;
  }
  virtual inline T curr(){
    return curr_;
  }
  virtual inline T prev(){
    return prev_;
  }
  virtual inline void backup(){
    prev_ = curr_;
  }
  virtual inline bool changed(){
      return prev_ != curr_;
  }
};

class HistoryBool : public HistoryVar<bool>{
private:
protected:
public:
  HistoryBool():
    HistoryBool(false,false){
  }
  HistoryBool(bool curr, bool prev):
    HistoryVar<bool>(curr,prev){
  }
  virtual inline bool F2T(){
    return !prev_ && curr_;
  }
  virtual inline bool T2F(){
    return prev_ && !curr_;
  }
  virtual inline bool T2T(){
    return prev_ && curr_;
  }
  virtual inline bool F2F(){
    return !prev_ && !curr_;
  }      
};

#endif //_HISTORY_VAR_H_
