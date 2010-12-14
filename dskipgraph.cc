
#include <string>
#include <map>

// external lib boost
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
#include <boost/program_options.hpp>
#include <boost/random.hpp>

// my include
#include "dskipgraph.h"

#include "reflection.h"

//#include "tcp_wrap.h"
//#include <cclog/cclog.h>
//#include <cclog/cclog_tty.h>

#include "singleton.hpp"
#include "get_random.h"
#include "shared_object.hpp"
#include "get_interface.h"
#include "dsg_logic.hpp"

using namespace mp::placeholders;
namespace rpc { using namespace msgpack::rpc; }

//#include "sg_logic.hpp"

class sg_host : public msgpack::rpc::dispatcher, public boost::noncopyable{
public:
	sg_host(msgpack::rpc::server* sv):
		m_sv(sv){}
	void dispatch(msgpack::rpc::request req){
		// get the name of method
		const std::string& method = req.method().as<std::string>();
		// call the method
		std::cerr << "@";
		bool success = ref_table.call(method, &req, m_sv);
		if(!success){ 
			std::cerr << "undefined function ["
								<< method << "] called" << std::endl;
			return;
		}
	}
	void regist(const std::string& name,
		reflection<msgpack::rpc::server>::reaction func){
		ref_table.regist(name,func);
	}
private:
	reflection<msgpack::rpc::server> ref_table;
	msgpack::rpc::server* m_sv;
	//const reflection::name_to_func* ref_table; //name to function
private:
	sg_host();
};

int main(int argc, char** argv){
//	cclog::reset(new cclog_tty(cclog::TRACE, std::cerr));

	const int random_seed = get_random();
	srand(random_seed);
	
	struct settings{
		uint16_t myport;
		uint16_t master_port;
		std::string master_name;
		std::string interface;
		uint8_t verbose;
		uint64_t vector;
	} s;
	{	// (argc argv) -> settings
		boost::program_options::options_description opt("options");
		opt.add_options()
			("help,h", "view help")
			("interface,i",boost::program_options::value<std::string>
			 (&s.interface)->default_value("eth0"), "my interface")
			("port,p",boost::program_options::value<unsigned short>
			 (&s.myport)->default_value(12321), "my port number")
			("address,a",boost::program_options::value<std::string>
			 (&s.master_name)->default_value("127.0.0.1"), "master's address")
			("mport,P",boost::program_options::value<unsigned short>
			 (&s.master_port)->default_value(12321), "master port number")
			("verbose,v",boost::program_options::value<uint8_t>
			 (&s.verbose)->default_value(0),"verbose level")
			("vector,V",boost::program_options::value<uint64_t>
			 (&s.vector)->default_value(
				 static_cast<uint64_t>(rand())<<32 | rand()),"vector ")
			;
 		boost::program_options::variables_map vm;
		store(parse_command_line(argc,argv,opt), vm);
		notify(vm);
		if(vm.count("help")){
			std::cerr << opt << std::endl;
			exit(0);
		}
		if(vm.count("verbose")){
			s.verbose++;
		}
	}

	
	{ // settings  ->  shared_data::instance()
		shared_data::instance().localhost
			= host(ntoa(get_myip_interface(s.interface.c_str())), s.myport);
		shared_data::instance().localvector = membership_vector(s.vector);
	}

	// start server
	msgpack::rpc::server sg_server;
	sg_host myhost(&sg_server); // main callback	
	
	{ // regist callbacks
#define REGIST(x) \
		myhost.regist(std::string(#x), \
									logic::x<msgpack::rpc::request, msgpack::rpc::server>)

		REGIST(set_r);
		REGIST(set_l);
		REGIST(set_r_ack);
		REGIST(set_r_nak);
		REGIST(unref_l);

		REGIST(get_nearest); 
		REGIST(die);
		REGIST(tellme_region);
		REGIST(answer_region);
		REGIST(offer_migrate_region);
		REGIST(migration);
		REGIST(non_migration);
#undef REGIST
	}
	sg_server.serve(&myhost);
	sg_server.listen("0.0.0.0", s.myport);
	sg_server.start(4);

	{ // debug output
		std::cout << "myhost="
							<< shared_data::instance().localhost.name << ":"
							<< shared_data::instance().localhost.port << std::endl
							<< "myvector=" << shared_data::instance().localvector << std::endl;
	}
	
	if(s.master_name == "127.0.0.1" && s.master_port == s.myport){
		// I am master
		const std::string keyname = "__node_master" + std::string(":") +
			boost::lexical_cast<std::string>(s.myport);
		
		shared_data::instance().nodelist
			.push_back(boost::shared_ptr<node>(new node())); // it contains all range
		{
			boost::shared_ptr<node> origin = shared_data::instance().nodelist.front();
			node::bucket_ref buk(origin->bucket);
			for(size_t i=0;i<1000;++i){
				buk->push_back(std::make_pair(
												 "k"+boost::lexical_cast<std::string>(i),
												 "v"+boost::lexical_cast<std::string>(i)));
			}
			assert(origin->maxlevel() == 0);
			origin->level_increment(); //  level must be 1
			shared_neighbor myself
				(new neighbor(range<key>(), shared_data::instance().localhost));
			origin->neighbor_in_lv(0, left) = myself;
			origin->neighbor_in_lv(0, right) = myself;
			{ // register the myself to the neighbor list
				mp::sync<weak_neighbor_set>::ref wn_ref(neighbor_list);
				weak_neighbor weak_myself(myself);
				wn_ref->insert(weak_myself);
			}
			origin->seq_number_ref(0, left) = 0;
			origin->seq_number_ref(0, right) = 0;
			origin->state_ref(0) = node::in;
		}
	}
	else{
		// treat minimum key
		const std::string keyname = "__node_"+ s.master_name + std::string(":") +
			boost::lexical_cast<std::string>(s.myport);
		const std::string myip = ntoa(get_myip_interface(s.interface.c_str()));
		const host master(s.master_name,s.master_port),myhost(myip,s.myport);;
		
		sg_server.get_session(master.get_address())
			.notify("treat",keyname,myhost , shared_data::instance().localvector);
	}
	
	//std::cout << shared_data::instance() << std::endl;
	
	sg_server.join();// wait for server ends
	return 0;
}
