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

SmallShell::SmallShell() :  jobs(JobsList()), prompt("smash"), pid(0), runningPid(-1), pwd(""), lastPwd(""), aliases(AliasesTable()){
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
    return cmd_line.find(" > ") != string::npos || cmd_line.find(" >> ") != string::npos;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
std::shared_ptr<Command> SmallShell::CreateCommand(const char *cmd_line) {
    // For example:

    char *cur_line = (char *) cmd_line;
    string cmd_s = _trim(string(cur_line));
    string cmd_no_sign = cmd_s;
    if (cmd_s.back() == '&') {
        cmd_no_sign.pop_back();
    }
    aliases.replaceAlias(cmd_no_sign);

    if(isRedirectionCommand(cmd_no_sign)){
        return std::make_shared<RedirectionCommand>(cmd_s, cmd_no_sign);
    }

    string firstWord = cmd_no_sign.substr(0, cmd_no_sign.find_first_of(" \n&"));

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
   } else if (firstWord.compare("kill") == 0) {
       return std::make_shared<KillCommand>(cmd_s, cmd_no_sign, &jobs);
    } else if (firstWord.compare("alias") == 0) {
        return std::make_shared<aliasCommand>(cmd_s, cmd_no_sign);
    } else if (firstWord.compare("unalias") == 0) {
        return std::make_shared<unaliasCommand>(cmd_s, cmd_no_sign);
//    } else if (firstWord.compare("listdir") == 0){
//        return std::make_shared<ListDirCommand>(cmd_s, cmd_no_sign);
//    } else if (firstWord.compare("getuser")){
//        return std::make_shared<GetUserCommand>(cmd_s, cmd_no_sign);
    } else if(firstWord.compare("watch")){
        return std::make_shared<WatchCommand>(cmd_s, cmd_no_sign);
    }else {
    return std::make_shared<ExternalCommand>(cmd_s, cmd_no_sign);
    }
}


void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
     jobs.removeFinishedJobs();
     bool backGround = _isBackgroundCommand(cmd_line);
     std::shared_ptr<Command> cmd = CreateCommand(cmd_line);
     backGround = backGround || _isBackgroundCommand(cmd->getPCmd().c_str());
     bool isExternal = dynamic_cast<ExternalCommand *>(cmd.get()) != nullptr;
     bool isRedirection = dynamic_cast<RedirectionCommand *>(cmd.get()) != nullptr;
     if(isRedirection){
            cmd->execute(this);
     }
     else if (isExternal){
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
     else{
         cmd->execute(this);
    }
     //Please note that you must fork smash process for some commands (e.g., external commands....)
}

std::string removeSign(const std::string& str) {
    if (!str.empty() && str[str.find_last_not_of(" \t\n\r\f\v")] == '&') {
        return str.substr(0, str.find_last_of('&'));

    }
    return str;
}

Command::Command(const string& cmd_line, const string& cmd_no_sign){
    cmd = cmd_line;
    parsed_cmd = cmd_no_sign;
    argv = new char*[COMMAND_MAX_ARGS + 1];
    argc = _parseCommandLine(removeSign(parsed_cmd).c_str(), argv);
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
    for(auto alias : aliases){
        cout << alias.first << "=" << "'" << alias.second << "'" << endl;
    }
}

bool AliasesTable::validFormat(const string &cmd){
    if(cmd.find_first_of('=') == string::npos){
        return false;
    }
    string key = cmd.substr(0, cmd.find_first_of('='));
    if(!all_of(key.begin(), key.end(), [](char c){return isalnum(c) || c == '_';})){
        return false;
    }
    if(cmd.find_first_of('=') + 1 != cmd.find_first_of('\'') ||
                                    cmd[cmd.find_last_not_of(WHITESPACE)] != '\''){
        return false;
    }
    return true;
}

void AliasesTable::addAlias(const string &cmd){
    string cmd_no_space = cmd.substr(cmd.find_first_not_of(WHITESPACE));
    if (!validFormat(cmd_no_space)){
        cerr << "smash error: alias: Invalid alias format" << endl;
        return;
    }
    string key = cmd_no_space.substr(0, cmd_no_space.find_first_of('='));
    string value = cmd_no_space.substr(cmd_no_space.find_first_of('\'') + 1, cmd_no_space.find_last_of('\'') - cmd_no_space.find_first_of('\'') - 1);
    // TODO: check if key is a reserved word or built in command in shell
    if(aliases.find(key) != aliases.end()){
        cerr << "smash error: alias: " << key << " already exists or is a reserved command" << endl;
    }
    else {
        aliases[key] = value;
    }
}

bool AliasesTable::removeAlias(const string &alias){
    if(aliases.find(alias) == aliases.end()){
        return false;
    }
    else{
        aliases.erase(alias);
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
    if(argc != 3){
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    int sig, jobId;
    try{
        sig = sigEditor(argv[1]);
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

    if(!((1 <= sig) && (sig <= 31))){
        cerr << "smash error: kill: job-id " << jobId << " does not exist" << endl;
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
    if(argc == 2){
        int jobId;
        try{
            jobId = stoi(argv[1]);
        }
        catch(...){
            cerr << "smash error: fg: invalid arguments" << endl;
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
        cerr << "smash error: alias: Not enough arguments" << endl;
    }
    else{
        for (int i = 1; i < argc; ++i) {
            if(!aliases.removeAlias(argv[i])){
                cerr << "smash error: alias: " << argv[i] << " alias does not exist" << endl;
                break;
            }
        }
    }
}


//maybe change this
RedirectionCommand::RedirectionType RedirectionCommand::getRedirectionType(){
    char sign = '\0';
    for(int i = 0; i < argc; i++){
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

void RedirectionCommand::execute(SmallShell *smash) {
    RedirectionType type = getRedirectionType();
    if(type == RedirectionType::Error){ //maybe?
        cout << "got here" << endl;
        cerr << "smash error: redirection: invalid arguments" << endl;
        return;
    }
    int fd;
    if(type == RedirectionType::Overwrite){
        fd = open(argv[argc - 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }
    else if(type == RedirectionType::Append){
        fd = open(argv[argc - 1], O_WRONLY | O_CREAT | O_APPEND, 0666);
    }
    if(fd == -1){
        perror("smash error: open failed");
        return;
    }
    //changing output to fd
    int stdout = dup(STDOUT_FILENO);
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
    }
    close(fd);
}

bool WatchCommand::isValidCommand(const string& command){
    if(access(command.c_str(), F_OK)){
        return true;
    }
    return false;
}

void WatchCommand::execute(SmallShell *smash){
    if(argc < 2){
        cerr << "smash error: watch: invalid arguments" << endl;
        return;
    }
    int secs = 2;
    int isDefault = false;
    try{
        secs = stoi(argv[1]);
    }
    catch(...){
        isDefault = true;
    }
    if(secs < 1){
        cerr << "smash error: watch: invalid arguments" << endl;
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
    Command* cmd = smash->CreateCommand(choppedCmd.c_str()).get();
}

int GetUserCommand::getUserId(string& pid){
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
    size_t pos = fileContent.find("Uid:");
    if(pos == string::npos){
        close(fd);
        return -1;
    }
    
}

int GetUserCommand::getGroupId(string& pid){

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
    
}