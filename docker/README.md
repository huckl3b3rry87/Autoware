# Autoware Docker
Assuming the NVIDIA drivers and Docker and nvidia-docker are properly
installed.

<<<<<<< HEAD

## Install docker
To use Docker, it must be first installed on your machine:
```
sudo apt-get install apt-transport-https ca-certificates
sudo apt-key adv --keyserver hkp://p80.pool.sks-keyservers.net:80 --recv-keys 58118E89F3A912897C070ADBF76221572C52609D

# For Ubuntu 14.04
Repo = "deb https://apt.dockerproject.org/repo ubuntu-trusty main"
# For Ubuntu 16.04
Repo = "deb https://apt.dockerproject.org/repo ubuntu-xenial main"
=======
[Docker installation](https://docs.docker.com/engine/installation/linux/docker-ce/ubuntu/)
>>>>>>> b94c18bdb82cf29214a06941597a07b004c39dbb


<<<<<<< HEAD
### How to build
Make sure that the file has the appropriate permissions:
```
chmod +x build.sh
```
Then build it with,
```
./build.sh kinetic
```
### How to run
Make sure that the file has the appropriate permissions:
```
chmod +x run.sh 
```

Run it with,
```
febbo@febbo-HP-ZBook-17-G2:~/Documents/workspace/Autoware/docker$ ./run.sh kinetic
```

Now that we have changed users, we can run ``Autoware`` with:
```
autoware@febbo-HP-ZBook-17-G2:~$ cd Autoware/ros/
autoware@febbo-HP-ZBook-17-G2:~/Autoware/ros$ ./run
```
Then car_demo can be launched by clicking on the ``Simulation`` tab and then the ``Gazebo`` tab.

### Notes
To begin the port car_demo into Autoware, and leave the nvidia image base, then the docker files where copied in the car_demo stack. This had to be done, because ros-kinetic does not build with gazebo 8 by default.












## Install NVIDIA Docker
```
wget -P /tmp https://github.com/NVIDIA/nvidia-docker/releases/download/v1.0.1/nvidia-docker_1.0.1-1_amd64.deb
sudo dpkg -i /tmp/nvidia-docker*.deb && rm /tmp/nvidia-docker*.deb
```
=======
[nvidia-docker installation](https://github.com/NVIDIA/nvidia-docker)
>>>>>>> b94c18bdb82cf29214a06941597a07b004c39dbb

## How to build
Build a docker image with:
```
<<<<<<< HEAD
$/Autoware/docker/ ./build_demo.bash
=======
$ cd Autoware/docker

# Ubuntu 14.04 (Indigo)
$ sh build.sh indigo

# Ubuntu 16.04 (Kinetic)
$ sh build.sh kinetic
>>>>>>> b94c18bdb82cf29214a06941597a07b004c39dbb
```

## How to run
Run the demo with
```
<<<<<<< HEAD
$/Autoware/docker/ ./run_demo.bash
=======
$ Default shared directory path is /home/$USER/shared_dir
$ sh run.sh

# If you select your shared directory path
$ sh run.sh {SHARED_DIR_PATH}
>>>>>>> b94c18bdb82cf29214a06941597a07b004c39dbb
```

#OLD!!
