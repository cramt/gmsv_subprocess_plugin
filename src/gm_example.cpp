#include "GarrysMod\Lua\Interface.h"
#include <stdio.h> 
#include <cstdint>
#include <list> 
#include <iterator> 
#include <iostream>
#include <boost/process.hpp>
#include <string>
#include <thread>
#include <sstream>
#include <map>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace boost::process;
using namespace std;
using boost::property_tree::ptree;
using boost::property_tree::read_json;
using boost::property_tree::write_json;

class nodejs_wrapper {
public:
	void init(string jsFile) {
		this->on_std_in = [](string line) {

		};
		this->nodeProcess = child("node " + jsFile, std_out > this->out, std_in < this->in);
		this->std_in_reader = thread([this] {this->std_in_reader_function(); });
	}
	void wait() {
		this->nodeProcess.wait();
	}
	function<void(string line)> on_std_in;
	void send(string line) {
		in << line << endl;
	}
private:
	void std_in_reader_function() {
		std::string line;
		while (out && std::getline(out, line) && !line.empty()) {
			this->on_std_in(line);
		}
	}
	thread std_in_reader;
	child nodeProcess;
	opstream in;
	ipstream out;
};

struct NFGSSendable {
	string command;
	string json;
};

class nodejs {
public:
	void init(string jsFile) {
		nodeProcess.init(jsFile);
		nodeProcess.on_std_in = [this](string line) {
			ptree pt;
			std::istringstream is(line);
			read_json(is, pt);
			NFGSSendable gotten;
			gotten.command = pt.get<string>("command");
			pt.erase("command");
			std::ostringstream buf;
			write_json(buf, pt, false);
			gotten.json = buf.str();
			this->sent.push_back(gotten);

		};
	}
	void wait() {
		this->nodeProcess.wait();
	}
	list<NFGSSendable> sent;
private:
	nodejs_wrapper nodeProcess;
};






using namespace GarrysMod::Lua;

struct Environment {
	CFunc hook;
	nodejs* node;
};

list<Environment> allNodeEnvironment;

LUA_FUNCTION(Think) {

	for (list<Environment>::iterator nodeEnv = allNodeEnvironment.begin(); nodeEnv != allNodeEnvironment.end(); ++nodeEnv) {
		for (list<NFGSSendable>::iterator commands = nodeEnv->node->sent.begin(); commands != nodeEnv->node->sent.end(); ++commands) {
			if (commands->command == "print") {

			}
			else if (commands->command == "event") {
				LUA->PushCFunction(nodeEnv->hook);
				LUA->PushString(commands->json.c_str());
				LUA->Call(2, 0);
				nodeEnv->node->sent.remove(*commands);
			}
			else if (commands->command == "exception") {

			}
		}
	}
	return 1;
}
LUA_FUNCTION(CallBackTest) {

	if (LUA->IsType(1, Type::FUNCTION)) {
		CFunc func = LUA->GetCFunction(1);
		LUA->PushCFunction(func);
		LUA->Call(1, 0);
		
		return 1;
	}


	return 0;
}
LUA_FUNCTION(test) {
	if (LUA->IsType(1, Type::NUMBER)) {
		char strOut[512];
		double fNumber = LUA->GetNumber(1);
		sprintf_s(strOut, "Thanks for the number - I love %f!!", fNumber);
		LUA->PushString(strOut);
		return 1;
	}
	LUA->PushString("This string is returned");
	return 1;
}
LUA_FUNCTION(instantiateNodeEnv) {
	if (LUA->IsType(1, Type::STRING) && LUA->IsType(2, Type::FUNCTION)) {
		string path = LUA->GetString(1);
		CFunc hook = LUA->GetCFunction(2);
		nodejs nodeEnv;
		nodeEnv.init(path);
		Environment env;
		env.hook = hook;
		env.node = &nodeEnv;
		LUA->PushString(reinterpret_cast<int>(env.node) + "");
		return 1;
	}
	LUA->PushNil();
	return 0;
}

GMOD_MODULE_OPEN() {

	const char* shared_object = "___NFGS___WRAPPER___TABLE___";

	LUA->PushSpecial(SPECIAL_GLOB);
	LUA->PushString(shared_object);
	LUA->CreateTable();
	LUA->SetTable(-3);
	LUA->GetField(-1, shared_object);

	LUA->PushCFunction(test);
	LUA->SetField(-2, "test");

	LUA->PushCFunction(CallBackTest);
	LUA->SetField(-2, "callbackTest");

	LUA->PushCFunction(Think);
	LUA->SetField(-2, "think");

	return 0;
}

//
// Called when your module is closed
//
GMOD_MODULE_CLOSE() {
	return 0;
}
