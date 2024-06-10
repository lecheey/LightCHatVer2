#pragma once
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include "Sha1.h"
#include "func.h"

class Client{
public:
	Client(){
		readUser();
	}
	~Client(){}
	
	void clientRuntime();
	void clientConnect();
	
	void userCreate();
	bool userLogin();
	bool userLogout(std::string &l);
	void userAdd(std::string &l);

	void userRuntime(std::string &_from);
	void userTyping(std::string &_from, std::string &_to);
	
	void saveMsg(std::string &_from, std::string &_to, std::string &_msg, std::string &_time);
	void getMsgList(std::string &_from, std::string &_to);
	void showMsgs(std::string &_from, std::string &_to);
	
	void readUser();
	void writeUser(std::string &l);


	int findLogin(std::string &user);
	int loginCheck(std::string &l);
	void getUserList(std::string &l);
	void showUsers();
	
	void splitUserList(std::string &m);
	std::string makeReq(std::string rtype, std::string v1, std::string v2);
	void splitReq(std::string m, std::string &v1, std::string &v2);

private:
	enum userstatus { online, offline };
	
	struct User{
		User() : _login(""), _pass(""), _name(""), _status(userstatus::offline){}
		User(std::string login) :
					_login(login), _pass(""), _name(""), _status(userstatus::offline){}		
		User(std::string login, std::string pass, std::string name) :
					_login(login), _pass(pass), _name(name), _status(userstatus::offline){}		
		//User(std::string login, std::string pass) :
		//			_login(login), _pass(pass), _name(""), _status(userstatus::offline){}		
		~User(){}
		
		void switchStatus();
		
		std::string _login;
		std::string _pass;
		std::string _name;
		userstatus _status;
	};
	
	struct Message{
		Message() : _time(""), _sender(""), _receiver(""), _msg(""){}
		Message(std::string msg) :  _msg(msg){}
		Message(std::string _from, std::string _to, std::string msg) :
					_time(""), _msg(msg), _sender(_from), _receiver(_to){}
		Message(std::string time, std::string _from, std::string _to, std::string msg) :
					_time(time), _msg(msg), _sender(_from), _receiver(_to){}
		~Message(){}
		
		std::string _sender;
		std::string _receiver;
		std::string _msg;
		std::string _time;
	};
	
	std::vector <User> _users{};
	std::vector <Message> _messages{};
	int _usercount{0};

	std::vector <std::string> _friends{};
	std::string _username;
	std::string _rcvrname;
};
