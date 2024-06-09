#include "signals.h"
#include "Commands.h"
#include "assert.h"
#include <iostream>
#include <signal.h>

using namespace std;

void ctrlZHandler(int sig_num) {
  SmallShell::getInstance().stopCurrentCommand();
}

void ctrlCHandler(int sig_num) {
  SmallShell::getInstance().killCurrentCommand();
}

void alarmHandler(int sig_num, siginfo_t *info, void *) {
  cout << "smash: got an alarm" << endl;
  SmallShell &smash = SmallShell::getInstance();
  pid_t pid = info->si_pid;
  std::string command_line;
  if (smash.getCurrentCommandPid() == pid) {
    command_line = smash.getCurrentCommand()->getCommandLine();
  } else {
    auto job = smash.getJobList()->getJobByPid(pid);
    assert(job == nullptr);
    command_line = job->command->getCommandLine();
  }
  kill(pid, SIGKILL);
  std::cout << "smash: " << command_line << " timed out!" << std::endl;
}
