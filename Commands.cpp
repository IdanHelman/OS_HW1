#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <linux/limits.h>
#include <iomanip>
#include <fcntl.h>
#include "Commands.h"
#include <chrono>
#include <thread>
#include <pwd.h>
#include<grp.h>
#include <regex>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/syscall.h>


using namespace std;

//these are for handling ctrl_c and watch command
//sig_atomic_t stop_loop = 0;

//bool cameFromWatch = false;

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

bool isEmptyLine(const char* cmd_line){
    string str(cmd_line);
    return str.find_first_not_of(WHITESPACE) == string::npos;
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

SmallShell::SmallShell() :  jobs(JobsList()), prompt("smash"), pid(0), runningPid(-1), pwd(""), lastPwd(""), aliases(AliasesTable()), stop_loop(0), cameFromWatch(false), isInChild(false){
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

AliasesTable& SmallShell::getAliases() {
    return aliases;
}

bool SmallShell::isRedirectionCommand(const string& cmd_line){
    return cmd_line.find(">") != string::npos || cmd_line.find(">>") != string::npos;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
std::shared_ptr<Command> SmallShell::CreateCommand(const char *cmd_line, bool* isBackground) {
    // For example:

    char *cur_line = (char *) cmd_line;
    string cmd_s = (string(cur_line));
    string cmd_no_sign = _trim(cmd_s);
    aliases.replaceAlias(cmd_no_sign);
    *isBackground = *isBackground || _isBackgroundCommand(cmd_no_sign.c_str());
    if (cmd_no_sign.back() == '&') {
        cmd_no_sign.pop_back();
    }

    string firstWord = cmd_no_sign.substr(0, cmd_no_sign.find_first_of(" \n&"));

    //first check for alias
    if (firstWord.compare("alias") == 0) {
        return std::make_shared<aliasCommand>(cmd_s, cmd_no_sign);
    } else if (firstWord.compare("unalias") == 0) {
        return std::make_shared<unaliasCommand>(cmd_s, cmd_no_sign);
    }

    //check for watch
    if(firstWord.compare("watch") == 0){
        return std::make_shared<WatchCommand>(cmd_s, cmd_no_sign);
    }

    //check for redirection or pipe
    else if(isRedirectionCommand(cmd_no_sign)){
        return std::make_shared<RedirectionCommand>(cmd_s, cmd_no_sign);
    } else if(cmd_no_sign.find("|") != string::npos || cmd_no_sign.find("|&") != string::npos){
        return std::make_shared<PipeCommand>(cmd_s, cmd_no_sign);
    }

    //other commands
    else if (firstWord.compare("chprompt") == 0) {
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
   } else if (firstWord.compare("kill") == 0) {
       return std::make_shared<KillCommand>(cmd_s, cmd_no_sign, &jobs);
    }  else if (firstWord.compare("listdir") == 0){
        return std::make_shared<ListDirCommand>(cmd_s, cmd_no_sign);
    } else if (firstWord.compare("getuser") == 0){
       return std::make_shared<GetUserCommand>(cmd_s, cmd_no_sign);
    }
    else {
        return std::make_shared<ExternalCommand>(cmd_s, cmd_no_sign);
    }
}


void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    jobs.removeFinishedJobs();
     if(isEmptyLine(cmd_line)){
        return;
     }
     bool backGround = _isBackgroundCommand(cmd_line);
     std::shared_ptr<Command> cmd = CreateCommand(cmd_line, &backGround);
     backGround = backGround || _isBackgroundCommand(cmd->getPCmd().c_str());
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
             cmd->execute(this);
         }
         else{ //parent
             if(backGround){
                jobs.addJob(cmd, f_pid, false);
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
     //not external command - including redirection and pipe
     else{
        WatchCommand* watchCmd = dynamic_cast<WatchCommand *>(cmd.get());
        if(watchCmd != nullptr){
            watchCmd->executeWatch(this, cmd);
        }
        else{
            cmd->execute(this);
        }
        //cmd->execute(this);
    }
}



Command::Command(const string& cmd_line, const string& cmd_no_sign){
    cmd = cmd_line;
    parsed_cmd = cmd_no_sign;
    argv = new char*[COMMAND_MAX_ARGS + 1];
    argc = _parseCommandLine(parsed_cmd.c_str(), argv);
}

Command::~Command(){
    delete[] argv;
}

const string& Command::getCmd() {
    return this->cmd;
}

const string& Command::getPCmd() {
    return this->parsed_cmd;
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
    //removeFinishedJobs();
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

void AliasesTable::printAliases(){
    for(auto alias : insertedAliases){
        cout << alias << "=" << "'" << aliases[alias] << "'" << endl;
    }
}

bool AliasesTable::validFormat(const string &cmd) {
    if (cmd.find_first_of('=') == string::npos) {
        return false;
    }
    string key = cmd.substr(0, cmd.find_first_of('='));
    if (!all_of(key.begin(), key.end(), [](char c) { return isalnum(c) || c == '_'; })) {
        return false;
    }
    if (cmd.find_first_of('=') + 1 != cmd.find_first_of('\'') ||
        cmd[cmd.find_last_not_of(WHITESPACE)] != '\'') {
        return false;
    }
    return true;
}

static bool isReservedWord(const string &word){
    return word == "chprompt" || word == "showpid" || word == "pwd" || word == "cd" || word == "quit"
           || word == "jobs" || word == "fg" || word == "kill" || word == "listdir" || word == "getuser" || word == "watch"
           || word == "alias" || word == "unalias" || word == ">" || word == ">>" || word == "|" || word == "|&";
}

void AliasesTable::addAlias(const string &cmd){
    string cmd_no_space = cmd.substr(cmd.find_first_not_of(WHITESPACE));
    bool valid = validFormat(cmd_no_space);
    string key = cmd_no_space.substr(0, cmd_no_space.find_first_of('='));
    string value = cmd_no_space.substr(cmd_no_space.find_first_of('\'') + 1, cmd_no_space.find_last_of('\'') - cmd_no_space.find_first_of('\'') - 1);
    bool reserved_or_exist = aliases.find(key) != aliases.end() || isReservedWord(key);
    if(reserved_or_exist){
        cerr << "smash error: alias: " << key << " already exists or is a reserved command" << endl;
    }
    else if(!valid){
        cerr << "smash error: alias: invalid alias format" << endl;
    }
    else {
        aliases[key] = value;
        insertedAliases.push_back(key);
    }
}

bool AliasesTable::removeAlias(const string &alias){
    if(aliases.find(alias) == aliases.end()){
        return false;
    }
    else{
        aliases.erase(alias);
        auto it = find(insertedAliases.begin(), insertedAliases.end(), alias);
        insertedAliases.erase(it);
        return true;
    }
}

void AliasesTable::replaceAlias(string &cmd_line) {
    string firstWord = cmd_line.substr(0, cmd_line.find_first_of(" \n&"));
    if(aliases.find(firstWord) != aliases.end()){
        string alias = aliases[firstWord];
        cmd_line = alias + cmd_line.substr(firstWord.length());
    }
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
        exit(1);
    }
    //complex command
    else{
        char* bash_cmd[] = { (char*)"/bin/bash", (char*)"-c", (char*)parsed_cmd.c_str(), NULL};
        execv("/bin/bash", bash_cmd);
        perror("smash error: execv failed");
        exit(1);
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

int KillCommand::sigEditor(char* sig){
    //int sigNum;
    string sigStr = sig;
    if(sigStr[0] != '-'){ //no - sign at the beggining
        throw std::exception();
    }
    if(sigStr.size() == 1){
        throw std::exception();
    }
    sigStr = sigStr.substr(1);
    return stoi(sigStr); //maybe add another check
}

void KillCommand::execute(SmallShell *smash){
    if(argc < 3){
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    int sig, jobId;
    try{
        jobId = stoi(argv[2]);
    }
    catch(...){
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }

    JobsList::JobEntry* jobPtr = smash->getJobsList().getJobById(jobId).get();
    //can you kill a job that is finished?
    if(jobPtr == nullptr || jobPtr->status == JobsList::JobStatus::Killed || jobPtr->status == JobsList::JobStatus::Finished){
        cerr << "smash error: kill: job-id " << jobId << " does not exist" << endl;
        return;
    }
    try{
        sig = sigEditor(argv[1]);
    }
    catch(...){
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
//    if(!((1 <= sig) && (sig <= 31))){
//        cerr << "smash error: kill: invalid arguments" << endl;
//        return;
//    }
    if(argc > 3){
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }

    int ret = kill(jobPtr->pid, sig);
    if(ret == -1){
        perror("smash error: kill failed");
        return;
    }

    cout << "signal number " << sig << " was sent to pid " << jobPtr->pid << endl;

    if(sig == SIGKILL){
        jobPtr->status = JobsList::JobStatus::Killed;
        smash->getJobsList().removeJobById(jobId);
    }
    else if(sig == SIGCONT){
        jobPtr->status = JobsList::JobStatus::Running;
    }
    else if(sig == SIGSTOP){
        jobPtr->status = JobsList::JobStatus::Stopped;
    }
}

void ForegroundCommand::execute(SmallShell *smash) {
    shared_ptr<JobsList::JobEntry> jobPtr = nullptr;
    if(argc >= 2){
        int jobId;
        try{
            jobId = stoi(argv[1]);
        }
        catch(...){
            cerr << "smash error: fg: invalid arguments" << endl;
            return;
        }
        if (jobId < 0){
            cerr << "smash error: fg: invalid arguments" << endl;
            return;
        }
        jobPtr = smash->getJobsList().getJobById(jobId);
        if(jobPtr == nullptr){
            cerr << "smash error: fg: job-id " << jobId << " does not exist" << endl;
            return;
        }
        if(argc > 2){
            cerr << "smash error: fg: invalid arguments" << endl;
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
        return;
    }
    int ret = kill(jobPtr->pid, SIGCONT);
    if(ret == -1){
        perror("smash error: kill failed");
        return;
    }
    smash->getJobsList().removeJobById(jobPtr->jobId);
    cout << jobPtr->commandPtr->getCmd() << " " << jobPtr->pid << endl;
    int wstatus;
    smash->setFgPid(jobPtr->pid);
    if(waitpid(jobPtr->pid, &wstatus, 0) == -1){
        smash->setFgPid(-1);
        perror("smash error: waitpid failed");
    }
    smash->setFgPid(-1);
}

void ChangeDirCommand::execute(SmallShell *smash) {
    if(argc > 2){
        cerr << "smash error: cd: too many arguments" << endl;
    }
    else if(argc == 1){
        smash->setLastPwd(smash->getPwd());
        return;
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
            char new_pwd[PATH_MAX];
            getcwd(new_pwd, PATH_MAX);
            smash->setPwd(new_pwd);
        }
    }
}

void aliasCommand::execute(SmallShell *smash) {
    AliasesTable& aliases = smash->getAliases();
    if(argc == 1){
        aliases.printAliases();
    }
    else{
        string firstWord = argv[0];
        aliases.addAlias(cmd.substr(firstWord.length() + 1));
    }
}

void unaliasCommand::execute(SmallShell *smash) {
    AliasesTable& aliases = smash->getAliases();
    if(argc == 1){
        cerr << "smash error: unalias: not enough arguments" << endl;
    }
    else{
        for (int i = 1; i < argc; ++i) {
            if(!aliases.removeAlias(argv[i])){
                cerr << "smash error: unalias: " << argv[i] << " alias does not exist" << endl;
                break;
            }
        }
    }
}


//maybe change this
RedirectionCommand::RedirectionType RedirectionCommand::getRedirectionType(){
    char sign = '\0';
    /*for(int i = 0; i < argc; i++){
        if(strcmp(argv[i], ">") == 0){
            if(sign != '\0'){
                cerr << "smash error: redirection: multiple redirection signs" << endl; //maybe remove this
                return RedirectionType::Error;
            }
            sign = '>';
            //break;
        }
        else if(strcmp(argv[i], ">>") == 0){
            if(sign != '\0'){
                cerr << "smash error: redirection: multiple redirection signs" << endl; //maybe remove this
                return RedirectionType::Error;
            }
            sign = 'a'; //a is for >> sign
            //break;
        }
    }*/
    if(parsed_cmd.find(">>") != string::npos){
        sign = 'a';
    }
    else if(parsed_cmd.find('>') != string::npos){
        sign = '>';
    }
    if(sign == '>'){
        return RedirectionType::Overwrite;
    }
    else if(sign == 'a'){
        return RedirectionType::Append;
    }
    else{
        return RedirectionType::Error;
    }
}

string RedirectionCommand::getChoppedCommand(){
    size_t pos = parsed_cmd.find('>');
    return parsed_cmd.substr(0, pos);
}

string RedirectionCommand::getTextFile(){
    size_t pos = parsed_cmd.find_last_of('>');
    return parsed_cmd.substr(pos + 1, parsed_cmd.length() - pos);
}

void RedirectionCommand::execute(SmallShell *smash) {
    RedirectionType type = getRedirectionType();
    if(type == RedirectionType::Error){ //maybe?
        cout << "got here" << endl;
        cerr << "smash error: redirection: invalid arguments" << endl;
        return;
    }
    int fd;
    string textFile = getTextFile();
    if(type == RedirectionType::Overwrite){
        fd = open(_trim(textFile).c_str() , O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }
    else if(type == RedirectionType::Append){
        fd = open(_trim(textFile).c_str() , O_WRONLY | O_CREAT | O_APPEND, 0666);
    }
    if(fd == -1){
        perror("smash error: open failed");
        return;
    }

    pid_t f_pid = fork();
    if (f_pid == -1){
        perror("smash error: fork failed");
    }
    else if (f_pid == 0){ //child
        smash->isInChild = true;
        if(setpgrp() == -1){
            perror("smash error: setpgrp failed");
        }
        //changing output to fd
        close(STDOUT_FILENO);
        int dupRet = dup(fd);

        if(dupRet == -1){
            perror("smash error: dup2 failed");
            close(fd);
            exit(1);
        }

        //executing the command
        string choppedCmd = getChoppedCommand();
        bool isBackground = false;
        shared_ptr<Command> cmd = smash->CreateCommand(choppedCmd.c_str(), &isBackground);
        cmd->execute(smash);
        exit(1);
    }
    else{ //parent
        waitpid(f_pid, nullptr, 0); //waiting for child to finish
    }
    //changing output to fd
    /*int stdout = dup(STDOUT_FILENO);
    //close(STDOUT_FILENO);

    //dup(fd);
    int dupRet = dup2(fd, STDOUT_FILENO);
    if(dupRet == -1){
        perror("smash error: dup2 failed");
        close(fd);
        return;
    }

    //executing the command
    string choppedCmd = getChoppedCommand();
    smash->executeCommand(choppedCmd.c_str());

    //changing output back to STD_OUT
    dupRet = dup2(stdout, STDOUT_FILENO);
    if(dupRet == -1){
        perror("smash error: dup2 failed");
        close(fd);
        return;
    }*/
    close(fd);
}

bool WatchCommand::isValidCommand(const string& command){
    if(access(command.c_str(), F_OK)){
        return true;
    }
    return false;
}

void WatchCommand::execute(SmallShell *smash){
    return;
}

void WatchCommand::executeWatch(SmallShell *smash, shared_ptr<Command> tempPtr){
    if(argc < 2){ //?
        cerr << "smash error: watch: command not specified" << endl;
        return;
    }
    int secs = 2;
    int isDefault = false;
    try{
        secs = KillCommand::sigEditor(argv[1]); //checks that the interval is starting with a -, if not assumes default
    }
    catch(...){
        isDefault = true;
    }
    if(secs < 1){
        cerr << "smash error: watch: invalid interval" << endl;
        return;
    }
    string choppedCmd;
    if(isDefault){
        size_t pos = cmd.find("watch");
        choppedCmd = cmd.substr(pos + 5);
    }
    else{
        size_t pos = cmd.find(argv[1]) + strlen(argv[1]);
        choppedCmd = cmd.substr(pos);
    }
    if(choppedCmd == ""){
        cerr << "smash error: watch: command not specified" << endl;
        return;
    }
    bool backGround = _isBackgroundCommand(choppedCmd.c_str());
    if(backGround == false){ //weird behavior for ctrl-c in the middle of the command
        while(!SmallShell::getInstance().stop_loop){
            system("clear");
            SmallShell::getInstance().cameFromWatch = true;
            pid_t f_pid = fork();
            if (f_pid == -1){
                perror("smash error: fork failed");
            }
            else if(f_pid == 0){ //child
                if(setpgrp() == -1){
                    perror("smash error: setpgrp failed");
                }
                smash->executeCommand(choppedCmd.c_str());
                exit(0);
            }
            else{
                if(SmallShell::getInstance().cameFromWatch == false){
                    return;
                }
                sleep(secs);
            }
        }
    }
    else{
        choppedCmd.pop_back(); //removing & from the end of the command
        pid_t f_pid = fork();
        if (f_pid == -1){
             perror("smash error: fork failed");
        }
        else if(f_pid == 0){ //child
            if(setpgrp() == -1){
                perror("smash error: setpgrp failed");
            }
            int dev_null = open("/dev/null", O_WRONLY);
            if(dev_null == -1){
                perror("smash error: open failed");
                exit(EXIT_FAILURE);
            }
            int stdout_copy = dup(STDOUT_FILENO);
            if(stdout_copy == -1){
                perror("smash error: dup failed");
                exit(EXIT_FAILURE);
            }

            if(dup2(dev_null, STDOUT_FILENO) == -1){
                perror("smash error: dup2 failed");
                close(dev_null);
                exit(EXIT_FAILURE);
            }
            
            while(1){ //should this stop for some reason?
                pid_t f_pid2 = fork();
                if (f_pid2 == -1){
                    perror("smash error: fork failed");
                }
                else if(f_pid2 == 0){ //child
                    if(setpgrp() == -1){
                        perror("smash error: setpgrp failed");
                    }
                    smash->executeCommand(choppedCmd.c_str());
                    exit(0);
                }
                else{
                    sleep(secs);
                }
                smash->executeCommand(choppedCmd.c_str());
                sleep(secs);
            }
            //not getting here
            if (dup2(stdout_copy, STDOUT_FILENO) == -1) {
                perror("smash error: dup2 failed");
                exit(EXIT_FAILURE);
            }
            close(dev_null);
            close(stdout_copy);
        }
        else{ //parent - adding the watch command to jobs list
            smash->getJobsList().addJob(tempPtr, f_pid, false);
        }
    }
    SmallShell::getInstance().stop_loop = 0;
}


int GetUserCommand::getUserId(string& pid){
    //creating the file for getting the info
    int fd = open(("/proc/" + pid + "/status").c_str(), O_RDONLY);
    if(fd == -1){
        return -1;
    }
    char buffer[1024];
    int bytesRead = read(fd, buffer, 1024);
    if(bytesRead == -1){ //is this needed?
        close(fd);
        perror("smash error: read failed");
        return -1;
    }
    string fileContent(buffer);

    //finding the Uid
    size_t pos = fileContent.find("Uid:");
    if(pos == string::npos){
        close(fd);
        return -1;
    }
    string uidStr = "";
    pos += 5;
    while(isdigit(fileContent[pos])){
        uidStr += fileContent[pos];
        pos++;
    }

    //cout << "uid is " << uid << endl;

    //converting to int and returning
    int uid;
    try{
        uid = stoi(uidStr);
    }
    catch(...){
        return -1;
    }

    close(fd);
    return uid;
}

int GetUserCommand::getGroupId(string& pid){
    //creating the file for getting the info
    int fd = open(("/proc/" + pid + "/status").c_str(), O_RDONLY);
    if(fd == -1){
        return -1;
    }
    char buffer[1024];
    int bytesRead = read(fd, buffer, 1024);
    if(bytesRead == -1){ //is this needed?
        close(fd);
        perror("smash error: read failed");
        return -1;
    }
    string fileContent(buffer);

    //finding the Group Id
    size_t pos = fileContent.find("Gid:");
    if(pos == string::npos){
        close(fd);
        return -1;
    }
    string gidStr = "";
    pos += 5;
    while(isdigit(fileContent[pos])){
        gidStr += fileContent[pos];
        pos++;
    }

    //cout << "uid is " << uid << endl;
    //converting to a number and returning
    int gid;
    try{
        gid = stoi(gidStr);
    }
    catch(...){
        return -1;
    }

    close(fd);
    return gid;
}

void GetUserCommand::execute(SmallShell *smash) {
    if(argc != 2){
        cerr << "smash error: getuser: too many arguments" << endl;
        return;
    }
    string strPid = argv[1];
    int pid;
    try{
        pid = stoi(strPid);
    }
    catch(...){
        cerr << "smash error: getuser: process " << strPid << " does not exist" << endl;
        return;
    }
    //checks that pid actually exists
    if(kill(pid, 0) == -1){
        if(errno == ESRCH){
            cerr << "smash error: getuser: process " << strPid << " does not exist" << endl;
            return;
        }
    }
    int uid = getUserId(strPid);
    int gid = getGroupId(strPid);
    if(uid == -1 || gid == -1){ //is this good?
        cerr << "smash error: getuser: process " << strPid << " does not exist" << endl;
    }
    struct passwd* pwd = getpwuid(uid);
    struct group* grp = getgrgid(gid);
    if(pwd == nullptr){ //is this good?
        perror("smash error: getpwuid failed");
        return;
    }
    if(grp == nullptr){
        perror("smash error: getgrgid failed");
        return;
    }
    cout << "User: " << pwd->pw_name << endl << "Group: " << grp->gr_name << endl;
    return;
}

void PipeCommand::execute(SmallShell *smash) {
    bool error = parsed_cmd.find("|&") != string::npos;
    bool isBackground = false;
    shared_ptr<Command> firstCmd = smash->CreateCommand(parsed_cmd.substr(0, parsed_cmd.find("|")).c_str(), &isBackground);
    shared_ptr<Command> secondCmd = smash->CreateCommand(parsed_cmd.substr(parsed_cmd.find("|") + 1 + error).c_str(), &isBackground);

    int pipefd[2];
    if(pipe(pipefd) == -1){
        perror("smash error: pipe failed");
    }

    int write_pipe = error ? 2 : 1;

    pid_t firstChildPid, secondChildPid;

    if ((firstChildPid = fork()) == 0){
        if(setpgrp() == -1){
            perror("smash error: setpgrp failed");
        }
        dup2(pipefd[1], write_pipe);
        close(pipefd[1]);
        close(pipefd[0]);
        firstCmd->execute(smash);
        exit(0);
    }
    if(firstChildPid == -1){
        perror("smash error: fork failed");
        exit(1);
    }
    if ((secondChildPid = fork()) == 0){
        if(setpgrp() == -1){
            perror("smash error: setpgrp failed");
        }
        dup2(pipefd[0], 0);
        close(pipefd[0]);
        close(pipefd[1]);
        secondCmd->execute(smash);
        exit(0);
    }
    if(secondChildPid == -1){
        perror("smash error: fork failed");
        exit(1);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(firstChildPid, nullptr, 0);
    waitpid(secondChildPid, nullptr, 0);

//    string firstCmd = parsed_cmd.substr(0, parsed_cmd.find(" |"));
//    string secondCmd = parsed_cmd.substr(parsed_cmd.find(" |") + 3);
//    int pipefd[2];
//    if(pipe(pipefd) == -1){
//        perror("smash error: pipe failed");
//    }
//
//    //execute first command
//    int write_pipe = error ? 2 : 1;
//    int out = dup(write_pipe);
//    dup2(pipefd[1], write_pipe);
//    close(pipefd[1]);
//    smash->executeCommand(firstCmd.c_str());
//    dup2(out, write_pipe);
//    close(out);
//
//    //execute second command
//    int in = dup(0);
//    dup2(pipefd[0], 0);
//    close(pipefd[0]);
//    smash->executeCommand(secondCmd.c_str());
//    dup2(in, 0);
//    close(in);
}

void ListDirCommand::execute(SmallShell *smash) {
    if (argc > 2) {
        cerr << "smash error: listdir: too many arguments" << endl;
        return;
    }
    struct linux_dirent {
        unsigned long  d_ino;     /* Inode number */
        unsigned long  d_off;     /* Offset to next linux_dirent */
        unsigned short d_reclen;  /* Length of this linux_dirent */
        char           d_name[];  /* Filename (null-terminated) */
    };

    string path = ".";
    if (argc == 2) {
        path = argv[1];
    }

    int dir_fd = open(path.c_str(), O_RDONLY | O_DIRECTORY);
    if (dir_fd == -1) {
        perror("smash error: open failed");
        return;
    }

    vector<string> files;
    map<string, string> others;

    char buf[1024];  // Buffer for directory entries
    int nread;
    char entry_stat;
    int bpos;
    struct linux_dirent* entry;

    while ((nread = syscall(SYS_getdents, dir_fd, buf, 1024)) > 0) {
        for (bpos = 0; bpos < nread;) {
            entry = (struct linux_dirent*)(buf + bpos);
            string entry_name = entry->d_name;

            // Ignore "." and ".." and hidden files entries
            if (entry_name == "." || entry_name == ".."|| entry_name[0] == '.') {
                bpos += entry->d_reclen;
                continue;
            }
            entry_stat = *(buf + bpos + entry->d_reclen - 1);
            if (DT_REG == entry_stat) {
                files.push_back(entry->d_name);
            } else if (DT_DIR == entry_stat) {
                others[entry->d_name] = "directory";
            }
            bpos += entry->d_reclen;
        }
    }
    if (nread == -1) {
        perror("smash error: getdents failed");
    }

    sort(files.begin(), files.end());
    for (const auto& file : files) {
        cout << "file: " << file << endl;
    }
    for (const auto& other : others) {
        cout << other.second << ": " << other.first << endl;
    }
    close(dir_fd);
}
