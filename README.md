# ZRFS

A remote file system (lib & FUSE) using [ZProtocol](https://github.com/kapnak/zprotocol-c).

## Table of contents
1. [Installation](#Installation)
   1. [Windows](#Windows)
   2. [From sources](#From-sources)
2. [Usage](#Usage)
3. [Contact & Sponsor](#Contact-and-Sponsor)


## Installation

### Windows
See the GitHub releases.

### From sources
#### Requirements

Debian :
```
pkg-config
libsodium
libfuse3-dev
```

Cygwin :
```
gcc-g++
make
pkg-config
libsodium-debuginfo
libsodium-devel
libsodium23
cygfuse
```

You also needs to get [ZProtocol](https://github.com/kapnak/zprotocol-c).
```
git clone https://github.com/kapnak/zprotocol-c
cd zprotocol-c
make build-shared
make install
```

### Building
```
git clone https://github.com/kapnak/zrfs-c
cd zrfs-c
make build-cli
make install-cli
```

## Usage


```
Host a server
    zrfs host <ip> <port> [server sk path]
    
Connect to a server and mount a share
    zrfs mount <ip> <port> <z_mount point> <server pk bs32 / 0 (unencrypted)> [client sk path]
    
Add an ACL on file for user identify by public key 'pk'
    zrfs acl add <file> <pk> <permission>
    
Remove an ACL / all ACL
    zrfs acl del <file> [pk]
    
Generate secret key / Get public key in base 32
    zrfs key <sk path>
```


## Getting started

#### Generate key for the client and copy the public key :
```
zrfs key client.sk
```

#### Go to the directory that you want to share and add permission for the client :
```
cd path/of/your/share
zrfs acl add <client public key> 3
```
3 allow the client to read & write on your share.  
You may restrict only reading with permission 2.  
Refer to the [Permission](#Permissions) section.


#### Start the server :
```
zrfs host 127.0.0.1 1234 ../server.sk
```
- `127.0.0.1 1234` is the address used to host the TCP server.
- `../server.sk` is the server secret key that will be generated if the file does not already exist.
You can now copy the server public key.

#### Finally mount the share on the client :
```
zrfs mount 127.0.0.1 1234 mnt <server public key> path/to/client.sk
```
- `127.0.0.1 1234` is the address of the server.
- `mnt` is the mount point, on Windows you must use a drive letter like `Z:`.
- `server public key` you can find it in the terminal after starting the server.
- `path/to/client.sk` is the client secret key, it will be generated if the file does not exist. Note that this key needs to have permissions on the server.

## Permissions

| Permission |     Name     | Meaning                                                            |
|:----------:|:------------:|--------------------------------------------------------------------|
|     0      |   Nothing    | Deny every request on the file.                                    |
|     1      |  Reference   | Allow the client to see the file when reading the paren directory. |
|     2      |     Read     | Allow the client to read the file / directory.                     |
|     3      |    Write     | Allow the client to write in the file / directory.                 |
|     4      | Administrate | Allow the client to change the permissions.                        |

If a file doesn't have any permission for the client, the permission of the parent will be used.  
If neither parents have permission, then the permission 0 (Nothing) will be used.

To add a permission for any client, you can use the public key 0.  
For example, the following command will give permission 2 for any client that doesn't have an acl (permission) for the `data` folder.
```
zrfs acl add 0 data 2
```


## Contact and Sponsor

Don't hesiate to contact me :
> Mail : kapnak.mail@gmail.com  
> Discord : kapnak


If you want to become a sponsor contact me, or if you just want to donate here is my wallet :  
Monero (XMR) :
```
444DEqjmWnuiiyuzxAPYwdQgcujJU1UYFAomsdS77wRE9ooPLcmEyqsLtNC11C5bMWPif5gcc7o6gMFXvvQQEbVVN6CNnBT
```