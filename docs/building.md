# Building
[PREVIOUS PAGE](features/implementation.md) | [NEXT PAGE](FAQ.md)

## Advice
as CCash is very lightweight it can run on practically any device but here are some tips:
* single core machines should toggle `MULTI_THREADED` to `false`
* if your server is sufficiently active, such that each time save frequency is met, changes having been made is highly likely then `CONSERVATIVE_DISK_SAVE` should be toggled to `false`
* `MAX_LOG_SIZE` should be adjusted as it takes up the most memory usage/storage of the ledger's features at 139 bytes in memory and 43 bytes in disk at default settings on the current version, so 7543 logs per MB of RAM. Setting to 0 will disable logs
* with no users memory usage is ~8.47 MB
* saving frequency being set to 0 will disable frequency saving and only save on close
* make backups of your save files!

## Docker & Ansible
If you want to use the docker package, deploy information can be found [here](deploy.md)

## Drogon Depedencies 

### Linux
#### Debian
```
sudo apt install libjsoncpp-dev uuid-dev openssl libssl-dev zlib1g-dev make cmake
```
#### CentOS 7.5
```
yum install git gcc gcc-c++ 
git clone https://github.com/Kitware/CMake
cd CMake/
./bootstrap
make
make install
yum install centos-release-scl devtoolset-8
scl enable devtoolset-8 bash
git clone https://github.com/open-source-parsers/jsoncpp
cd jsoncpp/
mkdir build
cd build
cmake ..
make
make install
yum install libuuid-devel openssl-devel zlib-devel
```
#### Other
anything that can download the appropriate dependencies
```
make & cmake
jsoncpp
libuuid
openssl
zlib
```
### MacOS
```
brew install jsoncpp ossp-uuid openssl zlib 
```

## Actually, building
```
git clone --recurse-submodule https://github.com/EntireTwix/CCash/
cd CCash
cd third_party/base64
AVX2_CFLAGS=-mavx2 SSSE3_CFLAGS=-mssse3 SSE41_CFLAGS=-msse4.1 SSE42_CFLAGS=-msse4.2 AVX_CFLAGS=-mavx make lib/libbase64.o
cd ../..
mkdir build
cd build
```

### CMake Variables
there are multiple flags responsible configuring CCash:
| name                     |       default        | description                                                                                                                                             | pros                             | cons                                                     |
| :----------------------- | :------------------: | ------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------- | -------------------------------------------------------- |
| USER_SAVE_LOC            |  "config/users.dat"  | where the users are saved                                                                                                                               | `N/A`                            | `N/A`                                                    |
| DROGON_CONFIG_LOC        | "config/config.json" | where the config is located                                                                                                                             | `N/A`                            | `N/A`                                                    |
| MAX_LOG_SIZE             |         100          | max number of logs per user, last `n` transactions. If both this and pre log are toggled to 0 logs will not be compiled.                                | large history                    | higher memory usage                                      |
| CONSERVATIVE_DISK_SAVE   |        `true`        | when `true` only saves when changes are made                                                                                                            | low # of disk operations         | some atomic overhead                                     |
| MULTI_THREADED           |        `true`        | when `true` the program is compiled to utilize `n` threads which corresponds to how many Cores your CPU has, plus 1 for saving                          | speed                            | memory lock overhead is wasteful on single core machines |
| RETURN_ON_DEL_NAME       |        `N/A`         | when defined, return on delete will be toggled and any accounts deleted will send their funds to the defined account, this prevent currency destruction | prevents destruction of currency | deleting accounts is made slower                         |
| ADD_USER_OPEN            |        `true`        | anybody can make a new account, if set to false only admins can add accounts via `AdminAddUser()`                                                       | `N/A`                            | spamming new users                                       |
| USE_DEPRECATED_ENDPOINTS |        `true`        | some endpoints have newer versions making them obsolete but old programs might still call these endpoints so they are simply marked deprecated.         | supports old programs            | old endpoints can be ineffecient                         |

EXAMPLE:
```
cmake ..
```
sets these flags to their defaults, an example of setting a flag would be 
```
cmake -DMULTI_THREADING=false ..
```
with `-D`

### Finishing building
lastly type in
```
cmake <flags of your choice or none> ..
make -j<threads>
sudo ./bank
```
the last command generates a blank save file in your defined location.

## Certs
make sure to edit `config.json` adding the certificate location if you're using HTTPS, I personally use [certbot](https://certbot.eff.org/). 
```json
{
  "listeners": [
      {
        "address": "0.0.0.0",
        "port": 80,
        "https": false
      },
      {
        "address": "0.0.0.0",
        "port": 443,
        "https": true,
        "cert": "",
        "key": ""
      }
  ]
}
```
editing
```json
"cert": "pubkey",
"key": "privkey"
```

Alternatively you can delete this entire section (Disabling HTTPS in the proccess)
```json
{
    "address": "0.0.0.0",
    "port": 443,
    "https": true,
    "cert": "",
    "key": ""
}
```
leaving
```json
{
  "listeners": [
      {
        "address": "0.0.0.0",
        "port": 80,
        "https": false
      }
  ]
}
```

## Usage
You can now run the program from the build location. For example
```
sudo ./bank admin 5
```
in this example CCash will be launched with the admin account named `"admin"`, and a saving frequency of every `5` minutes; without daemon being given its default is `false`.

Another example
```
sudo ./bank Kevin 0 true
```
in this example CCash will be launched with the admin account named `"Kevin"`, and a saving frequency of `0` meaning the server will only save when closed; daemon is set to `true` and so will be launched in the background.
