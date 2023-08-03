Contributing
============

Welcome! We'd love for you to contribute to Splash's code and make it an even more incredible tool for immersive visual experience!

Splash is currently being maintained by a small team of developers working for the [Société des arts technologiques (SAT)](https://sat.qc.ca/), a non-profit art centre based in Montréal, Canada for which this software was originally developed. This core team adds features and bug fixes according to the SAT's interests and needs; however, community contributions on this project are more than welcome! There are many ways to contribute, including submitting bug reports, improving documentation, adding unit tests, adding support for a new language, submitting feature requests, reviewing new submissions, or contributing code that can be incorporated into the project.

This document describes this project's development process. Please do your best to follow these guidelines, as doing so will ensure a better contributing experience for you, and for other contributors and maintainers of this project.


## Code of Conduct

By participating in this project, you agree to abide by the Splash [Code of Conduct](Code_of_conduct.md). We expect all contributors to follow the [Code of Conduct](Code_of_conduct.md) and to treat fellow humans with respect. It must be followed in all your interactions with the project.


## Important Resources

* [README](index.md)
* [Issue tracker](https://gitlab.com/splashmapper/splash/-/issues)


## Contributing

Contributing to Splash is achieved through Gitlab's Merge Request (MR) system. This include contribution by the core team. We also welcome external contributions through the contribution process described here.

Please send your merge request to [Splash repository](https://gitlab.com/splashmapper/splash/-/merge_requests). Merge requests must refer to a gitlab issue in the [Splash issues list](https://gitlab.com/splashmapper/splash/-/issues). If your merge request does not refer to a gitlab issue, you may be asked to fill an issue before a decision is made. For more technical details regarding MRs, see the section about the [Merge Request Process](#merge-request-process).

Before you submit your issue, please [search the issue tracker](https://gitlab.com/splashmapper/splash/-/issues) - maybe your question or issue has already been identified or addressed.

Splash has several issue types:

- Default issue: for all issues that do not match with following issue types.
- Bug report: inform of a newly discovered bug.
- Feature request: ask for a new feature.
- Request For Comment: propose a significant change , such as code refactoring, CI deployment, or any change in the repository that impacts the Splash community. See a more detailled description of the [RFC process](./RFC.md).

The preferred way of asking question to us is through opening a default issue on [Splash repository](https://gitlab.com/splashmapper/splash/-/issues). We'll get back to you as soon as possible!

**If you find a security vulnerability, do NOT open an issue. Email metalab-dev@sat.qc.ca instead.**


## Tests

### Adding tests

You are welcome to add unit tests if you spot some code that isn't covered by the existing tests.

We do not expect you to add unit tests when doing only minor changes to the codebase. We expect, however, the tests to pass when you're finished with your work.

When adding a new feature or doing major changes to the code, we expect you to add the relevant unit tests to the code.

In any case, please run the tests on your computer first and ensure that they pass before pushing them to the repository. The CI pipeline will spot any broken tests, but we prefer that you make the necessary verifications beforehand rather than making the CI fail needlessly.

Two types of test are used in this project:
- unit tests, meant to check that single functions work as designed, are written in C++ and use the [doctest](https://github.com/onqtam/doctest) unit test framework.
- integration tests, which test the overall behavior of Splash, are written in Python and use the [unittest](https://docs.python.org/3.7/library/unittest.html) module of Python.

Unit tests are evaluated in the CI whenever a new commit is sent to Gitlab. Integration tests on the other hand, can (sadly, for now) not be ran in the CI as they require an OpenGL 4.5 context. They should be ran manually, at least before doing a new release.

### Running Tests

To run the unit tests locally, considering that Splash has already been built once by following the dedicated documentation:

```bash
cd build
make check
```

Similarly to run the integration tests:
```bash
cd build
make check_integration
```

It is also possible to select which tests to run based on their filename. For example to run all tests mentionning images:

```bash
cd tests/integration_tests
splash -P integration_tests.py -- --pattern 'test*image*.py'
```

To run a single test:
```bash
cd tests/integration_tests
splash -P integration_tests.py -- --pattern 'test_sample.py'
```

### Evaluating test coverage

There is a specific build target meant to measure the test coverage:

```bash
cd build
make check_coverage
```

A `coverage` subdirectory will be created, containing the results. You just have to open `coverage/index.html` in your favorite browser to navigate the analysis.


## Improving Documentation

Should you have a suggestion for the documentation, you can open an issue and outline the problem or improvement you have - however, creating the fix yourself is much better!

All the documentation for Splash can be found [Splash website](https://splashmapper.xyz), and in the [source repository](https://gitlab.com/splashmapper/splashmapper.gitlab.io). Please refer to this repository to help with improving the documentation.

If you want to help improve the docs and it's a substantial change, create a new issue (or comment on a related existing one) in the documentation repository to let others know what you're working on. Small changes (typos, improvements to phrasing) do not need an issue.


## Performance profiling

### Using Tracy
[Tracy](https://github.com/wolfpld/tracy) is a low-overhead, high performance profiling tool for CPU and GPU. It allows for profiling a software over the network and gives very precise measurements based on code instrumentation.

Splash has a few instrumentation zones already defined, in `./src/core/world.cpp` and `./src/core/scene.cpp`. They allow for having a good overview of the software performances of the software and get a glimpse over what the overall performances look like. To activate profiling through Tracy, Splash must be built using the following option:
```bash
cmake -DPROFILE=ON ..
```

Once compiled and installed Splash can be used as usual. The impact of Tracy on the performances of Splash is minimal but has not be thoroughly measured, so it should be kept disabled by default. To show the measurements you need to run the Tracy Profiler. You can either run it manually after having built it [based on its documentation](https://github.com/wolfpld/tracy/releases), or you can run it through the `CMake` target:
```bash
make tracy-profiler
```

This can take some time to build the first time this command is called, but it should be faster afterwards.

You are strongly advised to read Tracy's documentation to learn how to use the profiler, and if needed how to add profiling zones in the code.

### Using flamegraphs
A header-only profiler has been implemented to measure the time spent in OpenGL operations. To activate profiling, Splash must be compiled with the `PROFILE_OPENGL` flag activated:
```bash
cmake -DPROFILE_OPENGL=ON ..
```

It supports multiple threads and uses a RAII implementation to ease its use. If you want to profile a block of code just include the header include/profilerGL.h into your source and call:
```cpp
PROFILEGL("My scope name")
```

every time you want to measure the time spent by the graphics driver in nanoseconds in the current scope. If you use PROFILE blocks in multiple threads they will be separated appropriately in the processed data. The hierarchy of execution scopes is also preserved so as to know which proportion of time is spent in the current scope or in its underlying profiled scopes. All the processing time not profiled will be accounted for in the duration of the directly higher profiled scope.

Currently, only one format of display is supported: [FlameGraph](https://github.com/brendangregg/FlameGraph). Whenever you want to process the profiling data you need to call:
```cpp
ProfilerGL::get().processTimings();
```

This will preprocess the data to sort through the cumulated timings in all profiled scopes. After that to postprocess the data for FlameGraph:
```cpp
ProfilerGL::get().processFlamegraph();
```

If you want to flush the data at this point:
```cpp
ProfilerGL::get().clearTimings();
```

Once you have done that just use the FlameGraph to create the browsable graph:
```cpp
flamegraph.pl /tmp/splash_profiling_data_${SCENE_NAME} > /tmp/profiling_data.svg
```

With `SCENE_NAME` being the name as shown in the user interface, in the performances section (lower left corner).

The resulting file can be opened the svg with your browser. As of now, there is no way to view the profiling information in the Splash GUI.


## Code

### Fixing an Issue

The list of outstanding feature requests and bugs can be found on the [GitLab issue tracker](https://gitlab.com/splashmapper/splash/-/issues). Pick an unassigned issue that you think you can accomplish and add a comment that you are attempting to do it.

### Testing without installing on the system

Sometimes it is preferable to test Splash without installing it on the target system. Or we just do not have the administrator rights to do it, but we still need to do the tests because the issue cannot be reproduced elsewhere. Or Splash was never installed in the first place and our newly built binary cannot find some assets.

In all these cases (non exhaustive list), a good workaround is to install Splash in a non-system directory, where we have write access. This is done by specifying a different installation prefix and install Splash without sudo rights:

```bash
mkdir -p build && cd build
cmake -DCMAKE_INSTALL_PREFIX=${HOME}/splash_debug  # It can be any directory we have access to
make -j$(nproc)
make install
# Then you can go to this directory and run Splash:
cd ${HOME}/splash_debug/bin
./splash
```

### Debugging

When debugging, the first step is to compile Splash in `Debug` mode. From the build directory, run:
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

As Splash runs multiple processes on a single computer (and hopefully someday on multiple computers), debugging its behavior can be a bit tricky. The main reason is that the rendering can be run in a subprocess executed when calling Splash from the command line. Therefore, when the software is invoked from `gdb`, `gdb` will not catch the issues in the subprocess.

A workaround is to run the subprocesses manually and to tell the main executable to communicate with these processes instead of spawning its own. We will use the default configuration as an example, located in `./data/share/splash/splash.json` and installed in `${PREFIX}/share/splash/`

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
splash --doNotSpawn --prefix debug ./data/share/splash/splash.json
```

In this command line:
- `--doNotSpawn`: do not spawn any child process (it has been run manually)
- `--prefix debug`: the prefix must be the same as the child process, otherwise they will not be able to communicate
- `./data/share/splash/splash.json`: the path to the file to run

Splash should then run as usual, except that you have total control over all executed processes. It is then possible to run both processes through `gdb`:

```bash
gdb --args splash --child --prefix debug local
gdb --args splash --doNotSpawn --prefix debug ./data/share/splash/splash.json
```

Then run each process in `gdb` using the `run` command.

### Adding tests

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

### Coding style

We use LLVM style for the C++ code, with a few exceptions. See the [clang-format configuration](https://gitlab.com/splashmapper/splash/tree/master/.clang-format) for more about this.

It is possible to let git ensure that you are conforming to the standards by using pre-commit hooks and clang-format:
```
sudo apt-get install clang-format exuberant-ctags
# Then in Splash's home folder:
rm -rf $(pwd)/.git/hooks && ln -s $(pwd)/.hooks $(pwd)/.git/hooks
```

For Python code we follow the [PEP 8 style guide](https://www.python.org/dev/peps/pep-0008/)

### Branching strategy with git

The [master](https://gitlab.com/splashmapper/splash/tree/master) branch contains Splash releases. Validated new developments are into the [develop](https://gitlab.com/splashmapper/splash/tree/develop) branch.

Modifications are made into a dedicated branch that needs to be merged into the **develop** branch through a Gitlab merge request. When you modification is ready, you need to prepare your merge request as follow:

* Update your **develop** branch. 
```
git fetch
git checkout develop
git pull origin develop
```
* Go back to your branch and rebase onto **develop**. Note that it would be appreciated that you only merge request from a single commit (i.e interactive rebase with squash and/or fixup before pushing).
```
git checkout NAME_OF_YOUR_BRANCH
git rebase -i develop
```

When your feature branch is ready, submit a Merge Request to **develop**. When the MR is approved and your changes merged, you can safely delete your feature branch.

Core developers will prepare new releases regularly by creating a *release branch* from **develop** (named *release/<new_version>*). This new branch allows developers to test the release's stability and prepare the relevant metadata. Eventually, when this new release has been tested and deemed stable enough, it will be merged in the **master** branch by the core team. This new commit on **master** will be tagged for future reference and the original release branch will then be deleted

### CI pipeline
	
This repository includes a CI pipeline, used to validate that incoming commits do not break the build, and that all unit tests still pass. It will be run automatically when a new commit is pushed to the repository. If the pipeline fails, it is your responsability to checkout the [CI Jobs page](https://gitlab.com/splashmapper/splash/-/jobs) and figure out how to fix it. A Merge Requests that breaks the pipeline will not be merged by the core developers.


## Merge Request Process

In order to propose a merge request, please follow the gitlab recommended process: fork,  mirror & merge request. This process is documented [here](https://docs.gitlab.com/ee/user/project/repository/forking_workflow.html).

When you are ready to submit you changes for review, either for preliminary review or for consideration of merging into the project, you can create a Merge Request to add your changes into **develop**. Only core developers are allowed to merge your branch into **develop**.

Do not forget to:

1. Update the relevant [README](./README.md) sections, if necessary.
2. Add the relevant unit tests, if needed.
3. Update documentation, if needed.
4. Rebase your changes in nice, clear commits if your branch's commit history is messy.

A core developer will merge your changes into **develop** when all feedback have been addressed.

### Review Process

The core developer team regularly checks the repository for new merge requests. We expect the CI pipeline to succeed before approval of the MR. If it doesn't, your MR will not be merged. 

If the MR concerns a minor issue, it will be merged immediately after one of the core developers approves it, if the CI succeeds. For more serious issues, we will wait a day or two after all feedback has been addressed before merging the request.

### Static analysis

Before releasing a new version of Splash, a static analysis pass should be done:

```bash
git checkout develop
git branch -D static_analysis # remove any previous branch by this name
git checkout -b static_analysis
git push -f origin static_analysis
```

This will trigger the static analysis CI pipeline. The results will be availabe as an artefact from the pipeline and as a report from Coverity, an online service which pushes static analysis a bit further. Sometimes Coverity can be very slow to process an open source project which is why there is also a Gitlab-only static analysis part. Results from Coverity are accessible [there](https://scan.coverity.com/projects/papermanu-splash).

The defects detected by static analysis should be fixed in a new commit, and submitted through a merge request.

### Release process

To release a new version of Splash, use the script provided in `./tools/release_version.py`. This script will take care of testing that the latest commit on **develop** build, update the version number as well as the [News.md](./News.md) file, and update the various branches accordingly.

```bash
./tools/release_version.py
```

The resulting branches will also be pushed to the repository. Note that you need to have write access to this repository for this to work.

The website should be updated to match the new release. Also the [Flatpak package](https://github.com/flathub/xyz.splashmapper.Splash) should be updated separately.

### Addressing Feedback

Once a MR has been submitted, your changes will be reviewed and constructive feedback may be provided. Feedback isn't meant as an attack, but to help make sure the highest-quality code makes it into the project. Changes will be approved once required feedback has been addressed.

## License

By contributing to this repository, you agree that your contributions will be licensed in accordance to the [LICENSE](./License.md) document at the root of this repository.
