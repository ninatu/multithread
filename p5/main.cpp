#include <iostream>
#include <string>
#include <set>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "parser.h"

// дескрипторы для хранения stdin и stdout
int fd_stdout, fd_stdin;
// pids дочерних процессов
std::set<pid_t> pids;
// pid шелла
pid_t shell_pid;
// pid процесса порождаюшего процессы
pid_t main_pid;
// индикатор активности процесса(был ли послан SIGINT)
bool active_process;

void log(pid_t pid, int status);
void forward_input(Comand *cmd, int *fdpipe);
void forward_output(Comand *cmd, int *fdpipe);
void run(const std::vector<Entity *> &cmds);
void handler(int signum);


int main(int argc, char *argv[])
{
    signal(SIGINT, &handler);
    shell_pid = main_pid = getpid();
    fd_stdout = dup(STDOUT_FILENO);
    fd_stdin = dup(STDIN_FILENO);
    fcntl(fd_stdout, F_SETFD, 1);
    fcntl(fd_stdin, F_SETFD, 1);
    pid_t pid;
    int status;

    Parser parser = Parser();
    while (true) {
        //ожидаем завершения фоновых процессов
        while((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            log(pid, status);
            pids.erase(pid);
        }
        dup2(fd_stdin, STDIN_FILENO);
        // делаем процесс активным
        active_process = true;
        pids.clear();
        //считаем последовательность команд
        std::string line = "";
        char ch = std::cin.get();
        while (!std::cin.eof() && ch != '\n') {
            line += ch;
            ch = std::cin.get();
        }
        // парсим
        parser.parse(line);
        // выполняем
        if (active_process) {
            // проверяем нужно ли запускать в фоном режиме
            if (parser.have_bkgr_mode()) {
                if ((pid = fork()) == 0) {
                    main_pid = getpid();
                    run(parser.get_cmds());
                    return 0;
                } else {
                    pids.insert(pid);
                    std::cerr << "Spawned child process " << pid <<  std::endl;
                }
            } else {
                run(parser.get_cmds());
            }
        }
        if (std::cin.eof())
            break;
    }
    while((pid = waitpid(-1, &status, 0)) >0) {
        log(pid, status);
        pids.erase(pid);
    }
    return 0;
}


void
handler(int signum)
{
    signal(signum, &handler);
   // std::cerr << "OK " << std::endl;
    if (signum == SIGINT) {
        if (getpid() == main_pid) {
            for (auto iter = pids.begin(); iter != pids.end(); iter++) {
                kill(*iter, SIGINT);
            }
            active_process = false;
        }
        if (getpid() != shell_pid)
            exit(1);
    }
}

//логирование
void
log(pid_t pid, int status)
{
    int code;
    if (WIFEXITED(status))
        code = WEXITSTATUS(status);
    else
        code = WIFEXITED(status);
    std::cerr << "Process " <<pid << " exited: " << code << std::endl;
}

void
forward_input(
        Comand *cmd,
        int* fdpipe)
{
    if (cmd->infile == "pipe") {
        fcntl(fdpipe[0], F_SETFD, 0);
        dup2(fdpipe[0], STDIN_FILENO);
        close(fdpipe[0]);
    } else if (cmd->infile != ""){
        int fd = open(cmd->infile.c_str(), O_RDONLY);
        dup2(fd, STDIN_FILENO);
        close(fd);
    } else {
        dup2(fd_stdin, STDIN_FILENO);
    }
}

void
forward_output(
        Comand *cmd,
        int* fdpipe)
{
    if (cmd->outfile == "pipe") {
        pipe(fdpipe);
        fcntl(fdpipe[1], F_SETFD, 0);
        fcntl(fdpipe[0], F_SETFD, 1);
        dup2(fdpipe[1], STDOUT_FILENO);
        close(fdpipe[1]);
    } else if (cmd->outfile != ""){
        int fd = open(cmd->outfile.c_str(), O_WRONLY|O_CREAT|O_APPEND|O_TRUNC, 0666);
        dup2(fd, STDOUT_FILENO);
        close(fd);
    } else {
        dup2(fd_stdout, STDOUT_FILENO);
    }
}

void
run(const std::vector<Entity *> &cmds)
{
    pid_t cur_pid;
    int cur_status;
    int fdpipe[2];
    int cnt_proc = 0;
    std::string prev_op("");
    for (auto iter = cmds.begin(); iter != cmds.end(); iter++) {
        // обработка ленивого || и &&
        if (!active_process)
            return;
        if ((prev_op == "||" && (WIFEXITED(cur_status) &&  !WEXITSTATUS(cur_status)))
                || (prev_op == "&&" && (!WIFEXITED(cur_status) || WEXITSTATUS(cur_status)))) {
            iter++;
            if (iter == cmds.end())
                return;
            Delimeter *del = dynamic_cast<Delimeter *>(*iter);
            prev_op = del->delim;
        } else {
            dup2(fd_stdin, STDIN_FILENO); // занимаем эти дискрипторы, чтобы потом избежать конфликтов
            dup2(fd_stdout, STDOUT_FILENO);
            Comand *cmd = dynamic_cast<Comand *>(*iter);
            // перенаправление ввода
            forward_input(cmd, fdpipe);
            // перенаправление вывода
            forward_output(cmd, fdpipe);
            // выполнение команды
            if ((cur_pid = fork()) == 0) {
                execvp(cmd->cmd, cmd->params);
                return;

            } else {
                pids.insert(cur_pid);
                close(STDIN_FILENO);
                close(STDOUT_FILENO);
                cnt_proc++;
                iter++;
                Delimeter *del;
                if (iter != cmds.end())
                    del = dynamic_cast<Delimeter *>(*iter);
                if (iter == cmds.end() || del->delim == "||" || del->delim == "&&" || del->delim == ";") {
                    pid_t pid; int status;
                    for(int i = 0; i < cnt_proc; i++) {
                        pid = wait(&status);                        
                        log(pid, status);
                        pids.erase(pid);
                        if (pid == cur_pid)
                            cur_status = status; // сохраняем exit код всего выражение(на случай нескольких |)
                    }
                    if (iter == cmds.end())
                        return;
                    cnt_proc = 0;
                }
                prev_op = del->delim;
            }
        }
    }
}




