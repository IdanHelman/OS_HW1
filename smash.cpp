#include <iostream>
#include <unistd.h>
//#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

extern sig_atomic_t stop_loop;

int main(int argc, char *argv[]) {
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    //TODO: setup sig alarm handler
    //int i = 0;
    SmallShell &smash = SmallShell::getInstance();
    while (true) {
        std::cout << smash.getPrompt() + "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        //cout << "hello " << i++ << endl;
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}