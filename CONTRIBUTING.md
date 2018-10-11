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
