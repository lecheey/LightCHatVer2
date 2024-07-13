#pragma once
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include "sha1.h"
#include "func.h"

class Client{
public:
	Client(){
		readConfig();
		readUser();
	}
	~Client(){}
	
	void clientRuntime();
	void clientConnect();
	void readConfig();
	void writeConfig();
	
	void userCreate();
	bool userLogin();
	bool userLogout(std::string &l);
	void userAdd(std::string &l);

	void userRuntime(std::string &_from);
	//void userTyping(std::string &_from, std::string &_to);
	void userTyping(int &_from, int &_to);
	
	void saveMsg(std::string &_from, std::string &_to, std::string &_msg, std::string &_time);
	void getMsgList(int &_from, int &_to);
	//void getMsgList(std::string &_from, std::string &_to);
	void showMsgs(std::string &_from, std::string &_to);
	
	void readUser();
	void writeUser(std::string l);

	int findLogin(std::string &user);
	int findFriend(std::string &user);
	int loginCheck(std::string &l);
	void getUserList(std::string &l);
	void showUsers();
	
	std::string makeReq(std::string rtype, std::string v1, std::string v2);
	void splitReq(std::string m, std::string &v1, std::string &v2);
	std::string makeMsg(std::string rtype, std::string v1, std::string v2, std::string v3, std::string v4);
	void splitMsg(std::string m, std::string &v1, std::string &v2, std::string &v3, std::string &v4);
	
	//void splitUserList(std::string &m);

private:
	enum userstatus { online, offline };
	
	struct User{
		User() : _uid(""), _login(""), _pass(""), _name(""), _status(userstatus::offline){}
		User(std::string login) :
					_uid(""), _login(login), _pass(""), _name(""), _status(userstatus::offline){}		
		
		User(std::string login, std::string uid) :
					_uid(uid), _login(login), _pass(""), _name(""), _status(userstatus::offline){}		

		User(std::string login, std::string pass, std::string name) :
					_uid(""), _login(login), _pass(pass), _name(name), _status(userstatus::offline){}		
		//User(std::string login, std::string pass) :
		//			_login(login), _pass(pass), _name(""), _status(userstatus::offline){}		
		~User(){}
		
		void switchStatus();
		
		std::string _login;
		std::string _pass;
		std::string _name;
		std::string _uid;
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

	std::vector <User> _friends{};
	std::string _username;
	int PORT;
	std::string SERVER_IP;
	std::string config_path = "config.ini";
};
