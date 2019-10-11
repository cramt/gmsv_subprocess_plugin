#include "GarrysMod\Lua\Interface.h"
#include <stdio.h> 
#include <boost/process.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <iostream>
#include <sstream>


using boost::property_tree::ptree;
using namespace boost::process;
using namespace std;

class process_wrapper {
public:
	void init(string command, function<void(string line)> cb_std_in, function<void(string line)> cb_std_err) {
		this->on_std_in = cb_std_in;
		this->on_std_err = cb_std_err;
		this->process = child(command, std_out > this->out, std_in < this->in, std_err > this->err);
		this->std_in_reader = thread([this] {this->std_in_reader_function(); });
		this->std_err_reader = thread([this] {this->std_in_reader_function(); });
	}
	void wait() {
		this->process.wait();
	}
	function<void(string line)> on_std_in;
	function<void(string line)> on_std_err;
	void send(string line) {
		in << line << endl;
	}
	bool isRunning() {
		return process.running();
	}
	void kill() {
		this->should_be_killed = true;
		this->std_in_reader.join();
		process.terminate();
	}
private:
	atomic<bool> should_be_killed = false;
	void std_in_reader_function() {
		string line;
		while (this->out && getline(this->out, line) && !this->should_be_killed) {
			this->on_std_in(line);
		}
	}
	void std_err_reader_function() {
		string line;
		while (this->err && getline(this->err, line) && !this->should_be_killed) {
			cout << line << endl;
			this->on_std_err(line);
		}
	}
	thread std_in_reader;
	thread std_err_reader;
	child process;
	opstream in;
	ipstream out;
	ipstream err;
};

class curr_process {
public:
	void init(string cmd) {
		process.init(cmd, [this](string line) {
			this->sent.push_back(line);
			}, [this](string line) {
				ptree out;
				out.put("command", "exception");
				out.put("eventName", "std_err");
				out.put("data", line);
				ostringstream oss;
				boost::property_tree::write_json(oss, out);
				this->sent.push_back(oss.str());
			});
	}
	void wait() {
		this->process.wait();
	}
	void send(string data) {
		if (this->isRunning()) {
			this->process.send(data);
		}
	}
	void kill() {
		if (this->isRunning()) {
			this->process.kill();
		}
	}
	bool isRunning() {
		return this->process.isRunning();
	}
	list<string> sent;
private:
	process_wrapper process;
};






using namespace GarrysMod::Lua;

curr_process* getProcEnv(const char* ptr) {
	unsigned long ptr_as_int = std::stoul(ptr, nullptr, 0);
	curr_process* procEnv = reinterpret_cast<curr_process*>(ptr_as_int);
	return procEnv;
}

LUA_FUNCTION(think) {
	if (LUA->IsType(1, Type::STRING)) {
		curr_process* procEnv = getProcEnv(LUA->GetString(1));
		LUA->CreateTable();
		if (!procEnv->isRunning()) {
			ptree out;
			out.put("command", "exception");
			out.put("eventName", "crash");
			out.put("data", "");
			ostringstream oss;
			boost::property_tree::write_json(oss, out);
			LUA->PushString(oss.str().c_str());
			LUA->SetField(-2, "1");
		}
		else {
			unsigned int i = 1;
			for (list<string>::iterator com = procEnv->sent.begin(); com != procEnv->sent.end(); ++com) {
				LUA->PushString(com->c_str());
				LUA->SetField(-2, to_string(i).c_str());
				i++;
			}
		}
		procEnv->sent.clear();
		return 1;
	}
	return 0;
}

LUA_FUNCTION(send) {
	if (LUA->IsType(1, Type::STRING) && LUA->IsType(2, Type::STRING)) {
		curr_process* procEnv = getProcEnv(LUA->GetString(1));
		string message = LUA->GetString(2);
		procEnv->send(message);
		return 0;
	}
	return 1;
}

LUA_FUNCTION(kill) {
	if (LUA->IsType(1, Type::STRING)) {
		curr_process* proc = getProcEnv(LUA->GetString(1));
		proc->kill();
		proc = new curr_process;
		delete proc;
		return 0;
	}
	return 1;
}

LUA_FUNCTION(instantiate) {
	if (LUA->IsType(1, Type::STRING)) {
		string path = LUA->GetString(1);
		curr_process* proc = new curr_process;
		proc->init(path);
		LUA->PushString(to_string(reinterpret_cast<unsigned long>(proc)).c_str());
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