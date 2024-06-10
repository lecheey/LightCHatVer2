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
#define PORT 7777

int sockd, bind_stat, connection_stat, connection;
struct sockaddr_in serveraddress, clientaddress;
socklen_t length;
char message[MESSAGE_LENGTH];
int bytes_read;

namespace fs = std::filesystem; 

void Server::serverRuntime(){
	std::cout << "Запуск сервера..." << std::endl;
	serverStart();

	while(true){
        fd_set readset;
        FD_ZERO(&readset);
        FD_SET(sockd, &readset);

        for(std::set<int>::iterator it = clients.begin(); it != clients.end(); it++)
            FD_SET(*it, &readset);

		//std::cout << "i1" << std::endl;

        timeval timeout;
        timeout.tv_sec = 180;
        timeout.tv_usec = 0;

        int mx = std::max(sockd, *max_element(clients.begin(), clients.end()));
        if(select(mx+1, &readset, NULL, NULL, &timeout) <= 0){
            std::cout << ERROR_MESSAGE << ": Selecting error" << std::endl;
            exit(3);
        }
		
		std::cout << "m" << std::endl;

        if(FD_ISSET(sockd, &readset)){
            int connection = accept(sockd, NULL, NULL);
			if(connection == -1){
				std::cout << ERROR_MESSAGE << ": Socket is unable to reach client!" << std::endl;
				exit(1);
			}	
			else{
				std::cout << "Пользователь подключен!" << std::endl;
			}
			
			std::cout << "a" << std::endl;
            
            fcntl(connection, F_SETFL, O_NONBLOCK);

            clients.insert(connection);
        }
        
		for(std::set<int>::iterator it = clients.begin(); it != clients.end(); it++){
		
			//std::cout << "i2" << std::endl;
			
            if(FD_ISSET(*it, &readset)){
				std::cout << "Начало" << std::endl;
		std::cout << *it << std::endl;

				if(_switcher < 1){
				bzero(message, sizeof(message));
				recv(*it, message, MESSAGE_LENGTH, 0);
				
				if(strncmp(message, QUIT, CODE_LENGTH) == 0){
					std::cout << "Клиент отключился!" << std::endl;
					std::cout << "Завершение работы сервера...!" << std::endl;
					exit(1);
				}
					
				std::cout << "середина" << std::endl;


				if(strncmp(message, "REGUSR", COMMAND_LENGTH) == 0){
						userReg(*it, message);
					}
				else if(strncmp(message, "LOG_IN", COMMAND_LENGTH) == 0){
						userAuth(*it, message);
					}
				else if(strncmp(message, "LOGOUT", COMMAND_LENGTH) == 0){
						userLogout(*it, message);
					}
				else if(strncmp(message, "GETLST", COMMAND_LENGTH) == 0){
						sendUserList(*it, message);
					}
				else if(strncmp(message, "OPTUSR", COMMAND_LENGTH) == 0){
						optDialog(*it, message);
						bzero(message, sizeof(message));
					}
				else if(strncmp(message, "HNDSHK", COMMAND_LENGTH) == 0){	
						++_switcher;	
						if(_switcher == 1){
						send(*it, message, sizeof(message), 0);
						bzero(message, sizeof(message));
						}
					}
				else if(strncmp(message, "USRTYP", COMMAND_LENGTH) == 0){
						//send(*it, message, sizeof(message), 0);
					}
				else{
					close(sockd);
					std::cout << "Завершение работы" << std::endl;
					exit(1);
				}
				}
				else{
				std::cout << "you r here" << std::endl;
				send(*it, message, sizeof(message), 0);
	
				bzero(message, sizeof(message));
				recv(*it, message, MESSAGE_LENGTH, 0);
				}
			}
		}
	}
	close(sockd);
	std::cout << "Завершение работы" << std::endl;
	exit(1);
}

void Server::serverStart(){
	sockd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockd < 0){
		std::cout << ERROR_MESSAGE << ": Socket creation falied!" << std::endl;
        exit(1);
    }
    
    fcntl(sockd, F_SETFL, O_NONBLOCK);
    
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
    
	//std::set<int> clients;
    clients.clear();
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
		send(index, ERROR_SYMBOLS, CODE_LENGTH, 0); // ошибка: стоп-символы
		std::cout << ERROR_MESSAGE << std::endl;
		break;
	default:
		send(index, RETURN, CODE_LENGTH, 0);
		break;
	}					
}				

void Server::userAuth(int index, std::string m){
	std::cout << "Вход в систему" << std::endl;
	
	std::string login_temp, pass_temp;
	splitReq(m, login_temp, pass_temp);

	int ind = findLogin(login_temp);
	if(index == -3){
		send(index, ERROR_NOTFOUND, CODE_LENGTH, 0);
		std::cout << ERROR_MESSAGE << std::endl;
	}
	else{
		if(_users[ind]._pass == pass_temp){
			send(index, SUCCESS, CODE_LENGTH, 0);
			_users[ind].switchStatus();
			_users[ind].switchStatusAuth();
			std::cout << "Успешно" << std::endl;	
		}
		else{
			send(index, ERROR_AUTH, CODE_LENGTH, 0);
			std::cout << ERROR_MESSAGE << std::endl;	
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

	User newUser(l, p);
	_users.push_back(newUser);
	++_usercount;
	
	writeUser();

	std::cout << "успешно" << std::endl;
}
/*
void Server::handShake(int index){
	if(swtchr = true){
		std::string msg = "пользователь зашел в чат";
		std::string msgPkg = makeReq("USRTYP", msg, getTime());
		std::cout << msgPkg << std::endl;
		strcpy(message, msgPkg.data());
		send(index, message, sizeof(message), 0);
		swtchr = false;
	}
	else{
		swtchr = true;
	}
}
*/
void Server::sendUserList(int index, std::string m){
	std::cout << "Передача списка пользователей...";
	std::string login_temp, dummy;
	splitReq(m, login_temp, dummy);
	//std::cout << login_temp << std::endl;
	
	if(is_authorized(login_temp)){
		std::string pkg = pkgUserList();
		strcpy(message, pkg.data());
		if(sizeof(message) <= MESSAGE_LENGTH){
			send(index, message, sizeof(message), 0);
			std::cout << "успешно" << std::endl;
		}
		else{
			send(index, ERROR_NOTFOUND, CODE_LENGTH, 0);
			std::cout << ERROR_MESSAGE << std::endl;
		}
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

std::string Server::pkgUserList(){
	std::string result;
	for(int i = 0; i < _users.size(); i++){
		result.append(_users[i]._login);
		result.append(";");
	}
	return result;
}

int Server::findLogin(std::string &user){
	for(int i = 0; i < _usercount; i ++){
		if(user == _users[i]._login){
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
		if(l == _users[i]._login) { return 4; }
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
