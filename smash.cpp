#include "Commands.h"
#include "signals.h"
#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (signal(SIGTSTP, ctrlZHandler) == SIG_ERR) {
    perror("smash error: failed to set ctrl-Z handler");
  }
  if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
    perror("smash error: failed to set ctrl-C handler");
  }

  struct sigaction siga;
  siga.sa_sigaction = alarmHandler;
  siga.sa_flags |= SA_SIGINFO;
  if (sigaction(SIGALRM, &siga, nullptr) == -1) {
    perror("smash error: failed to set alarm handler");
  }

  // TODO: setup sig alarm handler

  SmallShell &smash = SmallShell::getInstance();
  while (smash.isSmashWorking()) {
    std::cout << smash.getDisplayPrompt() << "> ";
    std::string cmd_line;
    std::getline(std::cin, cmd_line);
    smash.executeCommand(cmd_line.c_str());
  }
  return 0;
}