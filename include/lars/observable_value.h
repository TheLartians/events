#pragma once

#include <lars/event.h>
#include <tuple>
#include <type_traits>

namespace lars {

  template <class T> class ObservableValue {
  protected:
    T value;

  public:
    using OnChange = Event<const T &>;
    OnChange onChange;

    template <typename ... Args> ObservableValue(Args ... args):value(std::forward<Args>(args)...){
    }

    template <typename ... Args> void set(Args ... args){ 
      value = T(std::forward<Args>(args)...);
      onChange.notify(value);
    }
    
    const T & get()const{ 
      return value;
    }
    
    const T & operator*()const{ 
      return value;
    }
    
    const T * operator->()const{ 
      return &value;
    }
    
  };

  template <class T> explicit ObservableValue(T) -> ObservableValue<T>;

  template <class T, typename ... D> class DependentObservableValue: public ObservableValue<T> {
  private:
    std::tuple<typename ObservableValue<D>::OnChange::Observer ...> observers;
  
  public:

    DependentObservableValue(
      const std::function<T(const D &...)> &handler,
      const ObservableValue<D> & ... deps
    ):ObservableValue<T>(handler(deps.get()...)){
      auto update = [this,&deps...,handler](auto &){ this->set(handler(deps.get()...)); };
      observers = std::make_tuple(deps.onChange.createObserver(update)...);
    }

  };

  template<class F, typename ... D> explicit DependentObservableValue(
    F,
    ObservableValue<D>...
  ) -> DependentObservableValue<typename std::invoke_result<F, D...>::type, D...>;

}