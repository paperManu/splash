Contributing
============

Repositories
------------

The main repository for Splash is located on [Gitlab](https://gitlab.com/sat-metalab/splash). Another repository exists on [Github](https://github.com/papermanu/splash) on the account of the lead developer.


Coding style
------------

We use LLVM style, with a few exceptions. See the [clang-format configuration](./.clang-format) for more about this.

It is possible to let git ensure that you are conforming to the standards by using pre-commit hooks and clang-format:
```
sudo apt-get install clang-format exuberant-ctags
# Then in Splash's home folder:
rm -rf hooks && ln -s $(pwd)/.hooks $(pwd)/.git/hooks
```


Contributing
------------

Please send your pull request at the [SAT-Metalab's Gitlab repository](https://gitlab.com/sat-metalab/splash). If you do not know how to make a pull request, Gitlab provides some [help about collaborating on projects using issues and pull requests](https://docs.gitlab.com/ee/gitlab-basics/add-merge-request.html).


Branching strategy with git
---------------------------

The [master](https://gitlab.com/sat-metalab/splash/tree/master) branch contains Splash releases. Validated new developments are into the [develop](https://gitlab.com/sat-metalab/splash/tree/develop) branch.

Modifications are made into a dedicated branch that needs to be merged into the develop branch through a Gitlab merge request. When you modification is ready, you need to prepare your merge request as follow:

* Update your develop branch. 
```
git fetch
git checkout develop
git pull origin develop
```
* Go back to your branch and rebase onto develop. Note that it would be appreciated that you only merge request from a single commit (i.e interactive rebase with squash and/or fixup before pushing).
```
git checkout NAME_OF_YOUR_BRANCH
git rebase -i develop
```


Debugging
---------

As Splash runs multiple processes on a single computer (and hopefully someday on multiple computers), debugging its behavior can be a bit tricky. The main reason is that the rendering can be run in a subprocess executed when calling Splash from the command line. Therefore, when the software is invoked from `gdb`, `gdb` will not catch the issues in the subprocess.

A workaround is to run the subprocesses manually and to tell the main executable to communicate with these processes instead of spawning its own. We will use the default configuration as an example, located in `./data/splash.json` and installed in `${PREFIX}/share/splash/`

Splash processes communicate locally through IPC, and use a socket file to initiate the communication. By default this socket is named based on the main process ID, but it can be overriden from the command line. So first we will run the child process:

```bash
splash --child --prefix debug local
```

In this command line:
- `--child`: tell the process to run as a child
- `--prefix debug`: set the socket name prefix
- `local`: set the name of the scene to be handled by this process, as specified in the configuration file

We can now run the server:

```bash
splash --doNotSpawn --prefix debug ./data/splash.json
```

In this command line:
- `--spawnProcesses 0`: do not spawn any child process (it has been run manually)
- `--prefix debug`: the prefix must be the same as the child process, otherwise they will not be able to communicate
- `./data/splash.json`: the path to the file to run

Splash should then run as usual, except that you have total control over all executed processes. It is then possible to run both processes through `gdb`:

```bash
gdb --args splash --child --prefix debug local
gdb --args splash --doNotSpawn --prefix debug ./data/splash.json
```

Then run each process in `gdb` using the `run` command.


Adding tests
------------

Two types of test are used in this project:
- unit tests, meant to check that single functions work as designed, are written in C++ and use the [doctest](https://github.com/onqtam/doctest) unit test framework.
- integration tests, which test the overall behavior of Splash, are written in Python and use the [unittest](https://docs.python.org/3.7/library/unittest.html) module of Python.

Unit tests are evaluated in the CI whenever a new commit is sent to Gitlab. Integration tests on the other hand, can (sadly, for now) not be ran in the CI as they require an OpenGL 4.5 context. They should be run manually, at least before doing a new release.

To add a unit test, the steps are:
- add the source file for the test in `./tests/unit_tests/`, preferably keeping the same file structure as the source files in `./src` unless a very good reason is given
- fill in the source file with the test, see the other tests for reference
- add the source file to the source list in `./tests/CMakeLists.txt`, in the `target_sources` of the target `unitTests`
- call `make check` (after having built successfully Splash) to run all the unit tests.

To add an integration test, the steps are:
- add a source file for the test in `./tests/integration_tests/test_cases/`, named `test_TEST_SUBJECT.py`
- fill in the source file with a new class deriving from `SplashTestCase`. See the sample test file in `./tests/integration_tests/test_cases/test_sample.py` for reference.
- the source file will be automatically detected by the `unittest` module
- call `make check_integration` (after having built successfully Splash) to run all the integration tests.
