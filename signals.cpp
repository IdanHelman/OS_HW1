#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

//extern sig_atomic_t stop_loop;

//extern bool cameFromWatch;

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << endl;
    int pid =  SmallShell::getInstance().getFgPid();
    if (pid != -1) {
        if (kill(pid, SIGKILL) == -1){
            perror("smash error: kill failed");
        }
        SmallShell::getInstance().setFgPid(-1);
        cout << "smash: process " << pid << " was killed" << endl;
    }
    else if(SmallShell::getInstance().cameFromWatch == false){ //this is for not getting smash> smash> during Watch
    //    cout << SmallShell::getInstance().getPrompt() << "> " << flush;
    }
    SmallShell::getInstance().stop_loop = 1;
    SmallShell::getInstance().cameFromWatch = false;
}
