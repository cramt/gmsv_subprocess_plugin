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
	bool isRunning() {
		return nodeProcess.running();
	}
	void kill() {
		nodeProcess.terminate();
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

class nodejs {
public:
	void init(string jsFile) {
		nodeProcess.init(jsFile);
		nodeProcess.on_std_in = [this](string line) {
			this->sent.push_back(line);
		};
	}
	void wait() {
		this->nodeProcess.wait();
	}
	void send(string data) {
		if (this->isRunning()) {
			this->nodeProcess.send(data);
		}
	}
	void kill() {
		if (this->isRunning()) {
			this->nodeProcess.kill();
		}
	}
	bool isRunning() {
		return this->nodeProcess.isRunning();
	}
	list<string> sent;
private:
	nodejs_wrapper nodeProcess;
};






using namespace GarrysMod::Lua;

LUA_FUNCTION(callBackTest) {

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

LUA_FUNCTION(think) {
	if (LUA->IsType(1, Type::STRING)) {
		const char* ptr_as_string = LUA->GetString(1);
		unsigned long ptr_as_int = std::stoul(ptr_as_string, nullptr, 0);
		nodejs* nodeEnv = reinterpret_cast<nodejs*>(ptr_as_int);
		if (!nodeEnv->isRunning()) {
			LUA->PushBool(false);
			return 1;
		}
		LUA->CreateTable();
		unsigned int i = 1;
		for (list<string>::iterator com = nodeEnv->sent.begin(); com != nodeEnv->sent.end(); ++com) {
			LUA->PushString(com->c_str());
			LUA->SetField(-2, to_string(i).c_str());
			i++;
		}
		nodeEnv->sent.clear();
		return 0;
	}
	return 1;
}

LUA_FUNCTION(kill) {
	const char* ptr_as_string = LUA->GetString(1);
	unsigned long ptr_as_int = std::stoul(ptr_as_string, nullptr, 0);
	nodejs* nodeEnv = reinterpret_cast<nodejs*>(ptr_as_int);
	nodeEnv->kill();
	nodeEnv = NULL;
}

LUA_FUNCTION(instantiateNodeEnv) {
	if (LUA->IsType(1, Type::STRING) && LUA->IsType(2, Type::FUNCTION)) {
		string path = LUA->GetString(1);
		CFunc hook = LUA->GetCFunction(2);
		nodejs node;
		nodejs* nodePTR = &node;
		nodePTR->init(path);
		LUA->PushString(to_string(reinterpret_cast<unsigned long>(nodePTR)).c_str());
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

	LUA->PushCFunction(callBackTest);
	LUA->SetField(-2, "callbackTest");

	LUA->PushCFunction(think);
	LUA->SetField(-2, "think");

	LUA->PushCFunction(instantiateNodeEnv);
	LUA->SetField(-2, "instantiateNodeEnv");

	return 0;
}

//
// Called when your module is closed
//
GMOD_MODULE_CLOSE() {
	return 0;
}
