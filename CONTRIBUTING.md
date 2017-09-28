CONTRIBUTING
============

Repositories
------------

Two repositories are used to host Splash source code:
* [A Gitlab repository](https://gitlab.com/sat-metalab/splash) which is our working place, used internally at the SAT-Metalab,
* [A Github repository](https://github.com/sat-metalab/splash) which is used as a public repository, and is a mirror of the Gitlab one.


Coding style
------------

We use LLVM style, with a few exceptions. See the [clang-format configuration](./.clang-format) for more about this.

It is possible to let git ensure that you are conforming to the standards by using pre-commit hooks and clang-format:
```
sudo apt-get install clang-format
# Then in Splash's .git folder:
rm -rf hooks && ln -s ../.hooks hooks
```

Contributing
------------

Please send your pull request at the [SAT-Metalab's Github account](https://github.com/sat-metalab/splash). If you do not know how to make a pull request, Github provides some [help about collaborating on projects using issues and pull requests](https://help.github.com/categories/collaborating-on-projects-using-issues-and-pull-requests/).

Branching strategy with git
---------------------------

The [master](https://gitlab.com/sat-metalab/splash/tree/master) branch contains Splash releases. Validated new developments are into the [develop](https://github.com/sat-metalab/splash/tree/develop) branch.

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
