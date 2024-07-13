#include <iostream>
#include <string>
#include <ctime>
#include "func.h"

#if defined (__linux__)
#include <sys/utsname.h>
#include <unistd.h>
#endif

void userHelp(){
	std::cout << "[r]egistration\tдля регистрации нового пользователя\n[l]og in\tдля входа в аккаунт\n[c]atalog\tдля вывода списка пользователей\n[q]uit\tдля выхода из программы\n" << std::endl;
}

std::string getTime(){
	std::time_t result = std::time(nullptr);
	char mbstr[100];
	std::strftime(mbstr, sizeof(mbstr), "%Y-%m-%d %T", std::localtime(&result));
	return (std::string)mbstr;
}

void systemInfo(){
	#if defined (__linux__)
	struct utsname s;
	uname(&s);
	std::cout << s.sysname << ' ' << s.version << ' ' << s.machine << std::endl;
	std::cout << "Process ID: " << getpid() << std::endl;
	#elif defined (__WIN64__)
	
	#endif
}

void systemClear(){
	#if defined (__linux__)
	system("clear");
	#elif defined (__WIN64__)
	system("clr");
	#endif
}
