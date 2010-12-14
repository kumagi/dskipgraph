#ifndef SINGLETON_HPP
#define SINGLETON_HPP

#include <boost/noncopyable.hpp>
// singleton
template<typename Derived>
class singleton : private boost::noncopyable {
public:
  static Derived& instance() { static Derived the_inst; return the_inst; }
protected:
  singleton(){}
  ~singleton(){}
};


#endif
