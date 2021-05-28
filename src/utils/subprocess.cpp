#include "./utils/subprocess.h"

#include "./utils/scope_guard.h"

#include <signal.h>
#include <spawn.h>
#include <wait.h>
#include <wordexp.h>

namespace Splash::Utils
{
/**************/
Subprocess::Subprocess(const std::string& command, const std::string& args, const std::string& env)
{
    // Setting the process group to 0 is necessary to be able
    // to terminate processes launched by a bash script, when
    // the command is a bash script
    posix_spawnattr_init(&_spawnAttr);
    posix_spawnattr_setpgroup(&_spawnAttr, 0);
    posix_spawnattr_setflags(&_spawnAttr, POSIX_SPAWN_SETPGROUP);

    const auto allArgs = command + " " + args;

    wordexp_t argsAsWords;
    OnScopeExit { wordfree(&argsAsWords); };
    if (wordexp(allArgs.c_str(), &argsAsWords, 0) != 0)
    {
        _success = false;
        return;
    }

    wordexp_t envAsWords;
    OnScopeExit { wordfree(&envAsWords); };
    if (wordexp(env.c_str(), &envAsWords, 0) != 0)
    {
        _success = false;
        return;
    }

    const int status = posix_spawnp(&_pid, command.c_str(), nullptr, &_spawnAttr, argsAsWords.we_wordv, nullptr);

    if (status != 0)
        _success = false;
    else
        _success = true;
}

/**************/
Subprocess::~Subprocess()
{
    if (isRunning())
    {
        killpg(_pid, SIGTERM);
        posix_spawnattr_destroy(&_spawnAttr);
    }
}

/**************/
std::optional<int> Subprocess::getPID() const
{
    if (!_success)
        return {};
    return _pid;
}

/**************/
bool Subprocess::isRunning()
{
    if (!_success || _pid == 0)
        return false;

    const auto success = kill(_pid, 0);
    if (success == 0)
    {
        int wstatus;
        const auto return_pid = waitpid(_pid, &wstatus, WNOHANG);
        if (return_pid == -1)
            return true;

        if (return_pid == _pid)
        {
            _pid = 0;
            return false;
        }

        return true;
    }

    return false;
}
} // namespace Splash
