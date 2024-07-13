#include <iostream>
#include <unistd.h> // вызовы linux/bsd
#include <string.h>
#include <sys/socket.h> // сокет
#include <netinet/in.h> // IP
#include <fcntl.h>
#include <sys/time.h>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <set>
#include "server.h"
#include "mysql/mysql.h"

#define CODE_LENGTH 3
#define COMMAND_LENGTH 6
#define SUCCESS "801"
#define ERROR_AUTH "802"
#define ERROR_NOTFOUND "803"
#define ERROR_OCCUPIED "804"
#define ERROR_SYMBOLS "805"
#define ERROR_MESSAGE "Ошибка"
#define END "END"
#define RETURN "000"
#define QUIT "000"

#define MESSAGE_LENGTH 1024

int sockd, bind_stat, connection_stat, connection;
struct sockaddr_in serveraddress, clientaddress;
socklen_t length;
char message[MESSAGE_LENGTH];
int bytes_read;

MYSQL mysql;
MYSQL_RES* res;
MYSQL_ROW row;

namespace fs = std::filesystem; 

void Server::serverRuntime(){
	std::cout << "Запуск сервера..." << std::endl;
	serverStart();
	
	length = sizeof(clientaddress);
    connection = accept(sockd,(struct sockaddr*)&clientaddress, &length);
	if(connection == -1){
		std::cout << ERROR_MESSAGE << ": Socket is unable to reach client!" << std::endl;
		exit(1);
	}	
	else{
		std::cout << "Пользователь подключен!" << std::endl;
	}
           
	while(true){
		bzero(message, sizeof(message));
		recv(connection, message, MESSAGE_LENGTH, 0);
				
		if(strncmp(message, QUIT, CODE_LENGTH) == 0){
			std::cout << "Клиент отключился!" << std::endl;
			std::cout << "Завершение работы сервера...!" << std::endl;
			exit(1);
		}

		if(strncmp(message, "REGUSR", COMMAND_LENGTH) == 0){
				userReg(connection, message);
			}
		else if(strncmp(message, "LOG_IN", COMMAND_LENGTH) == 0){
				userAuth(connection, message);
			}
		else if(strncmp(message, "LOGOUT", COMMAND_LENGTH) == 0){
				userLogout(connection, message);
			}
		else if(strncmp(message, "GETLST", COMMAND_LENGTH) == 0){
				sendUserList(connection, message);
			}
		else if(strncmp(message, "GETMSG", COMMAND_LENGTH) == 0){
				sendMsgList(connection, message);
			}
		else if(strncmp(message, "OPTUSR", COMMAND_LENGTH) == 0){
				optDialog(connection, message);
			}
		else if(strncmp(message, "USRTYP", COMMAND_LENGTH) == 0){
				userTyping(connection, message);
			}
		else{
				close(sockd);
				std::cout << "Завершение работы" << std::endl;
				exit(1);
			}
	}
	close(sockd);
	mysql_close(&mysql);
	std::cout << "Завершение работы" << std::endl;
	exit(1);
}

void Server::serverStart(){
	sockd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockd < 0){
		std::cout << ERROR_MESSAGE << ": Socket creation falied!" << std::endl;
        exit(1);
    }
     
	serveraddress.sin_addr.s_addr = INADDR_ANY;
	serveraddress.sin_port = htons(PORT);
	serveraddress.sin_family = AF_INET;
	bind_stat = bind(sockd, (struct sockaddr*)&serveraddress, sizeof(serveraddress));
	if(bind_stat == -1){
		std::cout << ERROR_MESSAGE << ": Socket binding failed!" << std::endl;
		exit(1);
	}

	connection_stat = listen(sockd, 5);
	if(connection_stat == -1){
		std::cout << ERROR_MESSAGE << ": Socket is unable to listen for connection!" << std::endl;
		exit(1);
	}
	else{
		std::cout << "Listening for connections..." << std::endl;
	}
}

void Server::sqlConn(){
	std::cout << "Подключение к базе данных...";
	mysql_init(&mysql);
	if (&mysql == nullptr) {
		std::cout << ERROR_MESSAGE << " \nНевозможно создать MySQL-дескриптор" << std::endl;
	}
 
	if (!mysql_real_connect(&mysql, "localhost", sql_user.data(), sql_pass.data(), sql_db.data(), 0, NULL, 0)) {
		std::cout << ERROR_MESSAGE << "\nНевозможно подключиться к базе данных " << mysql_error(&mysql) << std::endl;
	}
	else {
		std::cout << "успешно" << std::endl;
	}
 
	mysql_set_character_set(&mysql, "utf8");
	std::cout << "connection characterset: " << mysql_character_set_name(&mysql) << std::endl;
	
	mysql_query(&mysql, "CREATE TABLE userdata(id INT AUTO_INCREMENT NOT NULL PRIMARY KEY, u_surname VARCHAR(255), u_name VARCHAR(255) NOT NULL, u_email VARCHAR(100))");
	
	mysql_query(&mysql, "CREATE TABLE userhash(id INT AUTO_INCREMENT NOT NULL PRIMARY KEY references userdata(id), u_pass VARCHAR(255) NOT NULL)");
	
	mysql_query(&mysql, "CREATE TABLE msgdata(id_sender INT reverences userdata(id), id_receiver INT reverences userdata(id), msg_date TIMESTAMP, msg_status INT, id_msg INT AUTO_INCREMENT NOT NULL PRIMARY KEY, msg TEXT)");

	mysql_query(&mysql, "DELIMITER | CREATE PROCEDURE add_id() BEGIN INSERT INTO userhash(id, u_pass) VALUES (new.id, null); END; | DELIMITER ;}");

	mysql_query(&mysql, "CREATE DEFINER=`lightchat`@`localhost` TRIGGER add_id AFTER INSERT ON userdata FOR EACH ROW BEGIN INSERT INTO userhash(id, u_pass) VALUES (new.id, null); END }");
}

void Server::userReg(int index, std::string m){
	std::cout << "Регистрация..." << std::endl;
	
	std::string login_temp, pass_temp;
	splitReq(m, login_temp, pass_temp);

	switch(loginCheck(login_temp)){
	case 1:
		userAdd(login_temp, pass_temp);
		send(index, SUCCESS, CODE_LENGTH, 0);
		std::cout << "Пользователь создан!" << std::endl;
		break;
	case 4:
		send(index, ERROR_OCCUPIED, CODE_LENGTH, 0); // ошибка: логин занят
		std::cout << ERROR_MESSAGE << std::endl;
		break;
	case 5:
		send(index, ERROR_SYMBOLS, CODE_LENGTH, 0); // ошибка: стоп-символ
		std::cout << ERROR_MESSAGE << std::endl;
		break;
	default:
		send(index, RETURN, CODE_LENGTH, 0);
		break;
	}					
}				

void Server::userAuth(int index, std::string m){
	std::cout << "Вход в систему...";
	
	std::string login_temp, pass_temp, _q;
	splitReq(m, login_temp, pass_temp); // извлечение введенных логина и пароля из запроса 
	
	int ind = findLogin(login_temp);
	if(index == -3){
		send(index, ERROR_NOTFOUND, CODE_LENGTH, 0);
		std::cout << ERROR_MESSAGE << "\nпользователь не найден" << std::endl;
	}
	else{
		_q = "SELECT u_pass FROM userhash WHERE id =" + _users[ind]._uid;
		mysql_query(&mysql, _q.c_str());
		res = mysql_store_result(&mysql);
		row = mysql_fetch_row(res);
		if(row[0] == pass_temp){	
		//if(_users[ind]._pass == pass_temp){ // если пароль был извлечен в массив пользователей 
			send(index, SUCCESS, CODE_LENGTH, 0);
			_users[ind].switchStatus();
			_users[ind].switchStatusAuth();

			recv(connection, message, MESSAGE_LENGTH, 0);
			
			bzero(message, sizeof(message));
			strcpy(message, (_users[ind]._uid).data());
			send(index, message, sizeof(message), 0);

			std::cout << "Успешно" << std::endl;	
		}
		else{
			send(index, ERROR_AUTH, CODE_LENGTH, 0);
			std::cout << ERROR_MESSAGE << "\nневерный пароль" << std::endl;	
		}
	}
}

void Server::userLogout(int index, std::string m){
	std::cout << "Выход из системы...";
	std::string login_temp, dummy;
	splitReq(m, login_temp, dummy);

	int ind = findLogin(login_temp);
	if(ind == -3){
		send(index, ERROR_NOTFOUND, CODE_LENGTH, 0);
		std::cout << ERROR_MESSAGE << std::endl;
	}
	else{
		if(_users[ind]._auth == true){
			send(index, SUCCESS, CODE_LENGTH, 0);
			_users[ind].switchStatus();
			_users[ind].switchStatusAuth();
			std::cout << " успешно\nПользователь вышел из системы" << std::endl;	
		}
		else{
			send(index, ERROR_AUTH, CODE_LENGTH, 0);
			std::cout << ERROR_MESSAGE << std::endl;	
		}
	}
}

void Server::userAdd(std::string &l, std::string &p){
	std::cout << "Создание нового аккаунта...";
	std::string _q;

	_q = "INSERT INTO userdata(u_name) values('" + l + "')";
	mysql_query(&mysql, _q.c_str());

	_q = "UPDATE userhash SET u_pass = '" + p + "' WHERE userhash.id IN ( SELECT id FROM userdata WHERE u_name = '" + l + "')";
	mysql_query(&mysql, _q.c_str());

	std::cout << "успешно" << std::endl;
	
	std::string _n, _id, _p;
	_q = "SELECT userdata.id, u_name, u_pass FROM userhash join userdata on userhash.id = userdata.id WHERE u_name = '" + l + "'";
	mysql_query(&mysql, _q.c_str());
	if (res = mysql_store_result(&mysql)) {
		while (row = mysql_fetch_row(res)) {
			_id = row[0];
			_n = row[1];
			//_p = row[2];

			//User newUser(_id, _n, _p);
			User newUser(_id, _n);
			_users.push_back(newUser);
			_usercount++;
		}
	}
}

void Server::sendUserList(int index, std::string m){
	std::cout << "Передача списка пользователей...";
	std::string login_temp, dummy;
	splitReq(m, login_temp, dummy);
	
	if(is_authorized(login_temp)){
		for(int i = 0; i < _usercount; i++){
			std::string pkg = makeReq("USTLST", _users[i]._uid, _users[i]._name);

			bzero(message, sizeof(message));
			strcpy(message, pkg.data());
			send(index, message, sizeof(message), 0);
				
			recv(connection, message, MESSAGE_LENGTH, 0);
		}
		send(index, END, CODE_LENGTH, 0);
		std::cout << "успешно" << std::endl;
	}
	else{
		errorAccess(index);
	}	
}
void Server::sendMsgList(int index, std::string m){
	std::cout << "Передача списка сообщений...";
	std::string from_temp, to_temp;
	splitReq(m, from_temp, to_temp);
	
	std::string _s, _r, _t, _m, _q; 
	_q = "SELECT id_sender, id_receiver, msg_date, msg From msgdata WHERE (id_sender =" + from_temp + " and id_receiver = " + to_temp + ") or (id_sender = " + to_temp + " and id_receiver = " + from_temp + ") ORDER BY msg_date";
	mysql_query(&mysql, _q.c_str());
	
	if (res = mysql_store_result(&mysql)) {
		while (row = mysql_fetch_row(res)) {
			_s = row[0];
			_r = row[1];
			_t = row[2];
			_m = row[3];
			std::string msg = makeMsg("USTLST", _s, _r, _t, _m);

			bzero(message, sizeof(message));
			strcpy(message, msg.data());
			send(index, message, sizeof(message), 0);
						
			recv(connection, message, MESSAGE_LENGTH, 0);
		}
		send(index, END, CODE_LENGTH, 0);
		std::cout << "успешно" << std::endl;
	}
	else{
		errorAccess(index);
	}
}

void Server::optDialog(int index, std::string m){
	std::cout << "Выбор диалога...";
	
	std::string from_temp, to_temp;
	splitReq(m, from_temp, to_temp);
	
	if(is_authorized(from_temp)){
		if(findLogin(to_temp) == -3){
			send(index, ERROR_NOTFOUND, CODE_LENGTH, 0);
			std::cout << ERROR_MESSAGE << std::endl;
		}
		else{
			send(index, SUCCESS, CODE_LENGTH, 0);
			std::cout << "успешно" << std::endl;
		}
	}	
	else{
		errorAccess(index);
	}
}

void Server::userTyping(int index, std::string m){	
	std::string from_temp, to_temp, _time, _msgbody, _q;
	splitMsg(m, from_temp, to_temp, _time, _msgbody);
	
	_q = "INSERT INTO msgdata(id_sender, id_receiver, msg_date, msg_status, msg) values (" + from_temp + "," + to_temp + ",'" + _time + "',1,'" + _msgbody + "')";
	mysql_query(&mysql, _q.c_str());

	send(index, SUCCESS, CODE_LENGTH, 0);	
}

void Server::readUser(){
	if(!_users.empty()){
		_users.clear();
	}

	std::string _n, _id, _p;
	mysql_query(&mysql, "SELECT userdata.id, u_name, u_pass FROM userhash join userdata on userhash.id = userdata.id");
	if (res = mysql_store_result(&mysql)) {
		while (row = mysql_fetch_row(res)) {
			_id = row[0];
			_n = row[1];
			//_p = row[2]; // сохранение пароля в массив объектов

			//User newUser(_id, _n, _p);
			User newUser(_id, _n);
			_users.push_back(newUser);
			_usercount++;
		}
	}
}

/*
// сохренение и загрузка из текстового файла 
void Server::writeUser(){
	fs::path filepath{"accounts.txt"};
	std::ofstream file(filepath, std::ios::out);
    if(file.is_open()){
		file << _usercount << '\n';
		for(int i = 0; i < _usercount; i++){
			file << _users[i]._login << ':' << _users[i]._pass <<';';
		}
		file.close();
    }
}

void Server::readUser(){
	fs::path filepath{"accounts.txt"};
	std::ifstream file(filepath, std::ios::in);
	
	std::string x, y, temp;
	if(file.is_open()){
		getline(file, temp, '\n');
		_usercount = stoi(temp);
		for(int i = 0; i < _usercount; i++){
			getline(file, x, ':');
			getline(file, y, ';');
			
			User user(x,y);
			_users.push_back(user);
		}	
		file.close();
	}
}
*/

std::string Server::makeReq(std::string rtype, std::string v1, std::string v2){
	std::string result;
		result.append(rtype);
		result.append(";");
		result.append(v1);
		result.append(";");
		result.append(v2);
		result.append(";");
	return result;
}

void Server::splitReq(std::string &m, std::string &v1, std::string &v2){
	std::string temparr[3];
	size_t pos{0};
	int i{0};
	while((pos = m.find(";")) != std::string::npos){
		temparr[i++] = m.substr(0, pos); 
		m.erase(0, pos + 1);
	}
	v1 = temparr[1];
	v2 = temparr[2];
}

std::string Server::makeMsg(std::string rtype, std::string v1, std::string v2, std::string v3, std::string v4){
	std::string result;
		result.append(rtype);
		result.append(";");
		result.append(v1); // отправитель
		result.append(";");
		result.append(v2); // получатель
		result.append(";");
		result.append(v3); // время
		result.append(";");
		result.append(v4); // сообщение
		result.append(";");
	return result;
}

void Server::splitMsg(std::string &m, std::string &v1, std::string &v2, std::string &v3, std::string &v4){
	std::string temparr[5];
	size_t pos{0};
	int i{0};
	while((pos = m.find(";")) != std::string::npos){
		temparr[i++] = m.substr(0, pos); 
		m.erase(0, pos + 1);
	}
	v1 = temparr[1];
	v2 = temparr[2];
	v3 = temparr[3];
	v4 = temparr[4];
}

int Server::findLogin(std::string &user){
	for(int i = 0; i < _usercount; i ++){
		if(user == _users[i]._name){
			return i;
		}
	}
	return -3;
}

int Server::loginCheck(std::string &l){
	for(int i = 0; i < l.size(); i++){
		if(l[i] == ';' || l [i] == '/' || l [i] == ':' ) { return 5; }
	}
	for(int i = 0; i < _users.size(); i++){
		if(l == _users[i]._name) { return 4; }
	}
	return 1;
}

bool Server::is_authorized(std::string &l){
	if(findLogin(l) == -3) { return false; }
	if(!_users[findLogin(l)].getStatusAuth()) { return false; }
	return true;
}

void Server::User::switchStatus(){
	if(this->_status == 1){
		this->_status = userstatus::online;
	}
	else{
		this->_status = userstatus::offline;
	}
}

void Server::User::switchStatusAuth(){
	if(this->_auth == false){
		this->_auth = true;
	}
	else{
		this->_auth = false;
	}
}

bool Server::User::getStatusAuth(){
	return _auth;
}

void Server::errorAccess(int &i){
	send(i, ERROR_AUTH, CODE_LENGTH, 0);
	std::cout << ERROR_MESSAGE << "Пользователь не авторизован!" << std::endl;
}

void Server::readConfig(){
	std::ifstream cfg(config_path, std::ios::in);
	if(cfg.is_open()){
		std::string x, y;
		while(getline(cfg, x, '\t')){
			getline(cfg, y, '\n');
				
			if(x == "PORT"){ PORT = stoi(y); }
			if(x == "sql_user"){ sql_user = y; }
			if(x == "sql_pass"){ sql_pass = y; }
			if(x == "sql_db"){ sql_db =  y; }
		}	
		cfg.close();
	}
	else{
		PORT = 7777;
		sql_user = "lightchat";
		sql_pass = "lightchat"; 
		sql_db = "lightchatdb";
		writeConfig();
	}
}

void Server::writeConfig(){
	std::ofstream cfg(config_path, std::ios::out);
    if(cfg.is_open()){
		cfg << "PORT";
		cfg << '\t';
		cfg << PORT;
		cfg << '\n';

		cfg << "sql_user";
		cfg << '\t';
		cfg << sql_user;
		cfg << '\n';

		cfg << "sql_pass";
		cfg << '\t';
		cfg << sql_pass;
		cfg << '\n';

		cfg << "sql_db";
		cfg << '\t';
		cfg << sql_db;
		cfg << '\n';

		cfg.close();
   } 	
   else{
		std::cout << "Ошибка открытия файла" << std::endl;
   }
}
