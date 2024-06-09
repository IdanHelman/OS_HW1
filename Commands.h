#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <list>
#include <memory>
#include <string>
#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define MAX_ARGV_LENGTH (2 * COMMAND_MAX_ARGS + 5)

class SmallShell;

class Command {
protected:
  const std::string command_line;
  char **argv;
  const int argc;
  bool background_command_flag;
  time_t startTime;
  int jobId;
  // TODO: Add your data members
public:
  Command(const std::string &cmd_line, const std::string &cmd_line_stripped,
          bool background_command_flag);
  virtual ~Command();
  virtual void execute(SmallShell *smash) = 0;
  const std::string getCommandLine() const;
  const time_t &getStartTime() const;
  bool isBackgroundCommand() const;
  int getJobId() const;
  void setJobId(int id);
  // virtual void prepare();
  // virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
  BuiltInCommand(const std::string &cmd_line,
                 const std::string &cmd_line_stripped)
      : Command(cmd_line, cmd_line_stripped, false) {}
  virtual ~BuiltInCommand() {}
};

class ChangePromptCommand : public BuiltInCommand {
public:
  ChangePromptCommand(const std::string &cmd_line,
                      const std::string &cmd_line_stripped);
  virtual ~ChangePromptCommand() {}
  void execute(SmallShell *smash) override;
};

class ExternalCommand : public Command {
public:
  ExternalCommand(const std::string &cmd_line,
                  const std::string &cmd_line_stripped,
                  bool background_command_flag);
  virtual ~ExternalCommand() {}
  void execute(SmallShell *smash) override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
public:
  PipeCommand(const std::string &cmd_line,
              const std::string &cmd_line_stripped);
  virtual ~PipeCommand() {}
  void execute(SmallShell *smash) override;
};

class RedirectionCommand : public Command {
  // TODO: Add your data members
public:
  explicit RedirectionCommand(const std::string &cmd_line,
                              const std::string &cmd_line_stripped);
  virtual ~RedirectionCommand() {}
  void execute(SmallShell *smash) override;
  // void prepare() override;
  // void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
  ChangeDirCommand(const std::string &cmd_line,
                   const std::string &cmd_line_stripped);

  virtual ~ChangeDirCommand() {}

  void execute(SmallShell *smash) override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
  GetCurrDirCommand(const std::string &cmd_line,
                    const std::string &cmd_line_stripped);
  virtual ~GetCurrDirCommand() {}
  void execute(SmallShell *smash) override;
};

class ShowPidCommand : public BuiltInCommand {
public:
  ShowPidCommand(const std::string &cmd_line,
                 const std::string &cmd_line_stripped);
  virtual ~ShowPidCommand() {}
  void execute(SmallShell *smash) override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
  // TODO: Add your data members
public:
  QuitCommand(const std::string &cmd_line,
              const std::string &cmd_line_stripped);
  virtual ~QuitCommand() {}
  void execute(SmallShell *smash) override;
};

class JobsList {
public:
  enum class JobState { Running, Stopped, Killed };
  struct JobEntry {
    JobEntry(std::shared_ptr<Command> command, int id, pid_t pid,
             JobState state)
        : command(command), id(id), pid(pid), state(state) {}

    std::shared_ptr<Command> command;
    int id;
    pid_t pid;
    JobState state;

    friend std::ostream &operator<<(std::ostream &os, const JobEntry &job);
  };

public:
  void addJob(std::shared_ptr<Command> cmd, pid_t pid, bool isStopped);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry *getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry *getLastJob();
  JobEntry *getLastStoppedJob();
  JobEntry *getJobByPid(pid_t jobPid);

  // TODO: Add extra methods or modify exisitng ones as needed

private:
  int getFreeID() const;
  std::list<std::shared_ptr<JobEntry>> jobs;
};

class JobsCommand : public BuiltInCommand {
  // TODO: Add your data members
public:
  JobsCommand(const std::string &cmd_line,
              const std::string &cmd_line_stripped);
  virtual ~JobsCommand() {}
  void execute(SmallShell *smash) override;
};

class ForegroundCommand : public BuiltInCommand {
  // TODO: Add your data members
public:
  ForegroundCommand(const std::string &cmd_line,
                    const std::string &cmd_line_stripped);
  virtual ~ForegroundCommand() {}
  void execute(SmallShell *smash) override;
};

class BackgroundCommand : public BuiltInCommand {
public:
  BackgroundCommand(const std::string &cmd_line,
                    const std::string &cmd_line_stripped);
  virtual ~BackgroundCommand() {}
  void execute(SmallShell *smash) override;
};

class TimeoutCommand : public BuiltInCommand {
  /* Optional */
  // TODO: Add your data members
public:
  explicit TimeoutCommand(const std::string &cmd_line);
  virtual ~TimeoutCommand() {}
  void execute(SmallShell *smash) override;
};

class FareCommand : public BuiltInCommand {
public:
  FareCommand(const std::string &cmd_line,
              const std::string &cmd_line_stripped);
  virtual ~FareCommand() {}
  void execute(SmallShell *smash) override;
};

class SetcoreCommand : public BuiltInCommand {
public:
  SetcoreCommand(const std::string &cmd_line,
                 const std::string &cmd_line_stripped);
  virtual ~SetcoreCommand() {}
  void execute(SmallShell *smash) override;
};

class KillCommand : public BuiltInCommand {
  /* Bonus */
  // TODO: Add your data members
public:
  KillCommand(const std::string &cmd_line,
              const std::string &cmd_line_stripped);
  virtual ~KillCommand() {}
  int sigNumParser() const;
  void execute(SmallShell *smash) override;
};

class SmallShell {
private:
  enum class CommandType {
    Regular,
    Redirect,
    RedirectAppend,
    Pipe,
    PipeErr,
  };

private:
  const std::string default_display_prompt;
  const pid_t smash_pid;
  std::string current_display_prompt;
  std::string last_dir;
  bool is_working;
  JobsList jobs;
  Command *current_command = nullptr;
  pid_t current_command_pid = -1;

  SmallShell();

  CommandType checkType(const std::string &cmd_line) const;

public:
  std::shared_ptr<Command> CreateCommand(const std::string &cmd_line);
  void CreateRedirectCommand(const std::string &cmd_line,
                             std::shared_ptr<Command> &outCommand,
                             std::string &outFileName);
  void CreatePipeCommand(const std::string &cmd_line,
                         std::shared_ptr<Command> &outCommand1,
                         std::shared_ptr<Command> &outCommand2);
  SmallShell(SmallShell const &) = delete;     // disable copy ctor
  void operator=(SmallShell const &) = delete; // disable = operator
  static SmallShell &getInstance()             // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char *cmd_line);
  void setDisplayPrompt(std::string new_display_line);
  void setLastDir(const std::string &last_dir);

  const pid_t getPid() const;
  std::string getDisplayPrompt() const;
  const std::string &getLastDir() const;

  bool isSmashWorking() const;
  void disableSmash();
  void killAllJobs();

  void stopCurrentCommand();
  void killCurrentCommand();
  JobsList *getJobList();
  Command *getCurrentCommand() const;
  pid_t getCurrentCommandPid() const;
  void setCurrentCommandPid(pid_t pid);
  void setCurrentCommand(Command *command);
};

#endif // SMASH_COMMAND_H_
