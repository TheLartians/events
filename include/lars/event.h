#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <utility>
#include <mutex>

namespace lars{
  
  template <typename ... Args> class Event;
  
  class Observer{
  public:
    struct Base{ virtual ~Base(){} };
    
    Observer(){}
    Observer(Observer &&other) = default;
    template <typename L> Observer(L && l):data(new L(std::move(l))){ }
    
    Observer & operator=(const Observer &other) = delete;
    Observer & operator=(Observer &&other) = default;
    
    template <typename L> Observer & operator=(L && l){ 
      data.reset(new L(std::move(l))); 
      return *this; 
    }
    
    template <typename H,typename ... Args> void observe(Event<Args...> & event,const H &handler){
      data.reset(new typename Event<Args...>::Observer(event.createObserver(handler)));
    }
    
    void reset(){ data.reset(); }
    operator bool() const { return bool(data); }
    
  private:
    std::unique_ptr<Base> data;
  };
  
  template <typename ... Args> class Event{
  private:

    using Handler = std::function<void(const Args &...)>;
    using HandlerID = size_t;

    struct StoredHandler {
      HandlerID id;
      Handler callback;
    };

    using HandlerList = std::vector<StoredHandler>;
    
    struct Data {
      HandlerID IDCounter = 0;
      HandlerList observers;
      std::mutex observerMutex;
    };

    std::shared_ptr<Data> data;

    HandlerID addHandler(Handler h)const{
      std::lock_guard<std::mutex> lock(data->observerMutex);
      data->observers.emplace_back(StoredHandler{data->IDCounter,h});
      return data->IDCounter++;
    }
        
  public:
    
    struct Observer:public lars::Observer::Base{
      std::weak_ptr<Data> data;
      HandlerID id;
      
      Observer(){}
      Observer(const std::weak_ptr<Data> &_data, HandlerID _id):data(_data), id(_id){}
      
      Observer(Observer &&other) = default;
      Observer(const Observer &other) = delete;
      
      Observer & operator=(const Observer &other) = delete;
      Observer & operator=(Observer &&other)=default;
      
      void observe(const Event &event, const Handler &handler){
        *this = event.createObserver(handler);
      }

      void reset(){
        if(auto d = data.lock()){ 
          std::lock_guard<std::mutex> lock(d->observerMutex);
          auto it = std::find_if(d->observers.begin(), d->observers.end(), [&](auto &o){ return o.id == id; });
          if (it != d->observers.end()) { d->observers.erase(it); }
        }
        data.reset();
      }
      
      ~Observer(){ reset(); }
    };
    
    Event():data(std::make_shared<Data>()){
    }
   
    void emit(Args ... args) const {
      data->observerMutex.lock();
      auto tmpObservers = data->observers;
      data->observerMutex.unlock();
      for(auto &observer: tmpObservers){
        observer.callback(args...);
      }
    }
    
    Observer createObserver(const Handler &h)const{
      return Observer(data, addHandler(h));
    }
    
    void connect(const Handler &h)const{
      addHandler(h);
    }
    
    void clearObservers(){
      std::lock_guard<std::mutex> lock(data->observerMutex);
      data->observers.clear();
    }
    
    size_t observerCount() const {
      std::lock_guard<std::mutex> lock(data->observerMutex);
      return data->observers.size();
    }

  };
  
}
