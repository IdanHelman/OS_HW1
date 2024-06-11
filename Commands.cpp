#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <linux/limits.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundCommand(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() :  jobs(JobsList()), prompt("smash"), pid(0), runningPid(-1), pwd(""), lastPwd(""){
    pid = getpid();
    if (pid == -1){
        perror("smash error: getpid failed");
    }
    char cpwd[PATH_MAX];
    pwd = getcwd(cpwd, PATH_MAX);
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

int SmallShell::getPid() const {
    return pid;
}

int SmallShell::getFgPid() const {
    return runningPid;
}

void SmallShell::setFgPid(int newPid){
    runningPid = newPid;
}

string SmallShell::getPrompt() const {
    return prompt;
}

JobsList& SmallShell::getJobsList() {
    return jobs;
}

string SmallShell::getPwd() const {
    return pwd;
}

string SmallShell::getLastPwd() const {
    return lastPwd;
}

void SmallShell::setPwd(const string &new_pwd) {
    this->pwd = new_pwd;
}

void SmallShell::setLastPwd(const string &new_pwd) {
    this->lastPwd = new_pwd;
}

void SmallShell::setPrompt(const string &new_prompt) {
    this->prompt = new_prompt;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
std::shared_ptr<Command> SmallShell::CreateCommand(const char *cmd_line) {
    // For example:
    char *cur_line = (char *) cmd_line;
    string cmd_s = _trim(string(cur_line));
    _removeBackgroundSign(cur_line);
    string cmd_no_sign = _trim(string(cur_line));

    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n&"));


    if (firstWord.compare("chprompt") == 0) {
        return std::make_shared<ChangePromptCommand>(cmd_s, cmd_no_sign);
    } else if (firstWord.compare("showpid") == 0) {
        return std::make_shared<ShowPidCommand>(cmd_s, cmd_no_sign);
    } else if (firstWord.compare("pwd") == 0) {
        return std::make_shared<GetCurrDirCommand>(cmd_s, cmd_no_sign);
    } else if (firstWord.compare("cd") == 0) {
        return std::make_shared<ChangeDirCommand>(cmd_s, cmd_no_sign);
    } else if (firstWord.compare("quit") == 0) {
        return std::make_shared<QuitCommand>(cmd_s, cmd_no_sign, &jobs);
    } else if (firstWord.compare("jobs") == 0) {
        return std::make_shared<JobsCommand>(cmd_s, cmd_no_sign, &jobs);
    } else if (firstWord.compare("fg") == 0) {
        return std::make_shared<ForegroundCommand>(cmd_s, cmd_no_sign, &jobs);
//    } else if (firstWord.compare("kill") == 0) {
//        return std::make_shared<KillCommand>(cmd_s, cmd_no_sign, &jobs);
//    } else if (firstWord.compare("alias") == 0) {
//        return std::make_shared<aliasCommand>(cmd_s, cmd_no_sign);
//    } else if (firstWord.compare("unalias") == 0) {
//        return std::make_shared<unaliasCommand>(cmd_s, cmd_no_sign);
    } else {
    return std::make_shared<ExternalCommand>(cmd_s, cmd_no_sign);
    }
}


void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
     bool backGround = _isBackgroundCommand(cmd_line);
     std::shared_ptr<Command> cmd = CreateCommand(cmd_line);
     bool isExternal = dynamic_cast<ExternalCommand *>(cmd.get()) != nullptr;
     if (isExternal){
         pid_t f_pid = fork();
         if (f_pid == -1){
             perror("smash error: fork failed");
         }
         else if (f_pid == 0){ //child
             if(setpgrp() == -1){
                 perror("smash error: setpgrp failed");
             }
             jobs.removeFinishedJobs();
             cmd->execute(this);
         }
         else{ //parent
             if(_isBackgroundCommand(cmd_line)){
                 jobs.addJob(cmd, getpid(), false);
             }
             else{
                 runningPid = f_pid;
                 int status;
                 if(waitpid(f_pid, &status, WUNTRACED) == -1){
                     perror("smash error: waitpid failed");
                 }
                 if (WIFSTOPPED(status)){
                     jobs.addJob(cmd, pid, true);
                 }
                 runningPid = -1;
             }
         }
     }
     else{
         jobs.removeFinishedJobs();
        cmd->execute(this);
    }
     //Please note that you must fork smash process for some commands (e.g., external commands....)
}

Command::Command(const string& cmd_line, const string& cmd_no_sign){
    cmd = cmd_line;
    parsed_cmd = cmd_no_sign;
    argv = new char*[COMMAND_MAX_ARGS + 1];
    argc = _parseCommandLine(cmd_no_sign.c_str(), argv);
}

Command::~Command(){
    delete[] argv;
}

const string& Command::getCmd() {
    return this->cmd;
}

JobsList::JobEntry::JobEntry(int jobId, pid_t pid, shared_ptr<Command> commandPtr, JobStatus status): jobId(jobId), pid(pid),
commandPtr(commandPtr), status(status) {}

JobsList::JobsList(): jobs(), maxId(0) {}

void JobsList::addJob(shared_ptr<Command> cmd, pid_t pid, bool isStopped){
    int jobId = getMaxId() + 1;
    //JobEntry newJob(jobId, pid, );
    jobs.push_back(make_shared<JobEntry>(jobId, pid, cmd, isStopped ? JobStatus::Stopped : JobStatus::Running));
}

//make sure that we are using non parsed string
std::ostream& operator<<(std::ostream& os, const JobsList::JobEntry& job){
    os << "[" << job.jobId << "] " << job.commandPtr->getCmd();
    return os;
}

bool operator==(std::shared_ptr<JobsList::JobEntry> job, int jobId){
    return job->jobId == jobId;
}

bool operator==(int jobId, std::shared_ptr<JobsList::JobEntry> job){
    return job == jobId;
}

void JobsList::printJobsList(){
    for(auto job: jobs){
        cout << *(job) << endl;
    }
}

int JobsList::getMaxId() {
    int max = 0;
    for(auto job : jobs){
        max = job->jobId > max ? job->jobId : max;
    }
    return max;
}

shared_ptr<JobsList::JobEntry> JobsList::getJobById(int jobId){
    auto it = std::find(jobs.begin(), jobs.end(), jobId);
    if(it != jobs.end()){
        return *it;
    }
    else{
        return nullptr;
    }
}

void JobsList::removeJobById(int jobId){
    auto it = std::find(jobs.begin(), jobs.end(), jobId);
    if(it != jobs.end()){
         jobs.erase(it);
    }
}

void JobsList::updateJobsStatus(){
    auto it = jobs.begin();
    while(it != jobs.end()){
        auto job = *it;
        if(job->status == JobStatus::Finished || job->status == JobStatus::Killed){

        }
        else{
            int wstatus;
            int res;
            res = waitpid(job->pid , &wstatus, WNOHANG);

            if((res == -1) || (res > 0 && WIFEXITED(wstatus))){
                job->status = JobStatus::Finished;
            }
            else if(WIFSTOPPED(wstatus)) {
                job->status = JobStatus::Stopped;
            }
            else if(WIFCONTINUED(wstatus)) {
                job->status = JobStatus::Running;
            }
        }
        it++;
    }
}

void JobsList::removeFinishedJobs(){
    updateJobsStatus();
    auto it = jobs.begin();
    while(it != jobs.end()){
        auto job = *it;
        if(job->status == JobStatus::Finished || job->status == JobStatus::Killed){
            it = jobs.erase(it);
        }
        else{
            it++;
        }
    }
}

//add printing
void JobsList::killAllJobs(){
    removeFinishedJobs();
    cout << "smash: sending SIGKILL signal to " << jobs.size() << " jobs:" << endl;
    //auto it = jobs.begin();
    for(auto jobPtr : jobs){
        int ret = kill(jobPtr->pid, SIGKILL);
        if(ret == -1){
            perror("smash error: kill failed");
        }
        else{
            cout << jobPtr->pid << ": " << jobPtr->commandPtr->getCmd() << endl;
            jobPtr->status = JobStatus::Killed;
        } //add waitpid
    }
    jobs.clear();
}

shared_ptr<JobsList::JobEntry> JobsList::getLastJob(){
    if(jobs.empty()){
        return nullptr;
    }
    return jobs.back();
}

shared_ptr<JobsList::JobEntry> JobsList::getLastStoppedJob(int *jobId){
    for(auto rit = jobs.rbegin(); rit != jobs.rend(); rit++){
        auto job = *rit;
        if(job->status == JobStatus::Stopped){
            return job;
        }
    }
    return nullptr;
}



void ChangePromptCommand::execute(SmallShell *smash) {
    if (argc == 1){
        smash->setPrompt("smash");
    }
    else{
        smash->setPrompt(argv[1]);
    }
}

void ShowPidCommand::execute(SmallShell *smash) {
    cout << "smash pid is " << smash->getPid() << endl;
}

void ExternalCommand::execute(SmallShell *smash) {
    //simple command
    if (parsed_cmd.find('?') == string::npos && parsed_cmd.find('*') == string::npos){
        execvp(argv[0], argv);
        perror("smash error: execvp failed");
    }
    //complex command
    else{
        char* bash_cmd[] = { (char*)"/bin/bash", (char*)"-c", (char*)parsed_cmd.c_str(), NULL};
        execv("/bin/bash", bash_cmd);
        perror("smash error: execv failed");
    }
}

void QuitCommand::execute(SmallShell *smash) {
    if(argc >= 2 && strcmp(argv[1], "kill") == 0){
        smash->getJobsList().killAllJobs();
    }
    exit(0);
}

void GetCurrDirCommand::execute(SmallShell *smash) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, PATH_MAX) == NULL){
        perror("smash error: getcwd failed");
    }
    else{
        cout << cwd << endl;
    }
}

void JobsCommand::execute(SmallShell *smash) {
    smash->getJobsList().printJobsList();
}

void ForegroundCommand::execute(SmallShell *smash) {
    shared_ptr<JobsList::JobEntry> jobPtr = nullptr;
    if(argc == 2){
        int jobId;
        try{
            jobId = stoi(argv[1]);
        }
        catch(...){
            cout << "smash error: fg: invalid arguments" << endl;
            return;
        }
        jobPtr = smash->getJobsList().getJobById(jobId);
        if(jobPtr == nullptr){
            cerr << "smash error: fg: job-id " << jobId << " does not exist" << endl;
            return;
        }
    }
    else if(argc == 1 ){
        if(smash->getJobsList().getSize() == 0){
            cerr << "smash error: fg: jobs list is empty" << endl;
            return;
        }
        jobPtr = smash->getJobsList().getLastJob();
    }
    else {
        cerr << "smash error: fg: invalid arguments" << endl;
    }
    int ret = kill(jobPtr->pid, SIGCONT);
    if(ret == -1){
        perror("smash error: kill failed");
        return;
    }
    smash->getJobsList().removeJobById(jobPtr->jobId);
    cout << jobPtr->commandPtr->getCmd() << jobPtr->pid << endl;
    int wstatus;
    if(waitpid(jobPtr->pid, &wstatus, 0) == -1){
        perror("smash error: waitpid failed");
    }
}

void ChangeDirCommand::execute(SmallShell *smash) {
    if(argc > 2){
        cerr << "smash error: cd: too many arguments" << endl;
    }
    else if (strcmp(argv[1], "-") == 0){
        if (smash->getLastPwd().empty()){
            cerr << "smash error: cd: OLDPWD not set" << endl;
        }
        else{
            if(chdir(smash->getLastPwd().c_str()) == -1){
                perror("smash error: chdir failed");
            }
            else{
                string last = smash->getLastPwd();
                smash->setLastPwd(smash->getPwd());
                smash->setPwd(last);
            }
        }
    }
    else{
        if(chdir(argv[1]) == -1){
            perror("smash error: chdir failed");
        }
        else{
            smash->setLastPwd(smash->getPwd());
            smash->setPwd(argv[1]);
        }
    }
}