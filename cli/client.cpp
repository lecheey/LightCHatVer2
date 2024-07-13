#include <iostream>
#include <unistd.h> // вызовы linux/bsd
#include <string.h>
#include <cstring>
#include <sys/socket.h> // сокет
#include <arpa/inet.h>
#include <fstream>
#include <filesystem>
#include "client.h"

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

int sockd, connection;
struct sockaddr_in serveraddress, clientaddress;
char message[MESSAGE_LENGTH];

namespace fs = std::filesystem; 

void Client::clientRuntime(){
	std::cout << "Подключение к " << SERVER_IP << "...";
	
	fs::path dir("userlog/");
	if(!fs::exists(dir)){
		create_directory(dir);
	}

	clientConnect();
	
	while(true){
		std::cout << getTime() << std::endl;
		std::cout << "Главное меню\n1 - регистрация 2 - вход q - выход: ";
		char choice;
		std::cin >> choice;
		
		if(choice == 'q'){
			break;
		}		
		if(choice == 'e'){
			send(sockd, QUIT, CODE_LENGTH, 0);
			break;
		}
		
		switch(choice){
			case '1':
				userCreate();
				break;
			case '2':
				if(userLogin()){
					getUserList(_username);
					userRuntime(_username);
				}
				else{
					std::cout << "Не удалось войти в систему!" << std::endl;
				}
				break;
			default:
				std::cout << "Неверное значение\n";
				break;
		}
	}
	close(sockd);
}

void Client::clientConnect(){
	sockd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockd == -1){
		std::cout << ERROR_MESSAGE << "\nСокет не был создан" << std::endl;
		exit(1);
	}

	serveraddress.sin_addr.s_addr = inet_addr(SERVER_IP.data());
	serveraddress.sin_port = htons(PORT);
	serveraddress.sin_family = AF_INET;
	connection = connect(sockd, (struct sockaddr*)&serveraddress, sizeof(serveraddress));
	if(connection == -1){
		std::cout << ERROR_MESSAGE << "\nСервер не отвечает" << std::endl;
		exit(1);
	}
	else{
		std::cout << "успешно" << std::endl;
	}
}

void Client::userCreate(){
	systemClear();
	std::cout << "Создание нового пользователя..." << std::endl;
	std::cin.ignore(256, '\n');
	
	while(true){
		std::string login;
		std::cout << "Придумайте логин: ";	
		std::getline(std::cin, login, '\n');
		
		if(login == "q") { break; }
		
		if(loginCheck(login) == 5){
			std::cout << "Недопустимые символы ( ; или : или / )" << std::endl; 
			continue;
		}
		if(loginCheck(login) == 4){
			std::cout << "Пользователь с таким логином уже есть вашей системе" << std::endl; 
			continue;
		}

		char pass[16];
		std::cout << "Придумайте пароль: ";
		std::cin.getline(pass, 16, '\n');
		uint* h = sha1(pass, std::strlen(pass));
		
		std::string req = makeReq("REGUSR", login, to_string(*h));	
		bzero(message, MESSAGE_LENGTH);
		strcpy(message, req.data());
		send(sockd, message, sizeof(message), 0);

		bzero(message, sizeof(message));
		recv(sockd, message, MESSAGE_LENGTH, 0);
		
		if(strncmp(message, SUCCESS, CODE_LENGTH) == 0 ){
			std::cout << "Пользователь создан!" << std::endl;
		}
		else if(strncmp(message, ERROR_SYMBOLS, CODE_LENGTH) == 0 ){
			std::cout << "Недопустимые символы ( ; или : или / )" << std::endl;
			continue;
		}
		else if(strncmp(message, ERROR_OCCUPIED, CODE_LENGTH) == 0 ){
			std::cout << "Логин занят" << std::endl;
			continue;
		}
		else if(strncmp(message, RETURN, CODE_LENGTH) == 0){
			std::cout << "Отмена" << std::endl;
			break;
		}
		else{
			std::cout << ERROR_MESSAGE << " сервера" << std::endl;
			continue;
		}

		userAdd(login);
		delete[] h;
		break;
	}
}

bool Client::userLogin(){
	std::cout << "Вход в систему" << std::endl;
	std::cin.ignore(256, '\n');
	
	int a = 3;
	std::string b;
	bool result{false};

	while(a >= 0){
		if(a == 0) {
			std::cout << "Превышен лимит попыток!" << std::endl;
			break;
		}
		if(a == 1) { b = "a"; }
			  else { b = "и"; }

		std::cout << "Осталось " << a << " попытк" << b << std::endl;
		
		std::string login;
		std::cout << "Введите логин: ";	
		std::getline(std::cin, login, '\n');
		
		if(login == "q") { break; }
		
		if(loginCheck(login) == 5){
			std::cout << "Недопустимые символы ( ; или : или / )" << std::endl; 
			--a;
			continue;
		}
		
		char pass[16];
		std::cout << "Введите пароль: ";
		std::cin.getline(pass, 16, '\n');
		uint* h = sha1(pass, std::strlen(pass));
		
		std::cout << "Проверка данных пользователя...";

		std::string req = makeReq("LOG_IN", login, to_string(*h));	
		bzero(message, MESSAGE_LENGTH);
		strcpy(message,req.data());
		send(sockd, message, sizeof(message), 0);

		bzero(message, sizeof(message));
		recv(sockd, message, MESSAGE_LENGTH, 0);
		
		if(strncmp(message, SUCCESS, CODE_LENGTH) == 0 ){
			if(findLogin(login) == -3){
				userAdd(login);
			}
			_users[findLogin(login)].switchStatus();
			send(sockd, SUCCESS, CODE_LENGTH, 0);
		
			bzero(message, sizeof(message));
			recv(sockd, message, MESSAGE_LENGTH, 0);
			
			_users[findLogin(login)]._uid = message;

			_username = login;
			std::cout << "Успешно!\nДобро пожаловать " << login << std::endl;
			result = true;
			break;
		}
		else if(strncmp(message, ERROR_AUTH, CODE_LENGTH) == 0 ){
			std::cout << ERROR_MESSAGE << "! Неверный пароль" << std::endl;
			--a;
			continue;
		}	
		else if(strncmp(message, ERROR_NOTFOUND, CODE_LENGTH) == 0 ){
			std::cout << ERROR_MESSAGE << "! Неверный логин" << std::endl;
			--a;
			continue;
		}	
		else{
			std::cout << ERROR_MESSAGE << " сервера" << std::endl;
			continue;
		}
	}
	return result;
}

bool Client::userLogout(std::string &l){
	std::string req = makeReq("LOGOUT", l, "0");	
	bzero(message, sizeof(message));
	strcpy(message, req.data());
	send(sockd, message, sizeof(message), 0);

	bzero(message, sizeof(message));
	recv(sockd, message, MESSAGE_LENGTH, 0);
		
	if(strncmp(message, ERROR_NOTFOUND, CODE_LENGTH) == 0){
		std::cout << "Такого пользователя нет в системе!" << std::endl;
	}
	else if(strncmp(message, ERROR_AUTH, CODE_LENGTH) == 0 ){
		std::cout << ERROR_MESSAGE << "\nПользователь не авторизован" << std::endl;
	}
	else if(strncmp(message, SUCCESS, CODE_LENGTH) == 0 ){
		_users[findLogin(l)].switchStatus();
		_messages.clear();
		_friends.clear();
		systemClear();
		std::cout << "Пользователь " << l <<  " вышел из чата!" << std::endl;
		return false; 
	}
	return true; 
}

void Client::userAdd(std::string &l){
	std::cout << "Создание локального аккаунта пользователя...";

	User newUser(l);
	_users.push_back(newUser);
	++_usercount;
	
	writeUser(l);

	std::cout << "успешно" << std::endl;
}

void Client::userRuntime(std::string &_from){
	std::string _to;
	int _f, _t;
	while(true){
		systemClear();
		std::cout << "Друзья в чате (введите имя)\n-----" << std::endl; 
		showUsers();
		std::cout << "-----" << std::endl;
		std::cout << _from << ": ";	
		std::getline(std::cin, _to, '\n');

		if(_to == "q"){
			userLogout(_from);
			break;
		}
		
		std::string req = makeReq("OPTUSR", _from, _to);	
		bzero(message, sizeof(message));
		strcpy(message, req.data());
		send(sockd, message, sizeof(message), 0);

		bzero(message, sizeof(message));
		recv(sockd, message, MESSAGE_LENGTH, 0);
		
		if(strncmp(message, ERROR_NOTFOUND, CODE_LENGTH) == 0){
			std::cout << "Такого пользователя нет в системе" << std::endl;
			continue;
		}
		else if(strncmp(message, ERROR_AUTH, CODE_LENGTH) == 0 ){
			std::cout << ERROR_MESSAGE << "\nПользователь не авторизован" << std::endl;
			break;
		}
		
		_f = findLogin(_from);
		_t = findFriend(_to);

		systemClear();
		std::cout << "---------- Собеседник  " << _to << " ----------" << std::endl; 
				
		getMsgList(_f, _t);
		//getMsgList(_from, _to);
		//showMsgs(_from, _to);
		userTyping(_f, _t);
	}
}

void Client::userTyping(int &_from, int &_to){
	//std::cin.ignore(256, '\n');
	std::string _time, _msgbody, msgPkg;

	while(true){
		std::cout << _users[_from]._login << ": ";
		std::getline(std::cin, _msgbody, '\n');
		_time = getTime();

		if(_msgbody == "q"){
			break;
		}

		msgPkg = makeMsg("USRTYP", _users[_from]._uid, _friends[_to]._uid, _time, _msgbody);
		bzero(message, sizeof(message));
		strcpy(message, msgPkg.data());
		send(sockd, message, sizeof(message), 0);
		//saveMsg(_from, _to, msg, _time);

		bzero(message, sizeof(message));
		recv(sockd, message, MESSAGE_LENGTH, 0);	
		//splitReq(message, msg, _time);
		//std::cout << _to << " (" <<  _time << "): " << msg << std::endl;
		//saveMsg(_to, _from, msg, _time);
	}
}


void Client::readUser(){
	fs::path filepath{"accounts.txt"};
	std::ifstream file(filepath, std::ios::in);
	
	std::string l, p, n, temp;
	if(file.is_open()){
		getline(file, temp, '\n');
		_usercount = stoi(temp);
		for(int i = 0; i < _usercount; i++){
			getline(file, l, ':');
			getline(file, p, ':');
			getline(file, n, ';');
			
			User user(l,p,n);
			_users.push_back(user);
		}	
		file.close();
	}
}

void Client::writeUser(std::string l){
	fs::path filepath{"accounts.txt"};
	std::ofstream file(filepath, std::ios::out);
    if(file.is_open()){
		file << _usercount << '\n';
		for(int i = 0; i < _usercount; i++){
			file << _users[i]._login << ':' << _users[i]._pass << ':' << _users[i]._name <<';';
		}
		file.close();
    }
	
	fs::path dir("userlog/" + l);
	if(!fs::exists(dir)){
		create_directory(dir);
	}
}

void Client::getUserList(std::string &l){
	std::cout << "Обновление списка пользователей...";
	
	std::string req = makeReq("GETLST", l, "0");	
	bzero(message, MESSAGE_LENGTH);
	strcpy(message,req.data());
	send(sockd, message, sizeof(message), 0);
	
	while(true){
		bzero(message, sizeof(message));
		read(sockd, message, MESSAGE_LENGTH);

		if(strncmp(message, END, CODE_LENGTH) == 0){ break; }
		
		if(strncmp(message, ERROR_AUTH, CODE_LENGTH) == 0 ){
			std::cout << ERROR_MESSAGE << "\nПользователь не авторизован" << std::endl;
			break;
		}
		
		std::string m, id_temp;
		splitReq(message, id_temp, m);

		User newFriend(m, id_temp);
		_friends.push_back(newFriend);
		send(sockd, SUCCESS, CODE_LENGTH, 0);
	}
}

void Client::getMsgList(int &_from, int &_to){
	std::string req = makeReq("GETMSG", _users[_from]._uid, _friends[_to]._uid);	
	bzero(message, MESSAGE_LENGTH);
	strcpy(message,req.data());
	send(sockd, message, sizeof(message), 0);
	
	while(true){
		bzero(message, sizeof(message));
		read(sockd, message, MESSAGE_LENGTH);

		if(strncmp(message, END, CODE_LENGTH) == 0){ break; }
		
		if(strncmp(message, ERROR_AUTH, CODE_LENGTH) == 0 ){
			std::cout << ERROR_MESSAGE << "\nПользователь не авторизован" << std::endl;
			break;
		}
		
		std::string from_temp, to_temp, _time, _msgbody;
		splitMsg(message, from_temp, to_temp, _time, _msgbody);
		
		if(_friends[_to]._uid == from_temp){
			std::cout << _friends[_to]._login << " (" << _time << "): " << _msgbody << std::endl;
		}
		
		if(_users[_from]._uid == from_temp){
			std::cout << _users[_from]._login << " (" << _time << "): " << _msgbody << std::endl;
		}	

		send(sockd, SUCCESS, CODE_LENGTH, 0);
	}

}
/*
void Client::getMsgList(std::string &_from, std::string &_to){
	std::string req = makeReq("GETMSG", _from, _to);	
	bzero(message, MESSAGE_LENGTH);
	strcpy(message,req.data());
	send(sockd, message, sizeof(message), 0);
	
	while(true){
		bzero(message, sizeof(message));
		read(sockd, message, MESSAGE_LENGTH);

		if(strncmp(message, END, CODE_LENGTH) == 0){ break; }
		
		if(strncmp(message, ERROR_AUTH, CODE_LENGTH) == 0 ){
			std::cout << ERROR_MESSAGE << "\nПользователь не авторизован" << std::endl;
			break;
		}
		
		std::string from_temp, to_temp, _time, _msgbody;
		splitMsg(message, from_temp, to_temp, _time, _msgbody);
		
		int i = 0;
		for(; i < _friends.size(); i++){
			if(_friends[i]._uid == from_temp){ break; }
		}
		
		std::cout << _friends[i]._login << " (" << _time << "): " << _msgbody << std::endl;

		send(sockd, SUCCESS, CODE_LENGTH, 0);
	}
}
*/
/*
void Client::saveMsg(std::string &_from, std::string &_to, std::string &_msg, std::string &_time){
	fs::path p = "usr/" + _username;
	fs::path dir(p);
	if(!fs::exists(dir)){
		create_directory(dir);
	}

	if(_to != "all"){
		fs::path filepath1{dir / (_rcvrname + ".txt")};
		std::ofstream file1(filepath1, std::ios::app);
		if(file1.is_open()){
    	   file1 << _time << ';' << _from << ';' << _to << ';' << _msg <<'\n';
    	   file1.close();
		}
	}
	else{
		fs::path filepath2{dir / "group.txt"};
		std::ofstream file2(filepath2, std::ios::app);
    	if(file2.is_open()){
    	   file2 << _time << ';' << _from << ';' << _to << ';' << _msg <<'\n';
    	   file2.close();
    	}
	}
}
void Client::getMsgList(std::string &_from, std::string &_to){
	fs::path p = "usr/" + _from;
	fs::path dir(p);
	if(fs::exists(dir)){
		std::string d, f, t, m, paths;

		paths = _to != "all" ?  (_to + ".txt") : "group.txt";
		
		fs::path filepath{dir / paths};
		std::ifstream file(filepath, std::ios::in);
		
		if(file.is_open()){
			while(getline(file, d, ';')){
				getline(file, f, ';');
				getline(file, t, ';');
				getline(file, m, '\n');

				Message msg(d, f, t,m);
				_messages.push_back(msg);
			}
			file.close();
		}
	}
}
void Client::showMsgs(std::string &_from, std::string &_to){
	for(int i = 0; i < _messages.size(); i++){
		if(_to == "all" && _to == _messages[i]._receiver){
			std::cout << _messages[i]._sender << " (" << _messages[i]._time <<  "): " <<  _messages[i]._msg << std::endl;
		}
		else if(_from == _messages[i]._sender && _to == _messages[i]._receiver){
			std::cout << _messages[i]._sender << " (" << _messages[i]._time <<  "): " <<  _messages[i]._msg << std::endl;
		}
		else if(_from == _messages[i]._receiver && _to == _messages[i]._sender){
			std::cout << _messages[i]._sender << " (" << _messages[i]._time <<  "): " <<  _messages[i]._msg << std::endl;
		}
	}
}
*/

int Client::findLogin(std::string &user){
	for(int i = 0; i < _usercount; i++){
		if(user == _users[i]._login){
			return i;
		}
	}
	return -3;
}

int Client::findFriend(std::string &user){
	for(int i = 0; i < _usercount; i++){
		if(user == _friends[i]._login){
			return i;
		}
	}
	return -3;
}

int Client::loginCheck(std::string &l){
	for(int i = 0; i < l.size(); i++){
		if(l[i] == ';' || l [i] == '/' || l [i] == ':' ) { return 5; }
	}

	for(int i = 0; i < _users.size(); i++){
		if(l == _users[i]._login) { return 4; }
	}
	return 1;
}

void Client::showUsers(){
	for(int i = 0; i < _friends.size(); i++){
		std::cout << _friends[i]._uid << " " << _friends[i]._login << std::endl;
	}
}

void Client::User::switchStatus(){
	if(this->_status == 1){
		this->_status = userstatus::online;
	}
	else{
		this->_status = userstatus::offline;
	}
}
/*
void Client::splitUserList(std::string &m){
	size_t pos{0};
	int i{0};
	while((pos = m.find(";")) != std::string::npos){
		_friends.push_back(m.substr(0, pos));
		m.erase(0, pos + 1);
	}
}
*/
std::string Client::makeReq(std::string rtype, std::string v1, std::string v2){
	std::string result;
		result.append(rtype);
		result.append(";");
		result.append(v1);
		result.append(";");
		result.append(v2);
		result.append(";");
	return result;
}

void Client::splitReq(std::string m, std::string &v1, std::string &v2){
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

std::string Client::makeMsg(std::string rtype, std::string v1, std::string v2, std::string v3, std::string v4){
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

void Client::splitMsg(std::string m, std::string &v1, std::string &v2, std::string &v3, std::string &v4){
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

void Client::readConfig(){
	std::ifstream cfg(config_path, std::ios::in);
	if(cfg.is_open()){
		std::string x, y;
		while(getline(cfg, x, '\t')){
			getline(cfg, y, '\n');
				
			if(x == "PORT"){ PORT = stoi(y); }
			else if(x == "SERVER_IP"){ SERVER_IP = y; }
		}	
		cfg.close();
	}
	else{
		PORT = 7777;
		SERVER_IP = "127.0.0.1";
		writeConfig();
	}
}

void Client::writeConfig(){
	std::ofstream cfg(config_path, std::ios::out);
    if(cfg.is_open()){
		cfg << "PORT";
		cfg << '\t';
		cfg << PORT;
		cfg << '\n';

		cfg << "SERVER_IP";
		cfg << '\t';
		cfg << SERVER_IP;
		cfg << '\n';
		
		cfg.close();
   } 	
   else{
		std::cout << "Ошибка открытия файла" << std::endl;
   }
}
