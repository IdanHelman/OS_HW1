#include "Commands.h"
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits.h>
#include <sstream>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY() cout << __PRETTY_FUNCTION__ << " --> " << std::endl;

#define FUNC_EXIT() cout << __PRETTY_FUNCTION__ << " <-- " << std::endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif
// Table of Content:
// Utillities functions ------------------------------------------- Line 30
// Small Shell Functions ------------------------------------------ Line 90
// Command Functions ---------------------------------------------- Line 215
// Joblist Functions ---------------------------------------------- Line 400

//                                                                      //
//------------------------- Utility functions --------------------------//
//                                                                      //
std::string _ltrim(const std::string &s) {
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

std::string _rtrim(const std::string &s) {
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string _trim(const std::string &s) { return _rtrim(_ltrim(s)); }

int _parseCommandLine(const std::string &cmd_line, char **args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(cmd_line).c_str());
  for (std::string s; iss >> s;) {
    args[i] = (char *)malloc(s.length() + 1);
    memset(args[i], 0, s.length() + 1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
  const std::string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
  const std::string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == std::string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing
  // spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

static void syscallError(const std::string &syscall) {
  std::string msg =
      std::string("smash error: " + syscall + std::string(" failed"));
  perror(msg.c_str());
}

//                                                                     //
//------------------------Small Shell functions------------------------//
//                                                                     //
SmallShell::SmallShell()
    : smash_pid(getpid()), current_display_prompt("smash"), last_dir(""),
      default_display_prompt("smash"), is_working(true) {}

// TODO: add your implementation

SmallShell::~SmallShell() {
  // TODO: add your implementation
}

SmallShell::CommandType
SmallShell::checkType(const std::string &cmd_line) const {
  if (std::string(cmd_line).find(">") != std::string::npos) {
    if (std::string(cmd_line).find(">>") != std::string::npos) {
      return CommandType::RedirectAppend;
    }
    return CommandType::Redirect;
  }

  if (std::string(cmd_line).find("|") != std::string::npos) {
    if (std::string(cmd_line).find("|&") != std::string::npos) {
      return CommandType::PipeErr;
    }
    return CommandType::Pipe;
  }

  return CommandType::Regular;
}

const std::string &SmallShell::getLastDir() const { return last_dir; }

void SmallShell::setDisplayPrompt(std::string new_display_line) {
  current_display_prompt = new_display_line;
}
void SmallShell::setLastDir(const std::string &last_dir) {
  this->last_dir = last_dir;
}

const pid_t SmallShell::getPid() const { return smash_pid; }
std::string SmallShell::getDisplayPrompt() const {
  return current_display_prompt;
}
bool SmallShell::isSmashWorking() const { return is_working; }
void SmallShell::disableSmash() { is_working = false; }
void SmallShell::killAllJobs() { jobs.killAllJobs(); }
JobsList *SmallShell::getJobList() { return &jobs; }
Command *SmallShell::getCurrentCommand() const { return current_command; }
pid_t SmallShell::getCurrentCommandPid() const { return current_command_pid; }

void SmallShell::setCurrentCommandPid(pid_t pid) { current_command_pid = pid; }
void SmallShell::setCurrentCommand(Command *command) {
  current_command = command;
}

void SmallShell::stopCurrentCommand() {
  std::cout << "smash: got ctrl-Z" << std::endl;

  if (current_command_pid == -1) {
    return;
  }

  kill(current_command_pid, SIGSTOP);
}

void SmallShell::killCurrentCommand() {
  std::cout << "smash: got ctrl-C" << std::endl;

  if (current_command_pid == -1) {
    return;
  }

  std::cout << "smash: process " << current_command_pid << " was killed"
            << std::endl;
  kill(current_command_pid, SIGKILL);
}
/**
 * Creates and returns a pointer to Command class which matches the given
 * command line (cmd_line)
 */
static std::shared_ptr<Command> CreateCommandImpl(const std::string &cmd_line,
                                                  const std::string &original) {
  // For example:

  std::string cmd_s = _trim(cmd_line);
  bool background_flag = cmd_s.back() == '&';
  if (background_flag) {
    cmd_s.pop_back();
  }
  std::string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  /* check if special command I.E pipe*/

  if (firstWord.compare("chprompt") == 0) {
    return std::make_shared<ChangePromptCommand>(original, cmd_s);
  } else if (firstWord.compare("showpid") == 0) {
    return std::make_shared<ShowPidCommand>(original, cmd_s);
  } else if (firstWord.compare("pwd") == 0) {
    return std::make_shared<GetCurrDirCommand>(original, cmd_s);
  } else if (firstWord.compare("cd") == 0) {
    return std::make_shared<ChangeDirCommand>(original, cmd_s);
  } else if (firstWord.compare("quit") == 0) {
    return std::make_shared<QuitCommand>(original, cmd_s);
  } else if (firstWord.compare("jobs") == 0) {
    return std::make_shared<JobsCommand>(original, cmd_s);
  } else if (firstWord.compare("fg") == 0) {
    return std::make_shared<ForegroundCommand>(original, cmd_s);
  } else if (firstWord.compare("kill") == 0) {
    return std::make_shared<KillCommand>(original, cmd_s);
  } else if (firstWord.compare("bg") == 0) {
    return std::make_shared<BackgroundCommand>(original, cmd_s);
  } else if (firstWord.compare("setcore") == 0) {
    return std::make_shared<SetcoreCommand>(original, cmd_s);
  } else if (firstWord.compare("fare") == 0) {
    return std::make_shared<FareCommand>(original, cmd_s);
  } else {
    return std::make_shared<ExternalCommand>(original, cmd_s, background_flag);
  }

  return nullptr;
}

std::shared_ptr<Command>
SmallShell::CreateCommand(const std::string &cmd_line) {
  return CreateCommandImpl(cmd_line, cmd_line);
}

void SmallShell::CreateRedirectCommand(const std::string &cmd_line,
                                       std::shared_ptr<Command> &outCommand,
                                       std::string &outFileName) {
  bool appendFlag = std::string(cmd_line).find(">>") != std::string::npos;
  size_t index = std::string(cmd_line).find(appendFlag ? ">>" : ">");

  auto command = _trim(std::string(cmd_line).substr(0, index));
  auto fileName =
      _trim(std::string(cmd_line).substr(index + (appendFlag ? 2 : 1)));

  outCommand = CreateCommandImpl(command, cmd_line);
  outFileName = fileName;
}

void SmallShell::CreatePipeCommand(const std::string &cmd_line,
                                   std::shared_ptr<Command> &outCommand1,
                                   std::shared_ptr<Command> &outCommand2) {
  bool errFlag = std::string(cmd_line).find("|&") != std::string::npos;
  size_t index = std::string(cmd_line).find(errFlag ? "|&" : "|");

  auto command1 = _trim(std::string(cmd_line).substr(0, index));
  auto command2 =
      _trim(std::string(cmd_line).substr(index + (errFlag ? 2 : 1)));

  outCommand1 = CreateCommandImpl(command1, cmd_line);
  outCommand2 = CreateCommandImpl(command2, cmd_line);
}

void SmallShell::executeCommand(const char *cmd_line) {
  jobs.removeFinishedJobs();

  auto type = checkType(cmd_line);
  if (type == CommandType::Regular) {
    auto command = CreateCommand(cmd_line);

    // Check if builtin or external
    bool isExternal = dynamic_cast<ExternalCommand *>(command.get()) != nullptr;
    if (isExternal) {
      int pid = fork();
      if (pid == -1) {
        syscallError("fork");
        return;
      }

      if (pid == 0) {
        // Forked child
        command->execute(this);
      } else {
        // Parent
        if (command->isBackgroundCommand()) {
          jobs.addJob(command, pid, false);
        } else {
          current_command_pid = pid;
          current_command = command.get();

          int waitStatus;
          if (waitpid(pid, &waitStatus, WUNTRACED) == -1) {
            syscallError("waitpid");
          }
          current_command_pid = -1;
          current_command = nullptr;

          if (WIFSTOPPED(waitStatus)) {
            jobs.addJob(command, pid, true);
            std::cout << "smash: process " << pid << " was stopped"
                      << std::endl;
          }
        }
      }

    } else {
      command->execute(this);
    }
  } else if (type == CommandType::Redirect ||
             type == CommandType::RedirectAppend) {
    std::shared_ptr<Command> command;
    std::string fileName;
    CreateRedirectCommand(cmd_line, command, fileName);

    bool isExternal = dynamic_cast<ExternalCommand *>(command.get()) != nullptr;
    if (isExternal) {
      int pid = fork();
      if (pid == -1) {
        syscallError("fork");
        return;
      }

      if (pid == 0) {
        // Forked child
        auto fd =
            open(fileName.c_str(),
                 type == CommandType::Redirect ? O_WRONLY | O_CREAT | O_TRUNC
                                               : O_WRONLY | O_CREAT | O_APPEND,
                 0666);
        if (fd == -1) {
          syscallError("open");
          exit(1);
        }

        close(STDOUT_FILENO);
        dup(fd);

        command->execute(this);
      } else {
        // Parent
        if (waitpid(pid, nullptr, 0) == -1) {
          syscallError("waitpid");
        }
      }
    } else {
      // Runs locally so we can open the file and switch the streams easily.
      std::fstream file(fileName, type == CommandType::Redirect
                                      ? std::fstream::out | std::fstream::trunc
                                      : std::fstream::out | std::fstream::app);
      auto originalBuf = std::cout.rdbuf(file.rdbuf());

      command->execute(this);

      std::cout.rdbuf(originalBuf);
    }
  } else if (type == CommandType::Pipe || type == CommandType::PipeErr) {
    std::shared_ptr<Command> command1, command2;
    CreatePipeCommand(cmd_line, command1, command2);

    int pipe[2];
    if (::pipe(pipe) == -1) {
      syscallError("pipe");
      return;
    }

    int read = pipe[0];
    int write = pipe[1];
    int output = type == CommandType::Pipe ? STDOUT_FILENO : STDERR_FILENO;

    bool isExternal1 =
        dynamic_cast<ExternalCommand *>(command1.get()) != nullptr;
    if (isExternal1) {
      int pid = fork();
      if (pid == -1) {
        syscallError("fork");
        return;
      }

      if (pid == 0) {
        // Forked child
        close(read);
        dup2(write, output);

        command1->execute(this);
      } else {
        // Parent
        if (waitpid(pid, nullptr, 0) == -1) {
          syscallError("waitpid");
        }
      }
    } else {
      int originalOut = dup(output);
      dup2(write, output);

      command1->execute(this);

      dup2(originalOut, output);
    }
    close(write);

    bool isExternal2 =
        dynamic_cast<ExternalCommand *>(command2.get()) != nullptr;
    if (isExternal2) {
      int pid = fork();
      if (pid == -1) {
        syscallError("fork");
        return;
      }

      if (pid == 0) {
        // Forked child
        dup2(read, STDIN_FILENO);

        command2->execute(this);
      } else {
        // Parent
        if (waitpid(pid, nullptr, 0) == -1) {
          syscallError("waitpid");
        }
      }
    } else {
      int originalIn = dup(STDIN_FILENO);
      dup2(read, STDIN_FILENO);

      command2->execute(this);

      dup2(originalIn, STDIN_FILENO);
    }
    close(read);
  }
}

//                                                                 //
//------------------------Command functions------------------------//
//                                                                 //
Command::Command(const std::string &cmd_line,
                 const std::string &cmd_line_stripped,
                 bool background_command_flag)
    : command_line(cmd_line), argv(new char *[MAX_ARGV_LENGTH]),
      argc(_parseCommandLine(cmd_line_stripped, argv)),
      background_command_flag(background_command_flag),
      startTime(time(nullptr)), jobId(-1) {}

Command::~Command() {
  for (int i = 0; i < argc; i++) {
    delete argv[i];
    argv[i] = NULL;
  }
  delete[] argv;
}

const std::string Command::getCommandLine() const { return command_line; }
const time_t &Command::getStartTime() const { return startTime; }
int Command::getJobId() const { return jobId; }
void Command::setJobId(int id) { jobId = id; }
bool Command::isBackgroundCommand() const { return background_command_flag; }

ChangePromptCommand::ChangePromptCommand(const std::string &cmd_line,
                                         const std::string &cmd_line_stripped)
    : BuiltInCommand(cmd_line, cmd_line_stripped) {}

void ChangePromptCommand::execute(SmallShell *smash) {
  if (argc == 1) {
    smash->setDisplayPrompt("smash");
  } else {
    smash->setDisplayPrompt(std::string(argv[1]));
  }
}

ShowPidCommand::ShowPidCommand(const std::string &cmd_line,
                               const std::string &cmd_line_stripped)
    : BuiltInCommand(cmd_line, cmd_line_stripped) {}

void ShowPidCommand::execute(SmallShell *smash) {
  std::cout << "smash pid is " << smash->getPid() << std::endl;
}

GetCurrDirCommand::GetCurrDirCommand(const std::string &cmd_line,
                                     const std::string &cmd_line_stripped)
    : BuiltInCommand(cmd_line, cmd_line_stripped) {}

void GetCurrDirCommand::execute(SmallShell *smash) {
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    std::cout << cwd << std::endl;
  } else {
    syscallError("getcwd");
  }
}

ChangeDirCommand::ChangeDirCommand(const std::string &cmd_line,
                                   const std::string &cmd_line_stripped)
    : BuiltInCommand(cmd_line, cmd_line_stripped) {}

void ChangeDirCommand::execute(SmallShell *smash) {
  if (argc > 2) {
    std::cerr << "smash error: cd: too many arguments" << std::endl;
    return;
  }

  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    syscallError("getcwd");
  }

  if (argc == 1) {
    if (chdir(getenv("HOME")) != 0) {
      syscallError("chdir");
      return;
    }
  } else {
    const char *target = argv[1];

    // Handle cd to previous directory.
    if (strcmp(target, "-") == 0) {
      auto lastDir = smash->getLastDir();

      if (lastDir.empty()) {
        std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
        return;
      } else {
        if (chdir(lastDir.c_str()) != 0) {
          syscallError("chdir");
          return;
        }
      }
    }

    // Handle general cd.
    else {
      if (chdir(target) != 0) {
        syscallError("chdir");
        return;
      }
    }
  }

  smash->setLastDir(cwd);
}

JobsCommand::JobsCommand(const std::string &cmd_line,
                         const std::string &cmd_line_stripped)
    : BuiltInCommand(cmd_line, cmd_line_stripped) {}
void JobsCommand::execute(SmallShell *smash) {
  smash->getJobList()->printJobsList();
}

QuitCommand::QuitCommand(const std::string &cmd_line,
                         const std::string &cmd_line_stripped)
    : BuiltInCommand(cmd_line, cmd_line_stripped) {}

void QuitCommand::execute(SmallShell *smash) {
  smash->disableSmash();
  if (argc >= 2 && std::string(argv[1]).compare("kill") == 0) {
    smash->killAllJobs();
  }
  // kill the jobs
}

ForegroundCommand::ForegroundCommand(const std::string &cmd_line,
                                     const std::string &cmd_line_stripped)
    : BuiltInCommand(cmd_line, cmd_line_stripped) {}

void ForegroundCommand::execute(SmallShell *smash) {
  JobsList::JobEntry *job;
  if (argc == 1) {
    job = smash->getJobList()->getLastJob();

    if (!job) {
      std::cerr << "smash error: fg: jobs list is empty" << std::endl;
      return;
    }
  } else if (argc == 2) {
    try {
      int id = std::stoi(argv[1]);
      if (std::to_string(id).length() != (std::string(argv[1]).length())) {
        throw std::exception();
      }
      job = smash->getJobList()->getJobById(id);

      if (!job) {
        std::cerr << "smash error: fg: job-id " << id << " does not exist"
                  << std::endl;
        return;
      }
    } catch (const std::exception &e) {
      std::cerr << "smash error: fg: invalid arguments" << std::endl;
      return;
    }
  } else {
    std::cerr << "smash error: fg: invalid arguments" << std::endl;
    return;
  }

  job->state = JobsList::JobState::Running;
  std::cout << job->command->getCommandLine() << " : " << job->pid << std::endl;

  auto pid = job->pid;
  auto command = job->command;

  auto jobs = smash->getJobList();
  jobs->removeJobById(job->id);

  if (kill(pid, SIGCONT) == -1) {
    syscallError("kill");
  }

  smash->setCurrentCommandPid(pid);
  smash->setCurrentCommand(this);
  int waitStatus;
  if (waitpid(pid, &waitStatus, WUNTRACED) == -1) {
    syscallError("waitpid");
  }
  smash->setCurrentCommandPid(-1);
  smash->setCurrentCommand(nullptr);

  if (WIFSTOPPED(waitStatus)) {
    jobs->addJob(command, pid, true);
    std::cout << "smash: process " << pid << " was stopped" << std::endl;
  }
}

BackgroundCommand::BackgroundCommand(const std::string &cmd_line,
                                     const std::string &cmd_line_stripped)
    : BuiltInCommand(cmd_line, cmd_line_stripped) {}

void BackgroundCommand::execute(SmallShell *smash) {
  JobsList::JobEntry *job;
  if (argc == 1) {
    job = smash->getJobList()->getLastStoppedJob();

    if (!job) {
      std::cerr << "smash error: bg: there is no stopped jobs to resume"
                << std::endl;
      return;
    }
  } else if (argc == 2) {
    try {
      int id = std::stoi(argv[1]);
      if (std::to_string(id).length() != std::string(argv[1]).length()) {
        throw std::exception();
      }

      job = smash->getJobList()->getJobById(id);

      if (!job) {
        std::cerr << "smash error: bg: job-id " << id << " does not exist"
                  << std::endl;
        return;
      }

      if (job->state != JobsList::JobState::Stopped) {
        std::cerr << "smash error: bg: job-id " << id
                  << " is already running in the background" << std::endl;
        return;
      }

    } catch (const std::exception &e) {
      std::cerr << "smash error: bg: invalid arguments" << std::endl;
      return;
    }
  } else {
    std::cerr << "smash error: bg: invalid arguments" << std::endl;
    return;
  }

  job->state = JobsList::JobState::Running;
  std::cout << job->command->getCommandLine() << " : " << job->pid << std::endl;

  if (kill(job->pid, SIGCONT) == -1) {
    syscallError("kill");
  }
}
KillCommand::KillCommand(const std::string &cmd_line,
                         const std::string &cmd_line_stripped)
    : BuiltInCommand(cmd_line, cmd_line_stripped) {}
void KillCommand::execute(SmallShell *smash) {
  if (argc != 3) {
    std::cerr << "smash error: kill: invalid arguments" << std::endl;
    return;
  }

  int signum, jobid;

  try {
    signum = sigNumParser();
    jobid = std::stoi(argv[2]);
    if (signum <= 0 || signum > 31 ||
        std::to_string(jobid).length() != (std::string(argv[2]).length())) {
      throw std::exception();
    }
  } catch (const std::exception &e) {
    std::cerr << "smash error: kill: invalid arguments" << std::endl;
    return;
  }

  JobsList::JobEntry *job_to_sig = smash->getJobList()->getJobById(jobid);
  if (!job_to_sig || job_to_sig->state == JobsList::JobState::Killed) {
    std::cerr << "smash error: kill: job-id " << jobid << " does not exist"
              << std::endl;
    return;
  }

  if (kill(job_to_sig->pid, signum) == -1) {
    syscallError("kill");
  }
  std::cout << "signal number " << signum << " was sent to pid "
            << job_to_sig->pid << std::endl;

  if (signum == SIGCONT) {
    job_to_sig->state = JobsList::JobState::Running;
  }
  if (signum == SIGSTOP) {
    job_to_sig->state = JobsList::JobState::Stopped;
  }
  if (signum == SIGKILL) {
    job_to_sig->state = JobsList::JobState::Killed;
  }
}

int KillCommand::sigNumParser() const {
  std::string s = argv[1];
  if (s[0] != '-') {
    throw std::exception();
  }
  s.erase(0, 1);
  int id = std::stoi(s);
  if (std::to_string(id).length() != (std::string(argv[1]).length() - 1)) {
    throw std::exception();
  }
  return id;
}

SetcoreCommand::SetcoreCommand(const std::string &cmd_line,
                               const std::string &cmd_line_stripped)
    : BuiltInCommand(cmd_line, cmd_line_stripped) {}

void SetcoreCommand::execute(SmallShell *smash) {

  JobsList::JobEntry *job;
  int coreNum;

  try {
    if (argc != 3) {
      throw std::exception();
    }

    int jobId = std::stoi(argv[1]);
    if (std::to_string(jobId).length() != std::string(argv[1]).length()) {
      throw std::exception();
    }

    coreNum = std::stoi(argv[2]);
    if (std::to_string(coreNum).length() != std::string(argv[2]).length()) {
      throw std::exception();
    }

    job = smash->getJobList()->getJobById(jobId);
    if (!job) {
      std::cerr << "smash error: setcore: job-id " << jobId << " does not exist"
                << std::endl;
      return;
    }

    if (coreNum < 0 || get_nprocs() <= coreNum) {
      std::cerr << "smash error: setcore: invalid core number" << std::endl;
      return;
    }

  } catch (const std::exception &e) {
    std::cerr << "smash error: setcore: invalid arguments" << std::endl;
    return;
  }

  cpu_set_t cpuSet;
  if (sched_getaffinity(job->pid, sizeof(cpu_set_t), &cpuSet) == -1) {
    syscallError("sched_getaffinity");
    return;
  }

  CPU_ZERO(&cpuSet);
  CPU_SET(coreNum, &cpuSet);

  if (sched_setaffinity(job->pid, sizeof(cpu_set_t), &cpuSet) == -1) {
    syscallError("sched_setaffinity");
    return;
  }
}

FareCommand::FareCommand(const std::string &cmd_line,
                         const std::string &cmd_line_stripped)
    : BuiltInCommand(cmd_line, cmd_line_stripped) {}

void FareCommand::execute(SmallShell *smash) {
  if (argc != 4) {
    std::cerr << "smash error: fare: invalid arguments" << std::endl;
    return;
  }

  int fd = open(argv[1], O_RDONLY);
  if (fd == -1) {
    syscallError("open");
    return;
  }
  close(fd);

  std::string contents;
  {
    std::ifstream file(argv[1]);
    std::stringstream ss;
    ss << file.rdbuf();
    contents = ss.str();
  }

  std::string source = argv[2];
  std::string target = argv[3];

  int counter = 0;
  auto index = contents.find(source);
  while (index != std::string::npos) {
    contents.erase(index, source.length());
    contents.insert(index, target);

    index = contents.find(source, index + target.length());
    counter++;
  }

  std::cout << "replaced " << counter << " instances of the string \"" << source
            << "\"" << std::endl;

  std::ofstream file(argv[1]);
  file << contents;
}

ExternalCommand::ExternalCommand(const std::string &cmd_line,
                                 const std::string &cmd_line_stripped,
                                 bool background_command_flag)
    : Command(cmd_line, cmd_line_stripped, background_command_flag) {}

void ExternalCommand::execute(SmallShell *smash) {
  // First change group ID to prevent shell signals from being received.
  if (setpgrp() != 0) {
    syscallError("setpgrp");
  }

  // Check if complex external command or regular.
  if (command_line.find('*') != std::string::npos ||
      command_line.find('?') != std::string::npos) {
    if (execl("/bin/bash", "/bin/bash", "-c", command_line.c_str(), nullptr) !=
        0) {
      syscallError("execl");
      exit(1);
    };
  } else {
    if (execvp(argv[0], argv) != 0) {
      syscallError("execvp");
      exit(1);
    };
  }
}

//                                                                 //
//------------------------JobList functions------------------------//
//                                                                 //
std::ostream &operator<<(std::ostream &os, const JobsList::JobEntry &job) {
  auto now = time(nullptr);
  int delta = (int)difftime(now, job.command->getStartTime());

  os << "[" << job.id << "] " << job.command->getCommandLine() << " : "
     << job.pid << " " << delta << " secs"
     << (job.state == JobsList::JobState::Stopped ? " (stopped)" : "");

  return os;
}

void JobsList::addJob(std::shared_ptr<Command> cmd, pid_t pid, bool isStopped) {
  removeFinishedJobs();
  if (cmd->getJobId() == -1) {
    cmd->setJobId(getFreeID());
  }
  auto job = std::make_shared<JobEntry>(cmd, cmd->getJobId(), pid,
                                        isStopped ? JobState::Stopped
                                                  : JobState::Running);

  // Keep the list sorted.
  auto it = jobs.begin();
  while (it != jobs.end() && (*it)->id < job->id) {
    ++it;
  }

  jobs.insert(it, job);
}

void JobsList::printJobsList() {
  // TODO: mask alarm signal when travesing joblist.
  removeFinishedJobs();

  for (auto &&job : jobs) {
    std::cout << (*job) << std::endl;
  }
}

void JobsList::killAllJobs() {
  // TODO: mask alarm signal when travesing joblist.
  auto size = jobs.size();
  std::cout << "smash: sending SIGKILL signal to " << size
            << " jobs:" << std::endl;
  for (auto &&job : jobs) {
    if (kill(job->pid, SIGKILL) == -1) {
      syscallError("kill");
    } else {
      std::cout << job->pid << ": " << job->command->getCommandLine()
                << std::endl;
    }
    if (waitpid(job->pid, nullptr, 0) == -1) {
      syscallError("waitpid");
    }
  }
  jobs.clear();
}

void JobsList::removeFinishedJobs() {
  // TODO: mask alarm signal when travesing joblist.

  auto it = jobs.begin();
  while (it != jobs.end()) {
    auto job = *it;
    if (job->state == JobState::Killed) {
      auto current = it++;
      jobs.erase(current);

      continue;
    }

    int waitStatus;
    int res = waitpid(job->pid, &waitStatus, WNOHANG);

    if (res == -1 || (res > 0 && WIFEXITED(waitStatus))) {
      auto current = it++;
      jobs.erase(current);
    } else if (WIFSTOPPED(waitStatus)) {
      auto current = it++;
      current->get()->state = JobState::Stopped;
    } else if (WIFCONTINUED(waitStatus)) {
      auto current = it++;
      current->get()->state = JobState::Running;
    } else {
      ++it;
    }
  }
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
  // TODO: mask alarm signal when travesing joblist.
  for (auto &&job : jobs) {
    if (job->id == jobId) {
      return job.get();
    }
  }

  return nullptr;
}
JobsList::JobEntry *JobsList::getJobByPid(pid_t jobPid) {
  // TODO: mask alarm signal when travesing joblist.
  for (auto &&job : jobs) {
    if (job->pid == jobPid) {
      return job.get();
    }
  }

  return nullptr;
}
void JobsList::removeJobById(int jobId) {
  // TODO: mask alarm signal when travesing joblist.
  auto it = jobs.begin();
  while (it != jobs.end() && (*it)->id != jobId) {
    ++it;
  }

  if (it != jobs.end()) {
    jobs.erase(it);
  }
}

JobsList::JobEntry *JobsList::getLastJob() {
  // TODO: mask alarm signal when travesing joblist.

  if (jobs.empty()) {
    return nullptr;
  }

  auto lastJob = jobs.back();
  return lastJob.get();
}

JobsList::JobEntry *JobsList::getLastStoppedJob() {
  // TODO: mask alarm signal when travesing joblist.

  auto it = jobs.rbegin();
  while (it != jobs.rend() && (*it)->state != JobState::Stopped) {
    ++it;
  }

  if (it == jobs.rend()) {
    return nullptr;
  }

  return it->get();
}

int JobsList::getFreeID() const {
  // TODO: mask alarm signal when travesing joblist.

  if (jobs.empty()) {
    return 1;
  }

  return jobs.back()->id + 1;
}