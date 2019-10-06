#include "GarrysMod\Lua\Interface.h"
#include <stdio.h> 
#include <boost/process.hpp>

using namespace boost::process;
using namespace std;

class process_wrapper {
public:
	void init(string command) {
		this->on_std_in = [](string line) {

		};
		this->nodeProcess = child(command, std_out > this->out, std_in < this->in);
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

class curr_process {
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
	process_wrapper nodeProcess;
};






using namespace GarrysMod::Lua;

curr_process* getNodeEnv(const char* ptr) {
	unsigned long ptr_as_int = std::stoul(ptr, nullptr, 0);
	curr_process* nodeEnv = reinterpret_cast<curr_process*>(ptr_as_int);
	return nodeEnv;
}

LUA_FUNCTION(think) {
	if (LUA->IsType(1, Type::STRING)) {
		curr_process* nodeEnv = getNodeEnv(LUA->GetString(1));
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
		return 1;
	}
	return 0;
}

LUA_FUNCTION(send) {
	if (LUA->IsType(1, Type::STRING) && LUA->IsType(2, Type::STRING)) {
		curr_process* nodeEnv = getNodeEnv(LUA->GetString(1));
		string message = LUA->GetString(2);
		nodeEnv->send(message);
		return 0;
	}
	return 1;
}

LUA_FUNCTION(kill) {
	if (LUA->IsType(1, Type::STRING)) {
		curr_process* nodeEnv = getNodeEnv(LUA->GetString(1));
		nodeEnv->kill();
		nodeEnv = new curr_process;
		delete nodeEnv;
		return 0;
	}
	return 1;
}

LUA_FUNCTION(instantiate) {
	if (LUA->IsType(1, Type::STRING)) {
		string path = LUA->GetString(1);
		curr_process* node = new curr_process;
		node->init(path);
		LUA->PushString(to_string(reinterpret_cast<unsigned long>(node)).c_str());
		return 1;
	}
	LUA->PushNil();
	return 0;
}

GMOD_MODULE_OPEN() {

	const char* shared_object = "___SUBPROCESS___WRAPPER___TABLE___";

	LUA->PushSpecial(SPECIAL_GLOB);
	LUA->PushString(shared_object);
	LUA->CreateTable();
	LUA->SetTable(-3);
	LUA->GetField(-1, shared_object);

	LUA->PushCFunction(think);
	LUA->SetField(-2, "think");

	LUA->PushCFunction(instantiate);
	LUA->SetField(-2, "instantiate");

	LUA->PushCFunction(kill);
	LUA->SetField(-2, "kill");

	LUA->PushCFunction(send);
	LUA->SetField(-2, "send");

	return 0;
}

//
// Called when your module is closed
//
GMOD_MODULE_CLOSE() {
	return 0;
}
