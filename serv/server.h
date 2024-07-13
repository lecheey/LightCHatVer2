#pragma once
#include <iostream>
#include <string.h>
#include <vector>
#include <set>
#include "func.h"

class Server{
public:
	Server(){
		readConfig();
		sqlConn();
		readUser();
	};
	~Server(){}
	
	void serverRuntime();
	void serverStart();
	void sqlConn();
	void readConfig();
	void writeConfig();
	
	void userReg(int index, std::string m);
	void userAdd(std::string &l, std::string &p);
	void userAuth(int index, std::string m);
	void userLogout(int index, std::string m);	
//	void handShake(int index);
	
	void sendUserList(int index, std::string m);
	void sendMsgList(int index, std::string m);
	void optDialog(int index, std::string m);
	void userTyping(int index, std::string m);	

	void writeUser();
	void readUser();
	void readMsg();
	void getUserList();

	int findLogin(std::string &user);
	bool is_authorized(std::string &l);
	void errorAccess(int &i);
	
	std::string makeReq(std::string rtype, std::string v1, std::string v2);
	void splitReq(std::string &m, std::string &v1, std::string &v2);
	std::string makeMsg(std::string rtype, std::string v1, std::string v2, std::string v3, std::string v4);
	void splitMsg(std::string &m, std::string &v1, std::string &v2, std::string &v3, std::string &v4);

	std::string pkgUserList();
	int loginCheck(std::string &l);

private:
	enum userstatus { online, offline };
	
	struct User{
		User() : _name(""), _surname(""), _pass(""), _uid(0),_email("") ,_status(userstatus::offline), _auth(false){}
		
		User(std::string uid, std::string name) :
					_name(name), _surname(""), _pass(""), _uid(uid), _email(""), _status(userstatus::offline), _auth(false){}
		
		//User(std::string uid, std::string uname, std::string pass) :
		//			_name(uname), _surname(""), _pass(pass), _uid(uid), _email(""), _status(userstatus::offline), _auth(false){}
		
		~User(){}
		
		void switchStatus();
		void switchStatusAuth();
		bool getStatusAuth();
		
		std::string _name; // обязательное поле, для входа в систему
		std::string _surname;
		std::string _pass; // обязательное поле
		std::string _email;
		std::string _uid; // обязательное поле, присваивается БД

		userstatus _status;
		bool _auth;
	};
/*
	struct Message{
		Message() : _time(""), _sender(""), _receiver(""), _msg(""){}
		Message(std::string _from, std::string _to, std::string time, std::string msg) :
					_time(time), _msg(msg), _sender(_from), _receiver(_to){}
		~Message(){}
		
		std::string _sender;
		std::string _receiver;
		std::string _msg;
		std::string _time;
	};
*/
	std::vector<User> _users{}; // для временного хранения авторизованных пользователей
	//std::vector<Message> _messages{};
	int _usercount{0};
	int _msgcount{0};

	// настраиваемые переменные
	int PORT;
	std::string sql_user;
	std::string sql_pass; 
	std::string sql_db;
	std::string config_path = "config.ini";
};
