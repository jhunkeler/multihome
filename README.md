# multihome

![Actions](https://github.com/jhunkeler/multihome/workflows/CMake/badge.svg)

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

Under multihome's scheme the NFS shared home directory houses a `HOME` unique to each host accessed by your account.

```
=============             ======================================
+ NFS /home +    /------- + /home/example/home_local/computerA +
=====*=======   /         ======================================
     |         /           
     |        /           ======================================
     |       /----------- + /home/example/home_local/computerB +
=====*=======             ======================================
+ Multihome +             
=====*=======             ======================================
     |       \----------- + /home/example/home_local/computerC +
     |        \           ======================================
     |         \            
     |          \         ======================================
     |           \------- + /home/example/home_local/computerD +
     |                    ======================================
     |
     |
     |    GROUP           ======================================
     \-(clusterA[0-9]+)-- + /home/example/home_local/clusterA +
     |                    ======================================
     |
     |    GROUP           ======================================
     \-(clusterB[0-9]+)-- + /home/example/home_local/clusterB +
                          ======================================
```

## Installing

```
$ git clone https://github.com/jhunkeler/multihome
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
Creating configuration directory: /home/example/.multihome
Creating host group configuration: /home/example/.multihome/host_group
Creating home directory: /home/example/home_local/hostname
Creating symlink to original home directory: /home/example/home_local/hostname/topdir
Creating user skel directory: /home/example/.multihome/skel/
Creating transfer configuration: /home/example/.multihome/transfer
Pulling account skeleton: /etc/skel/
Pulling user-defined account skeleton: /home/example/.multihome/skel/
Creating marker file: /home/example/home_local/hostname/.multihome_controlled
```

Passing the`-s` (`--script`) option generates the initialization script needed to manage your home directories, `~/.multihome/init.[c]sh`, and can be applied by adding the appropriate snippet below to the top of your shell profile.

### POSIX SH

**/home/example/.profile:**

```bash
if [ -f "$HOME/.multihome/init.sh" ]; then
    # Switch to managed home directory
    . $HOME/.multihome/init.sh

    # Reinitialize the system shell profile
    [ -f "/etc/profile" ] && . /etc/profile

    # Initialize managed home directory's shell profile
    [ -f "$HOME/.profile" ] && . $HOME/.profile
fi
```

### BASH

**/home/example/.bash_profile:**

```bash
if [ -f "$HOME/.multihome/init.sh" ]; then
    # Switch to managed home directory
    . $HOME/.multihome/init.sh

    # Reinitialize the system shell profile
    [ -f "/etc/profile" ] && . /etc/profile

    # Initialize managed home directory's shell profile
    [ -f "$HOME/.bash_profile" ] && . $HOME/.bash_profile
fi
```

### ZSH

**/home/example/.zshrc:**

```bash
if [ -f "$HOME/.multihome/init.sh" ]; then
    # Switch to managed home directory
    . $HOME/.multihome/init.sh

    # Reinitialize the system shell profile
    [ -f "/etc/zsh/zprofile" ] && . /etc/zsh/zprofile

    # Initialize managed home directory's shell profile
    [ -f "$HOME/.zshrc" ] && . $HOME/.zshrc
fi
```

### [T]CSH

**/home/example/.cshrc**

```tcsh
if ( -f "$HOME/.multihome/init.csh" ) then
    # Switch to managed home directory
    source $HOME/.multihome/init.csh

    # Reinitialize the system shell profile
    if ( -f "/etc/csh.login" ) then
        source /etc/csh.login
    endif

    # Initialize managed home directory's shell profile
    if ( -f "$HOME/.cshrc" )
      source $HOME/.cshrc
    endif
fi
```

## Managing host groups

The `~/.multihome/host_group` configuration file allows one to create logical (shared) home directories based on hostname patterns.

### Configuration Format

```config
# comment (inline comments are OK too)
HOST_REGEX = GROUP_NAME
```

Host pattern matching is implemented using [POSIX.2 (extended) regular expression](https://en.m.wikibooks.org/wiki/Regular_Expressions/POSIX_Basic_Regular_Expressions) syntax.

#### Example

```bash
# Map any hosts starting with "example" to home directory named "example"
example.* = example

# Map hosts "cluster_machine1", "cluster_machine2", "other_machine8", and
# "other_machine9" to the home directory, "special_boxes".
(cluster_machine[1,2]|other_machine[8,9])$ = special_boxes

# Map remaining "cluster_machine" hosts to the "cluster_machines" home directory
cluster_machine.* = cluster_machines

# Map dev/test/prod systems
^dlproduct.* = product_dev
^tlproduct.* = product_test
^plproduct.* = product_prod
```

## Managing data

### Via custom account skeleton

When `multihome` creates a new home directory it copies the contents of `$HOME/.multihome/skel/`.

For example, if you are an avid `vim` user and don't want to maintain its configuration manually for each host, you can create symbolic links in the `skel` directory pointing to the configuration data.

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

When `multihome` initializes a home directory these customizations will applied automatically:

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
lrwxrwxrwx   1 example example    16 Sep  1 14:18 .vim -> /home/example/.vim
lrwxrwxrwx   1 example example    18 Sep  1 14:18 .vimrc -> /home/example/.vimrc
-rwxr-xr-x   1 example example   100 Oct 29  2017 .Xclients
-rw-r--r--   1 example example  1500 Aug 11 09:04 .xinitrc
```

### Via transfer configuration

The `~/.multihome/transfer` configuration file allows one to link or copy files and directories from their base home directory into the newly created home directory. This method can be used in tandem with `skel`, however if the account skeleton provides a file, yet the transfer configuration wants to create a link instead (types `H` and `L`), the regular file will be replaced by the link.

#### Configuration format

```
# comment (inline comments are OK too)
TYPE WHERE
```

#### Types

- `H`: Create a hardlink from `/home/example/WHERE` to `/home/example/home_local/HOST`
- `L`: Create a symbolic link from `/home/example/WHERE` to `/home/example/home_local/HOST`
- `T`: Transfer file or directory from `/home/example/WHERE` to `/home/example/home_local/HOST`

#### Example

```bash
$ cat << EOF > ~/.multihome/transfer
H notes.txt          # Hardlink to /home/example/notes.txt
L .Xauthority        # Symlink to /home/example/.Xauthority file
L .vimrc             # Symlink to /home/example/.vimrc file
T .vim/              # Copy /home/example/.vim directory
```

Transferring directories requires a trailing slash:

```
# T my_data          # result (bad): /home/example/home_local/my_data/my_data/
T my_data/           # result:       /home/example/home_local/mydata/
```

### Synchronizing data

Passing the `-u` (`--update`) option copies files from `/etc/skel`, `~/.multihome/skel`, and processes any directives present in the `~/.multihome/transfer` configuration. The destination file(s) will be replaced if the source file (`/home/example/file`) is newer than the destination file (`/home/example/home_local/file`).

```
$ multihome -u
Pulling account skeleton: /etc/skel/
Pulling user-defined account skeleton: /home/example/.multihome/skel/
```


## Known issues / FAQ

* SSH reads its configuration from `/home/example/.ssh` instead of `/home/example/home_local/.ssh`. This is a security feature and there is no way to override this behavior unless you recompile SSH/D from source. As a workaround use a symbolic link to improve your quality of life. At least `~/.ssh` will exist and point to the right place.

#### ~/.multihome/transfer:
```
L .ssh
```

* X11 fails to forward correctly
```
X11 connection rejected because of wrong authentication.
xterm: Xt error: Can't open display: x:yy.z
```

#### ~/.multihome/transfer    
```
L .Xauthority
# Note: A hardlink or direct transfer will not work here. The file must be a symlink.
```

* X11 still fails to forward correctly.

    If you are dropping your environment ensure you carry over your `DISPLAY` variable set by SSH at login. Multihome has nothing to do with this.

* When I login nothing happens!

    If your shell RC (e.g. `~/.bashrc`) file makes a hardcoded reference to your shell profile (e.g. `~/.bash_profile`) the shell will enter an infinite loop. To interrupt this loop hit `ctrl-c` multiple times. Now edit your shell scripts, correct the problem, log out, and log back into the system.

* Support my shell, heretic!

    PRs are welcome, of course.
