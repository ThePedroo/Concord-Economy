# Introduction

A bot, powered by [Concord](https://github.com/Cogmasters/concord) and using [PostgreSQL](https://www.postgresql.org) database to store data.

## Compiling

As explained before, this bot uses [Concord](https://github.com/Cogmasters/concord) to run, so you will need to install it.

First, install all dependencies required by this bot and Concord:

```console
# apt update && apt install -y clang git make libcurl4-openssl-dev postgresql postgresql-server-dev-açç
```

Then, you will need to clone the repository, and after it, compile it:

```console
# git clone https://github.com/Cogmasters/concord && cd concord && make -j4 && make install
```

Done! Now you will need to put the information of your [PostgreSQL](https://www.postgresql.org) database and the bot itself. and you can compile the bot with:

```console
$ make
```

After compiling, you can proceed to run it with:

```console
$ make run
```

Or either with:

```console
$ ./ConcordBot
```

## Usage

You can proceed to get some `concoins` using the `.daily` command, in case you want to know other commands, use `.help`.

## Information

* Project owner Discord tag: `Pedro.js#9446`

* Concord lead developer Discord Tag: `müller#1001`

* Concord support server: [Discord](https://discord.gg/YcaK3puy49)

## Notes

This project was made for people to learn how to use C, [Concord](https://github.com/Cogmasters/concord) and [PostgreSQL](https://www.postgresql.org) and was not meant to be used professionally, but improvements are always welcome!
