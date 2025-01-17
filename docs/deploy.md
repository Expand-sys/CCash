# Deploying CCash

CCash can deployed to a remote machine pretty simply.

A pre-built docker image is supplied in the repos [GitHub Packages](https://github.com/features/packages) container registry [EntireTwix/CCash](https://github.com/EntireTwix/CCash/packages/851105).

It can be run with docker like so:

```
docker run -itp 443:443 -v ccashconfig:/ccash/config -e ADMIN_A=<admin-username> -e SAVE_FREQ=<in minutes> ghcr.io/entiretwix/ccash/ccash:latest
```

## Ansible
Additionally CCash can be deployed to any infrastructure able to run Rocky/Alma Linux 8/9 x86_64 virtual or not, we will be eventually updating it to allow it to run on other OS's but for now RHEL is what works. 

As CCash is intended to be run as root, the playbook is run also as root. The playbook also builds CCash from the latest github push, so there may be bugs.

In order to use the ansible playbook, clone the playbook to any pc with the ability to access the server through SSH and with Ansible installed, edit the inventory file to contain the IP address of the target server and run the following commands:
```ansible-galaxy install -r deployment/requirements.yml
```ansible-playbook -i deployment/inventory deployment/main.yml -k```
When this is complete the server will have ccash installed to the user dir, this is customizable in the vars/default.yml file along with the admin username and save frequency.
To start CCash run:
```systemctl start ccash```
To run ccash at start up run:
```systemctl enable ccash```

## Build
Previously this used GitHub Workflows, I(Expand) dont know how to do those but its not that hard to deploy stuff manually. To run the pre configured docker image run the above command and you are off to the races it will deploy a self signed certificate and use that for deployment. As this is not a user facing deployment the certificate is self signed and thus will throw an error on chrome, though this will still work if you ignore it. For production you should deploy with a reverse proxy and a correct certificate for your domain.

To build this manually you may download the dockerfile only, it will pull the latest repository the commands for building are
```
docker build --build-arg ADMIN_A=<admin-username> --build-arg SAVE_FREQ=<in minutes> -t ccash . --no-cache
docker run -itp 443:443 -v ccashconfig:/ccash/config -e ADMIN_A=<admin-username> -e SAVE_FREQ=<in minutes> ccash
```
if you have the know how you may edit the docker file and add CMAKE commands as listed in the build section of the CCash documentation.

## Deploy

You can deploy this docker image to be run on a remote machine in a few steps or you can deploy manually in this case we are using [Debian OS](https://www.debian.org/) running on the [Linode](https://www.linode.com/) cloud provider, but most OS and cloud providers will work, assuming the machine can run an SSH server.

Additionally, there is a dockerfile where you can build it yourself or a repository available on [dockerhub](https://hub.docker.com/r/expandsys/ccash) for you to just pull and run on any machine quickly and easily.

### Configure the machine
For docker deployment there is one supported config and that is the command listed above, 
 ADMIN_A = Admin account name, must be all lowercase, and the account must be created before use. To do this either use the [CCashDeploy](https://hub.docker.com/r/expandsys/ccashdeploy) docker image and use the CCashFrontend to register the account or use curl to send the raw command to the endpoint.
 SAVE_FREQ = Saving frequency in minutes, pretty simple

