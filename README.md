# multihome

[![CircleCI](https://circleci.com/gh/jhunkeler/multihome.svg?style=svg)](https://circleci.com/gh/jhunkeler/multihome)

NFS mounted home directories are common when operating in a clustered environment and so are the problems that come along with it. Multihome manages your `HOME` environment variable on a per-host basis. When you log into a system, Multihome creates a new home directory using the system's default account skeleton, changes your `HOME` to point to it, then initializes your shell session from there. This allows you, as the user, to maintain unique home directories on any system within the cluster; complete with their own individualized settings.

## Usage

```
Partition a home directory per-host when using a centrally mounted /home

  -s, --script               Generate runtime script
  -u, --update               Synchronize user skeleton and transfer
                             configuration
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Show version and exit
```

## Your cluster

Without multihome your cluster probably resembles something like this. Each computer logged into uses the same home directory. Your shell history, your compiled programs, everything... always comes from the same place.

```
============                 ============
+ Computer +                 + Computer +
============                 ============
            \               /
             \             /
              =============
              + NFS /home +
              =============
             /             \
            /               \
============                 ============
+ Computer +                 + Computer +
============                 ============
```

## Your multihome cluster

Your home directory is still served over NFS but now under multihome's control, so each login on a host produces a new `HOME`. 

```
=============           ======================================
+ NFS /home +    /----- + /home/example/home_local/computerA +
=====*=======   /       ======================================
     |         /         
     |        /         ======================================
     |       /--------- + /home/example/home_local/computerB +
=====*=======            ======================================
+ Multihome +           
=============           ======================================
             \--------- + /home/example/home_local/computerC +
              \         ======================================
               \          
                \       ======================================
                 \----- + /home/example/home_local/computerD +
                        ======================================
```

## Installing

```
$ git clone https://github.com/jhunk/multihome
$ cd multihome
$ mkdir build
$ cd build
$ cmake ..
$ sudo make install
```

## Setup

```
#
# Example username: "example"
# Example hostname: "hostname"
#
$ multihome -s
Creating home directory: /home/example/home_local/hostname
Creating symlink to original home directory: /home/example/home_local/hostname/topdir
Creating user skel directory: /home/example/.multihome/skel/
Injecting account skeleton: /etc/skel/
Injecting user-defined account skeleton: /home/example/.multihome/skel/
Parsing transfer configuration, if present
Creating marker file: /home/example/home_local/hostname/.multihome_controlled
```

Passing the`-s` (`--script`) option generates the initialization script needed to manage your home directories, `~/.multihome/init`, and can be applied by adding the following snippet to the top of your ~/.bash_profile` (or other POSIX-compatible shell initialization scripts):

```bash
if [ -f "$HOME/.multihome/init" ]; then
    # Switch to managed home directory
    . $HOME/.multihome/init

    # Reinitialize the system shell profile
    [ -f "/etc/profile" ] && . /etc/profile

    # Initialize managed home directory's shell profile
    [ -f "$HOME/.bash_profile" ] && . $HOME/.bash_profile
fi
```

## Managing data

### With a custom account skeleton

When `multihome` creates a new home directory it copies the contents of `/etc/skel`, then `$HOME/.multihome/skel`.

```
$ mkdir -p ~/.multihome/skel
$ cd ~/.multihome/skel
$ ln -s /home/example/.vim
$ ln -s /home/example/.vimrc
```

The contents of `~/.multihome/skel` are now:

```
$ ls -la
total 8
drwxr-xr-x 2 example example 4096 Sep  1 14:18 .
drwxr-xr-x 3 example example 4096 Aug 30 10:26 ..
lrwxrwxrwx 1 example example   16 Sep  1 14:18 .vim -> /home/example/.vim
lrwxrwxrwx 1 example example   18 Sep  1 14:18 .vimrc -> /home/example/.vimrc
```

When `multihome` initializes a new home directory you will notice your customizations have been incorporated into the base account skeleton:

```
$ ls -la /home/example/home_local/hostname
total 60
drwxr-xr-x   3 example example  4096 Aug 27 23:31 .
drwxr-xr-x 119 example example 12288 Sep  1 00:42 ..
-rw-r--r--   1 example example    21 Jul 10 12:57 .bash_logout
-rw-r--r--   1 example example    57 Jul 10 12:57 .bash_profile
-rw-r--r--   1 example example  3838 Jul 10 12:57 .bashrc
drwxr-xr-x  11 example example  4096 Aug 27 23:31 .config
-rw-r--r--   1 example example  4855 Oct 29  2017 .dir_colors
-rw-r--r--   1 example example   141 Aug 11 09:04 .profile
-rw-r--r--   1 example example  3729 Feb  6  2020 .screenrc
lrwxrwxrwx  1 example example   16 Sep  1 14:18 .vim -> /home/example/.vim
lrwxrwxrwx  1 example example   18 Sep  1 14:18 .vimrc -> /home/example/.vimrc
-rwxr-xr-x   1 example example   100 Oct 29  2017 .Xclients
-rw-r--r--   1 example example  1500 Aug 11 09:04 .xinitrc
```

### Via transfer configuration


To avoid broken shells, errors produced by the `~/.multihome/transfer` configuration are reported on `stderr`, but will not halt the program.

#### Configuration format

```
# comment (inline comments are OK too)
TYPE WHERE
```

#### Types

- `H`: Create a hardlink from `/home/example/WHERE` to `/home/example/home_local/`
- `L`: Create a symbolic link from `/home/example/WHERE` to `/home/example/home_local/`
- `T`: Transfer file or directory from `/home/example/WHERE` to `/home/example/home_local/`

#### Example

```bash
$ cat << EOF > ~/.multihome/transfer
H .Xauthority        # Hardlink to /home/example/.Xauthority file
L .vim               # Symlink to /home/example/.vim directory
T .vimrc             # Copy /home/example/.vim directory
```

Transferring directories requires a trailing slash:

```
# T my_data          # incorrect -> /home/example/home_local/my_data/my_data/
T my_data/           # correct -> /home/example/home_local/mydata/
```

### Synchronizing data

Passing the `-u` (`--update`) option copies files from `/etc/skel`, `~/.multihome/skel`, and processes any directives present in the `~/.multihome/transfer` configuration. The destination file(s) will be replaced if the source file (`/home/example/file`) is newer than the destination file (`/home/example/home_local/file`).

## Known issues / FAQ

* SSH reads its configuration from `/home/example/.ssh` instead of `/home/example/home_local/.ssh`. This is a security feature and there is no way to override this behavior unless you recompile SSH/D from source. As a workaround use a symbolic link to improve your quality of life. At least `~/.ssh` will exist and point to the right place.

    *~/.multihome/transfer*:
```
L .ssh
```

* X11 fails to forward correctly

    *~/.multihome/transfer*:
```
H .Xauthority
``` 

* X11 still fails to forward correctly.

    If you are dropping your environment ensure you carry over your `DISPLAY` variable set by SSH at login. Multihome has nothing to do with this.

* When I login nothing happens!

    If your `/home/your_user/.bashrc` (often called by `~/.bash_profile`) makes a hard-coded reference to `/home/your_user/.bash_profile` the shell will enter an infinite loop. To interrupt this loop hit `ctrl-c` multiple times. Now edit your shell scripts, correct the problem, log out, and log back into the system.

* Support my shell! I use `tcsh`, heretic!

    I have no interest in supporting shells that aren't `sh`-compatible. PRs are welcome, of course.
