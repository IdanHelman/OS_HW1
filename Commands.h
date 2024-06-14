#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <map>

#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

using namespace std;

class SmallShell;

class Command {
// TODO: Add your data members
protected:
    string cmd;
    string parsed_cmd;
    int argc;
    char** argv;

public:
    Command(const string& cmd_line, const string& cmd_no_sign);

    const string& getCmd();
    const string& getPCmd();
    virtual ~Command();

    virtual void execute(SmallShell *smash) = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const string& cmd_line, const string& cmd_no_sign): Command(cmd_line, cmd_no_sign) {}

    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const string& cmd_line, const string& cmd_no_sign): Command(cmd_line, cmd_no_sign) {}

    virtual ~ExternalCommand() {}

    void execute(SmallShell *smash) override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const string& cmd_line, const string& cmd_no_sign): Command(cmd_line, cmd_no_sign) {}

    virtual ~PipeCommand() {}

    void execute(SmallShell *smash) override;
};

class WatchCommand : public Command {
   // TODO: Add your data members
public:
   WatchCommand(const string& cmd_line, const string& cmd_no_sign): Command(cmd_line, cmd_no_sign) {}

   virtual ~WatchCommand() {}

   void execute(SmallShell *smash) override;

   static bool isValidCommand(const string& command);
};

class RedirectionCommand : public Command {
   // TODO: Add your data members
public:
    enum RedirectionType {Error, Overwrite, Append};
    explicit RedirectionCommand(const string& cmd_line, const string& cmd_no_sign): Command(cmd_line, cmd_no_sign) {}

    virtual ~RedirectionCommand() {}

    void execute(SmallShell *smash) override;

    RedirectionType getRedirectionType();

    string getChoppedCommand();

};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
    ChangeDirCommand(const string& cmd_line, const string& cmd_no_sign): BuiltInCommand(cmd_line, cmd_no_sign) { }

    virtual ~ChangeDirCommand() {}

    void execute(SmallShell *smash) override;
};

class ChangePromptCommand : public BuiltInCommand {
public:
    ChangePromptCommand(const string& cmd_line, const string& cmd_no_sign): BuiltInCommand(cmd_line, cmd_no_sign) { }

    virtual ~ChangePromptCommand() {}

    void execute(SmallShell *smash) override;

};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const string& cmd_line, const string& cmd_no_sign): BuiltInCommand(cmd_line, cmd_no_sign) { }

    virtual ~GetCurrDirCommand() {}

    void execute(SmallShell *smash) override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const string& cmd_line, const string& cmd_no_sign): BuiltInCommand(cmd_line, cmd_no_sign) { }

    virtual ~ShowPidCommand() {}

    void execute(SmallShell *smash) override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
    QuitCommand(const string& cmd_line, const string& cmd_no_sign, JobsList *jobs): BuiltInCommand(cmd_line, cmd_no_sign) { }

    virtual ~QuitCommand() {}

    void execute(SmallShell *smash) override;
};


class JobsList {
public:
    enum JobStatus{Running, Finished, Killed, Stopped};
    class JobEntry{
    public:
        int jobId;
        pid_t pid;
        shared_ptr<Command> commandPtr;
        JobStatus status;

        JobEntry(int jobId, pid_t pid, shared_ptr<Command> commandPtr, JobStatus status);
        ~JobEntry() = default;

        int& getJobId(){
            return jobId;
        }

        pid_t getpid(){
            return pid;
        }

        JobStatus& getStatus(){
            return status;
        }

        bool operator==(int jobId) {
            return this->jobId == jobId;
        }

        friend std::ostream& operator<<(std::ostream& os, const JobEntry& job);
    };

    vector<shared_ptr<JobEntry>> jobs;
    int maxId;

public:
    JobsList();

    ~JobsList() = default;

    int getSize(){
        return jobs.size();
    }

    void addJob(shared_ptr<Command> cmd, pid_t pid, bool isStopped = false);

    int getMaxId();

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    void updateJobsStatus();

    shared_ptr<JobsList::JobEntry> getJobById(int jobId);

    void removeJobById(int jobId);

    shared_ptr<JobsList::JobEntry> getLastJob();

    shared_ptr<JobsList::JobEntry> getLastStoppedJob(int *jobId);
    // TODO: Add extra methods or modify exisitng ones as needed
};

std::ostream& operator<<(std::ostream& os, const JobsList::JobEntry& job);

bool operator==(std::shared_ptr<JobsList::JobEntry> job, int jobId);

bool operator==(int jobId, std::shared_ptr<JobsList::JobEntry> job);

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsCommand(const string& cmd_line, const string& cmd_no_sign, JobsList *jobs): BuiltInCommand(cmd_line, cmd_no_sign) { }

    virtual ~JobsCommand() {}

    void execute(SmallShell *smash) override;
};

class KillCommand : public BuiltInCommand {
   // TODO: Add your data members
public:
   KillCommand(const string& cmd_line, const string& cmd_no_sign, JobsList *jobs): BuiltInCommand(cmd_line, cmd_no_sign) { }

   virtual ~KillCommand() {}

   void execute(SmallShell *smash) override;

   static int sigEditor(char* sig);
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ForegroundCommand(const string& cmd_line, const string& cmd_no_sign, JobsList *jobs): BuiltInCommand(cmd_line, cmd_no_sign) { }

    virtual ~ForegroundCommand() {}

    void execute(SmallShell *smash) override;
};

class ListDirCommand : public BuiltInCommand {
public:
    ListDirCommand(string cmd_line, const string& cmd_no_sign): BuiltInCommand(cmd_line, cmd_no_sign) { }

    virtual ~ListDirCommand() {}

    void execute(SmallShell *smash) override;
};

class GetUserCommand : public BuiltInCommand {
public:
   GetUserCommand(const string& cmd_line, const string& cmd_no_sign): BuiltInCommand(cmd_line, cmd_no_sign) { }

   virtual ~GetUserCommand() {}

   void execute(SmallShell *smash) override;

   int getUserId(string& pid);

   int getGroupId(string& pid);

   string getInfoFromText(string& text, string& info);
};

class aliasCommand : public BuiltInCommand {
public:
    aliasCommand(const string& cmd_line, const string& cmd_no_sign): BuiltInCommand(cmd_line, cmd_no_sign) {}

    virtual ~aliasCommand() {}

    void execute(SmallShell *smash) override;
};

class unaliasCommand : public BuiltInCommand {
public:
    unaliasCommand(const string& cmd_line, const string& cmd_no_sign): BuiltInCommand(cmd_line, cmd_no_sign) {}

    virtual ~unaliasCommand() {}

    void execute(SmallShell *smash) override;
};

class AliasesTable {
    map<string, string> aliases;

  //  vector<string> reservedWords;

    bool validFormat(const string& cmd);
public:

    AliasesTable() = default;

    ~AliasesTable() = default;

    void addAlias(const string& cmd);

    bool removeAlias(const string& alias);

    void printAliases();

    void replaceAlias(string& cmd_line);
};

class SmallShell {
private:
    // TODO: Add your data members
    SmallShell();
    JobsList jobs;
    string prompt;
    int pid;
    int runningPid;
    string pwd;
    string lastPwd;
    AliasesTable aliases;

public:
    std::shared_ptr<Command> CreateCommand(const char *cmd_line);
    string getPrompt() const;
    JobsList& getJobsList();
    int getPid() const;
    int getFgPid() const;
    void setFgPid(int newPid);
    AliasesTable& getAliases();
    string getPwd() const;
    string getLastPwd() const;
    void setPwd(const string& pwd);
    void setLastPwd(const string& lastPwd);
    void setPrompt(const string& prompt);
    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell();

    void executeCommand(const char *cmd_line);

    bool isRedirectionCommand(const string& cmd_line);

    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
