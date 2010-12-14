#ifndef SHARED_OBJECT_HPP
#define SHARED_OBJECT_HPP
#include "singleton.hpp"
#include <boost/shared_ptr.hpp>
#include "dskipgraph.h"

struct shared_data : public singleton<shared_data>{
	// host info
	host localhost;
	mp::sync<std::vector<host> > otherhost;
	
	// storage
	std::vector<boost::shared_ptr<node> > nodelist;
	// vector
	membership_vector localvector;
};

inline
std::ostream& operator<<(std::ostream& ost, mp::sync<std::vector<host> >& hosts){
	mp::sync<std::vector<host> >::ref h_ref(hosts);
	std::vector<host>::const_iterator it = h_ref->begin();
	while(it != h_ref->end()){
		ost << *it << " : ";
		++it;
	}
	return ost;
}
	
	
#endif
