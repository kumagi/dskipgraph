#ifndef DSG_LOGIC_HPP
#define DSG_LOGIC_HPP

#include <boost/function.hpp>
#include <map>
#include <string>
#include <algorithm>
#include <boost/functional/hash.hpp>


#include "msgpack_macro.h"

#include <boost/function.hpp>
#include <boost/any.hpp>

#include "msgpack.hpp"
#include <msgpack/rpc/server.h>
#include "shared_object.hpp"
#include "dskipgraph.h"

typedef std::string key;
typedef std::string value;

RPC_OPERATION1(die, std::string, name);
RPC_OPERATION1(dump, std::string, name);
RPC_OPERATION2(set, key, set_key, value, set_value);
RPC_OPERATION4(link, key, target_key, int, level, key, org_key, host, origin);

RPC_OPERATION2(found, key, found_key, value, found_value);
RPC_OPERATION1(notfound, key, target_key);
//RPC_OPERATION2(range_search, range, target_range, host, origin)
//RPC_OPERATION3(range_found, range, target_range, std::vector<key>,keys, std::vector<value>,values);
//RPC_OPERATION2(range_notfound, range, target_range, std::vector<key>, keys);
RPC_OPERATION3(treat, key, org_key,host,origin, membership_vector, org_vector);
RPC_OPERATION5(introduce, key, org_key, key, target_key, host, origin,
	membership_vector, org_vector, int, level);

RPC_OPERATION3(search, key, target_key, int, level, host, origin);
namespace logic{

// local copy
mp::sync<std::vector<host> >& otherhost = shared_data::instance().otherhost;
std::vector<boost::shared_ptr<node> >& nodelist = shared_data::instance().nodelist;
const host& localhost = shared_data::instance().localhost;
const membership_vector& localvector = shared_data::instance().localvector;


template <typename request, typename server>
void die(request*, server*){
	exit(0);
};

template <typename request, typename server>
void dump(request*, server*){
	std::cout << "vector:" << localvector << std::endl
						<< "host:" << localhost << std::endl
						<< "nodelist [" << nodelist << "]" << std::endl
						<< "otherhost [" << otherhost << "]" << std::endl;
};
}
typedef range<key> range_t;
typedef std::vector<range_t> range_vector;
namespace std{
std::ostream& operator<<(std::ostream& ost, const range_vector& rangelist){
	return ost;
};
}
namespace logic{


RPC_OPERATION5(set_r, key, expect_right, key, new_right, host,origin,int,sec_num, int,level);
RPC_OPERATION1(set_r_ack, key, target_origin);
RPC_OPERATION1(set_r_nak, key, target_origin);
RPC_OPERATION4(set_l, key,new_left, host, origin, int, sec_num, int,level);
RPC_OPERATION1(tellme_region, int, dummy);
RPC_OPERATION2(answer_region, range_vector, region, int, highest_load);

struct answer_region{ // response
	std::vector<range<key> > region; // all regions I have
	std::vector<int> load_map; // load avarage of each regions (for future
	MSGPACK_DEFINE(region, load_map);
};
struct offer_migrate_region{ // offer
	range<key> target_region;
	range<key> expected_region; // cas semantics
	MSGPACK_DEFINE(target_region, expected_region);
};
struct migration{ // responce
	range<key> offered_region;
	std::vector<key> key_list;
	std::vector<value> value_list;
	MSGPACK_DEFINE(offered_region, key_list, value_list);
};
struct non_migration{ // response
	range<key> offered_region;
	MSGPACK_DEFINE(offered_region);
};
}
namespace logic{

template <typename request, typename server>
void set_r(request* req, server* sv){
	
}

template <typename request, typename server>
void set_r_ack(request* req, server* sv){
	
}
template <typename request, typename server>
void set_r_nak(request* req, server* sv){
	
}

template <typename request, typename server>
void set_l(request* req, server* sv){
	
}

template <typename request, typename server>
void unref_l(request* req, server* sv){
	
}

template <typename request, typename server>
void get_nearest(request* req, server* sv){
}

template <typename request, typename server>
void tellme_region(request* req, server* sv){
}


template <typename request, typename server>
void answer_region(request* req, server* sv){
}
template <typename request, typename server>
void offer_migrate_region(request* req, server* sv){
}
template <typename request, typename server>
void migration(request* req, server* sv){
}
template <typename request, typename server>
void non_migration(request* req, server* sv){
}

}
#endif
