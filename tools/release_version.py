#!/usr/bin/env python3

"""
Release script for Splash.
Can be adapted to other software with minimal effort.

Usage: ./tools/release_version.py
"""

import atexit
import datetime
import multiprocessing
import os
import re
import shutil
import subprocess
import sys

from enum import IntEnum, unique
from typing import List


libs_root_path = os.path.join("/", "tmp", "releases")
version_file = "CMakeLists.txt"
metainfo_file = os.path.join("data", "share", "metainfo", "com.gitlab.sat_metalab.Splash.metainfo.xml")

version_pattern = "\s+VERSION (\d+).(\d+).(\d+)"
git_path = "git@gitlab.com:sat-metalab"
git_project = "Splash"
remote_repo = 'origin'
bringup_branch = "master"
working_branch = "develop"
releasing_branch = "releasing"
success = True


@unique
class VersionIncrease(IntEnum):
    """
    Version increase type
    """
    MAJOR = 1
    MINOR = 2
    PATCH = 3


def git_add(file_list: List[str]) -> None:
    for file in file_list:
        subprocess.call(f"git add {file}", shell=True)


def git_clone(repo_url: str) -> int:
    return subprocess.call(f"git clone {repo_url}", shell=True)


def git_checkout(branch_name: str, is_new: bool = False) -> int:
    if is_new:
        return subprocess.call(f"git checkout -b {branch_name}", shell=True)
    else:
        return subprocess.call(f"git checkout {branch_name}", shell=True)


def git_commit(message: str) -> int:
    return subprocess.call(f"git commit -m \"{message}\"", shell=True)


def git_merge(branch_name: str, force: bool = False) -> int:
    if force:
        return subprocess.call(f"git merge --no-ff -X theirs {branch_name}", shell=True)
    else:
        return subprocess.call(f"git merge --no-ff {branch_name}", shell=True)


def git_tag(tag_name: str) -> int:
    return subprocess.call(f"git tag {tag_name}", shell=True)


def git_push(remote_repo: str, remote_branch: str) -> int:
    return subprocess.call(f"git push {remote_repo} {remote_branch}:{remote_branch} --tags", shell=True)


def get_git_config(property: str, default_value: str) -> str:
    config_property = default_value
    git_config_full = subprocess.check_output('git config --list', shell=True, encoding="utf-8").strip().split('\n')
    for config in git_config_full:
        prop = config.split('=')
        if len(prop) < 2:
            break
        if prop[0] == property:
            config_property = prop[1]
    return config_property


def parse_version_number(project: str, regex_pattern: str) -> List[int]:
    config_file = os.path.join(libs_root_path, project, version_file)
    version_number = [-1, -1, -1]
    with open(config_file) as file:
        major = minor = patch = -1
        for line in file:
            version_line = re.search(regex_pattern, line)
            if version_line:
                major = int(version_line.group(1))
                minor = int(version_line.group(2))
                patch = int(version_line.group(3))
                break

    if major != -1 and minor != -1 and patch != -1:
        version_number = [major, minor, patch]
    else:
        printerr(f"Current version number not found in {config_file}")

    return version_number


def update_config_file(project: str, new_version: List[int], regex_pattern: str) -> None:
    config_file = os.path.join(libs_root_path, project, version_file)
    changed_file = f"{config_file}.tmp"
    with open(config_file) as old_file:
        with open(changed_file, 'w') as new_file:
            for line in old_file:
                version_line = re.search(regex_pattern, line)
                if version_line:
                    line = f"\tVERSION {new_version[0]}.{new_version[1]}.{new_version[2]}"
                new_file.write(line)

    os.rename(changed_file, config_file)
    git_add([config_file])


def update_metainfo_file(project: str, version: List[int]) -> None:
    changed_file = f"{metainfo_file}.tmp"
    with open(metainfo_file) as old_file:
        with open(changed_file, 'w') as new_file:
            found_releases = False
            for line in old_file:
                release_line = re.search("    <release version=\"\d+\.+\d+\.+\d\" date=", line)
                if release_line and not found_releases:
                    found_releases = True
                    new_line = f"    <release version=\"{version[0]}.{version[1]}.{version[2]}\" date=\"{datetime.date.today()}\" />\n"
                    new_file.write(new_line)
                new_file.write(line)

    os.rename(changed_file, metainfo_file)
    git_add([metainfo_file])


def increase_version_number(version: List[int], version_increase: VersionIncrease) -> None:
    if version:
        if version_increase == VersionIncrease.MAJOR:
            version[1] = 0
            version[2] = 0
        elif version_increase == VersionIncrease.MINOR:
            version[2] = 0
        version[version_increase - 1] += 1
    else:
        printerr("Invalid version number, cannot proceed to increase it.")


def update_changelog(project: str, version: List[int]) -> None:
    print("Generating release notes")
    orig_file_name = "NEWS.md"
    new_file_name = "NEWS.md.new"
    authors_file_name = "AUTHORS.md"

    latest_tag = subprocess.check_output(f"git log --all --format=format:%H --grep=\"Merge branch\" | head -n 1 | tr -d '\n'", shell=True, encoding="utf-8")
    tag_date = subprocess.check_output(f"git log -1 --format=%ai {latest_tag} | tr -d '\n'", shell=True, encoding="utf-8")
    commits = re.split(r'[a-z0-9]{40} ', subprocess.check_output(f"git log --first-parent --since=\"{tag_date}\" | tr -d '\n'", shell=True, encoding="utf-8").strip())
    commits = commits[1:-1]
    with open(new_file_name, "w") as new_file:
        with open(orig_file_name, "r") as old_file:
            for i, line in enumerate(old_file.readlines()):
                if i != 2:
                    new_file.write(line)
                    continue
                new_file.write(f"\n{project} {version[0]}.{version[1]}.{version[2]} ({datetime.date.today()})\n-------------------------\n")
                for commit in commits:
                    new_file.write(f"* {commit}\n")

                new_file.write("\n")
    subprocess.call([get_git_config("core.editor", "vim"), new_file_name])
    os.rename(new_file_name, orig_file_name)
    subprocess.call(os.path.join(sys.path[0], "make_authors_from_git.sh"), shell=True)
    git_add([orig_file_name])
    if subprocess.call(os.path.join(sys.path[0], "make_authors_from_git.sh"), shell=True) == 0:
        git_add([authors_file_name])


def printerr(err: str) -> None:
    sys.stderr.write(err + "\n")
    exit(2)


@atexit.register  # Only clean on exit if the release was successful.
def cleanup_folder() -> None:
    global success
    if os.path.exists(libs_root_path) and success:
        shutil.rmtree(libs_root_path)
    success = False


if __name__ == "__main__":
    assert(sys.version_info[0] == 3 and sys.version_info[1] > 6), f"This script must be ran with at least Python 3.7, detected Python {sys.version_info[0]}.{sys.version_info[1]}"

    release_version = []
    version_increase: VersionIncrease = VersionIncrease.PATCH

    cleanup_folder()
    os.mkdir(libs_root_path)
    os.chdir(libs_root_path)

    choice = input("Is it a: \n\t1/ Major release \n\t2/ Minor release \n\t3/ Patch release?\n"
                   "This will impact the new version number (x.y.z matches the choices 1.2.3.): ")
    if choice == "1":
        version_increase = VersionIncrease.MAJOR
    elif choice == "2":
        version_increase = VersionIncrease.MINOR
    elif choice == "3":
        version_increase = VersionIncrease.PATCH
    else:
        printerr("Wrong choice. Aborting the release.")

    lib_repo = f"{git_path}/{git_project}.git"
    if git_clone(lib_repo) != 0:
        print(f"Could not fetch codebase for project {git_project} at {lib_repo}.")
        exit(1)

    os.chdir(os.path.join(libs_root_path, git_project))
    git_checkout(working_branch)

    release_version = parse_version_number(git_project, version_pattern)
    assert(release_version != [-1, -1, -1])
    increase_version_number(release_version, version_increase)

    print("Version number updated, now building dependencies.")

    if subprocess.call(f"./make_deps.sh", shell=True) != 0:
        printerr("Error while building dependencies.")

    print("Now executing unit tests.")

    build_dir = os.path.join(libs_root_path, git_project, "build")

    if not os.path.isdir(build_dir):
        os.mkdir(build_dir)
    os.chdir(build_dir)

    if subprocess.call(f"cmake -DCMAKE_BUILD_TYPE=Release .. && make -j{multiprocessing.cpu_count()} check", shell=True) != 0:
        printerr(f"{git_project} unit tests failed, stopping the release.")

    os.chdir(os.path.join(libs_root_path, git_project))

    print("All unit tests passed successfully, now creating new branches for release.")

    new_branch = f"{releasing_branch}/{release_version[0]}.{release_version[1]}.{release_version[2]}"
    print(f"Creating branch {new_branch} for release of project {git_project}")
    git_checkout(new_branch, True)

    update_changelog(git_project, release_version)
    update_config_file(git_project, release_version, version_pattern)
    update_metainfo_file(git_project, release_version)
    git_commit("Updated NEWS and version number")

    assert(git_checkout(bringup_branch) == 0), f"Could not checkout branch {bringup_branch}"
    assert(git_merge(new_branch, True) == 0), f"Merge from branch {new_branch} into {bringup_branch} did not work"

    git_tag(f"{release_version[0]}.{release_version[1]}.{release_version[2]}")

    release_branch = f"release/{release_version[0]}.{release_version[1]}.{release_version[2]}"
    assert(git_checkout(release_branch, True) == 0), f"Could not create branch {release_branch}"
    assert(git_checkout(new_branch) == 0), f"Could not checkout branch {bringup_branch}"

    if version_increase == VersionIncrease.PATCH:
        release_version[2] += 1
    else:
        release_version[2] = 1
    update_config_file(git_project, release_version, version_pattern)
    git_commit("Post-release version bump")

    assert(git_checkout(working_branch) == 0), f"Could not checkout branch {working_branch}"
    assert(git_merge(new_branch, True) == 0), f"Merge from branch {new_branch} into {working_branch} did not work"

    print("Pushing branches to remote.")
    do_push=input(f"Do you want to push {bringup_branch} and {working_branch} branches to {remote_repo}? [y/N]")
    if do_push == "y":
        assert(git_push(remote_repo, bringup_branch) == 0), f"Failed to push branch {bringup_branch} into {remote_repo}/{bringup_branch}"
        assert(git_push(remote_repo, release_branch) == 0), f"Failed to push branch {release_branch} into {remote_repo}/{release_branch}"
        assert(git_push(remote_repo, working_branch) == 0), f"Failed to push branch {working_branch} into {remote_repo}/{working_branch}"

    success = True
