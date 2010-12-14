#ifndef REFLECTION_H
#define REFLECTION_H


#include <string>
#include <algorithm>

#include <msgpack.h>
#include <msgpack/rpc/server.h>

#include <boost/unordered_map.hpp>
#include <boost/function.hpp>
#include <boost/functional/hash.hpp>
#include <boost/function.hpp>
#include <boost/any.hpp>

#include "msgpack_macro.h"



typedef std::string key;
typedef std::string value;

template <typename Server> 
class reflection{
public:
	typedef typename boost::function <void(msgpack::rpc::request*, Server*)>
	reaction;
	typedef boost::unordered_map<std::string, reaction> table;
private:
	table func_table;
public:
	void regist(const std::string& name, reaction func){
		assert(func_table.find(name) == func_table.end() &&"you can't register one function two times");
		typedef std::pair<typename table::iterator, bool> its;
		its it = func_table.insert(std::make_pair(name,func));
		assert(it.second != false && "double insertion");
	}
	bool call(const std::string& name, msgpack::rpc::request* req,
		Server* sv)const{
		typename table::const_iterator it = func_table.find(name);
		if(it == func_table.end()) {
			assert(!"undefined function calll");
			return false;
		}
		it->second(req,sv);
		return true;
	}
	//const name_to_func* get_table_ptr(void)const{return &table;};
};



#endif
