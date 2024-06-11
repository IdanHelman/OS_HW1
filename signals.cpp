#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << endl;
    int pid =  SmallShell::getInstance().getFgPid();
    if (pid == -1) {
        return;
    }
    if (kill(pid, SIGKILL) == -1){
        perror("smash error: kill failed");
    }
    SmallShell::getInstance().setFgPid(-1);
    cout << "smash: process " << pid << " was killed" << endl;
}
