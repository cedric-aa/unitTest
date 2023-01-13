# Redgum - Bluetooth Mesh Code for Bluetooth Module

## Getting Started

* Install VS Code on your computer [https://code.visualstudio.com/download]
* Install nRF Connect SDK, all the toolchain and nRF Connect for VS Code Extensions [https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/gs_assistant.html]
* Setup Git [https://docs.github.com/en/get-started/quickstart/set-up-git]

## Git

Please start your git branches with your first name in e.g. `brian_blah`.

### Starting out a new branch

You should always start from the latest copy of main
```shell
git switch main
git fetch
git pull
```

To create a new branch from the current branch you are viewing. This will also move you to this branch:
```shell
git checkout -b brian_blah
```

### Other useful commands

To checkout a remote branch:

```shell
git fetch
git switch [name_of_remote_branch_without/origin]
```

To pull changes to your branch from a remote branch

```shell
git fetch
git merge origin/[name_of_remote_branch]
```

To switch between to another local branch

```shell
git switch [name_of_local_branch]
```

To clean up local branches that have been deleted on origin

```shell
git fetch -p && for branch in $(git for-each-ref --format '%(refname) %(upstream:track)' refs/heads | awk '$2 == "[gone]" {sub("refs/heads/", "", $1); print $1}'); do git branch -D $branch; done
```

To add this as an alias - use the following:

```shell
alias gclean="git fetch -p && for branch in $(git for-each-ref --format '%(refname) %(upstream:track)' refs/heads | awk '$2 == "[gone]" {sub("refs/heads/", "", $1); print $1}'); do git branch -D $branch; done"
```
