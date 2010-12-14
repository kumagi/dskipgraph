#ifndef DSKIPGRAPH_H
#define DSKIPGRAPH_H
#include <stdint.h>
#include <string>
#include <msgpack/rpc/server.h>
#include <mp/sync.h>

#include <iostream>
#include <boost/io/ios_state.hpp>
#include <boost/noncopyable.hpp>
#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/functional/hash.hpp>
#include <boost/unordered_set.hpp>

enum side{
	left = 0,
	right = 1,
};
enum side inverse(enum side s){return static_cast<side>(1-static_cast<int>(s));}


struct host{
	std::string name;
	uint16_t port;
	host(){}
	host(const std::string& name_, const uint16_t& port_)
		:name(name_),port(port_){}

	msgpack::rpc::ip_address get_address()const{
		return msgpack::rpc::ip_address(name,port);
	}
	static host get_host(const msgpack::rpc::ip_address& ad){
		sockaddr_in addr;
		ad.get_addr(reinterpret_cast<sockaddr*>(&addr));
		in_addr ip = addr.sin_addr;
		return host(inet_ntoa(ip), ad.get_port());
	}
	bool operator==(const host& rhs)const{
		return name == rhs.name && port == rhs.port;
	}
	bool operator!=(const host& rhs)const{
		return !(operator==(rhs));
	}
	bool operator<(const host& rhs)const{
		if(name < rhs.name)return true;
		else if(rhs.name < name) return false;
		else if(port < rhs.port) return true;
		else return false;
	}
	struct hash{
		size_t operator()(const host& h)const{
			return boost::hash<std::string>()(h.name) + h.port;
		}
	};
	MSGPACK_DEFINE(name,port);
};
std::ostream&  operator<<(std::ostream& ost, const host& h){
	ost << "(" << h.name << ":" << h.port << ")";
	return ost;
}

struct membership_vector{
	uint64_t vector;
	explicit membership_vector(uint64_t v):vector(v){}
	membership_vector():vector(0){}
	int match(const membership_vector& o)const{
		const uint64_t matched = ~(vector ^ o.vector);
		uint64_t bit = 1;
		int cnt = 0;
		while((matched & bit) && cnt < 64){
			bit *= 2;
			cnt++;
		}
		return cnt;
	}
	void dump()const{
		const char* bits = reinterpret_cast<const char*>(&vector);
		for(int i=7;i>=0;--i){
			fprintf(stderr,"%02x",(unsigned char)255&bits[i]);
		}
	}
	bool operator==(const membership_vector& rhs)const
	{ return vector == rhs.vector;}
	MSGPACK_DEFINE(vector); // serialize and deserialize ok
};
std::ostream&  operator<<(std::ostream& ost, const membership_vector& v){
	boost::io::ios_flags_saver ifs( ost );
	ost << std::hex << v.vector;
	return ost;
} 

typedef std::string key;

template <typename key>
class range{
	key begin_,end_;
	bool border_begin_,border_end_; // true = contain, false = not
public:
// construction
	range()
		:begin_(),end_(),border_begin_(true),border_end_(false) // it contains everything
	{}
	
	range(const range& org)
		:begin_(org.begin_),
		 end_(org.end_),
		 border_begin_(org.border_begin_),
		 border_end_(org.border_end_){assert(begin_<end_ || 
				 (begin_ == end_ && (border_begin_ || border_end_)));}
	range(const key& _begin, const key& _end,
				bool _border_begin = true, bool _border_end = false) // [ ) is default
		:begin_(_begin),end_(_end),border_begin_(_border_begin),border_end_(_border_end){}
	
	// getter
	const key& min()const{return begin_;}
	const key& max()const{return end_;}
	bool min_contains()const{return border_begin_;}
	bool max_contains()const{return border_end_;}
	// setter
	key& min(){return begin_;}
	key& max(){return end_;}
	// checker
	bool contains(const key& t)const{
		if(begin_ == t && border_begin_) return true;
		if(end_ == t && border_end_) return true;
		if(begin_ < t && t < end_) return true;
		return false;
	}
	bool operator==(const range& rhs)const{
		return min() == rhs.min() && max() == rhs.max() 
			&& min_contains() == rhs.min_contains()
			&& max_contains() == rhs.max_contains();
	}
	MSGPACK_DEFINE(begin_,end_,border_begin_,border_end_)
};
typedef range<key> range_t;

template <typename key>
std::ostream&  operator<<(std::ostream& ost, const range_t& r){
	ost << (r.min_contains() ? "[" : "(") << r.min() << " ~ " << r.max()
			<< (r.max_contains() ? "]" : ")");
	return ost;
}

template <typename key>
std::ostream& operator<<(std::ostream& ost, const std::vector<key>& v){
	ost << "[";
	for(typename std::vector<key>::const_iterator it = v.begin();
			it != v.end() ; ++it){
		ost << (*it) << ",";
	}
	ost << "]";
	return ost;
}

class neighbor{
	range<key> keyrange_;
	host host_;
	friend struct hash;
public:
	neighbor(const range<key>& r,const host& h = host())
		:keyrange_(r),host_(h){}

	// getter
	const range<key>& get_range()const{return keyrange_;}
	const key& get_side(const enum side s)const{
		if(s == left) return keyrange_.min();
		assert(s == right);
		return keyrange_.max();
	}
	// setter
	range<key>& set_range(const range<key>& o){return keyrange_ = o;}
	const host& get_host()const{ return host_; }
	const msgpack::rpc::address get_address()const{ return host_.get_address(); }

	// comperator
	bool operator<(const neighbor& rhs)const{
		return keyrange_.min() < rhs.keyrange_.min();
	}
	bool operator==(const neighbor& rhs)const{
		return keyrange_ == rhs.keyrange_	&& host_ == rhs.host_;
	}
	struct hash{
		size_t operator()(const neighbor& neighbor)const{
			return boost::hash<key>()(neighbor.keyrange_.min())
				+ boost::hash<key>()(neighbor.keyrange_.max())
				+ host::hash()(neighbor.host_);
		}
	};
};
typedef boost::shared_ptr<neighbor> shared_neighbor;
typedef boost::weak_ptr<neighbor> weak_neighbor;
bool operator==(const weak_neighbor& lhs, const weak_neighbor& rhs){
	shared_neighbor lhs_lk(lhs.lock()),rhs_lk(rhs.lock());
	assert(lhs_lk != NULL); assert(rhs_lk != NULL);
	return *lhs_lk == *rhs_lk;
}
struct shared_neighbor_hash{
	size_t operator()(const shared_neighbor& h)const{
		return neighbor::hash()(*h);
	}
};
struct weak_neighbor_hash{
	size_t operator()(const weak_neighbor& h)const{
		shared_neighbor sn(h.lock());
		return neighbor::hash()(*sn);
	}
};
typedef boost::unordered_set<weak_neighbor, weak_neighbor_hash> weak_neighbor_set;
mp::sync<weak_neighbor_set > neighbor_list;


std::ostream& operator<<(std::ostream& ost, const neighbor& nbr){
	ost << nbr.get_range() << "#" << nbr.get_host();
	return ost;
}

class node : public boost::noncopyable{
public:
	enum state{
		out, // out of global list
		ins, // wait for ack or nak for set_r
		inswait, // received set_r_nak and in backoff
		in, // right node ok
		del, // sent set_r for left for deletion
		delwait, // received set_r_nak for deletion and in backoff
		grace, // deleted and wait for perfectry unrefed
	};
	typedef std::pair<key,std::string> kvp;
	typedef boost::array<shared_neighbor,2> both_side;
	typedef boost::array<size_t, 2> seq_number;
	range_t myrange;
	
	typedef std::vector<kvp> bucket_t;
	typedef bucket_t::iterator bucket_iterator;
	typedef mp::sync<bucket_t>::ref bucket_ref;
	mp::sync<bucket_t> bucket;
	
	std::vector<both_side> neighbors;
	std::vector<seq_number> seq_numbers;
	std::vector<state> states;
public:
	node(){}
	explicit node(const range_t& r):myrange(r){}

	// getter
	const range_t& range()const{return myrange;}
	bool neighbor_in_lv_exists(const size_t lv, enum side s)const{
		assert(lv < neighbors.size());
		return (neighbors[lv][static_cast<size_t>(s)] != NULL);
	}
	const key& neighbor_in_lv_key(const size_t lv, enum side s)const{
		assert(neighbor_in_lv_exists(lv,s));
		return neighbors[lv][static_cast<size_t>(s)]->get_side(inverse(s));
	}
	shared_neighbor& neighbor_in_lv(const size_t lv, enum side s){
		return neighbors[lv][static_cast<size_t>(s)];
	}
	size_t get_seq_number(const size_t lv, enum side s)const{
		assert(neighbor_in_lv_exists(lv,s));
		return seq_numbers[lv][static_cast<size_t>(s)];
	}
	size_t& seq_number_ref(const size_t lv, enum side s){
		assert(neighbor_in_lv_exists(lv,s));
		return seq_numbers[lv][static_cast<size_t>(s)];
	}
	state& state_ref(const size_t lv){
		assert(neighbor_in_lv_exists(lv,left));
		return states[lv];
	}
	int maxlevel()const{return neighbors.size();}
	void level_increment(){
		neighbors.push_back(both_side());
		seq_numbers.push_back(seq_number());
		states.push_back(out);
		assert(neighbors.size() == seq_numbers.size());
		assert(seq_numbers.size() == states.size());
	}
};

std::ostream& operator<<(std::ostream& ost, const node& n)
{
	ost << n.range().min() << "[";
	for(int i=0;i<2;i++){
		int max = n.maxlevel();
		for(int j=0;j<max;++j)
			if(n.neighbor_in_lv_exists(j,static_cast<enum side>(i)))
				ost << "lv{" << j << "}" << 
					n.neighbor_in_lv_key(j,static_cast<enum side>(i)) << " ";
		if(i==0){ost<< "|";}
	}
	ost << "]" << n.range().max();
	return ost;
}
/*
static std::ostream& operator<<(std::ostream& ost, const std::pair<key, sg_node>& kvp){
	ost << " key:" << kvp.first << " value:" << kvp.second;
	return ost;
}
*/
#endif
