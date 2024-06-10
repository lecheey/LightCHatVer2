#pragma once
#include <iostream>
#include <string.h>
#include <vector>
#include <set>
#include "func.h"

class Server{
public:
	Server(){
		readUser();
	};
	~Server(){}
	
	void serverRuntime();
	void serverStart();
	
	void userReg(int index, std::string m);
	void userAdd(std::string &l, std::string &p);
	void userAuth(int index, std::string m);
	void userLogout(int index, std::string m);	
	void handShake(int index);
	
	void sendUserList(int index, std::string m);
	void optDialog(int index, std::string m);

	void writeUser();
	void readUser();
	void getUserList();

	int findLogin(std::string &user);
	bool is_authorized(std::string &l);
	void errorAccess(int &i);
	
	std::string makeReq(std::string rtype, std::string v1, std::string v2);
	void splitReq(std::string &m, std::string &v1, std::string &v2);
	std::string pkgUserList();
	int loginCheck(std::string &l);

private:
	enum userstatus { online, offline };
	
	struct User{
		User() : _login(""), _pass(""), _status(userstatus::offline), _auth(false){}
		User(std::string login, std::string pass) :
					_login(login), _pass(pass), _status(userstatus::offline), _auth(false){}
		~User(){}
		
		void switchStatus();
		void switchStatusAuth();
		bool getStatusAuth();
		
		std::string _login;
		std::string _pass;
		userstatus _status;
		bool _auth;
	};
/*	
	struct Message{
		Message() : _time(""), _sender(""), _receiver(""), _msg(""){}
		Message(std::string msg) : _msg(msg){}
		Message(std::string time, std::string _from, std::string _to, std::string msg) :
					_time(time), _msg(msg), _sender(_from), _receiver(_to){}
		~Message(){}
		
		std::string _sender;
		std::string _receiver;
		std::string _msg;
		std::string _time;
	};
*/
	std::vector<User> _users{};
	//std::vector<Message> _messages{};
	int _usercount{0};

	int _sndr;
	int _rcvr;

	std::set<int> clients;
	int _switcher{0};
};
