# multihome

NFS mounted home directories are common you're operating in a clustered environment, and so are the problems that go along with it. Multihome manages your `HOME` environment variable on a per-host basis. When you log into system, Multihome creates a new home directory using the system's default account skeleton, changes your `HOME` to point to it, then initializes your shell session from there. This allows you, as the user, to maintain unique home directories on any system within the cluster; complete with their own individualized settings.

## Usage
```
Partition a home directory per-host when using a centrally mounted /home

  -s, --script               Generate runtime script
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
$ git clone https://github.com/exampleeler/multihome
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

This generates the script, `~/.multihome/init`. To activate Multihome for your account add the following to the top of your `.bash_profile` (or other POSIX-compatible profile scripts):

```bash
if [ -f "$HOME/.multihome/init" ]; then
    . $HOME/.multihome/init
fi
```

What does the `init` script actually do?

```bash
$ cat ~/.multihome/init
#
# This script was generated on 08-30-2020 @ 10:26:11
#

MULTIHOME=/usr/local/bin/multihome
if [ -x $MULTIHOME ]; then
    HOME_OLD=$HOME
    # Drop environment
    env -i
    # Redeclare HOME
    HOME=$($MULTIHOME)
    if [ "$HOME" != "$HOME_OLD" ]; then
        cd $HOME
    fi
fi
```

If Multihome is available drop the current environment, reset the `HOME` variable to point to a new location, and changed the directory to that path. Execution of the environment continues from that point within the original `~/.bash_profile`, so it's important to keep this script as generic as possible.

For example your profile script should look like this:
```bash
if [ -f "$HOME/.multihome/init" ]; then
    . $HOME/.multihome/init
fi

if [ -f "$HOME/.bashrc" ]; then
    . $HOME/.bashrc
fi
```